#!/bin/bash

# Simple RViz Launcher (avoids snap library conflicts)

source /opt/ros/humble/setup.bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/ros2_ws/install/setup.bash"
RVIZ_CONFIG="$SCRIPT_DIR/ros2_ws/src/mars_rover_navigation/config/lunar_habitat_visualization.rviz"

echo "🖥️  Launching RViz2 for Path Visualization..."
echo ""

# Launch without snap conflicts
env -u LD_LIBRARY_PATH rviz2 -d "$RVIZ_CONFIG" &

echo ""
echo "✅ RViz2 launched!"
echo ""
echo "To visualize the rover path:"
echo "1. Click 'Add' button (bottom left)"
echo "2. Select 'By topic' tab"
echo "3. Add: /rover_path (Path)"
echo "4. Add: /plan (Path) and /local_plan (Path)"
echo "5. Add: /detected_objects (MarkerArray)"
echo "6. Add: /lidar (LaserScan)"
echo "7. Keep Fixed Frame as 'odom'"
echo ""
