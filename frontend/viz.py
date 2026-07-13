"""Plotly figure builders for the QPE frontend.

Colors follow the validated reference palette (dataviz skill): categorical
slots are assigned to entities in fixed order — Serial=blue, OpenMP=aqua,
MPI=yellow — and never repainted when a filter changes the series count.
Light and dark variants are separately validated against their surfaces.
"""

import plotly.graph_objects as go

# Fixed entity -> categorical slot (light, dark)
SERIES = {
    "Serial": ("#2a78d6", "#3987e5"),
    "OpenMP": ("#1baf7a", "#199e70"),
    "MPI":    ("#eda100", "#c98500"),
}
# Phase colors reuse the same three slots (phases are the series in the
# breakdown chart, engines are the axis categories there).
PHASES = {
    "Initialization": ("#2a78d6", "#3987e5"),
    "Query loading":  ("#1baf7a", "#199e70"),
    "Query execution": ("#eda100", "#c98500"),
}

# Ordinal ramp for risk level 1..5 (blue sequential, steps 250-650)
RISK_RAMP = ["#86b6ef", "#5598e7", "#2a78d6", "#1c5cab", "#104281"]

CHROME = {
    "light": dict(surface="#fcfcfb", ink="#0b0b0b", muted="#898781",
                  grid="#e1e0d9", baseline="#c3c2b7"),
    "dark": dict(surface="#1a1a19", ink="#ffffff", muted="#898781",
                 grid="#2c2c2a", baseline="#383835"),
}
FONT = 'system-ui, -apple-system, "Segoe UI", sans-serif'


def series_color(name, mode="light"):
    light, dark = SERIES[name]
    return dark if mode == "dark" else light


def _layout(fig, mode, title, xtitle, ytitle, legend=True):
    c = CHROME[mode]
    fig.update_layout(
        title=dict(text=title, font=dict(size=15, color=c["ink"], family=FONT)),
        font=dict(family=FONT, size=12, color=c["muted"]),
        paper_bgcolor="rgba(0,0,0,0)",
        plot_bgcolor=c["surface"],
        margin=dict(l=48, r=16, t=48, b=44),
        showlegend=legend,
        legend=dict(bgcolor="rgba(0,0,0,0)", font=dict(color=c["ink"])),
        hovermode="x unified",
    )
    fig.update_xaxes(title=xtitle, gridcolor=c["grid"], linecolor=c["baseline"],
                     zeroline=False, title_font=dict(color=c["muted"]))
    fig.update_yaxes(title=ytitle, gridcolor=c["grid"], linecolor=c["baseline"],
                     zeroline=False, title_font=dict(color=c["muted"]))
    return fig


def scaling_figure(data, metric, title, ytitle, mode="light",
                   ideal=None, serial_baseline=None):
    """Line chart of a per-worker metric for each parallel engine.

    data: {engine: (workers_list, values_list)}
    ideal: None | "diagonal" (y=x) | float (horizontal reference)
    serial_baseline: optional float drawn as a horizontal Serial reference
    """
    fig = go.Figure()
    all_workers = sorted({w for ws, _ in data.values() for w in ws})

    if ideal is not None and all_workers:
        if ideal == "diagonal":
            ix, iy = all_workers, all_workers
        else:
            ix, iy = all_workers, [ideal] * len(all_workers)
        fig.add_trace(go.Scatter(
            x=ix, y=iy, name="Ideal", mode="lines",
            line=dict(color=CHROME[mode]["muted"], width=2, dash="dash"),
            hoverinfo="skip",
        ))
    if serial_baseline is not None and all_workers:
        fig.add_trace(go.Scatter(
            x=all_workers, y=[serial_baseline] * len(all_workers),
            name="Serial", mode="lines",
            line=dict(color=series_color("Serial", mode), width=2, dash="dot"),
        ))
    for engine in SERIES:  # fixed order, fixed colors
        if engine not in data:
            continue
        workers, values = data[engine]
        fig.add_trace(go.Scatter(
            x=workers, y=values, name=engine, mode="lines+markers",
            line=dict(color=series_color(engine, mode), width=2),
            marker=dict(size=8),
            hovertemplate="%{y:.4f}<extra>" + engine + "</extra>",
        ))
    return _layout(fig, mode, title, "Threads / processes (p)", ytitle)


def phase_breakdown_figure(runs, mode="light"):
    """Stacked bar: engine on the x axis, runtime split by phase.

    runs: list of EngineRun (with .summary containing init/load/exec).
    """
    c = CHROME[mode]
    labels = [f"{r.engine} (p={r.workers})" if r.engine != "Serial" else "Serial"
              for r in runs]
    fig = go.Figure()
    for phase, key in [("Initialization", "init"), ("Query loading", "load"),
                       ("Query execution", "exec")]:
        light, dark = PHASES[phase]
        fig.add_trace(go.Bar(
            x=labels,
            y=[r.summary.get(key, 0) for r in runs],
            name=phase,
            marker=dict(color=dark if mode == "dark" else light,
                        line=dict(color=c["surface"], width=2)),  # 2px spacer
            hovertemplate="%{y:.4f} s<extra>" + phase + "</extra>",
        ))
    fig.update_layout(barmode="stack", bargap=0.45)
    return _layout(fig, mode, "Runtime breakdown by phase", "", "Seconds")


def query_times_figure(labels, times, mode="light"):
    """Horizontal bar of engine-reported per-query times (single measure ->
    single hue; the axis carries identity)."""
    fig = go.Figure(go.Bar(
        x=times, y=labels, orientation="h",
        marker=dict(color=series_color("Serial", mode),
                    line=dict(color=CHROME[mode]["surface"], width=2)),
        hovertemplate="%{x:.4f} s<extra></extra>",
    ))
    fig.update_layout(bargap=0.4, height=max(220, 40 * len(labels) + 120))
    fig.update_yaxes(autorange="reversed")
    return _layout(fig, mode, "Per-query execution time", "Seconds", "",
                   legend=False)


def risk_histogram(counts, mode="light"):
    """Bar chart of record counts by risk level (ordinal ramp, light->dark)."""
    levels = sorted(counts.index.tolist())
    fig = go.Figure(go.Bar(
        x=[str(l) for l in levels],
        y=[counts[l] for l in levels],
        marker=dict(color=[RISK_RAMP[min(int(l) - 1, 4)] for l in levels],
                    line=dict(color=CHROME[mode]["surface"], width=2)),
        hovertemplate="%{y:,} records<extra>risk %{x}</extra>",
    ))
    fig.update_layout(bargap=0.35)
    return _layout(fig, mode, "Records by risk level", "Risk level", "Records",
                   legend=False)


def top_commands_figure(counts, mode="light"):
    """Horizontal bar of the most frequent base commands (single hue)."""
    fig = go.Figure(go.Bar(
        x=counts.values[::-1], y=counts.index[::-1].tolist(), orientation="h",
        marker=dict(color=series_color("Serial", mode),
                    line=dict(color=CHROME[mode]["surface"], width=2)),
        hovertemplate="%{x:,} records<extra>%{y}</extra>",
    ))
    fig.update_layout(bargap=0.35, height=max(260, 26 * len(counts) + 120))
    return _layout(fig, mode, "Most frequent base commands", "Records", "",
                   legend=False)


def activity_figure(monthly, mode="light"):
    """Single-series line of records per month (no legend: title names it)."""
    fig = go.Figure(go.Scatter(
        x=monthly.index.tolist(), y=monthly.values,
        mode="lines+markers",
        line=dict(color=series_color("Serial", mode), width=2),
        marker=dict(size=8),
        hovertemplate="%{y:,} records<extra>%{x}</extra>",
    ))
    return _layout(fig, mode, "Commands per month", "", "Records", legend=False)
