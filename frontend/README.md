# QPE Streamlit Frontend

A web UI for the Parallel Query Processing System. It wraps the existing
compiled engines (`QPESeq`, `QPEOMP`, `QPEMPI`) without modifying them.

## Run

```bash
frontend/run.sh
```

The first run creates `frontend/.venv` and installs Streamlit, pandas, and
Plotly; afterwards it just launches the app (default: <http://localhost:8501>).

The engine binaries must be built (`make` at the repo root) — the app's
sidebar shows their status and has a build button.

## Pages

- **Query Runner** — write `;`-separated SQL (SELECT / INSERT / DELETE),
  pick a dataset and one or more engines (with thread/process counts), and
  see parsed result tables, per-phase timings, per-query time charts, and a
  cross-engine runtime breakdown. Raw engine output is always available.
- **Benchmark** — strong-scaling sweep: serial baseline plus OpenMP/MPI at
  chosen worker counts. Plots runtime, speedup vs. the ideal diagonal, and
  efficiency, and estimates the Amdahl parallel fraction (the live-measured
  version of `analysis.py`). Results are downloadable as CSV.
- **Datasets** — generate synthetic data via
  `data-generation/generate_commands.py`, and explore any CSV (schema
  preview, risk-level distribution, top commands, monthly activity).

## How engine invocation works

The engines read their queries from `sample-queries.txt` in the _current
working directory_. The frontend therefore writes your queries to a private
temp directory and runs each engine from there, passing the binary and data
file as absolute paths:

- Serial: `QPESeq <data.csv>`
- OpenMP: `QPEOMP <data.csv> <threads>` (plus `OMP_NUM_THREADS`)
- MPI: `mpirun -np <procs> QPEMPI <data.csv>` (falls back to
  `--oversubscribe` when procs exceed available slots)

Note: SELECT output is capped at 20 printed rows per query by the engines'
compile-time `ROW_LIMIT`; total record counts are still reported and shown.

**Dataset protection:** the engines rewrite their data file on exit
(persisting INSERT/DELETE — but the write-back drops the CSV header and
doesn't quote embedded commas, which corrupts rows over repeated runs). By
default the frontend runs every engine against a temporary _copy_ of the
dataset, so files on disk are never modified; the Query Runner has a
"write changes back" checkbox to opt into the engines' native persistence.
The Benchmark page always uses a copy so timings stay comparable. The
dataset explorer tolerates previously rewritten (headerless) files.
