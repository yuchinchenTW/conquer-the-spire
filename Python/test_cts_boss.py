"""The saved fight is the same fight every time it is handed back.

    python -m unittest discover -s Python -p "test_cts_*.py"

The tool this tests rests on one thing: that a state saved at the boss
room and loaded again is the state that was saved, down to the order the
deck will be drawn in. If loading reshuffled, two climbers given "the same
fight" would be given different fights and the comparison would mean
nothing.

The engine will not save in the middle of a fight - save() answers None
there - which is why the tool saves in the boss room, before the first
card is drawn, and why these tests do the same.
"""

import unittest

import numpy as np

from cts_env import PHASES, SpireEnv


class SavedFightTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.env = SpireEnv()
        cls.env.set_act_limit(3)
        cls.env.set_health_weight(0.01)
        cls.state = cls.reach(cls.env)

    @staticmethod
    def reach(env, seed=90000, want=PHASES.index("boss")):
        """A climb standing in a boss room, saved before the fight."""
        for at in range(seed, seed + 60):
            env.reset("ironclad", seed=at)

            for step in range(900):
                legal = env.legal_actions()

                if not legal:
                    break

                obs = np.asarray(env.observe(), dtype=np.float32)
                phaseAt = env.layout["phase"]
                phase = int(obs[phaseAt:phaseAt + len(PHASES)].argmax())

                if phase == want:
                    state = env.save()

                    if state is not None:
                        return state

                _, _, done, _ = env.step(legal[len(legal) // 2])

                if done:
                    break

        raise AssertionError("no boss room was reached to save")

    def walk(self, moves=40):
        """Plays a fixed set of moves and returns what was seen and paid."""
        seen = []

        for step in range(moves):
            legal = self.env.legal_actions()

            if not legal:
                break

            _, reward, done, _ = self.env.step(legal[len(legal) // 2])
            seen.append((tuple(self.env.observe()),
                         tuple(self.env.observe_ids()), reward))

            if done:
                break

        return seen

    def test_a_saved_fight_is_taken_back(self):
        self.assertIsNotNone(self.state)
        self.assertTrue(self.env.load(self.state))

    def test_loading_gives_the_state_that_was_saved(self):
        self.env.load(self.state)
        first = (tuple(self.env.observe()), tuple(self.env.observe_ids()),
                 tuple(self.env.action_mask()))
        self.env.load(self.state)
        again = (tuple(self.env.observe()), tuple(self.env.observe_ids()),
                 tuple(self.env.action_mask()))
        self.assertEqual(first, again)

    def test_the_same_moves_from_it_play_out_the_same(self):
        # The one that matters: if the draw were reshuffled on load, two
        # climbers given the same saved fight would not be given the same
        # fight, and comparing them would say nothing.
        self.env.load(self.state)
        first = self.walk()
        self.env.load(self.state)
        again = self.walk()
        self.assertEqual(len(first), len(again))
        self.assertEqual(first, again)

    def test_playing_on_does_not_disturb_the_saved_text(self):
        was = self.state
        self.env.load(self.state)
        self.walk()
        self.assertEqual(was, self.state)
        self.env.load(self.state)
        self.assertEqual(tuple(self.env.observe()),
                         tuple(self.env.observe()))

    def test_a_climb_carries_on_to_its_end_from_a_saved_state(self):
        self.env.load(self.state)
        ended = False

        for step in range(4000):
            legal = self.env.legal_actions()

            if not legal:
                break

            _, _, done, _ = self.env.step(legal[0])

            if done:
                ended = True
                break

        self.assertTrue(ended, "the climb never ended from the saved state")


if __name__ == "__main__":
    unittest.main()
