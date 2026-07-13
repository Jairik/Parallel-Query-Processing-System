#!/usr/bin/env bash
# Bootstrap a virtualenv (first run only) and launch the Streamlit frontend.
set -euo pipefail
cd "$(dirname "$0")"

if [ ! -d .venv ]; then
    echo "Creating virtualenv and installing dependencies (first run only)…"
    python3 -m venv .venv
    .venv/bin/pip install --quiet --upgrade pip
    .venv/bin/pip install --quiet -r requirements.txt
fi

# Avoid pyarrow/mimalloc segfaults in Streamlit's script-runner thread
export ARROW_DEFAULT_MEMORY_POOL="${ARROW_DEFAULT_MEMORY_POOL:-system}"

exec .venv/bin/streamlit run app.py "$@"
