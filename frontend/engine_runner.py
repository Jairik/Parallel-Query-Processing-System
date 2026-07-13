"""Bridge between the Streamlit frontend and the compiled QPE engines.

The engines (QPESeq / QPEOMP / QPEMPI) read their queries from a file named
`sample-queries.txt` in the *current working directory*. To run arbitrary
queries without touching the repo, each run happens inside a private temp
directory containing only that file; the binary and the data file are passed
as absolute paths, so all existing engine behavior is preserved unchanged.
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass, field

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA_DIR = os.path.join(REPO_ROOT, "data-generation")
GENERATOR = os.path.join(DATA_DIR, "generate_commands.py")
SAMPLE_QUERIES = os.path.join(REPO_ROOT, "sample-queries.txt")

# Engine display name -> binary name (relative to repo root)
ENGINES = {"Serial": "QPESeq", "OpenMP": "QPEOMP", "MPI": "QPEMPI"}

# Engines print at most this many rows per SELECT (compile-time ROW_LIMIT)
ENGINE_ROW_LIMIT = 20

# CSV schema written by generate_commands.py (the engines' write-back on
# exit drops this header line, so readers must be able to restore it)
SCHEMA_COLUMNS = [
    "command_id", "raw_command", "base_command", "shell_type", "exit_code",
    "timestamp", "sudo_used", "working_directory", "user_id", "user_name",
    "host_name", "risk_level",
]

ANSI_RE = re.compile(r"\x1b\[[0-9;]*m")


def strip_ansi(text):
    return ANSI_RE.sub("", text)


# ---------------------------------------------------------------------------
# Binaries & build
# ---------------------------------------------------------------------------

def binary_path(engine):
    return os.path.join(REPO_ROOT, ENGINES[engine])


def binaries_status():
    """Map of engine name -> bool (binary exists and is executable)."""
    return {e: os.access(binary_path(e), os.X_OK) for e in ENGINES}


def mpi_launcher():
    return shutil.which("mpirun") or shutil.which("mpiexec")


def build_project():
    """Run `make` at the repo root. Returns (ok, combined_output)."""
    proc = subprocess.run(
        ["make", "-C", REPO_ROOT],
        capture_output=True, text=True, timeout=600,
    )
    return proc.returncode == 0, proc.stdout + proc.stderr


# ---------------------------------------------------------------------------
# Datasets
# ---------------------------------------------------------------------------

def is_lfs_pointer(path):
    try:
        with open(path, "rb") as f:
            return f.read(40).startswith(b"version https://git-lfs")
    except OSError:
        return False


def has_header(path):
    try:
        with open(path, "rb") as f:
            return f.readline().startswith(b"command_id,")
    except OSError:
        return False


def list_datasets():
    """All CSVs in data-generation/, flagging unfetched git-lfs pointers."""
    out = []
    if not os.path.isdir(DATA_DIR):
        return out
    for name in sorted(os.listdir(DATA_DIR)):
        if not name.endswith(".csv"):
            continue
        path = os.path.join(DATA_DIR, name)
        lfs = is_lfs_pointer(path)
        out.append({
            "name": name,
            "path": path,
            "size_mb": os.path.getsize(path) / 1e6,
            "lfs_pointer": lfs,
            "headerless": not lfs and not has_header(path),
        })
    return out


def usable_datasets():
    return [d for d in list_datasets() if not d["lfs_pointer"]]


def generate_dataset(num_rows, out_path):
    """Run data-generation/generate_commands.py. Returns (ok, output)."""
    proc = subprocess.run(
        [sys.executable, GENERATOR, str(num_rows), out_path],
        capture_output=True, text=True, timeout=1800,
    )
    return proc.returncode == 0, proc.stdout + proc.stderr


def default_queries():
    try:
        with open(SAMPLE_QUERIES) as f:
            return f.read()
    except OSError:
        return "SELECT * FROM Commands WHERE risk_level = 5;\n"


# ---------------------------------------------------------------------------
# Running an engine
# ---------------------------------------------------------------------------

@dataclass
class QueryResult:
    query: str
    kind: str = "message"          # "table" | "message" | "error"
    columns: list = field(default_factory=list)
    rows: list = field(default_factory=list)
    total_records: int = None
    query_time: float = None       # engine-reported seconds
    truncated: int = 0             # rows omitted by the engine's ROW_LIMIT
    message: str = ""


@dataclass
class EngineRun:
    engine: str
    workers: int
    ok: bool
    raw_output: str
    stderr: str = ""
    wall_time: float = 0.0
    summary: dict = field(default_factory=dict)   # init/load/exec/total seconds
    results: list = field(default_factory=list)   # list[QueryResult]
    command: str = ""


def _build_command(engine, data_file, workers):
    binary = binary_path(engine)
    if engine == "Serial":
        return [binary, data_file], {}
    if engine == "OpenMP":
        # QPEOMP takes <datafile> <threads>; env var set as well for safety
        return [binary, data_file, str(workers)], {"OMP_NUM_THREADS": str(workers)}
    launcher = mpi_launcher()
    if launcher is None:
        raise RuntimeError("Neither mpirun nor mpiexec found on PATH.")
    return [launcher, "-np", str(workers), binary, data_file], {}


def run_engine(engine, data_file, queries, workers=1, timeout=600,
               protect_data=True):
    """Execute one engine over `queries` (a SQL string, ';'-separated).

    The engines rewrite their data file on exit (persisting INSERT/DELETE,
    but dropping the CSV header and comma-quoting in the process). With
    protect_data=True (default) the engine runs against a temp copy so the
    dataset on disk is left untouched; pass False to keep the engines'
    native write-back behavior.
    """
    tmpdir = tempfile.mkdtemp(prefix="qpe-run-")
    try:
        if protect_data:
            data_copy = os.path.join(tmpdir, os.path.basename(data_file))
            shutil.copyfile(data_file, data_copy)
            data_file = data_copy
        cmd, extra_env = _build_command(engine, data_file, workers)
        env = {**os.environ, **extra_env}
        with open(os.path.join(tmpdir, "sample-queries.txt"), "w") as f:
            f.write(queries if queries.rstrip().endswith(";") else queries + ";")

        start = time.perf_counter()
        try:
            proc = subprocess.run(
                cmd, cwd=tmpdir, env=env,
                capture_output=True, text=True, timeout=timeout,
            )
        except subprocess.TimeoutExpired as exc:
            return EngineRun(
                engine=engine, workers=workers, ok=False,
                raw_output=strip_ansi(exc.stdout or ""),
                stderr=f"Timed out after {timeout}s.\n" + strip_ansi(exc.stderr or ""),
                wall_time=time.perf_counter() - start, command=" ".join(cmd),
            )

        # OpenMPI refuses np > slots by default; retry oversubscribed
        if engine == "MPI" and proc.returncode != 0 and "slots" in proc.stderr:
            cmd = cmd[:1] + ["--oversubscribe"] + cmd[1:]
            start = time.perf_counter()
            proc = subprocess.run(
                cmd, cwd=tmpdir, env=env,
                capture_output=True, text=True, timeout=timeout,
            )
        wall = time.perf_counter() - start

        stdout = strip_ansi(proc.stdout)
        summary, results = parse_output(stdout)
        return EngineRun(
            engine=engine, workers=workers,
            ok=proc.returncode == 0,
            raw_output=stdout, stderr=strip_ansi(proc.stderr),
            wall_time=wall, summary=summary, results=results,
            command=" ".join(cmd),
        )
    finally:
        shutil.rmtree(tmpdir, ignore_errors=True)


# ---------------------------------------------------------------------------
# Output parsing
# ---------------------------------------------------------------------------

_SUMMARY_PATTERNS = {
    "init": re.compile(r"Engine Initialization Time:\s*([0-9.]+)"),
    "load": re.compile(r"Query Loading Time:\s*([0-9.]+)"),
    "exec": re.compile(r"Query Execution Time:\s*([0-9.]+)"),
    "total": re.compile(r"Total Execution Time:\s*([0-9.]+)"),
}
_RECORDS_RE = re.compile(r"Total Records:\s*(\d+)\s*\|\s*Query Time:\s*([0-9.]+)")
_TRUNCATED_RE = re.compile(r"\.\.\. \((\d+) more records\) \.\.\.")
_EXEC_TIME_RE = re.compile(r"Execution Time:\s*([0-9.]+)")
_ROWS_AFFECTED_RE = re.compile(r"Rows affected:\s*(\d+)")
_NOISE = ("Starting main...", "Initializing Engine...", "Engine Initialized.",
          "Running with ")


def parse_output(stdout):
    """Parse engine stdout into (summary_times, [QueryResult]).

    Best-effort: with MPI at np > 1 the per-rank output can interleave, so
    anything unparseable simply stays visible in the raw output.
    """
    summary = {}
    for key, pat in _SUMMARY_PATTERNS.items():
        m = pat.search(stdout)
        if m:
            summary[key] = float(m.group(1))

    lines = stdout.splitlines()
    results = []
    i = 0
    while i < len(lines):
        line = lines[i]
        if not line.startswith("Executing Query:"):
            i += 1
            continue
        query = line[len("Executing Query:"):].strip()
        # Collect this query's block (up to the next query or the summary)
        j = i + 1
        while j < len(lines) and not lines[j].startswith("Executing Query:") \
                and "Execution Summary" not in lines[j]:
            j += 1
        results.append(_parse_block(query, lines[i + 1:j]))
        i = j
    return summary, results


def _is_separator(line):
    return line.startswith("+") and set(line.strip()) <= {"+", "-"}


_RESULT_MARKERS = ("+", "|", "No data found", "Insert ", "Delete ",
                   "Tokenization failed", "No command detected",
                   "Unsupported command", "Error:", "Total Records:")


def _parse_block(query, block):
    res = QueryResult(query=query)
    # The engine echoes the query with its original newlines, so the block
    # may open with the query's continuation lines; fold them back in
    # (dropping '#' comment lines) before looking for results.
    start = 0
    parts = [] if query.lstrip().startswith("#") else [query]
    while start < len(block):
        text = block[start].strip()
        if text and not any(text.startswith(n) for n in _NOISE):
            if any(text.startswith(m) for m in _RESULT_MARKERS):
                break
            if not text.startswith("#"):
                parts.append(text)
        start += 1
    if parts:
        res.query = " ".join(" ".join(parts).split())
    block = block[start:]
    for k, line in enumerate(block):
        text = line.strip()
        if not text or any(text.startswith(n) for n in _NOISE):
            continue
        if _is_separator(line):
            _parse_table(res, block[k:])
            break
        if text.startswith("No data found"):
            res.kind = "table"
            res.total_records = 0
            break
        if text.startswith(("Insert", "Delete")):
            res.message = text
            res.kind = "error" if "failed" in text or "Error" in text else "message"
            m = _EXEC_TIME_RE.search(text)
            if m:
                res.query_time = float(m.group(1))
            m = _ROWS_AFFECTED_RE.search(text)
            if m:
                res.total_records = int(m.group(1))
            break
        if text.startswith(("Tokenization failed", "No command detected",
                            "Unsupported command", "Error:")):
            res.kind = "error"
            res.message = text
            break
    # Records/time line may sit after the table
    tail = "\n".join(block)
    m = _RECORDS_RE.search(tail)
    if m:
        res.total_records = int(m.group(1))
        res.query_time = float(m.group(2))
    m = _TRUNCATED_RE.search(tail)
    if m:
        res.truncated = int(m.group(1))
    return res


def _parse_table(res, lines):
    """Parse a printTable block. Cell boundaries come from the '+' positions
    of the separator row, so '|' characters inside values are handled."""
    sep = lines[0]
    bounds = [i for i, ch in enumerate(sep) if ch == "+"]
    if len(bounds) < 2:
        return

    def cells(row):
        return [row[a + 1:b].strip() for a, b in zip(bounds[:-1], bounds[1:])]

    res.kind = "table"
    header_seen = False
    for line in lines[1:]:
        if _is_separator(line):
            continue
        if not line.startswith("|"):
            break  # table finished
        if not header_seen:
            res.columns = cells(line)
            header_seen = True
        else:
            res.rows.append(cells(line))
