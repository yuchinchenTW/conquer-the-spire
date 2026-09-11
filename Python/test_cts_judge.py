"""Run with: python -m unittest discover -s Python -p test_cts_judge.py"""

import contextlib
import io
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

import torch

import cts_judge as judge
from cts_checkpoint import atomic_save
from cts_play import checkpoint_path


class CheckpointTests(unittest.TestCase):
    def test_trainer_save_keeps_optimizer_names_and_reward_metadata(self):
        from cts_train import Trainer

        trainer = Trainer.__new__(Trainer)
        trainer.net = torch.nn.Linear(2, 1)
        trainer.opt = torch.optim.Adam(trainer.net.parameters())
        trainer.net(torch.ones(1, 2)).sum().backward()
        trainer.opt.step()
        trainer.args = SimpleNamespace(character="IRONCLAD", acts=3, width=2,
                                       net="card", gamma=.999, hp_weight=.01,
                                       max_hp_weight=-1., curse_penalty=-1.)
        for key in ("updates", "steps", "episodes", "floats", "id_count",
                    "actions", "bestScore", "bestAt", "bestFloors", "deep",
                    "rate", "pressure", "decayedAt"):
            setattr(trainer, key, 1)
        trainer.scores = []
        trainer.pressures = [1., 1., 1.]
        with tempfile.TemporaryDirectory() as directory:
            trainer.folder = directory
            trainer.save()
            kept = torch.load(trainer.checkpoint, weights_only=False)
            self.assertEqual(kept["hp_weight"], .01)
            self.assertEqual(kept["gamma"], .999)
            self.assertIsNone(kept["curse_penalty"])
            self.assertEqual(kept["opt_names"], ["weight", "bias"])
            self.assertEqual(len(kept["opt"]["state"]), 2)
            torch.testing.assert_close(kept["net"]["weight"],
                                       trainer.net.weight)

    def test_fresh_training_archives_judged_winners(self):
        from cts_train import Trainer

        trainer = Trainer.__new__(Trainer)
        with tempfile.TemporaryDirectory() as directory:
            trainer.folder = directory
            names = ("best-flat.pt", "best-look2.pt", "judged.csv")
            for name in names:
                (Path(directory) / name).touch()
            with contextlib.redirect_stdout(io.StringIO()):
                trainer.setAside()
            for name in names:
                self.assertFalse((Path(directory) / name).exists())
                self.assertTrue((Path(directory) / "before-1" / name).exists())

    def test_failed_save_keeps_previous_checkpoint(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "checkpoint.pt"
            atomic_save({"updates": 1}, path)
            with mock.patch("cts_checkpoint.torch.save",
                            side_effect=RuntimeError):
                with self.assertRaises(RuntimeError):
                    atomic_save({"updates": 2}, path)
            self.assertEqual(
                torch.load(path, weights_only=False)["updates"], 1)
            self.assertEqual(list(Path(directory).iterdir()), [path])

    def test_snapshot_survives_source_replacement(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "checkpoint.pt"
            atomic_save({"updates": 1}, path)
            with judge.snapshot(path, directory) as (frozen, digest):
                atomic_save({"updates": 2}, path)
                self.assertEqual(
                    torch.load(frozen, weights_only=False)["updates"], 1)
                self.assertEqual(judge.digest_file(frozen), digest)
            self.assertFalse(frozen.exists())
            self.assertEqual(
                torch.load(path, weights_only=False)["updates"], 2)

    def test_rejects_same_size_archive_with_corrupt_tensor(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "checkpoint.pt"
            atomic_save({"tensor": torch.arange(20)}, path)
            data = bytearray(path.read_bytes())
            # Flip a bit of the tensor, not of the zip directory; the length
            # stays the same.
            pattern = torch.arange(20).numpy().tobytes()
            at = data.index(pattern)
            data[at] ^= 1
            path.write_bytes(data)
            with mock.patch("cts_judge.time.sleep"):
                with self.assertRaises(judge.SnapshotBusy):
                    with judge.snapshot(path, directory):
                        self.fail("corrupt archive accepted")
            self.assertEqual(list(Path(directory).iterdir()), [path])

    def test_modes_and_explicit_file_choose_correct_weights(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ("checkpoint.pt", "best.pt", "best-flat.pt",
                         "best-look2.pt"):
                (root / name).touch()
            self.assertEqual(checkpoint_path(root),
                             str(root / "best-look2.pt"))
            self.assertEqual(checkpoint_path(root, "flat"),
                             str(root / "best-flat.pt"))
            self.assertEqual(checkpoint_path(root / "checkpoint.pt"),
                             str(root / "checkpoint.pt"))
            (root / "best-flat.pt").unlink()
            self.assertEqual(checkpoint_path(root, "flat"),
                             str(root / "best.pt"))


class JudgeTests(unittest.TestCase):
    def options(self, directory):
        return judge.arguments([directory, "--once", "--climbs", "2",
                                "--report-climbs", "2", "--seed", "10",
                                "--report-seed", "100", "--envs", "2"])

    def checkpoint(self, path, update):
        atomic_save(dict(updates=update, steps=update * 100,
                         character="ironclad", acts=3, floats=4997, ids=206,
                         actions=687, hp_weight=.01, gamma=.999), path)

    def test_selection_only_promotion_ties_restart_and_snapshot_identity(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "checkpoint.pt"
            args = self.options(directory)
            self.checkpoint(source, 1)
            observations = []

            def evaluate(net, kept, options, mode, split, config):
                self.assertEqual(config["hp_weight"], .01)
                update = kept["updates"]
                observations.append((update, mode, split))
                # The trainer saves again midway; both modes must still use 1.
                if update == 1:
                    self.checkpoint(source, 2)
                    wins = 1
                else:
                    wins = {("flat", "selection"): 1, ("flat", "report"): 2,
                            ("look2", "selection"): 2, ("look2", "report"): 0}[
                                mode, split]
                return dict(seed=10 if split == "selection" else 100, climbs=2,
                            wins=wins, win_rate=wins / 2, floors=20,
                            boss_rate=.5, through=.5, seconds=1.)

            with mock.patch.object(judge, "load", side_effect=lambda p, d:
                                   (None, torch.load(p, weights_only=False))), \
                    mock.patch.object(judge, "evaluate",
                                      side_effect=evaluate), \
                    contextlib.redirect_stdout(io.StringIO()):
                judge.judge_once(args, {"engine": "test"})
                self.assertEqual({row[0] for row in observations}, {1})
                judge.judge_once(args, {"engine": "test"})
                before = len(observations)
                judge.judge_once(args, {"engine": "test"})
                self.assertEqual(len(observations), before)
                args.seed = 20
                with self.assertRaises(ValueError):
                    judge.judge_once(args, {"engine": "test"})
            flat = torch.load(root / "best-flat.pt", weights_only=False)
            look = torch.load(root / "best-look2.pt", weights_only=False)
            # A tie, even though the report improved.
            self.assertEqual(flat["updates"], 1)
            self.assertEqual(look["updates"], 2)  # Selection won, report lost.
            self.assertEqual(look["judged"]["report"]["wins"], 0)
            rows = judge.read_history(root / "judged.csv")
            self.assertEqual(len(rows), 8)
            self.assertEqual({r["update"] for r in rows}, {"1", "2"})

    def test_hp_override_required_for_legacy_checkpoint(self):
        args = self.options("unused")
        kept = dict(character="ironclad", acts=3, floats=1, ids=1, actions=1)
        with self.assertRaises(ValueError):
            judge.protocol_of(args, kept, {})
        args.hp_weight = .01
        _, config = judge.protocol_of(args, kept, {})
        self.assertEqual(config["hp_weight"], .01)

    def test_disjoint_seed_ranges_required(self):
        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit):
                judge.arguments(["--seed", "10", "--report-seed", "11"])

    def test_only_one_writer_and_lock_released(self):
        with tempfile.TemporaryDirectory() as directory:
            with judge.judge_lock(directory):
                with self.assertRaises(RuntimeError):
                    with judge.judge_lock(directory):
                        self.fail("second writer acquired lock")
            with judge.judge_lock(directory):
                pass


if __name__ == "__main__":
    unittest.main()
