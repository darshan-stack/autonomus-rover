cmake_minimum_required(VERSION 3.8)
project(mars_rover_navigation)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(PCL REQUIRED)
find_package(pcl_conversions REQUIRED)

# Include directories
include_directories(
  include
  ${PCL_INCLUDE_DIRS}
)

# Object detection node
add_executable(object_detector src/object_detector.cpp)
ament_target_dependencies(object_detector
  rclcpp
  sensor_msgs
  geometry_msgs
  visualization_msgs
  pcl_conversions
)
target_link_libraries(object_detector ${PCL_LIBRARIES})

# Enhanced object detection node
add_executable(enhanced_object_detector src/enhanced_object_detector.cpp)
ament_target_dependencies(enhanced_object_detector
  rclcpp
  sensor_msgs
  geometry_msgs
  nav_msgs
  visualization_msgs
  pcl_conversions
)
target_link_libraries(enhanced_object_detector ${PCL_LIBRARIES})

# Environmental monitoring node
add_executable(environmental_monitor src/environmental_monitor.cpp)
ament_target_dependencies(environmental_monitor
  rclcpp
  sensor_msgs
)

# Path tracking node
add_executable(path_tracker src/path_tracker.cpp)
ament_target_dependencies(path_tracker
  rclcpp
  nav_msgs
  geometry_msgs
)

# Autonomous patrol node
add_executable(autonomous_patrol src/autonomous_patrol.cpp)
ament_target_dependencies(autonomous_patrol
  rclcpp
  geometry_msgs
  nav_msgs
  sensor_msgs
)

# HQ Control Panel node
add_executable(hq_control_panel src/hq_control_panel.cpp)
ament_target_dependencies(hq_control_panel
  rclcpp
  nav_msgs
  visualization_msgs
)

# Install executables
install(TARGETS
  object_detector
  enhanced_object_detector
  environmental_monitor
  path_tracker
  autonomous_patrol
  hq_control_panel
  DESTINATION lib/${PROJECT_NAME}
)

# Install launch files
install(DIRECTORY
  launch
  config
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
