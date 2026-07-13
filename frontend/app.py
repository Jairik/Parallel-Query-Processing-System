"""Streamlit frontend for the Parallel Query Processing System.

Wraps the existing compiled engines (QPESeq / QPEOMP / QPEMPI) without
modifying them: queries are written to a per-run temp directory (the engines
read `sample-queries.txt` from their working directory) and output is parsed
back into tables, timings, and charts.

Run with:  frontend/run.sh   (or: streamlit run frontend/app.py)
"""

import os

# pyarrow's bundled mimalloc can segfault in Streamlit's script-runner thread
# (mi_thread_init); force Arrow onto the system allocator before it loads.
os.environ.setdefault("ARROW_DEFAULT_MEMORY_POOL", "system")

import pandas as pd
import streamlit as st

import engine_runner as er
import viz

st.set_page_config(page_title="Parallel QPE", page_icon="⚡", layout="wide")


def theme_mode():
    try:
        return "dark" if st.context.theme.type == "dark" else "light"
    except Exception:
        return "light"


MODE = theme_mode()


# ---------------------------------------------------------------------------
# Shared helpers
# ---------------------------------------------------------------------------

@st.cache_data(show_spinner="Loading dataset…")
def load_dataset(path, mtime, nrows):
    # Files rewritten by the engines lose their header and comma-quoting;
    # restore the schema and skip any rows the write-back corrupted.
    kwargs = {} if er.has_header(path) else {"header": None,
                                             "names": er.SCHEMA_COLUMNS}
    df = pd.read_csv(path, nrows=nrows, on_bad_lines="skip", **kwargs)
    for col in ("risk_level", "user_id", "exit_code"):
        df[col] = pd.to_numeric(df[col], errors="coerce")
    return df


def dataset_selectbox(key):
    datasets = er.usable_datasets()
    pointers = [d for d in er.list_datasets() if d["lfs_pointer"]]
    if not datasets:
        st.warning(
            "No usable datasets found in `data-generation/`. "
            + (f"({len(pointers)} file(s) are unfetched git-lfs pointers.) "
               if pointers else "")
            + "Generate one on the **Datasets** page."
        )
        return None
    labels = {f"{d['name']}  ({d['size_mb']:.1f} MB)": d["path"] for d in datasets}
    choice = st.selectbox("Dataset", list(labels), key=key)
    return labels[choice]


def render_query_result(res, idx):
    sql_lines = [l for l in res.query.splitlines() if not l.lstrip().startswith("#")]
    label = " ".join(" ".join(sql_lines).split()) or " ".join(res.query.split())
    if len(label) > 90:
        label = label[:90] + "…"
    with st.expander(f"Query {idx + 1}: {label}", expanded=(idx == 0)):
        if res.kind == "table":
            if res.rows:
                st.dataframe(pd.DataFrame(res.rows, columns=res.columns),
                             width="stretch", hide_index=True)
            elif res.total_records == 0:
                st.info("No data found.")
            caption = []
            if res.total_records is not None:
                caption.append(f"{res.total_records:,} total records")
            if res.truncated:
                caption.append(
                    f"showing first {er.ENGINE_ROW_LIMIT} "
                    f"(engine ROW_LIMIT; {res.truncated:,} not printed)")
            if res.query_time is not None:
                caption.append(f"engine query time {res.query_time:.4f} s")
            if caption:
                st.caption(" · ".join(caption))
        elif res.kind == "error":
            st.error(res.message or "Query failed.")
        else:
            msg = res.message
            if res.query_time is not None:
                msg = msg or f"Done in {res.query_time:.4f} s"
            st.success(msg)


def render_run(run):
    if not run.ok:
        st.error(f"`{run.command}` exited with an error.")
        if run.stderr:
            st.code(run.stderr)
    cols = st.columns(4)
    cols[0].metric("Engine init", _fmt_s(run.summary.get("init")))
    cols[1].metric("Query execution", _fmt_s(run.summary.get("exec")))
    cols[2].metric("Engine total", _fmt_s(run.summary.get("total")))
    cols[3].metric("Wall clock", _fmt_s(run.wall_time))

    for idx, res in enumerate(run.results):
        render_query_result(res, idx)

    timed = [(f"Q{idx + 1}", r.query_time) for idx, r in enumerate(run.results)
             if r.query_time is not None]
    if len(timed) > 1:
        labels, times = zip(*timed)
        st.plotly_chart(viz.query_times_figure(list(labels), list(times), MODE),
                        width="stretch")

    with st.expander("Raw engine output"):
        st.caption(f"`{run.command}`")
        st.code(run.raw_output or "(empty)", language=None)
        if run.stderr:
            st.caption("stderr")
            st.code(run.stderr, language=None)


def _fmt_s(value):
    return f"{value:.4f} s" if value is not None else "—"


# ---------------------------------------------------------------------------
# Sidebar
# ---------------------------------------------------------------------------

st.sidebar.title("⚡ Parallel QPE")
page = st.sidebar.radio("Section", ["Query Runner", "Benchmark", "Datasets"],
                        label_visibility="collapsed")
st.sidebar.divider()

status = er.binaries_status()
st.sidebar.caption("Engine binaries")
for engine, built in status.items():
    st.sidebar.markdown(("✅ " if built else "❌ ") + f"`{er.ENGINES[engine]}`")
if er.mpi_launcher() is None:
    st.sidebar.warning("mpirun/mpiexec not found — MPI runs are disabled.")

if st.sidebar.button("Build engines (`make`)",
                     width="stretch"):
    with st.sidebar.status("Running make…"):
        ok, output = er.build_project()
    (st.sidebar.success if ok else st.sidebar.error)(
        "Build succeeded." if ok else "Build failed.")
    with st.sidebar.expander("Build output"):
        st.code(output, language=None)

st.sidebar.divider()
timeout = st.sidebar.number_input("Run timeout (seconds)", 30, 3600, 600, 30)


# ---------------------------------------------------------------------------
# Page: Query Runner
# ---------------------------------------------------------------------------

def page_query_runner():
    st.header("Query Runner")
    st.caption("Run SQL-like queries (SELECT / INSERT / DELETE, `;`-separated) "
               "against any dataset with the serial, OpenMP, or MPI engine.")

    data_file = dataset_selectbox("runner_dataset")
    if data_file is None:
        return

    available = [e for e, built in er.binaries_status().items() if built]
    if er.mpi_launcher() is None and "MPI" in available:
        available.remove("MPI")
    if not available:
        st.error("No engine binaries found — build them from the sidebar.")
        return

    engines = st.multiselect("Engines", available,
                             default=[available[0]], key="runner_engines")
    ecols = st.columns(2)
    threads = ecols[0].number_input("OpenMP threads", 1, 64, 4,
                                    disabled="OpenMP" not in engines)
    procs = ecols[1].number_input("MPI processes", 1, 64, 4,
                                  disabled="MPI" not in engines)

    queries = st.text_area("Queries", er.default_queries(), height=260,
                           key="runner_queries")
    st.caption(f"Lines starting with `#` are comments. SELECT output is capped "
               f"at {er.ENGINE_ROW_LIMIT} printed rows per query by the engine "
               f"(compile-time `ROW_LIMIT`); totals are still reported.")
    writeback = st.checkbox(
        "Write changes back to the dataset file",
        value=False,
        help="The engines persist the table to the data file on exit, so "
             "INSERTs/DELETEs survive between runs — but the write-back "
             "drops the CSV header and does not quote commas, which can "
             "corrupt rows over repeated runs. Off (default): the engine "
             "runs against a temporary copy and the file is untouched.")

    if st.button("Run queries", type="primary", disabled=not engines):
        runs = []
        with st.status("Running engines…", expanded=True) as box:
            for engine in engines:
                workers = {"Serial": 1, "OpenMP": threads, "MPI": procs}[engine]
                st.write(f"Running **{engine}** "
                         + (f"(p={workers})…" if engine != "Serial" else "…"))
                runs.append(er.run_engine(engine, data_file, queries,
                                          workers, timeout,
                                          protect_data=not writeback))
            box.update(label="Done", state="complete", expanded=False)
        st.session_state["runner_runs"] = runs

    runs = st.session_state.get("runner_runs", [])
    if not runs:
        return

    if len(runs) > 1:
        st.subheader("Engine comparison")
        st.plotly_chart(viz.phase_breakdown_figure(runs, MODE),
                        width="stretch")
        st.dataframe(pd.DataFrame([{
            "Engine": r.engine,
            "p": r.workers,
            "Init (s)": r.summary.get("init"),
            "Query exec (s)": r.summary.get("exec"),
            "Engine total (s)": r.summary.get("total"),
            "Wall clock (s)": round(r.wall_time, 4),
        } for r in runs]), width="stretch", hide_index=True)

    tabs = st.tabs([f"{r.engine}" + (f" (p={r.workers})" if r.engine != "Serial"
                                     else "") for r in runs])
    for tab, run in zip(tabs, runs):
        with tab:
            render_run(run)


# ---------------------------------------------------------------------------
# Page: Benchmark
# ---------------------------------------------------------------------------

def page_benchmark():
    st.header("Benchmark & Scaling Analysis")
    st.caption("Strong-scaling sweep: the serial engine sets the baseline, "
               "then OpenMP and MPI run the same workload at each worker "
               "count. Speedup, efficiency, and the Amdahl parallel fraction "
               "are computed from live measurements.")

    data_file = dataset_selectbox("bench_dataset")
    if data_file is None:
        return

    status = er.binaries_status()
    if not status["Serial"]:
        st.error("The serial engine (baseline) is not built.")
        return

    ncpu = os.cpu_count() or 4
    candidates = sorted({1, 2, 3, 4, 6, 8, 12, 16, 24, 32} | {ncpu})
    default = [p for p in (1, 2, 4, 8) if p <= ncpu]

    ccols = st.columns(2)
    with ccols[0]:
        run_omp = st.checkbox("OpenMP sweep", value=status["OpenMP"],
                              disabled=not status["OpenMP"])
        omp_counts = st.multiselect("Thread counts", candidates, default,
                                    disabled=not run_omp)
    with ccols[1]:
        mpi_possible = status["MPI"] and er.mpi_launcher() is not None
        run_mpi = st.checkbox("MPI sweep", value=mpi_possible,
                              disabled=not mpi_possible)
        mpi_counts = st.multiselect("Process counts", candidates, default,
                                    disabled=not run_mpi)
    st.caption(f"This machine reports {ncpu} CPUs.")

    ocols = st.columns(3)
    metric_label = ocols[0].radio("Timing metric",
                                  ["Query execution time", "Total engine time"])
    metric = "exec" if metric_label.startswith("Query") else "total"
    repeats = ocols[1].number_input("Runs per configuration (best is kept)",
                                    1, 5, 1)
    with ocols[2]:
        with st.popover("Workload (queries)"):
            queries = st.text_area("Benchmark queries", er.default_queries(),
                                   height=240, key="bench_queries")

    if st.button("Run benchmark", type="primary"):
        plan = [("Serial", 1)]
        plan += [("OpenMP", p) for p in sorted(omp_counts)] if run_omp else []
        plan += [("MPI", p) for p in sorted(mpi_counts)] if run_mpi else []

        rows, failures = [], []
        progress = st.progress(0.0)
        with st.status("Benchmarking…", expanded=True) as box:
            total = len(plan) * repeats
            done = 0
            for engine, p in plan:
                best = None
                for _ in range(int(repeats)):
                    st.write(f"{engine} p={p} …")
                    run = er.run_engine(engine, data_file, queries, p, timeout)
                    done += 1
                    progress.progress(done / total)
                    if not run.ok or metric not in run.summary:
                        failures.append((engine, p, run.stderr or "no timing "
                                         "found in output"))
                        continue
                    if best is None or run.summary[metric] < best.summary[metric]:
                        best = run
                if best is not None:
                    rows.append({"engine": best.engine, "p": p,
                                 "time": best.summary[metric],
                                 "init": best.summary.get("init"),
                                 "total": best.summary.get("total"),
                                 "wall": best.wall_time})
            box.update(label="Benchmark complete", state="complete",
                       expanded=False)
        st.session_state["bench_rows"] = rows
        st.session_state["bench_failures"] = failures
        st.session_state["bench_metric_label"] = metric_label

    rows = st.session_state.get("bench_rows")
    if not rows:
        return
    for engine, p, err in st.session_state.get("bench_failures", []):
        st.error(f"{engine} p={p} failed: {err[:400]}")

    df = pd.DataFrame(rows)
    serial = df[df.engine == "Serial"]
    if serial.empty:
        st.error("Serial baseline failed — speedup/efficiency unavailable.")
        st.dataframe(df, width="stretch", hide_index=True)
        return
    t_serial = float(serial.time.iloc[0])

    par = df[df.engine != "Serial"].copy()
    par["speedup"] = t_serial / par.time
    par["efficiency"] = par.speedup / par.p

    # Amdahl parallel-fraction estimate: f = (1 - 1/S) / (1 - 1/p), p > 1
    gt1 = par[par.p > 1]
    amdahl = {}
    for engine, grp in gt1.groupby("engine"):
        f = ((1 - 1 / grp.speedup) / (1 - 1 / grp.p)).mean()
        amdahl[engine] = f

    st.subheader("Results")
    mcols = st.columns(2 + 2 * len(amdahl))
    mcols[0].metric("Serial baseline", f"{t_serial:.4f} s")
    i = 1
    for engine, grp in par.groupby("engine"):
        best = grp.loc[grp.speedup.idxmax()]
        mcols[i].metric(f"{engine} best speedup",
                        f"{best.speedup:.2f}×", f"at p={int(best.p)}",
                        delta_color="off")
        i += 1
        if engine in amdahl:
            mcols[i].metric(f"{engine} Amdahl f", f"{amdahl[engine]:.3f}")
            i += 1

    def per_engine(col):
        return {e: (g.p.tolist(), g[col].tolist())
                for e, g in par.groupby("engine")}

    metric_label = st.session_state.get("bench_metric_label", "time")
    c1, c2 = st.columns(2)
    with c1:
        st.plotly_chart(viz.scaling_figure(
            per_engine("time"), "time", f"{metric_label} vs p", "Seconds",
            MODE, serial_baseline=t_serial), width="stretch")
        st.plotly_chart(viz.scaling_figure(
            per_engine("efficiency"), "efficiency",
            "Efficiency E(p) = S(p)/p", "Efficiency", MODE, ideal=1.0),
            width="stretch")
    with c2:
        st.plotly_chart(viz.scaling_figure(
            per_engine("speedup"), "speedup", "Speedup S(p) vs p", "Speedup",
            MODE, ideal="diagonal"), width="stretch")

        table = par[["engine", "p", "time", "speedup", "efficiency"]].round(4)
        st.dataframe(table, width="stretch", hide_index=True)
        st.download_button(
            "Download results (CSV)",
            df.merge(par[["engine", "p", "speedup", "efficiency"]],
                     on=["engine", "p"], how="left").to_csv(index=False),
            file_name="benchmark_results.csv", mime="text/csv")


# ---------------------------------------------------------------------------
# Page: Datasets
# ---------------------------------------------------------------------------

def page_datasets():
    st.header("Datasets")
    st.caption("Generate synthetic command-log data "
               "(`data-generation/generate_commands.py`) and explore any "
               "dataset before querying it.")

    datasets = er.list_datasets()
    if datasets:
        st.dataframe(pd.DataFrame([{
            "File": d["name"],
            "Size (MB)": round(d["size_mb"], 2),
            "Status": ("⚠️ git-lfs pointer — not fetched" if d["lfs_pointer"]
                       else "⚠️ header missing (rewritten by an engine run)"
                       if d["headerless"] else "✅ ready"),
        } for d in datasets]), width="stretch", hide_index=True)
        if any(d["lfs_pointer"] for d in datasets):
            st.caption("Pointer files can be fetched with `git lfs pull`, or "
                       "simply generate fresh data below.")

    st.subheader("Generate data")
    with st.form("generate"):
        gcols = st.columns([1, 1, 1])
        num_rows = gcols[0].number_input("Rows", 1_000, 10_000_000, 50_000,
                                         step=1_000)
        fname = gcols[1].text_input("Output file (in data-generation/)",
                                    "commands_custom.csv")
        gcols[2].markdown("&nbsp;")
        submitted = gcols[2].form_submit_button("Generate", type="primary")
    if submitted:
        out_path = os.path.join(er.DATA_DIR, os.path.basename(fname))
        with st.spinner(f"Generating {num_rows:,} rows…"):
            ok, output = er.generate_dataset(int(num_rows), out_path)
        if ok:
            st.success(output.strip() or f"Wrote {out_path}")
            load_dataset.clear()
            st.rerun()
        else:
            st.error(output)

    usable = er.usable_datasets()
    if not usable:
        return
    st.subheader("Explore")
    labels = {f"{d['name']}  ({d['size_mb']:.1f} MB)": d["path"] for d in usable}
    choice = st.selectbox("Dataset to explore", list(labels))
    path = labels[choice]

    CAP = 250_000
    df = load_dataset(path, os.path.getmtime(path), CAP)
    if len(df) == CAP:
        st.caption(f"Charts use the first {CAP:,} rows of this file.")

    mcols = st.columns(4)
    mcols[0].metric("Rows loaded", f"{len(df):,}")
    mcols[1].metric("Distinct users", f"{df.user_id.nunique():,}")
    sudo = df.sudo_used.astype(str).str.lower().isin(["true", "1"]).mean()
    mcols[2].metric("Sudo usage", f"{sudo:.1%}")
    mcols[3].metric("Mean risk level", f"{df.risk_level.mean():.2f}")

    st.dataframe(df.head(100), width="stretch", hide_index=True)

    c1, c2 = st.columns(2)
    with c1:
        risk_counts = df.risk_level.dropna().astype(int).value_counts()
        st.plotly_chart(viz.risk_histogram(risk_counts, MODE),
                        width="stretch")
        monthly = (pd.to_datetime(df.timestamp, errors="coerce",
                                  format="mixed")
                   .dt.to_period("M").value_counts().sort_index())
        monthly.index = monthly.index.astype(str)
        st.plotly_chart(viz.activity_figure(monthly, MODE),
                        width="stretch")
    with c2:
        st.plotly_chart(viz.top_commands_figure(
            df.base_command.value_counts().head(12), MODE),
            width="stretch")


PAGES = {"Query Runner": page_query_runner,
         "Benchmark": page_benchmark,
         "Datasets": page_datasets}
PAGES[page]()
