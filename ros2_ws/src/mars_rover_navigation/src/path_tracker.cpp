#include <rclcpp/rclcpp.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <nav_msgs/msg/path.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <std_msgs/msg/string.hpp>
#include <vector>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <chrono>

struct PathPoint {
  double x;
  double y;
  double yaw;
  rclcpp::Time timestamp;
  double linear_velocity;
  double angular_velocity;
};

class PathTracker : public rclcpp::Node
{
public:
  PathTracker() : Node("path_tracker"), total_distance_(0.0)
  {
    // Subscribe to odometry
    odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
      "/odom", 10,
      std::bind(&PathTracker::odomCallback, this, std::placeholders::_1));
    
    // Publishers
    path_pub_ = this->create_publisher<nav_msgs::msg::Path>("/rover_path", 10);
    
    telemetry_pub_ = this->create_publisher<std_msgs::msg::String>(
      "/navigation_telemetry", 10);
    
    mission_log_pub_ = this->create_publisher<std_msgs::msg::String>(
      "/mission_log", 10);
    
    // Parameters
    this->declare_parameter("log_interval", 1.0);  // seconds
    this->declare_parameter("min_distance_threshold", 0.05);  // meters
    this->declare_parameter("log_file_path", "/tmp/rover_mission_log.txt");
    
    // Timer for periodic logging
    double log_interval = this->get_parameter("log_interval").as_double();
    log_timer_ = this->create_wall_timer(
      std::chrono::milliseconds(static_cast<int>(log_interval * 1000)),
      std::bind(&PathTracker::logCallback, this));
    
    start_time_ = this->now();
    last_x_ = 0.0;
    last_y_ = 0.0;
    
    RCLCPP_INFO(this->get_logger(), "Path Tracker Node Started");
    RCLCPP_INFO(this->get_logger(), "Mission logging initiated");
  }
  
  ~PathTracker()
  {
    // Save final mission log
    saveMissionLog();
  }

private:
  void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
  {
    double current_x = msg->pose.pose.position.x;
    double current_y = msg->pose.pose.position.y;
    
    // Extract yaw from quaternion
    double siny_cosp = 2 * (msg->pose.pose.orientation.w * msg->pose.pose.orientation.z +
                             msg->pose.pose.orientation.x * msg->pose.pose.orientation.y);
    double cosy_cosp = 1 - 2 * (msg->pose.pose.orientation.y * msg->pose.pose.orientation.y +
                                 msg->pose.pose.orientation.z * msg->pose.pose.orientation.z);
    double current_yaw = std::atan2(siny_cosp, cosy_cosp);
    
    // Check if rover has moved significantly
    double distance = sqrt(pow(current_x - last_x_, 2) + pow(current_y - last_y_, 2));
    double min_threshold = this->get_parameter("min_distance_threshold").as_double();
    
    if (distance > min_threshold || path_points_.empty())
    {
      PathPoint point;
      point.x = current_x;
      point.y = current_y;
      point.yaw = current_yaw;
      point.timestamp = this->now();
      point.linear_velocity = sqrt(pow(msg->twist.twist.linear.x, 2) +
                                   pow(msg->twist.twist.linear.y, 2));
      point.angular_velocity = msg->twist.twist.angular.z;
      
      path_points_.push_back(point);
      
      total_distance_ += distance;
      last_x_ = current_x;
      last_y_ = current_y;
      
      // Publish path visualization
      publishPath();
    }
  }
  
  void publishPath()
  {
    nav_msgs::msg::Path path_msg;
    path_msg.header.frame_id = "odom";
    path_msg.header.stamp = this->now();
    
    for (const auto& point : path_points_)
    {
      geometry_msgs::msg::PoseStamped pose;
      pose.header.frame_id = "odom";
      pose.header.stamp = point.timestamp;
      pose.pose.position.x = point.x;
      pose.pose.position.y = point.y;
      pose.pose.position.z = 0.0;
      
      // Convert yaw to quaternion
      pose.pose.orientation.x = 0.0;
      pose.pose.orientation.y = 0.0;
      pose.pose.orientation.z = sin(point.yaw / 2.0);
      pose.pose.orientation.w = cos(point.yaw / 2.0);
      
      path_msg.poses.push_back(pose);
    }
    
    path_pub_->publish(path_msg);
  }
  
  void logCallback()
  {
    if (path_points_.empty())
      return;
    
    auto current_time = this->now();
    auto duration = current_time - start_time_;
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    
    ss << "=== NAVIGATION TELEMETRY ===" << "\n";
    ss << "Mission Time: " << duration.seconds() << " s\n";
    ss << "Waypoints Logged: " << path_points_.size() << "\n";
    ss << "Total Distance: " << total_distance_ << " m\n";
    
    if (!path_points_.empty())
    {
      const auto& current = path_points_.back();
      ss << "\nCurrent Position:\n";
      ss << "  X: " << current.x << " m\n";
      ss << "  Y: " << current.y << " m\n";
      ss << "  Heading: " << (current.yaw * 180.0 / M_PI) << " deg\n";
      ss << "  Linear Vel: " << current.linear_velocity << " m/s\n";
      ss << "  Angular Vel: " << current.angular_velocity << " rad/s\n";
    }
    
    if (path_points_.size() >= 2)
    {
      double avg_velocity = total_distance_ / duration.seconds();
      ss << "\nAverage Speed: " << avg_velocity << " m/s\n";
    }
    
    ss << "\nStatus: TRACKING ACTIVE\n";
    
    std_msgs::msg::String msg;
    msg.data = ss.str();
    telemetry_pub_->publish(msg);
  }
  
  void saveMissionLog()
  {
    std::string log_path = this->get_parameter("log_file_path").as_string();
    std::ofstream log_file(log_path);
    
    if (!log_file.is_open())
    {
      RCLCPP_ERROR(this->get_logger(), "Failed to open log file: %s", log_path.c_str());
      return;
    }
    
    log_file << "=== LUNAR HABITAT ROVER MISSION LOG ===" << "\n";
    log_file << "Mission Start: " << start_time_.seconds() << "\n";
    log_file << "Mission End: " << this->now().seconds() << "\n";
    log_file << "Total Duration: " << (this->now() - start_time_).seconds() << " s\n";
    log_file << "Total Distance: " << total_distance_ << " m\n";
    log_file << "Total Waypoints: " << path_points_.size() << "\n\n";
    
    log_file << "=== PATH DATA ===" << "\n";
    log_file << "Timestamp(s), X(m), Y(m), Heading(deg), Linear_Vel(m/s), Angular_Vel(rad/s)\n";
    
    for (const auto& point : path_points_)
    {
      log_file << std::fixed << std::setprecision(3);
      log_file << point.timestamp.seconds() << ", "
               << point.x << ", "
               << point.y << ", "
               << (point.yaw * 180.0 / M_PI) << ", "
               << point.linear_velocity << ", "
               << point.angular_velocity << "\n";
    }
    
    log_file.close();
    
    RCLCPP_INFO(this->get_logger(), "Mission log saved to: %s", log_path.c_str());
    
    // Publish final summary
    std::stringstream ss;
    ss << "=== MISSION COMPLETE ===" << "\n";
    ss << "Log saved to: " << log_path << "\n";
    ss << "Total distance traveled: " << total_distance_ << " m\n";
    ss << "Total waypoints: " << path_points_.size() << "\n";
    
    std_msgs::msg::String msg;
    msg.data = ss.str();
    mission_log_pub_->publish(msg);
  }
  
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr path_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr telemetry_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr mission_log_pub_;
  rclcpp::TimerBase::SharedPtr log_timer_;
  
  std::vector<PathPoint> path_points_;
  double total_distance_;
  double last_x_, last_y_;
  rclcpp::Time start_time_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<PathTracker>());
  rclcpp::shutdown();
  return 0;
}
