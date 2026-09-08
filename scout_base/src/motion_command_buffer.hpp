// Copyright 2026 Hive Matrix AI
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <chrono>
#include <cmath>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

#include "agilex_ugv_sdk/core/commands.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "rclcpp/time.hpp"

namespace scout_base
{

class MotionCommandBuffer
{
public:
  using SteadyClock = std::chrono::steady_clock;

  MotionCommandBuffer(bool omni, std::string base_frame, double timeout_s)
  : omni_(omni), base_frame_(std::move(base_frame)), timeout_s_(timeout_s)
  {
    if (!std::isfinite(timeout_s_) || timeout_s_ <= 0.0) {
      throw std::invalid_argument("cmd_vel_timeout must be finite and positive");
    }
  }

  bool update(
    const geometry_msgs::msg::Twist & message, const rclcpp::Time & received_at,
    SteadyClock::time_point received_steady = SteadyClock::now())
  {
    return store(message, received_at, received_steady);
  }

  bool update(
    const geometry_msgs::msg::TwistStamped & message, const rclcpp::Time & received_at,
    SteadyClock::time_point received_steady = SteadyClock::now())
  {
    if ((!message.header.frame_id.empty() && message.header.frame_id != base_frame_) ||
      message.header.stamp.sec < 0 || message.header.stamp.nanosec >= 1'000'000'000U)
    {
      return false;
    }
    const bool unstamped = message.header.stamp.sec == 0 && message.header.stamp.nanosec == 0;
    const auto stamped_at = unstamped ? received_at :
      rclcpp::Time(message.header.stamp, received_at.get_clock_type());
    const double age_s = (received_at - stamped_at).seconds();
    if (age_s < 0.0 || age_s > timeout_s_) {
      return false;
    }
    return store(message.twist, stamped_at, received_steady);
  }

  agilex::ugv::MotionCommand command(
    const rclcpp::Time & now,
    SteadyClock::time_point steady_now = SteadyClock::now()) const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!has_command_ || now.get_clock_type() != stamped_at_.get_clock_type()) {
      return {};
    }
    const double age_s = (now - stamped_at_).seconds();
    const double elapsed_s = std::chrono::duration<double>(steady_now - received_steady_).count();
    if (age_s < 0.0 || age_s > timeout_s_ || elapsed_s < 0.0 || elapsed_s > timeout_s_) {
      return {};
    }
    return latest_command_;
  }

private:
  bool store(
    const geometry_msgs::msg::Twist & message, const rclcpp::Time & stamped_at,
    SteadyClock::time_point received_steady)
  {
    if (!std::isfinite(message.linear.x) || !std::isfinite(message.angular.z) ||
      (omni_ && !std::isfinite(message.linear.y)))
    {
      return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    latest_command_ = {message.linear.x, message.angular.z, omni_ ? message.linear.y : 0.0};
    stamped_at_ = stamped_at;
    received_steady_ = received_steady;
    has_command_ = true;
    return true;
  }

  const bool omni_;
  const std::string base_frame_;
  const double timeout_s_;
  mutable std::mutex mutex_;
  agilex::ugv::MotionCommand latest_command_;
  rclcpp::Time stamped_at_{0, 0, RCL_ROS_TIME};
  SteadyClock::time_point received_steady_;
  bool has_command_{false};
};

}  // namespace scout_base
