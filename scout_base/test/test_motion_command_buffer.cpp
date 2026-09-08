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

#include <chrono>
#include <limits>

#include "gtest/gtest.h"
#include "motion_command_buffer.hpp"

namespace scout_base
{
namespace
{

using SteadyClock = MotionCommandBuffer::SteadyClock;
const SteadyClock::time_point kReceived{std::chrono::seconds{10}};

rclcpp::Time ros_time(int seconds, unsigned nanoseconds = 0)
{
  return rclcpp::Time(seconds, nanoseconds, RCL_ROS_TIME);
}

geometry_msgs::msg::Twist velocity()
{
  geometry_msgs::msg::Twist message;
  message.linear.x = 0.2;
  message.linear.y = -0.1;
  message.angular.z = 0.3;
  return message;
}

geometry_msgs::msg::TwistStamped stamped_velocity(
  int seconds = 10, unsigned nanoseconds = 0)
{
  geometry_msgs::msg::TwistStamped message;
  message.header.stamp.sec = seconds;
  message.header.stamp.nanosec = nanoseconds;
  message.header.frame_id = "base_link";
  message.twist = velocity();
  return message;
}

void expect_stopped(const agilex::ugv::MotionCommand & command)
{
  EXPECT_DOUBLE_EQ(command.linear_velocity_mps, 0.0);
  EXPECT_DOUBLE_EQ(command.angular_velocity_radps, 0.0);
  EXPECT_DOUBLE_EQ(command.lateral_velocity_mps, 0.0);
}

TEST(MotionCommandBuffer, StartsStopped)
{
  MotionCommandBuffer buffer(false, "base_link", 0.5);
  expect_stopped(buffer.command(ros_time(10), kReceived));
}

TEST(MotionCommandBuffer, TwistUsesReceiptTimeAndIgnoresSkidSteerLateralMotion)
{
  MotionCommandBuffer buffer(false, "base_link", 0.5);
  ASSERT_TRUE(buffer.update(velocity(), ros_time(10), kReceived));
  const auto command = buffer.command(ros_time(10, 500'000'000), kReceived);
  EXPECT_DOUBLE_EQ(command.linear_velocity_mps, 0.2);
  EXPECT_DOUBLE_EQ(command.angular_velocity_radps, 0.3);
  EXPECT_DOUBLE_EQ(command.lateral_velocity_mps, 0.0);
  expect_stopped(buffer.command(ros_time(10, 500'000'001), kReceived));
}

TEST(MotionCommandBuffer, BothMessagesPreserveOmniLateralMotion)
{
  MotionCommandBuffer buffer(true, "base_link", 0.5);
  ASSERT_TRUE(buffer.update(velocity(), ros_time(10), kReceived));
  EXPECT_DOUBLE_EQ(buffer.command(ros_time(10), kReceived).lateral_velocity_mps, -0.1);
  ASSERT_TRUE(buffer.update(stamped_velocity(), ros_time(10), kReceived));
  EXPECT_DOUBLE_EQ(buffer.command(ros_time(10), kReceived).lateral_velocity_mps, -0.1);
}

TEST(MotionCommandBuffer, StampedCommandsExpireFromTheirSourceTimestamp)
{
  MotionCommandBuffer buffer(false, "base_link", 0.5);
  ASSERT_TRUE(buffer.update(stamped_velocity(9, 800'000'000), ros_time(10), kReceived));
  EXPECT_DOUBLE_EQ(buffer.command(ros_time(10, 200'000'000), kReceived).linear_velocity_mps, 0.2);
  expect_stopped(buffer.command(ros_time(10, 300'000'001), kReceived));
}

TEST(MotionCommandBuffer, ZeroStampAndEmptyFrameUseReceiptTimeAndBaseFrame)
{
  MotionCommandBuffer buffer(false, "base_link", 0.5);
  auto message = stamped_velocity(0);
  message.header.frame_id.clear();
  ASSERT_TRUE(buffer.update(message, ros_time(10), kReceived));
  EXPECT_DOUBLE_EQ(buffer.command(ros_time(10), kReceived).linear_velocity_mps, 0.2);
  expect_stopped(buffer.command(ros_time(11), kReceived));
}

TEST(MotionCommandBuffer, RejectsExpiredFutureAndMalformedStamps)
{
  MotionCommandBuffer buffer(false, "base_link", 0.5);
  EXPECT_FALSE(buffer.update(stamped_velocity(9), ros_time(10), kReceived));
  EXPECT_FALSE(buffer.update(stamped_velocity(11), ros_time(10), kReceived));
  EXPECT_FALSE(buffer.update(stamped_velocity(-1), ros_time(10), kReceived));
  EXPECT_FALSE(buffer.update(stamped_velocity(10, 1'000'000'000), ros_time(10), kReceived));
  expect_stopped(buffer.command(ros_time(10), kReceived));
}

TEST(MotionCommandBuffer, OnlyAcceptsTheConfiguredBodyFrame)
{
  MotionCommandBuffer buffer(false, "robot/base_link", 0.5);
  auto message = stamped_velocity();
  EXPECT_FALSE(buffer.update(message, ros_time(10), kReceived));
  message.header.frame_id = "odom";
  EXPECT_FALSE(buffer.update(message, ros_time(10), kReceived));
  message.header.frame_id = "robot/base_link";
  EXPECT_TRUE(buffer.update(message, ros_time(10), kReceived));
}

TEST(MotionCommandBuffer, RejectedMessagesDoNotRefreshTheWatchdog)
{
  MotionCommandBuffer buffer(false, "base_link", 0.5);
  ASSERT_TRUE(buffer.update(stamped_velocity(), ros_time(10), kReceived));
  EXPECT_FALSE(
    buffer.update(
      stamped_velocity(9), ros_time(10, 400'000'000),
      kReceived + std::chrono::milliseconds{400}));
  expect_stopped(
    buffer.command(
      ros_time(10, 600'000'000),
      kReceived + std::chrono::milliseconds{600}));
}

TEST(MotionCommandBuffer, SteadyClockStopsMotionWhenRosTimeIsPaused)
{
  MotionCommandBuffer buffer(false, "base_link", 0.5);
  ASSERT_TRUE(buffer.update(stamped_velocity(), ros_time(10), kReceived));
  expect_stopped(buffer.command(ros_time(10), kReceived + std::chrono::milliseconds{501}));
}

TEST(MotionCommandBuffer, StopsOnBackwardOrDifferentRosClocks)
{
  MotionCommandBuffer buffer(false, "base_link", 0.5);
  ASSERT_TRUE(buffer.update(velocity(), ros_time(10), kReceived));
  expect_stopped(buffer.command(ros_time(9), kReceived));
  expect_stopped(buffer.command(rclcpp::Time(10, 0, RCL_SYSTEM_TIME), kReceived));
}

TEST(MotionCommandBuffer, RejectsNonFiniteVelocities)
{
  MotionCommandBuffer buffer(true, "base_link", 0.5);
  auto message = velocity();
  message.linear.x = std::numeric_limits<double>::quiet_NaN();
  EXPECT_FALSE(buffer.update(message, ros_time(10), kReceived));
  message = velocity();
  message.angular.z = std::numeric_limits<double>::infinity();
  EXPECT_FALSE(buffer.update(message, ros_time(10), kReceived));
  auto stamped = stamped_velocity();
  stamped.twist.linear.y = std::numeric_limits<double>::quiet_NaN();
  EXPECT_FALSE(buffer.update(stamped, ros_time(10), kReceived));
  expect_stopped(buffer.command(ros_time(10), kReceived));
}

TEST(MotionCommandBuffer, RejectsInvalidTimeouts)
{
  EXPECT_THROW(MotionCommandBuffer(false, "base_link", 0.0), std::invalid_argument);
  EXPECT_THROW(MotionCommandBuffer(false, "base_link", -1.0), std::invalid_argument);
  EXPECT_THROW(
    MotionCommandBuffer(
      false, "base_link",
      std::numeric_limits<double>::quiet_NaN()), std::invalid_argument);
  EXPECT_THROW(
    MotionCommandBuffer(
      false, "base_link",
      std::numeric_limits<double>::infinity()), std::invalid_argument);
}

}  // namespace
}  // namespace scout_base
