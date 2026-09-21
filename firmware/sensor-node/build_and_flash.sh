#!/bin/bash
# Wrapper script to build and flash the sensor-node firmware

set -e

PROJECT_DIR=$(dirname "$0")
ROOT_DIR="$PROJECT_DIR/../.."

# 1. Find PlatformIO
if command -v pio &> /dev/null; then
    PIO_CMD="pio"
elif [ -f "$HOME/.platformio/penv/bin/pio" ]; then
    PIO_CMD="$HOME/.platformio/penv/bin/pio"
elif [ -f "$HOME/.venv-platformio/bin/pio" ]; then
    PIO_CMD="$HOME/.venv-platformio/bin/pio"
elif [ -f "$HOME/.venv-pio/bin/pio" ]; then
    PIO_CMD="$HOME/.venv-pio/bin/pio"
else
    echo "Error: 'pio' command not found!"
    echo "Please install PlatformIO or run this script inside the PlatformIO terminal."
    exit 1
fi

echo "Using PlatformIO at: $PIO_CMD"

# 2. Extract upload port from AGENTS.md
UPLOAD_PORT=""
if [ -f "$ROOT_DIR/AGENTS.md" ]; then
    # Look for Waveshare Port line
    PORT_LINE=$(grep "Sensor Node" -A 10 "$ROOT_DIR/AGENTS.md" | grep "Default Port:" || true)
    if [ -n "$PORT_LINE" ]; then
        UPLOAD_PORT=$(echo "$PORT_LINE" | sed -n 's/.*Default Port: \*\*\(.*\)\*\*.*/\1/p' || true)
    fi
    # If not found in markdown table, try getting it via the scan script directly, or fallback
    if [ -z "$UPLOAD_PORT" ]; then
        # Try to find /dev/ttyACM* or /dev/ttyUSB*
        UPLOAD_PORT=$(ls /dev/ttyACM* 2>/dev/null | head -n 1)
        if [ -z "$UPLOAD_PORT" ]; then
            UPLOAD_PORT=$(ls /dev/ttyUSB* 2>/dev/null | head -n 1)
        fi
    fi
fi

cd "$PROJECT_DIR"

if [ -n "$UPLOAD_PORT" ]; then
    echo "Flashing to port: $UPLOAD_PORT"
    "$PIO_CMD" run -e yolo_uno -t upload --upload-port "$UPLOAD_PORT"
else
    echo "No port found in AGENTS.md or via auto-scan. Flashing with auto-detect port..."
    "$PIO_CMD" run -e yolo_uno -t upload
fi
