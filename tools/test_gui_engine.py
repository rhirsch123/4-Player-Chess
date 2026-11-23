import unittest

try:
    from .engine_client import EngineClient
except ImportError:
    from engine_client import EngineClient


class GuiEngineSequenceTest(unittest.TestCase):
    def test_g2g4_b7d7_reaches_yellow_with_moves(self):
        engine = EngineClient.find_default()
        engine.start()
        self.addCleanup(engine.close)

        engine.set_position(moves=["g2g4", "b7d7"])

        self.assertEqual(engine.get_turn(), "Y")
        moves = engine.legal_moves()
        self.assertGreater(len(moves), 0)
        self.assertIn("g13g12", moves)
        self.assertIn("g13g11", moves)


if __name__ == "__main__":
    unittest.main()
