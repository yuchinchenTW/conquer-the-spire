"""Training a climber with PPO.

This is the learner that sits on top of the engine: a policy over the fixed
head of moves, masked to the legal ones, trained on rollouts from a row of
climbs stepped together.

    python cts_train.py --character ironclad --acts 1
    python cts_train.py --character ironclad --acts 1      # again: it resumes

Everything is written to ``runs/<character>/`` as it goes: the weights and the
optimiser state, how far it has got, and a csv of the curve. Stopping it with
Ctrl-C saves first, and starting it again picks up where it left off. Pass
``--fresh`` to start over instead.

What to watch: ``floors`` is the first thing that moves, then ``boss`` (the
share of climbs that put an act boss down). ``win`` - the whole spire - stays
at zero for a long while; that is the shape of the problem, not a fault.
"""

import argparse
import csv
import os
import signal
import sys
import time

import numpy as np

try:
    import torch
    import torch.nn as nn
    import torch.nn.functional as functional
except ImportError:  # pragma: no cover
    print("this needs torch: pip install torch")
    raise

from cts_env import CHARACTERS, SpireEnv, action_table
from cts_net import CardPolicy
from cts_log import SUMMARY_FIELDS, vec_summaries
from cts_plot import read as read_curve, write_html
from cts_stats import clear as clear_stats
from cts_stats import curses_of
from cts_stats import render as render_stats
from cts_stats import totals_of
from cts_stats import table_of
from cts_vec import VecSpireEnv

try:
    from torch.utils.tensorboard import SummaryWriter
except ImportError:  # pragma: no cover - tensorboard is optional
    SummaryWriter = None

# One embedding table covers every id the state names: cards, relics, potions,
# rooms and monsters all fit under this, and a second table says which kind of
# slot each one came from.
ID_VOCAB = 512


class Policy(nn.Module):
    """A masked policy over the fixed head, with a value head beside it."""

    def __init__(self, floats, ids, actions, width=512, embed=24):
        super().__init__()

        self.ids = ids
        self.embed = nn.Embedding(ID_VOCAB, embed)
        self.slot = nn.Embedding(ids, embed)
        self.trunk = nn.Sequential(
            nn.Linear(floats + ids * embed, width),
            nn.ReLU(),
            nn.Linear(width, width),
            nn.ReLU(),
        )
        self.policy = nn.Linear(width, actions)
        self.value = nn.Linear(width, 1)

        # A small head to start with keeps the first moves close to uniform.
        nn.init.orthogonal_(self.policy.weight, gain=0.01)
        nn.init.zeros_(self.policy.bias)

    def forward(self, obs, ids):
        slots = torch.arange(self.ids, device=ids.device)
        pieces = self.embed(ids.clamp(0, ID_VOCAB - 1)) + self.slot(slots)
        flat = torch.cat([obs, pieces.flatten(1)], dim=1)
        hidden = self.trunk(flat)

        return self.policy(hidden), self.value(hidden).squeeze(-1)

    def act(self, obs, ids, mask):
        """Returns ``(action, logp, entropy, value)`` for one tick."""
        logits, value = self.forward(obs, ids)
        logits = logits.masked_fill(mask == 0, -1e9)
        dist = torch.distributions.Categorical(logits=logits)
        action = dist.sample()

        return action, dist.log_prob(action), dist.entropy(), value

    def judge(self, obs, ids, mask, action):
        """Returns ``(logp, entropy, value)`` for moves already made."""
        logits, value = self.forward(obs, ids)
        logits = logits.masked_fill(mask == 0, -1e9)
        dist = torch.distributions.Categorical(logits=logits)

        return dist.log_prob(action), dist.entropy(), value


class Trainer(object):
    def __init__(self, args):
        self.args = args
        # The card slots and the trunk are all matrix multiplies; letting
        # them use the tensor cores costs nothing that matters here.
        torch.set_float32_matmul_precision("high")

        self.device = torch.device(
            "cuda" if torch.cuda.is_available() and not args.cpu else "cpu")

        self.vec = VecSpireEnv(args.envs)
        self.floats = self.vec.observation_size
        self.id_count = self.vec.id_count
        self.actions = self.vec.action_count

        if args.net == "card":
            # A net built around what a card is rather than which slot it
            # sits in. It needs to know where everything sits in the state,
            # which the engine hands over.
            plan = SpireEnv()

            self.net = CardPolicy(plan.layout, plan.id_layout,
                                  action_table(),
                                  width=args.width).to(self.device)
        else:
            self.net = Policy(self.floats, self.id_count, self.actions,
                              args.width).to(self.device)
        self.opt = torch.optim.Adam(self.net.parameters(), lr=args.lr,
                                    eps=1e-5)

        # The act limit belongs to the engine. Masking the move out here
        # used to leave a climb that cleared the act standing at the top with
        # nothing it was allowed to do, going nowhere until it was called off
        # - and the boss it had just beaten was never counted.
        self.vec.set_act_limit(args.acts)

        if args.hp_weight >= 0.0:
            self.vec.set_health_weight(args.hp_weight)

        self.updates = 0
        self.steps = 0
        self.episodes = 0
        self.loss = 0.0

        # What the last report gathered, kept for printing after the end.
        self.pickBatches = 0
        self.lastPicks = {}
        self.lastCurses = []
        self.lastTotals = {"runs": 0, "wins": 0, "deaths": 0, "floors": 0,
                           "win_rate": 0.0, "avg_floors": 0.0}
        self.history = []
        self.folder = os.path.join(args.out, args.character)
        self.stopping = False

        os.makedirs(self.folder, exist_ok=True)

        # Events for tensorboard, when it is about and wanted.
        self.board = None

        if SummaryWriter is not None and not args.no_board:
            self.board = SummaryWriter(os.path.join(self.folder, "events"))

        if not args.fresh:
            self.load()

        # An update count is however many more to do this time, so that
        # picking a climb up again does not find itself already finished.
        self.target = (self.updates + args.updates) if args.updates else 0
        self.startSteps = self.steps

        self.obs, self.ids, self.mask = self.vec.reset(
            args.character, args.seed + self.updates * args.envs)

    # ------------------------------------------------------------- keeping
    @property
    def checkpoint(self):
        return os.path.join(self.folder, "checkpoint.pt")

    def save(self):
        torch.save(
            {
                "net": self.net.state_dict(),
                "opt": self.opt.state_dict(),
                "updates": self.updates,
                "steps": self.steps,
                "episodes": self.episodes,
                "character": self.args.character,
                "acts": self.args.acts,
                "floats": self.floats,
                "ids": self.id_count,
                "actions": self.actions,
                "width": self.args.width,

                # Which net it is. Not "net": that is where the weights go.
                "kind": self.args.net,
            },
            self.checkpoint,
        )

    def load(self):
        if not os.path.exists(self.checkpoint):
            print("nothing to pick up in %s; starting fresh" % self.folder)

            return

        kept = torch.load(self.checkpoint, map_location=self.device,
                          weights_only=False)

        # A checkpoint of another shape belongs to another engine.
        for name, mine in [("floats", self.floats), ("ids", self.id_count),
                           ("actions", self.actions),
                           ("width", self.args.width),
                           ("kind", self.args.net)]:
            if kept.get(name) != mine:
                print("the checkpoint was made for %s=%s and this is %s; "
                      "starting fresh" % (name, kept.get(name), mine))

                return

        if kept.get("character") != self.args.character:
            print("the checkpoint is a %s; starting fresh for %s" %
                  (kept.get("character"), self.args.character))

            return

        self.net.load_state_dict(kept["net"])
        self.opt.load_state_dict(kept["opt"])

        # The saved optimiser brings the rate it was saved with, which would
        # quietly ignore a rate asked for on the way back in.
        for group in self.opt.param_groups:
            group["lr"] = self.args.lr
        self.updates = int(kept.get("updates", 0))
        self.steps = int(kept.get("steps", 0))
        self.episodes = int(kept.get("episodes", 0))

        print("picked up %s at update %d (%d moves, %d climbs)" %
              (self.args.character, self.updates, self.steps, self.episodes))

    # ------------------------------------------------------------ the loop
    def rollout(self):
        """Collects one batch and returns it, along with what ended in it."""
        steps = self.args.steps
        envs = self.args.envs

        obs = torch.zeros((steps, envs, self.floats), device=self.device)
        ids = torch.zeros((steps, envs, self.id_count), dtype=torch.long,
                          device=self.device)
        masks = torch.zeros((steps, envs, self.actions), dtype=torch.bool,
                            device=self.device)
        actions = torch.zeros((steps, envs), dtype=torch.long,
                              device=self.device)
        logps = torch.zeros((steps, envs), device=self.device)
        values = torch.zeros((steps, envs), device=self.device)
        rewards = torch.zeros((steps, envs), device=self.device)
        dones = torch.zeros((steps, envs), device=self.device)

        finished = []

        for step in range(steps):
            legal = np.asarray(self.mask, dtype=np.uint8)
            state = torch.as_tensor(np.asarray(self.obs),
                                    device=self.device).float()
            named = torch.as_tensor(np.asarray(self.ids),
                                    device=self.device).long()
            allowed = torch.as_tensor(legal, device=self.device).bool()

            with torch.no_grad():
                action, logp, _, value = self.net.act(state, named, allowed)

            obs[step] = state
            ids[step] = named
            masks[step] = allowed
            actions[step] = action
            logps[step] = logp
            values[step] = value

            picks = action.cpu().numpy()
            self.obs, self.ids, self.mask, reward, done, info = self.vec.step(
                picks)

            rewards[step] = torch.as_tensor(np.asarray(reward),
                                            device=self.device)
            dones[step] = torch.as_tensor(np.asarray(done, dtype=np.float32),
                                          device=self.device)
            self.steps += envs

            if done.any():
                counts = vec_summaries(self.vec, last=True)

                # Only the ones that ended on this tick: the list holds the
                # whole batch, so its length is not what just happened.
                for i, ended in enumerate(done):
                    if ended:
                        finished.append((float(info["returns"][i]), counts[i]))
                        self.episodes += 1

        with torch.no_grad():
            state = torch.as_tensor(np.asarray(self.obs),
                                    device=self.device).float()
            named = torch.as_tensor(np.asarray(self.ids),
                                    device=self.device).long()
            _, last = self.net.forward(state, named)

        return (obs, ids, masks, actions, logps, values, rewards, dones,
                last, finished)

    def advantages(self, rewards, values, dones, last):
        """Generalised advantage, walked backwards over the batch."""
        steps = rewards.shape[0]
        out = torch.zeros_like(rewards)
        running = torch.zeros_like(last)

        for step in reversed(range(steps)):
            alive = 1.0 - dones[step]
            nextValue = last if step == steps - 1 else values[step + 1]
            delta = (rewards[step] + self.args.gamma * nextValue * alive -
                     values[step])
            running = delta + (self.args.gamma * self.args.lam * alive *
                               running)
            out[step] = running

        return out, out + values

    def learn(self, batch):
        (obs, ids, masks, actions, logps, values, rewards, dones, last,
         _) = batch

        adv, returns = self.advantages(rewards, values, dones, last)

        flat = lambda one: one.reshape((-1,) + one.shape[2:])
        obs, ids, masks = flat(obs), flat(ids), flat(masks)
        actions, logps = flat(actions), flat(logps)
        adv, returns = flat(adv), flat(returns)

        adv = (adv - adv.mean()) / (adv.std() + 1e-8)

        total = obs.shape[0]
        size = max(1, total // self.args.minibatches)
        losses = []

        for _ in range(self.args.epochs):
            order = torch.randperm(total, device=self.device)

            for at in range(0, total, size):
                cut = order[at:at + size]
                newLogp, entropy, value = self.net.judge(
                    obs[cut], ids[cut], masks[cut], actions[cut])

                ratio = (newLogp - logps[cut]).exp()
                clipped = torch.clamp(ratio, 1.0 - self.args.clip,
                                      1.0 + self.args.clip)
                policyLoss = -torch.min(ratio * adv[cut],
                                        clipped * adv[cut]).mean()
                valueLoss = functional.mse_loss(value, returns[cut])
                loss = (policyLoss + self.args.value * valueLoss -
                        self.args.entropy * entropy.mean())

                self.opt.zero_grad(set_to_none=True)
                loss.backward()
                nn.utils.clip_grad_norm_(self.net.parameters(),
                                         self.args.clip_grad)
                self.opt.step()
                losses.append(float(loss.detach()))

        return sum(losses) / len(losses) if losses else 0.0

    def run(self):
        print("training %s on %s with the %s net, %d climbs side by side, "
              "%d moves a batch" %
              (self.args.character, self.device, self.args.net,
               self.args.envs, self.args.steps * self.args.envs))

        if self.args.acts:
            print("the climb ends after act %d" % self.args.acts)

        recent = []
        started = time.perf_counter()

        while not self.stopping and (self.target == 0 or
                                     self.updates < self.target):
            batch = self.rollout()
            loss = self.learn(batch)

            self.loss = loss
            self.updates += 1
            recent.extend(batch[-1])
            recent = recent[-200:]

            if self.updates % self.args.every == 0:
                self.report(recent, loss, started)

            if self.updates % self.args.keep == 0:
                self.save()

        self.save()

        # Only report at the end when the last batch has not been reported
        # already, so the curve has no repeated point on it.
        if self.updates % self.args.every != 0:
            self.report(recent, self.loss, started)

        if self.board is not None:
            self.board.close()

        print("saved to %s" % self.checkpoint)

    def trimPicks(self, path):
        """Drops all but the last few batches out of the choices file.

        The table is now kept over millions of climbs, so a batch of it is
        the whole set of cards, relics and potions rather than a handful of
        rows. Left alone the file would outgrow everything else in the folder,
        and nothing reads further back than the last batch anyway.
        """
        try:
            with open(path, "r", newline="") as handle:
                rows = list(csv.reader(handle))
        except OSError:
            return

        if len(rows) < 2:
            return

        head, body = rows[0], rows[1:]
        stamps = []

        for row in body:
            if row and (not stamps or row[0] != stamps[-1]):
                stamps.append(row[0])

        if len(stamps) <= self.args.pick_keep:
            return

        keeping = set(stamps[-self.args.pick_keep:])
        spare = path + ".new"

        try:
            with open(spare, "w", newline="") as handle:
                writer = csv.writer(handle)

                writer.writerow(head)

                for row in body:
                    if row and row[0] in keeping:
                        writer.writerow(row)

            os.replace(spare, path)
        except OSError:
            # Something has the file open, which on Windows is enough to stop
            # it being replaced. It will be trimmed the next time round.
            try:
                os.remove(spare)
            except OSError:
                pass

    def writePicks(self, picks, curses=None):
        """Writes the choices out, one row a thing, with the update number on
        it so that a pick rate can be followed over time."""
        path = os.path.join(self.folder, "picks.csv")
        fresh = not os.path.exists(path)

        with open(path, "a", newline="") as handle:
            writer = csv.writer(handle)

            if fresh:
                writer.writerow(["updates", "steps", "kind", "id", "name",
                                 "picks", "seen", "pick_rate", "runs",
                                 "wins", "win_rate", "avg_floors"])

            for kind, rows in sorted(picks.items()):
                for row in rows:
                    writer.writerow([self.updates, self.steps, kind,
                                     row["id"], row["name"], row["picks"],
                                     row["offered"],
                                     round(row["pick_rate"], 4), row["runs"],
                                     row["wins"], round(row["win_rate"], 4),
                                     round(row["avg_floors"], 3)])

            # Under a kind of their own, so that the page and the window can
            # show them without sifting the cards for them.
            for row in (curses or []):
                writer.writerow([self.updates, self.steps, "curse_taken",
                                 row["id"], row["name"], row["picks"],
                                 row["offered"], round(row["pick_rate"], 4),
                                 row["runs"], row["wins"],
                                 round(row["win_rate"], 4),
                                 round(row["avg_floors"], 3)])

        self.pickBatches += 1

        # Now and then, so that a long climb does not leave a huge file.
        if self.pickBatches % 25 == 0:
            self.trimPicks(path)

        if self.board is None:
            return

        # How much of the deck it is poisoning itself with, as one number to
        # follow.
        self.board.add_scalar("picks/curses_taken",
                              sum(row["picks"] for row in (curses or [])),
                              self.steps)

        # A line each for the ones seen often enough to mean anything, so
        # that a preference can be watched as it forms.
        for kind in ["card_taken", "relic_taken", "potion_taken"]:
            for row in picks.get(kind, [])[:self.args.pick_lines]:
                name = row["name"].replace(" ", "_")

                self.board.add_scalar("pick_rate/%s/%s" % (kind, name),
                                      row["pick_rate"], self.steps)

                if row["runs"] >= self.args.pick_least:
                    self.board.add_scalar(
                        "floors/%s/%s" % (kind, name), row["avg_floors"],
                        self.steps)
                    self.board.add_scalar(
                        "win_rate/%s/%s" % (kind, name), row["win_rate"],
                        self.steps)

    def report(self, recent, loss, started):
        spent = max(1e-9, time.perf_counter() - started)

        if recent:
            returns = np.array([one[0] for one in recent])
            floors = np.array([one[1]["floors"] for one in recent])
            bosses = np.array([one[1]["bosses_won"] for one in recent])
            wins = np.array([one[1]["won_the_spire"] for one in recent])
            fights = np.array([one[1]["fights_won"] for one in recent])
            line = ("return %7.1f  floors %5.2f  fights %5.2f  boss %5.1f%%"
                    "  win %5.1f%%" %
                    (returns.mean(), floors.mean(), fights.mean(),
                     100.0 * (bosses > 0).mean(), 100.0 * wins.mean()))
            row = [self.updates, self.steps, self.episodes,
                   float(returns.mean()), float(floors.mean()),
                   float(fights.mean()), float((bosses > 0).mean()),
                   float(wins.mean()), loss]
        else:
            line = "no climb has ended yet"
            row = [self.updates, self.steps, self.episodes, 0, 0, 0, 0, 0,
                   loss]

        print("update %-6d %-11s %s  loss %7.3f  %5.0f moves/s" %
              (self.updates, "(%d climbs)" % self.episodes, line, loss,
               (self.steps - self.startSteps) / spent))

        with open(os.path.join(self.folder, "curve.csv"), "a",
                  newline="") as handle:
            csv.writer(handle).writerow(row)

        if self.board is not None:
            for name, value in zip(
                    ["return", "floors", "fights", "boss_rate", "win_rate",
                     "loss"], row[3:]):
                self.board.add_scalar(name, value, self.steps)

            self.board.flush()

        # What it has been choosing lately, and how those climbs went. The
        # table is cleared afterwards, so every one of these is a window on
        # what the policy is doing now rather than everything since the start.
        # How many climbs the table has counted so far. It is only emptied
        # once it has seen a window's worth of them, so that a relic offered
        # once or twice a climb still has enough behind it to mean anything.
        counted = totals_of(self.vec)
        picks = table_of(self.vec, least=self.args.pick_least)

        # Curses are counted from the first one: they are never offered
        # beside a real card, so a handful in a window is already the whole
        # story and a threshold would hide it.
        curses = curses_of(self.vec, least=1)

        # The table is about to be forgotten, so what was in it is kept for
        # anything that wants to print it after the last batch.
        self.lastPicks = picks
        self.lastCurses = curses
        self.lastTotals = counted

        self.writePicks(picks, curses)

        if counted["runs"] >= self.args.pick_window:
            clear_stats(self.vec)

        # And the page anybody can open in a browser while this runs.
        write_html(self.folder, self.args.character, read_curve(self.folder),
                   picks=picks, curses=curses, climbs=counted["runs"])


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Train a climber with PPO.")

    parser.add_argument("--character", default="ironclad",
                        choices=sorted(CHARACTERS))
    parser.add_argument("--acts", type=int, default=0,
                        help="end the climb after this act; 0 for all of it")
    parser.add_argument("--envs", type=int, default=128)
    parser.add_argument("--steps", type=int, default=128)
    parser.add_argument("--updates", type=int, default=0,
                        help="do this many more, then stop; 0 to keep going")
    parser.add_argument("--lr", type=float, default=3e-4)
    parser.add_argument("--gamma", type=float, default=0.995)
    parser.add_argument("--lam", type=float, default=0.95)
    parser.add_argument("--clip", type=float, default=0.2)
    parser.add_argument("--clip-grad", type=float, default=0.5,
                        dest="clip_grad")
    parser.add_argument("--entropy", type=float, default=0.01)
    parser.add_argument("--value", type=float, default=0.5)
    parser.add_argument("--epochs", type=int, default=4)
    parser.add_argument("--minibatches", type=int, default=4)
    parser.add_argument("--width", type=int, default=512)
    parser.add_argument("--net", choices=["card", "flat"], default="card",
                        help="card: every card is one shared embedding and a "
                             "move is scored from the thing it is about. "
                             "flat: one weight for every move, the older way")
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--every", type=int, default=5,
                        help="print every this many updates")
    parser.add_argument("--keep", type=int, default=10,
                        help="save every this many updates; closing the "
                             "window without stopping first costs at most "
                             "this much work")
    parser.add_argument("--out", default="runs")
    parser.add_argument("--fresh", action="store_true",
                        help="ignore whatever was saved and start over")
    parser.add_argument("--cpu", action="store_true")
    parser.add_argument("--pick-least", type=int, default=10,
                        dest="pick_least",
                        help="how many times a thing has to turn up in a "
                             "window before its rates are written down")
    parser.add_argument("--hp-weight", type=float, default=-1.0,
                        dest="hp_weight",
                        help="what a point of health lost costs, against a "
                             "floor being worth one; below zero keeps the "
                             "engine's own 0.05")
    parser.add_argument("--pick-window", type=int, default=100000,
                        dest="pick_window",
                        help="how many climbs the table of choices covers "
                             "before it is emptied and counted afresh")
    parser.add_argument("--pick-keep", type=int, default=200,
                        dest="pick_keep",
                        help="how many batches of choices picks.csv keeps")
    parser.add_argument("--pick-lines", type=int, default=24,
                        dest="pick_lines",
                        help="how many of each kind get a line of their own "
                             "in tensorboard")
    parser.add_argument("--no-board", action="store_true",
                        dest="no_board",
                        help="do not write tensorboard events")
    parser.add_argument("--picks", action="store_true",
                        help="print what it has been choosing at the end")

    args = parser.parse_args(argv)

    torch.manual_seed(args.seed)
    np.random.seed(args.seed)

    trainer = Trainer(args)

    def stop(signum, frame):
        # Let the loop finish the batch it is on, then save and go. A second
        # interrupt is taken as meaning now, and gives up that batch rather
        # than the checkpoint before it.
        del frame

        if trainer.stopping:
            print("\nstopping now; this batch is given up")
            signal.signal(signum, signal.SIG_DFL)

            raise KeyboardInterrupt

        print("\nstopping after this batch; the weights will be saved")
        trainer.stopping = True

    signal.signal(signal.SIGINT, stop)

    # Ctrl-Break as well, on the systems that have it.
    if hasattr(signal, "SIGBREAK"):
        signal.signal(signal.SIGBREAK, stop)

    try:
        trainer.run()
    except KeyboardInterrupt:
        trainer.save()
        print("\nsaved to %s" % trainer.checkpoint)

    if args.picks:
        print()

        # What the last report gathered: the engine's own table was emptied
        # then, so that every report is a window on the policy as it is now.
        print(render_stats(trainer.lastTotals, trainer.lastPicks,
                           trainer.lastCurses, top=8))

    return 0


if __name__ == "__main__":
    sys.exit(main())
