#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="${SCRIPT_DIR}/.build-venv"

# ---------------------------------------------------------------------------
# Arguments
# ---------------------------------------------------------------------------
PYTHON_VERSION="${1:-}"

# ---------------------------------------------------------------------------
# Prerequisites check
# ---------------------------------------------------------------------------
check_cmd() {
    if ! command -v "$1" &>/dev/null; then
        echo "ERROR: '$1' is required but not found in PATH." >&2
        exit 1
    fi
}

check_cmd uv
check_cmd cmake
check_cmd make
check_cmd cc

# ---------------------------------------------------------------------------
# Git submodule
# ---------------------------------------------------------------------------
if [ ! -f "${SCRIPT_DIR}/libcaption/CMakeLists.txt" ]; then
    echo ">>> Initializing git submodule libcaption..."
    git -C "${SCRIPT_DIR}" submodule update --init libcaption
fi

# ---------------------------------------------------------------------------
# Clean stale CMake artifacts so a fresh build is always done
# ---------------------------------------------------------------------------
echo ">>> Cleaning previous libcaption build artifacts..."
rm -f \
    "${SCRIPT_DIR}/libcaption/CMakeCache.txt" \
    "${SCRIPT_DIR}/libcaption/Makefile" \
    "${SCRIPT_DIR}/libcaption/libcaption.a"

# ---------------------------------------------------------------------------
# Create venv with the requested Python version
# ---------------------------------------------------------------------------
UV_VENV_ARGS=()
if [ -n "${PYTHON_VERSION}" ]; then
    UV_VENV_ARGS+=(--python "${PYTHON_VERSION}")
fi

echo ">>> Creating build venv${PYTHON_VERSION:+ for Python ${PYTHON_VERSION}}..."
uv venv --clear "${UV_VENV_ARGS[@]}" "${VENV_DIR}"

VENV_PYTHON="${VENV_DIR}/bin/python"

# ---------------------------------------------------------------------------
# Install build dependencies
# ---------------------------------------------------------------------------
echo ">>> Installing build dependencies..."
uv pip install --python "${VENV_PYTHON}" \
    "Cython" \
    "numpy" \
    "setuptools" \
    "wheel"

# ---------------------------------------------------------------------------
# Build the wheel
# ---------------------------------------------------------------------------
echo ">>> Building wheel..."
"${VENV_PYTHON}" setup.py bdist_wheel

# ---------------------------------------------------------------------------
# Report result
# ---------------------------------------------------------------------------
echo ""
echo ">>> Build complete. Wheel(s) in dist/:"
ls -1 "${SCRIPT_DIR}/dist/"*.whl
