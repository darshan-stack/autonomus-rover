#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/laser_scan.hpp>
#include <geometry_msgs/msg/point.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <visualization_msgs/msg/marker_array.hpp>
#include <std_msgs/msg/string.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <vector>
#include <cmath>
#include <sstream>
#include <iomanip>

struct DetectedObject {
  int id;
  std::string name;
  std::string type;
  geometry_msgs::msg::Point position;
  geometry_msgs::msg::Point global_position;
  double size;
  double distance;
  int point_count;
  rclcpp::Time first_seen;
  rclcpp::Time last_seen;
};

class EnhancedObjectDetector : public rclcpp::Node
{
public:
  EnhancedObjectDetector() : Node("enhanced_object_detector"), object_id_counter_(0)
  {
    // Subscribe to lidar data
    scan_sub_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
      "/lidar", 10,
      std::bind(&EnhancedObjectDetector::scanCallback, this, std::placeholders::_1));
    
    // Subscribe to odometry for global position
    odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
      "/odom", 10,
      std::bind(&EnhancedObjectDetector::odomCallback, this, std::placeholders::_1));
    
    // Publishers
    objects_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>(
      "/detected_objects", 10);
    
    object_info_pub_ = this->create_publisher<std_msgs::msg::String>(
      "/object_information", 10);
    
    telemetry_pub_ = this->create_publisher<std_msgs::msg::String>(
      "/hq_telemetry", 10);
    
    // Parameters
    this->declare_parameter("min_cluster_size", 3);
    this->declare_parameter("max_cluster_distance", 0.5);
    this->declare_parameter("rock_size_threshold", 0.4);
    this->declare_parameter("large_obstacle_threshold", 1.0);
    
    // Timer for periodic telemetry
    telemetry_timer_ = this->create_wall_timer(
      std::chrono::seconds(5),
      std::bind(&EnhancedObjectDetector::publishTelemetry, this));
    
    current_x_ = 0.0;
    current_y_ = 0.0;
    current_yaw_ = 0.0;
    
    RCLCPP_INFO(this->get_logger(), "Enhanced Object Detector Node Started");
  }

private:
  void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
  {
    current_x_ = msg->pose.pose.position.x;
    current_y_ = msg->pose.pose.position.y;
    
    // Extract yaw from quaternion
    double siny_cosp = 2 * (msg->pose.pose.orientation.w * msg->pose.pose.orientation.z +
                             msg->pose.pose.orientation.x * msg->pose.pose.orientation.y);
    double cosy_cosp = 1 - 2 * (msg->pose.pose.orientation.y * msg->pose.pose.orientation.y +
                                 msg->pose.pose.orientation.z * msg->pose.pose.orientation.z);
    current_yaw_ = std::atan2(siny_cosp, cosy_cosp);
  }
  
  void scanCallback(const sensor_msgs::msg::LaserScan::SharedPtr msg)
  {
    std::vector<std::vector<geometry_msgs::msg::Point>> clusters;
    std::vector<geometry_msgs::msg::Point> current_cluster;
    
    int min_cluster_size = this->get_parameter("min_cluster_size").as_int();
    double max_cluster_distance = this->get_parameter("max_cluster_distance").as_double();
    
    geometry_msgs::msg::Point last_point;
    bool first_point = true;
    
    // Convert laser scan to points and cluster them
    for (size_t i = 0; i < msg->ranges.size(); ++i)
    {
      float range = msg->ranges[i];
      
      // Skip invalid readings
      if (std::isnan(range) || std::isinf(range) || 
          range < msg->range_min || range > msg->range_max)
        continue;
      
      // Convert polar to Cartesian coordinates
      float angle = msg->angle_min + i * msg->angle_increment;
      geometry_msgs::msg::Point point;
      point.x = range * cos(angle);
      point.y = range * sin(angle);
      point.z = 0.0;
      
      if (first_point)
      {
        current_cluster.push_back(point);
        last_point = point;
        first_point = false;
        continue;
      }
      
      // Calculate distance to last point
      double distance = sqrt(
        pow(point.x - last_point.x, 2) + 
        pow(point.y - last_point.y, 2)
      );
      
      if (distance < max_cluster_distance)
      {
        current_cluster.push_back(point);
      }
      else
      {
        if (current_cluster.size() >= (size_t)min_cluster_size)
        {
          clusters.push_back(current_cluster);
        }
        current_cluster.clear();
        current_cluster.push_back(point);
      }
      
      last_point = point;
    }
    
    if (current_cluster.size() >= (size_t)min_cluster_size)
    {
      clusters.push_back(current_cluster);
    }
    
    // Process and classify objects
    detected_objects_.clear();
    processAndClassifyObjects(clusters, msg->header);
    
    // Publish visualization and information
    publishObjects(msg->header);
    publishObjectInformation();
    
    RCLCPP_INFO(this->get_logger(), "Detected %zu objects", detected_objects_.size());
  }
  
  void processAndClassifyObjects(
    const std::vector<std::vector<geometry_msgs::msg::Point>>& clusters,
    const std_msgs::msg::Header& header)
  {
    double rock_threshold = this->get_parameter("rock_size_threshold").as_double();
    double large_threshold = this->get_parameter("large_obstacle_threshold").as_double();
    
    for (size_t i = 0; i < clusters.size(); ++i)
    {
      DetectedObject obj;
      obj.id = object_id_counter_++;
      obj.point_count = clusters[i].size();
      obj.first_seen = this->now();
      obj.last_seen = this->now();
      
      // Calculate centroid
      obj.position.x = 0; obj.position.y = 0; obj.position.z = 0;
      for (const auto& point : clusters[i])
      {
        obj.position.x += point.x;
        obj.position.y += point.y;
      }
      obj.position.x /= clusters[i].size();
      obj.position.y /= clusters[i].size();
      
      // Calculate size (approximate diameter)
      double min_x = 1e6, max_x = -1e6, min_y = 1e6, max_y = -1e6;
      for (const auto& point : clusters[i])
      {
        min_x = std::min(min_x, (double)point.x);
        max_x = std::max(max_x, (double)point.x);
        min_y = std::min(min_y, (double)point.y);
        max_y = std::max(max_y, (double)point.y);
      }
      obj.size = std::max(max_x - min_x, max_y - min_y);
      
      // Calculate distance from rover
      obj.distance = sqrt(obj.position.x * obj.position.x + 
                         obj.position.y * obj.position.y);
      
      // Transform to global coordinates
      obj.global_position.x = current_x_ + obj.position.x * cos(current_yaw_) - 
                              obj.position.y * sin(current_yaw_);
      obj.global_position.y = current_y_ + obj.position.x * sin(current_yaw_) + 
                              obj.position.y * cos(current_yaw_);
      obj.global_position.z = 0.0;
      
      // Classify object based on size
      if (obj.size < rock_threshold)
      {
        obj.type = "ROCK";
        obj.name = "Rock_" + std::to_string(obj.id);
      }
      else if (obj.size < large_threshold)
      {
        obj.type = "BOULDER";
        obj.name = "Boulder_" + std::to_string(obj.id);
      }
      else
      {
        obj.type = "STRUCTURE";
        obj.name = "Structure_" + std::to_string(obj.id);
      }
      
      detected_objects_.push_back(obj);
    }
  }
  
  void publishObjects(const std_msgs::msg::Header& header)
  {
    visualization_msgs::msg::MarkerArray marker_array;
    
    for (const auto& obj : detected_objects_)
    {
      // Create object marker
      visualization_msgs::msg::Marker marker;
      marker.header.frame_id = "base_link";
      marker.header.stamp = this->now();
      marker.ns = "detected_objects";
      marker.id = obj.id;
      marker.type = visualization_msgs::msg::Marker::CYLINDER;
      marker.action = visualization_msgs::msg::Marker::ADD;
      marker.pose.position = obj.position;
      marker.pose.position.z = obj.size / 2.0;
      marker.pose.orientation.w = 1.0;
      marker.scale.x = obj.size;
      marker.scale.y = obj.size;
      marker.scale.z = obj.size;
      
      // Color based on type
      if (obj.type == "ROCK")
      {
        marker.color.r = 0.7; marker.color.g = 0.5; marker.color.b = 0.3;
      }
      else if (obj.type == "BOULDER")
      {
        marker.color.r = 0.5; marker.color.g = 0.5; marker.color.b = 0.5;
      }
      else
      {
        marker.color.r = 0.9; marker.color.g = 0.1; marker.color.b = 0.1;
      }
      marker.color.a = 0.8;
      marker.lifetime = rclcpp::Duration::from_seconds(0.5);
      
      marker_array.markers.push_back(marker);
      
      // Create text label
      visualization_msgs::msg::Marker text_marker;
      text_marker.header = marker.header;
      text_marker.ns = "object_labels";
      text_marker.id = obj.id + 10000;
      text_marker.type = visualization_msgs::msg::Marker::TEXT_VIEW_FACING;
      text_marker.action = visualization_msgs::msg::Marker::ADD;
      text_marker.pose.position = obj.position;
      text_marker.pose.position.z = obj.size + 0.3;
      text_marker.scale.z = 0.2;
      text_marker.color.r = 1.0;
      text_marker.color.g = 1.0;
      text_marker.color.b = 1.0;
      text_marker.color.a = 1.0;
      text_marker.text = obj.name;
      text_marker.lifetime = rclcpp::Duration::from_seconds(0.5);
      
      marker_array.markers.push_back(text_marker);
    }
    
    objects_pub_->publish(marker_array);
  }
  
  void publishObjectInformation()
  {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "=== DETECTED OBJECTS ===" << "\n";
    ss << "Total Objects: " << detected_objects_.size() << "\n\n";
    
    for (const auto& obj : detected_objects_)
    {
      ss << "Object: " << obj.name << "\n";
      ss << "  Type: " << obj.type << "\n";
      ss << "  Distance: " << obj.distance << " m\n";
      ss << "  Size: " << obj.size << " m\n";
      ss << "  Local Pos: (" << obj.position.x << ", " << obj.position.y << ")\n";
      ss << "  Global Pos: (" << obj.global_position.x << ", " << obj.global_position.y << ")\n";
      ss << "  Points: " << obj.point_count << "\n\n";
    }
    
    std_msgs::msg::String msg;
    msg.data = ss.str();
    object_info_pub_->publish(msg);
  }
  
  void publishTelemetry()
  {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    auto now = this->now();
    
    ss << "=== HQ TELEMETRY REPORT ===" << "\n";
    ss << "Timestamp: " << now.seconds() << "\n";
    ss << "Rover Position: (" << current_x_ << ", " << current_y_ << ")\n";
    ss << "Rover Heading: " << (current_yaw_ * 180.0 / M_PI) << " deg\n";
    ss << "Objects in View: " << detected_objects_.size() << "\n";
    ss << "Status: OPERATIONAL\n";
    
    std_msgs::msg::String msg;
    msg.data = ss.str();
    telemetry_pub_->publish(msg);
  }
  
  rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr scan_sub_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr objects_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr object_info_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr telemetry_pub_;
  rclcpp::TimerBase::SharedPtr telemetry_timer_;
  
  std::vector<DetectedObject> detected_objects_;
  int object_id_counter_;
  double current_x_, current_y_, current_yaw_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<EnhancedObjectDetector>());
  rclcpp::shutdown();
  return 0;
}
