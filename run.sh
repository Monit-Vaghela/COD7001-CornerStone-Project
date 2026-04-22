#!/bin/bash

# ─────────────────────────────────────────────
# run.sh — VHDL Simulator Pipeline Runner
# Usage:
#   ./run.sh <circuit.cir> <simulation.tes> -s|-c [-UIwave|-UIdash|-check]
# ─────────────────────────────────────────────

set -e

# ── Helpers ──────────────────────────────────
usage() {
    echo "Usage: ./run.sh <circuit.cir> <simulation.tes> -s|-c [-UIwave|-UIdash|-check]"
    echo ""
    echo "  -s         Run sequential simulation only"
    echo "  -c         Run both sequential and parallel simulation"
    echo "  -UIwave    Open GTKWave with parallel waveform (requires -c)"
    echo "  -UIdash    Open dashboard in browser (requires -c)"
    echo "  -check     Run comparator on sequential vs parallel traces (requires -c)"
    echo ""
    echo "Examples:"
    echo "  ./run.sh tests/fulladder/fulladder.cir tests/fulladder/fulladder.tes -s"
    echo "  ./run.sh tests/fulladder/fulladder.cir tests/fulladder/fulladder.tes -c"
    echo "  ./run.sh tests/fulladder/fulladder.cir tests/fulladder/fulladder.tes -c -UIwave"
    echo "  ./run.sh tests/fulladder/fulladder.cir tests/fulladder/fulladder.tes -c -UIdash"
    echo "  ./run.sh tests/fulladder/fulladder.cir tests/fulladder/fulladder.tes -c -check"
    exit 1
}

# ── Argument parsing ─────────────────────────
if [ "$#" -lt 3 ]; then
    usage
fi

CIR_PATH="$1"
TES_PATH="$2"
MODE="$3"
EXTRA="$4"

# Validate files exist
if [ ! -f "$CIR_PATH" ]; then
    echo "Error: circuit file '$CIR_PATH' not found"
    exit 1
fi

if [ ! -f "$TES_PATH" ]; then
    echo "Error: simulation file '$TES_PATH' not found"
    exit 1
fi

# Validate mode
if [ "$MODE" != "-s" ] && [ "$MODE" != "-c" ]; then
    echo "Error: mode must be -s or -c"
    usage
fi

# Validate extra flags
if [ -n "$EXTRA" ]; then
    if [ "$EXTRA" != "-UIwave" ] && [ "$EXTRA" != "-UIdash" ] && [ "$EXTRA" != "-check" ]; then
        echo "Error: unknown flag '$EXTRA'"
        usage
    fi
    if [ "$MODE" != "-c" ]; then
        echo "Error: '$EXTRA' can only be used with -c"
        exit 1
    fi
fi

# ── Derive JSON output paths ─────────────────
# Strip extension from paths to get base
CIR_BASE="${CIR_PATH%.*}"
TES_BASE="${TES_PATH%.*}"
CIR_JSON="${CIR_BASE}_cir.json"
TES_JSON="${TES_BASE}_tes.json"

# ── Stage 1: Compile ─────────────────────────
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "[1/3] Running compiler..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
./compiler "$CIR_PATH" "$TES_PATH"

# ── Stage 2: Circuit Builder + Simulator ─────
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "[2/3] Running circuit builder ($MODE)..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
./circuitBuilder "$CIR_JSON" "$TES_JSON" "$MODE"

# ── Stage 3: Extra actions ───────────────────
if [ -z "$EXTRA" ]; then
    echo ""
    echo "Done."
    exit 0
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "[3/3] Running $EXTRA..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if [ "$EXTRA" = "-UIwave" ]; then
    if ! command -v gtkwave &> /dev/null; then
        echo "Error: gtkwave is not installed. Install with: sudo apt install gtkwave"
        exit 1
    fi
    echo "Opening GTKWave with output/wavePar.vcd..."
    gtkwave output/wavePar.vcd &

elif [ "$EXTRA" = "-UIdash" ]; then
    echo "Starting dashboard server..."
    echo "Open your browser and go to: http://localhost:8080/dashboard.html"
    cd dashboard
    python3 -m http.server 8080

elif [ "$EXTRA" = "-check" ]; then
    echo "Running comparator on sequential vs parallel traces..."
    ./comparator output/traceSeq.bin output/tracePar.bin
fi

echo ""
echo "Done."