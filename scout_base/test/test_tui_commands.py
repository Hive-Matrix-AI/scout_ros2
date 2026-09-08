# Copyright 2026 Hive Matrix AI
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from types import SimpleNamespace

from geometry_msgs.msg import Twist, TwistStamped
import pytest
from rclpy.time import Time

from scout_base_tools.app import parse_args, ScoutTestNode
from scout_base_tools.core import Velocity


@pytest.mark.parametrize('stamped', [False, True])
@pytest.mark.parametrize('velocity', [Velocity(0.2, 0.3, -0.1), Velocity()])
def test_velocity_message_type_values_and_stop(stamped, velocity):
    messages = []
    stamp = Time(seconds=10).to_msg()
    node = SimpleNamespace(
        stamped=stamped,
        command_frame='robot/base_link',
        command_publisher=SimpleNamespace(publish=messages.append),
        get_clock=lambda: SimpleNamespace(
            now=lambda: SimpleNamespace(to_msg=lambda: stamp)),
    )

    ScoutTestNode.publish_velocity(node, velocity)

    assert len(messages) == 1
    message = messages[0]
    assert isinstance(message, TwistStamped if stamped else Twist)
    if stamped:
        assert message.header.frame_id == 'robot/base_link'
        assert message.header.stamp == stamp
    twist = message.twist if stamped else message
    assert twist.linear.x == velocity.linear
    assert twist.linear.y == velocity.lateral
    assert twist.angular.z == velocity.angular


def test_stamped_cli_options_preserve_the_twist_default():
    defaults = parse_args([])
    assert not defaults.stamped
    assert defaults.frame_id == 'base_link'
    stamped = parse_args(['--stamped', '--frame-id', 'robot/base_link'])
    assert stamped.stamped
    assert stamped.frame_id == 'robot/base_link'
