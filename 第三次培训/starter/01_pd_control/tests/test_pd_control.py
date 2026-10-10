import unittest

from pd_control import pd_torque


class PDControlTests(unittest.TestCase):
    def test_general_pd(self):
        self.assertAlmostEqual(
            pd_torque(0.2, -0.3, 0.5, 0.1, 8.0, 1.5, 0.4, 100.0),
            3.4,
        )

    def test_damping_mode(self):
        self.assertAlmostEqual(
            pd_torque(9.0, 2.0, -4.0, 0.0, 0.0, 3.0, 0.0, 100.0),
            -6.0,
        )

    def test_feedforward_only(self):
        self.assertAlmostEqual(
            pd_torque(1.0, 2.0, -5.0, 7.0, 0.0, 0.0, 2.75, 100.0),
            2.75,
        )

    def test_positive_limit(self):
        self.assertAlmostEqual(
            pd_torque(0.0, 0.0, 10.0, 0.0, 20.0, 0.0, 0.0, 12.0),
            12.0,
        )

    def test_negative_limit(self):
        self.assertAlmostEqual(
            pd_torque(10.0, 0.0, 0.0, 0.0, 20.0, 0.0, 0.0, 12.0),
            -12.0,
        )


if __name__ == "__main__":
    unittest.main()
