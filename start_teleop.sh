#!/bin/bash

# Keyboard teleop for rover remote control over /cmd_vel

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

source /opt/ros/humble/setup.bash
source "$SCRIPT_DIR/ros2_ws/install/setup.bash"

echo "========================================="
echo "ROVER TELEOP (KEYBOARD)"
echo "========================================="
echo "Controls:"
echo "  i: forward     ,: backward"
echo "  j/l: rotate left/right"
echo "  u/o/m/.: diagonals"
echo "  k: stop"
echo ""
echo "Tip: hold Shift for holonomic mode keys (not used for this rover)."
echo ""
echo "Publishing to /cmd_vel ..."

ros2 run teleop_twist_keyboard teleop_twist_keyboard --ros-args -r /cmd_vel:=/cmd_vel
