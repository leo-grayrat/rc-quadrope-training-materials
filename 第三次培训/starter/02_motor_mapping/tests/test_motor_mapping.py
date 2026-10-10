import unittest

from motor_mapping import joint_command_to_motor


class MotorMappingTests(unittest.TestCase):
    def assert_command(self, result, expected):
        self.assertEqual(
            set(result),
            {"p_des", "omega_des", "tau_motor", "kp_motor", "kd_motor"},
        )
        for key, value in expected.items():
            self.assertAlmostEqual(result[key], value)

    def test_ratio_six(self):
        self.assert_command(
            joint_command_to_motor(0.4, -0.2, 6.0, 36.0, 3.0, 6.0),
            {
                "p_des": 2.4,
                "omega_des": -1.2,
                "tau_motor": 1.0,
                "kp_motor": 1.0,
                "kd_motor": 1.0 / 12.0,
            },
        )

    def test_ratio_three(self):
        self.assert_command(
            joint_command_to_motor(-0.5, 0.6, -9.0, 18.0, 4.5, 3.0),
            {
                "p_des": -1.5,
                "omega_des": 1.8,
                "tau_motor": -3.0,
                "kp_motor": 2.0,
                "kd_motor": 0.5,
            },
        )

    def test_noninteger_ratio(self):
        ratio = 19.0 / 3.0
        self.assert_command(
            joint_command_to_motor(0.3, 0.15, 9.5, 40.0, 8.0, ratio),
            {
                "p_des": ratio * 0.3,
                "omega_des": ratio * 0.15,
                "tau_motor": 9.5 / ratio,
                "kp_motor": 40.0 / (ratio * ratio),
                "kd_motor": 8.0 / (ratio * ratio),
            },
        )


if __name__ == "__main__":
    unittest.main()
