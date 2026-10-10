import unittest

from trajectory import linear_target


class TrajectoryTests(unittest.TestCase):
    def test_start(self):
        q, dq = linear_target(-0.4, 0.8, 0.0, 2.0)
        self.assertAlmostEqual(q, -0.4)
        self.assertAlmostEqual(dq, 0.6)

    def test_middle(self):
        q, dq = linear_target(-0.4, 0.8, 1.0, 2.0)
        self.assertAlmostEqual(q, 0.2)
        self.assertAlmostEqual(dq, 0.6)

    def test_end(self):
        q, dq = linear_target(-0.4, 0.8, 2.0, 2.0)
        self.assertAlmostEqual(q, 0.8)
        self.assertAlmostEqual(dq, 0.0)

    def test_after_end(self):
        q, dq = linear_target(1.2, -0.3, 9.0, 3.0)
        self.assertAlmostEqual(q, -0.3)
        self.assertAlmostEqual(dq, 0.0)

    def test_other_direction(self):
        q, dq = linear_target(1.0, -1.0, 0.5, 2.0)
        self.assertAlmostEqual(q, 0.5)
        self.assertAlmostEqual(dq, -1.0)


if __name__ == "__main__":
    unittest.main()
