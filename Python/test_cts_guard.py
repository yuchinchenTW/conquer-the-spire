"""The guard that stops a run whose wins have fallen away.

    python -m unittest discover -s Python -p "test_cts_*.py"
"""

import contextlib
import io
import os
import tempfile
import unittest
from types import SimpleNamespace

import torch

from cts_train import WELL_OVER, Trainer


def climber(guard=0.5, least=0.08):
    """A trainer with nothing but what the guard reads and writes."""
    trainer = Trainer.__new__(Trainer)
    trainer.net = torch.nn.Linear(2, 1)
    trainer.opt = torch.optim.Adam(trainer.net.parameters())
    trainer.args = SimpleNamespace(character="ironclad", acts=3, width=2,
                                   net="card", gamma=.999, hp_weight=.01,
                                   max_hp_weight=-1., curse_penalty=-1.,
                                   guard=guard, guard_least=least)
    for key in ("updates", "steps", "episodes", "floats", "id_count",
                "actions", "bestScore", "bestAt", "bestFloors", "deep",
                "rate", "pressure", "decayedAt", "wellAt"):
        setattr(trainer, key, 0)
    trainer.scores = []
    trainer.pressures = [1., 1., 1.]
    trainer.wins = []
    trainer.wellest = 0.0
    trainer.stopping = False

    return trainer


def feed(trainer, shares):
    """Reports \\p shares of climbs won, one report each, quietly."""
    with contextlib.redirect_stdout(io.StringIO()) as said:
        for share in shares:
            trainer.updates += 1
            trainer.watch(share)

    return said.getvalue()


class GuardTests(unittest.TestCase):
    def setUp(self):
        self.room = tempfile.TemporaryDirectory()
        self.addCleanup(self.room.cleanup)

    def climber(self, **how):
        trainer = climber(**how)
        trainer.folder = self.room.name

        return trainer

    def test_a_short_run_is_never_stopped(self):
        # Nothing is known until the window is full, however bad it looks.
        trainer = self.climber()
        feed(trainer, [0.0] * (WELL_OVER - 1))
        self.assertFalse(trainer.stopping)
        self.assertEqual(trainer.wellest, 0.0)

    def test_the_healthiest_moment_is_kept(self):
        trainer = self.climber()
        said = feed(trainer, [0.2] * WELL_OVER)
        self.assertAlmostEqual(trainer.wellest, 0.2)
        self.assertEqual(trainer.wellAt, WELL_OVER)
        self.assertIn("well.pt", said)
        kept = torch.load(trainer.well, weights_only=False)
        self.assertEqual(kept["wellest"], 0.2)
        self.assertEqual(kept["updates"], WELL_OVER)

        # A better stretch writes over it; a worse one leaves it alone.
        feed(trainer, [0.3] * WELL_OVER)
        self.assertAlmostEqual(trainer.wellest, 0.3)
        at = trainer.wellAt
        feed(trainer, [0.25] * WELL_OVER)
        self.assertAlmostEqual(trainer.wellest, 0.3)
        self.assertEqual(trainer.wellAt, at)
        self.assertFalse(trainer.stopping)

    def test_wins_falling_under_half_stops_the_run(self):
        trainer = self.climber()
        feed(trainer, [0.2] * WELL_OVER)
        # 19% to 0.1% was the real collapse; this is the same shape.
        said = feed(trainer, [0.001] * WELL_OVER)
        self.assertTrue(trainer.stopping)
        self.assertIn("fallen away", said)
        self.assertIn("20.0%", said)

    def test_a_dip_that_is_not_a_collapse_is_left_alone(self):
        trainer = self.climber()
        feed(trainer, [0.2] * WELL_OVER)
        # A bad patch of a fifth of the window, and the rest as before.
        feed(trainer, [0.0] * (WELL_OVER // 5) + [0.2] * WELL_OVER)
        self.assertFalse(trainer.stopping)

    def test_the_least_best_keeps_it_quiet_early(self):
        # An act limit means the wins are nearly nothing for a long while,
        # and nothing halving is not news.
        trainer = self.climber()
        feed(trainer, [0.04] * WELL_OVER)
        feed(trainer, [0.0] * WELL_OVER)
        self.assertFalse(trainer.stopping)
        self.assertAlmostEqual(trainer.wellest, 0.04)

    def test_guard_zero_never_stops(self):
        trainer = self.climber(guard=0.0)
        feed(trainer, [0.2] * WELL_OVER)
        feed(trainer, [0.0] * WELL_OVER)
        self.assertFalse(trainer.stopping)
        # And it still keeps the healthy weights, which cost nothing.
        self.assertTrue(os.path.exists(trainer.well))

    def test_a_deeper_fall_can_be_asked_for(self):
        trainer = self.climber(guard=0.3)
        feed(trainer, [0.2] * WELL_OVER)
        feed(trainer, [0.1] * WELL_OVER)        # half: not enough now
        self.assertFalse(trainer.stopping)
        feed(trainer, [0.02] * WELL_OVER)       # a tenth: enough
        self.assertTrue(trainer.stopping)

    def test_an_inherited_mark_writes_the_weights_it_stands_on(self):
        # A checkpoint carries the high mark but the experiment bats copy
        # only the checkpoint, so a run can start guarded against a mark
        # with no well.pt behind it. The weights it starts from are where
        # that mark was set, so they become it.
        trainer = self.climber()
        trainer.wellest = 0.2
        trainer.wellAt = 1234
        self.assertFalse(os.path.exists(trainer.well))
        trainer.save(trainer.well)
        self.assertTrue(os.path.exists(trainer.well))
        feed(trainer, [0.0] * WELL_OVER)
        self.assertTrue(trainer.stopping)

    def test_carried_over_a_restart(self):
        trainer = self.climber()
        feed(trainer, [0.2] * WELL_OVER)
        trainer.save()
        kept = torch.load(trainer.checkpoint, weights_only=False)
        self.assertAlmostEqual(kept["wellest"], 0.2)
        self.assertEqual(len(kept["wins"]), WELL_OVER)

        # The next run picks the high mark up, so it is guarded against
        # what the last one managed and not against its own first report.
        again = self.climber()
        again.wellest = float(kept["wellest"])
        again.wellAt = int(kept["well_at"])
        again.wins = list(kept["wins"])
        self.assertAlmostEqual(again.wellest, 0.2)
        feed(again, [0.0] * (WELL_OVER // 4))
        self.assertFalse(again.stopping)      # the window still remembers
        feed(again, [0.0] * WELL_OVER)
        self.assertTrue(again.stopping)


if __name__ == "__main__":
    unittest.main()
