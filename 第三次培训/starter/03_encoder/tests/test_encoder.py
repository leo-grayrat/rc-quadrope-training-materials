import math
import unittest

from encoder import update_encoder


def deg(value):
    return math.radians(value)


class EncoderTests(unittest.TestCase):
    def test_no_wrap(self):
        turns, position = update_encoder(deg(40), deg(65), 0)
        self.assertEqual(turns, 0)
        self.assertAlmostEqual(position, deg(65))

    def test_low_to_high_decrements_turn(self):
        turns, position = update_encoder(deg(10), deg(350), 0)
        self.assertEqual(turns, -1)
        self.assertAlmostEqual(position, deg(-10))

    def test_high_to_low_increments_turn(self):
        turns, position = update_encoder(deg(350), deg(10), 0)
        self.assertEqual(turns, 1)
        self.assertAlmostEqual(position, deg(370))

    def test_existing_turn_count_is_preserved(self):
        turns, position = update_encoder(deg(120), deg(140), 2)
        self.assertEqual(turns, 2)
        self.assertAlmostEqual(position, deg(860))

    def test_two_successive_crossings(self):
        turns, position1 = update_encoder(deg(350), deg(10), 0)
        turns, position2 = update_encoder(deg(10), deg(350), turns)
        self.assertEqual(turns, 0)
        self.assertAlmostEqual(position1, deg(370))
        self.assertAlmostEqual(position2, deg(350))


if __name__ == "__main__":
    unittest.main()
