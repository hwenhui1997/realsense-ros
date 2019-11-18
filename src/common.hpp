#pragma once
#include <deque>

#include <Eigen/Dense>
#include <librealsense2/rs.hpp>
#include <opencv2/opencv.hpp>

#include <cv_bridge/cv_bridge.h>
#include <geometry_msgs/Vector3Stamped.h>
#include <ros/ros.h>
#include <sensor_msgs/Imu.h>

#define __FILENAME__                                                           \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define FATAL(M, ...)                                                          \
  fprintf(stdout, "\033[31m[FATAL] [%s:%d] " M "\033[0m\n", __FILENAME__,      \
          __LINE__, ##__VA_ARGS__);                                            \
  exit(-1)

#define ROS_GET_PARAM(X, Y)                                                    \
  if (nh.getParam(X, Y) == false) {                                            \
    std::cerr << "Failed to get ROS param [" << X << "]!" << std::endl;        \
    exit(-1);                                                                  \
  }


enum image_type { ir= 0, depth = 1 ,rgb = 2};



static inline uint64_t str2ts(const std::string &s) {
  uint64_t ts = 0;
  size_t end = s.length() - 1;

  int idx = 0;
  for (int i = 0; i <= end; i++) {
    const char c = s.at(end - i);

    if (c != '.') {
      const uint64_t base = static_cast<uint64_t>(pow(10, idx));
      ts += std::atoi(&c) * base;
      idx++;
    }
  }

  return ts;
}

static cv::Mat frame2cvmat(const rs2::frame &frame, const int width,
                           const int height,const image_type img_type)
{
    const cv::Size size(width, height);
    const auto stride = cv::Mat::AUTO_STEP;
    if(img_type == image_type::ir)
        return cv::Mat(size, CV_8UC1, (void *)frame.get_data(), stride);
    else if(img_type == image_type::depth)
        return cv::Mat(size, CV_16U, (void *)frame.get_data(), stride);
    else if(img_type == image_type::rgb)
        return cv::Mat(size, CV_8UC3, (void *)frame.get_data(), stride);
}

//计算正确的图片时间戳
static uint64_t vframe2ts(const rs2::video_frame &vf) {
    // -- 图片meta时间戳
    const auto frame_meta_key = RS2_FRAME_METADATA_FRAME_TIMESTAMP;
    const auto frame_ts_us = vf.get_frame_metadata(frame_meta_key);
    const auto frame_ts_ns = static_cast<uint64_t>(frame_ts_us) * 1000;
    // -- imu的打戳时间
    const auto sensor_meta_key = RS2_FRAME_METADATA_SENSOR_TIMESTAMP;
    const auto sensor_ts_us = vf.get_frame_metadata(sensor_meta_key);
    const auto sensor_ts_ns = static_cast<uint64_t>(sensor_ts_us) * 1000;
    // -- 一半的曝光时间
    const auto half_exposure_time_ns = frame_ts_ns - sensor_ts_ns;

    // 图片的正确时间戳
    const auto ts_ms = vf.get_timestamp();//这个是图片的
    const auto ts_ns = str2ts(std::to_string(ts_ms));
    const auto ts_corrected_ns = ts_ns - half_exposure_time_ns;

    return static_cast<uint64_t>(ts_corrected_ns);
}



//创建图片消息
static sensor_msgs::ImagePtr create_image_msg(const rs2::video_frame &vf,
                                              const std::string &frame_id,const image_type img_type)
{
    const uint64_t ts_ns = vframe2ts(vf);
    ros::Time msg_stamp;
    msg_stamp.fromNSec(ts_ns);

    std_msgs::Header header;
    header.frame_id = frame_id;
    header.stamp = msg_stamp;

    const int width = vf.get_width();
    const int height = vf.get_height();
    cv::Mat cv_frame = frame2cvmat(vf, width, height,img_type);
    if(img_type == image_type::ir)
        return cv_bridge::CvImage(header, "mono8", cv_frame).toImageMsg();
    else if(img_type == image_type::depth)
        return cv_bridge::CvImage(header, "mono16", cv_frame).toImageMsg();
    else if(img_type == image_type::rgb)
        return cv_bridge::CvImage(header, "rgb8", cv_frame).toImageMsg();
}

static geometry_msgs::Vector3Stamped
create_vec3_msg(const rs2::motion_frame &f, const std::string &frame_id) {
  // Form msg stamp
  double ts_s = f.get_timestamp() * 1e-3;
  ros::Time stamp;
  stamp.fromSec(ts_s);
  // printf("[%s]: %.9f\n", frame_id.c_str(), ts_s);

  // Form msg header
  std_msgs::Header header;
  header.frame_id = frame_id;
  header.stamp = stamp;

  // Form msg
  const rs2_vector data = f.get_motion_data();
  geometry_msgs::Vector3Stamped msg;
  msg.header = header;
  msg.vector.x = data.x;
  msg.vector.y = data.y;
  msg.vector.z = data.z;

  return msg;
}

static sensor_msgs::Imu create_imu_msg(const double ts,
                                       const Eigen::Vector3d &gyro,
                                       const Eigen::Vector3d &accel) {
  sensor_msgs::Imu msg;

  msg.header.frame_id = "imu0";
  msg.header.stamp = ros::Time{ts};
  msg.angular_velocity.x = gyro(0);
  msg.angular_velocity.y = gyro(1);
  msg.angular_velocity.z = gyro(2);
  msg.linear_acceleration.x = accel(0);
  msg.linear_acceleration.y = accel(1);
  msg.linear_acceleration.z = accel(2);

  return msg;
}

static void debug_imshow(const cv::Mat &frame_left,
                         const cv::Mat &frame_right) {
  // Display in a GUI
  cv::Mat frame;
  cv::hconcat(frame_left, frame_right, frame);
  cv::namedWindow("Stereo Module", cv::WINDOW_AUTOSIZE);
  cv::imshow("Stereo Module", frame);

  if (cv::waitKey(1) == 'q') {
    exit(-1);
  }
}
