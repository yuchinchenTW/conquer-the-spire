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
import shutil
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

from cts_env import CHARACTERS, PHASES, SpireEnv, action_table
from cts_net import CardPolicy, FORESIGHTS
from cts_log import SUMMARY_FIELDS, vec_summaries
from cts_plot import COLUMNS as CURVE_COLUMNS
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

# How improbable a move may be and still be worth taking on a search's word.
# exp(-6) is about one in four hundred: rare enough that the policy is not
# already doing it, common enough that the ratio in the loss stays sane.
BEST_OVER = 20
"""How many reports a best is judged over.

One report is two hundred climbs and bounces by several points either way,
so the best single report is mostly the luckiest one. Twenty of them at the
default cadence is a hundred updates, which is steady enough to mean
something and short enough to catch a peak while it is happening.
"""

LEANING = 1.02
"""How much the pressure to stay undecided moves in one update.

Two percent a time: a hundred and sixteen updates to go from the asked-for
coefficient to ten times it, which is slow beside the thing it is steering
and so cannot start swinging against it.
"""

MOST_PRESSURE = 10.0
"""How many times the asked-for coefficient the pressure may reach.

There has to be a ceiling, or a policy that will not spread out for some
other reason - too few legal moves, a value head that is simply right -
would have the coefficient climbing for ever until nothing else in the loss
mattered. Ten times the usual hundredth is already a tenth, which is high
enough that a run sitting on the ceiling is a run being held back rather
than a run being kept honest, and the number is meant to be reached only
when something else is wrong.
"""

BEST_MARGIN = 0.5
"""How much better than the standing best a run has to be to be kept.

A single report's spread is about two and a half points, so a mean of twenty
of them still wobbles by half a point either way. Without a margin every
other report clears the bar by a hundredth and writes a hundred megabytes for
nothing.
"""

LOGP_FLOOR = -6.0

# Where health and the floor sit in the state, for the foresight targets.
# The run block writes act, floor, column, gold, health, in that order, so
# the two wanted are the second and the fifth of it.
_RUN = SpireEnv().layout["run"]
FLOOR_AT = _RUN + 1
HEALTH_AT = _RUN + 4

# Which slot of the state says where the climber is standing, and which of
# those slots are a fight. The looking below is worth three and a half floors
# out of a fight and nothing at all in one, so it has to be able to tell.
PHASE_AT = SpireEnv().layout["phase"]
PHASE_COUNT = len(PHASES)
FIGHT_PHASES = (PHASES.index("battle"), PHASES.index("boss"))

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
        """Returns ``(action, logp, entropy, value, scores)`` for one tick.

        The masked scores come back too, the same as the card net's, so that
        whatever looks a move ahead does not have to ask the trunk twice for
        numbers it already has.
        """
        logits, value = self.forward(obs, ids)
        logits = logits.masked_fill(mask == 0, -1e9)
        dist = torch.distributions.Categorical(logits=logits)
        action = dist.sample()

        return (action, dist.log_prob(action), dist.entropy(), value,
                logits)

    def judge(self, obs, ids, mask, action):
        """Returns ``(logp, entropy, value, foresight, scores)``.

        Nothing foreseen - this net has no such head - so that slot is None,
        and the masked scores come last, the same shape as the card net's, so
        that the loss does not have to know which net it is holding.
        """
        logits, value = self.forward(obs, ids)
        logits = logits.masked_fill(mask == 0, -1e9)
        dist = torch.distributions.Categorical(logits=logits)

        return dist.log_prob(action), dist.entropy(), value, None, logits


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
        self.vec.set_deep_share(args.deep)

        if args.hp_weight >= 0.0:
            self.vec.set_health_weight(args.hp_weight)

        if args.max_hp_weight >= 0.0:
            self.vec.set_max_health_weight(args.max_hp_weight)

        if args.curse_penalty >= 0.0:
            self.vec.set_curse_penalty(args.curse_penalty)

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

        # The best the climber has ever been, and the reports it is judged
        # over. Set before load(), which brings them back when there is a run
        # to carry on.
        self.bestScore = None
        self.bestAt = 0
        self.bestFloors = 0.0
        self.scores = []

        # The share of the most it could be undecided by, last update.
        self.spread = 0.0

        # How many moves were walked before being made, and how many of them
        # the walking changed. A run whose looking never changes anything is
        # paying for nothing, and the two numbers say so.
        self.looked = 0
        self.overruled = 0

        # And how many the log-probability floor turned away. A floor that
        # bars most of the looking is a run paying for a search it then
        # declines to use.
        self.barred = 0

        # Climbs picked up part-way up. Counted apart from the climbs proper,
        # because everything the run is read by - the floors, the won share,
        # the best so far - is about a climb that started at the bottom, and
        # these did not.
        self.deep = 0

        # What the run is actually holding, as against what was asked for on
        # the way in. Both move while it runs and both are carried in the
        # checkpoint, so that picking a run up again does not hand it back
        # the rate and the pressure it had already worked away from.
        self.rate = args.lr
        self.pressure = args.entropy
        self.decayedAt = 0

        os.makedirs(self.folder, exist_ok=True)

        # A climber that is not being carried on leaves nothing of itself
        # in the way of the one that replaces it. The curve and the choices
        # are written to as the run goes, so without this the page would draw
        # one climber's line running into another's, and the tables would go
        # on showing the old one for as long as the new took to pass its
        # update count. Set aside rather than thrown away: it is the only
        # copy of how the last one did.
        if args.fresh or not self.load():
            self.setAside()

        # Events for tensorboard, when it is about and wanted. Opened after
        # the setting aside, so that it writes into an empty folder.
        self.board = None

        if SummaryWriter is not None and not args.no_board:
            self.board = SummaryWriter(os.path.join(self.folder, "events"))

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

    @property
    def best(self):
        """Where the best weights are kept, beside the working ones."""
        return os.path.join(self.folder, "best.pt")

    def noteBest(self, score, floors):
        """Keeps a copy of the weights whenever the climber is at its best.

        The one checkpoint is written over every few updates, so a run that
        gets worse writes its best self away and there is nothing to go back
        to. This one is only ever written when the climber is doing better
        than it has ever done, so the good weights survive whatever happens
        after them.

        Judged on a mean of the last few reports rather than on one of them.
        A single report is two hundred climbs and bounces by several points
        either way, so the best single report is mostly the luckiest one, and
        latching onto it early would mean never saving again.
        """
        self.scores.append((float(score), float(floors)))

        if len(self.scores) > BEST_OVER:
            self.scores.pop(0)

        if len(self.scores) < BEST_OVER:
            return

        smooth = sum(one[0] for one in self.scores) / len(self.scores)
        deep = sum(one[1] for one in self.scores) / len(self.scores)

        if (self.bestScore is not None and
                smooth <= self.bestScore + BEST_MARGIN):
            return

        was = self.bestScore
        self.bestScore = smooth
        self.bestAt = self.updates
        self.bestFloors = deep

        self.save(self.best)

        print("   kept the best so far: return %.1f, floors %.2f, over the "
              "last %d reports%s"
              % (smooth, deep, BEST_OVER,
                 "" if was is None else " (was %.1f)" % was))

    def hold(self):
        """Keeps the policy undecided, and slows the run down when it stalls.

        Two things, both of which only ever fire because the run stopped
        getting better rather than after some number of moves picked in
        advance - there is no end to this run to count backwards from.

        The pressure to stay undecided is a floor and not a setting. A policy
        this far in is nearly made up: it holds about a tenth of the spread a
        coin would, which is little enough that it stops trying things and
        starts drifting on whatever it already believes. So when the spread
        falls under what is asked for, the coefficient leans up; when it rises
        over, the coefficient leans back down - never below the number asked
        for, which is what makes it a floor.

        The rate comes down when the climber has gone a long while without
        being at its best *and* is spread out enough to have been trying. A
        rate that carried a run to three billion moves is too large to hold it
        there, and the usual answer - decay it towards the end - has no end
        here to decay towards. So the plateau names the moment instead: no new
        best in as many updates as asked, and the rate halves. Once per
        stretch, so that a long flat run steps down rather than falling
        through the floor in one go.

        The spread has to be up to what was asked before any of that, because
        otherwise the two answer the same question and undo each other. While
        the pressure is still pushing, the return is being held down on
        purpose - a climber made to try things does worse at first - so "no
        new best" is the pressure's own doing and not a plateau. Cutting the
        rate for it would leave a run that explores as much as it can and
        learns as slowly as it can, and would make the flatness it was reading
        come true. A flat run that *is* spread out has nothing left to blame
        but the rate.
        """
        if self.spread > 0.0:
            self.pressure *= (LEANING if self.spread < self.args.spread
                              else 1.0 / LEANING)
            self.pressure = min(max(self.pressure, self.args.entropy),
                                self.args.entropy * MOST_PRESSURE)

        stale = self.updates - max(self.bestAt, self.decayedAt)
        trying = self.spread >= self.args.spread

        if (self.args.patience > 0 and trying and
                stale >= self.args.patience and
                self.rate > self.args.lr_floor):
            self.rate = max(self.rate * self.args.decay, self.args.lr_floor)
            self.decayedAt = self.updates

            for group in self.opt.param_groups:
                group["lr"] = self.rate

            print("   no better in %d updates: the rate is now %.2e"
                  % (stale, self.rate))

    def save(self, path=None):
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

                # Carried so that picking a run up again does not write over
                # a best that the weights coming back cannot match yet.
                "best_score": self.bestScore,
                "best_at": self.bestAt,
                "best_floors": self.bestFloors,
                "scores": self.scores,
                "deep": self.deep,
                "rate": self.rate,
                "pressure": self.pressure,
                "decayed_at": self.decayedAt,
            },
            path or self.checkpoint,
        )

    def setAside(self):
        """Moves the last climber's numbers out of the way, into before-N.

        Not deleted: it is the only record of how that one did, and a curve
        to measure the next one against.
        """
        # best.pt goes with them. Left where it was, a climber starting over
        # would carry the old one's best around and never beat it, and the
        # file would say it belonged to a run that is no longer there.
        leftovers = [name for name in ("curve.csv", "picks.csv",
                                       "progress.html", "progress.png",
                                       "best.pt", "events")
                     if os.path.exists(os.path.join(self.folder, name))]

        if not leftovers:
            return

        for number in range(1, 1000):
            aside = os.path.join(self.folder, "before-%d" % number)

            if not os.path.exists(aside):
                break
        else:
            return

        os.makedirs(aside, exist_ok=True)

        for name in leftovers:
            shutil.move(os.path.join(self.folder, name),
                        os.path.join(aside, name))

        print("the last climber's curve and choices are in %s" % aside)

    def load(self):
        """Returns whether a climber was picked up to carry on."""
        if not os.path.exists(self.checkpoint):
            print("nothing to pick up in %s; starting fresh" % self.folder)

            return False

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

                return False

        if kept.get("character") != self.args.character:
            print("the checkpoint is a %s; starting fresh for %s" %
                  (kept.get("character"), self.args.character))

            return False

        self.net.load_state_dict(kept["net"])
        self.opt.load_state_dict(kept["opt"])

        # The saved optimiser brings the rate it was saved with. What the run
        # should carry on at is the rate it had worked its way down to, unless
        # it was never keeping one, in which case it is whatever was asked for
        # on the way back in.
        self.rate = float(kept.get("rate", self.args.lr))
        self.pressure = float(kept.get("pressure", self.args.entropy))
        self.decayedAt = int(kept.get("decayed_at", 0))
        self.deep = int(kept.get("deep", 0))

        for group in self.opt.param_groups:
            group["lr"] = self.rate
        self.updates = int(kept.get("updates", 0))
        self.steps = int(kept.get("steps", 0))
        self.episodes = int(kept.get("episodes", 0))
        self.bestScore = kept.get("best_score")
        self.bestAt = int(kept.get("best_at", 0))
        self.bestFloors = float(kept.get("best_floors", 0.0))
        self.scores = list(kept.get("scores", []))

        print("picked up %s at update %d (%d moves, %d climbs)" %
              (self.args.character, self.updates, self.steps, self.episodes))

        return True

    def reranked(self, state, named, allowed, action, logp):
        """The policy's own top moves, re-ordered by how their fights end.

        Returns what to do and its log probability *under the policy*, which
        is what the ratio in the loss below is against. Handing back the
        chosen move with the sampled move's probability would put a number in
        the batch that does not belong to what was played, and PPO would be
        correcting towards the wrong thing.
        """
        width = self.args.peek
        scores, _, _ = self.net.forward(state, named)
        scores = scores.masked_fill(~allowed, -1e9)
        top = torch.topk(scores, min(width, scores.shape[1]), dim=1).indices

        # topk fills the row out of the masked-away moves when fewer than
        # `width` are legal; those slots read as empty to the engine.
        offered = torch.where(allowed.gather(1, top), top,
                              torch.full_like(top, self.actions))
        picked = torch.as_tensor(
            self.vec.rank(offered.cpu().numpy().astype(np.uintp)),
            device=self.device).long()

        # An index at or past the head is the engine saying it had nothing to
        # look into, and has to be turned away before it indexes anything.
        safe = picked.clamp(max=self.actions - 1)
        usable = (picked < self.actions) & allowed.gather(
            1, safe[:, None]).squeeze(1)
        kept = torch.where(usable, safe, action)
        logps = torch.distributions.Categorical(logits=scores).log_prob(kept)

        # And a move the policy all but rules out is left alone however well
        # its fight came out. The ratio in the loss is exp(new - old), so a
        # move taken at a probability of 1e-22 puts an astronomical number in
        # it the moment the policy warms to it at all: the first run of this
        # reached a loss of three hundred million. PPO corrects towards what
        # it nearly did, not towards an oracle - handing it an action from
        # outside its own distribution is what distillation is for, not this.
        return (torch.where(logps > LOGP_FLOOR, kept, action),
                torch.where(logps > LOGP_FLOOR, logps, logp))

    def lookedAhead(self, state, allowed, action, logp, scores):
        """Walks a couple of the policy's moves and keeps the better one.

        The policy names a move by guessing what it comes to. This walks it:
        the climb is copied, the move made on the copy, and what the copy is
        worth read off the value head - plus what the move paid on the way,
        because a move is worth what it paid as well as what it left.

        Only out of a fight, and only a couple of moves. Both of those were
        measured rather than chosen. Over 800 climbs against the same seeds,
        looking out of a fight was worth three and a half floors and nearly
        doubled the wins; looking in one was worth nothing, because a step
        into a fight is followed by cards nobody has drawn yet and the head
        cannot tell one from another through that. And walking eight moves
        instead of two cost half the climb: given eight readings of a head
        that is only roughly right, the largest is mostly the largest
        mistake, and two readings barely have room for one.

        Returns what to do and its log probability *under the policy*, which
        is what the ratio in the loss is against - the same reckoning
        reranked() makes, and for the same reason.
        """
        width = self.args.look
        top = torch.topk(scores, min(width, scores.shape[1]), dim=1).indices

        # topk fills a row out of the moves masked away when fewer than
        # `width` are legal; those repeat the first, which costs a walk and
        # changes nothing.
        offered = torch.where(allowed.gather(1, top), top, top[:, :1])

        # In a fight the walking is not worth its keep, so those climbs are
        # not walked at all. Skipping them is most of what makes this cheap:
        # out of a fight is about one move in twelve, so walking every climb
        # every step would be eleven twelfths waste.
        where = state[:, PHASE_AT:PHASE_AT + PHASE_COUNT].argmax(dim=1)
        fighting = torch.zeros_like(where, dtype=torch.bool)

        for phase in FIGHT_PHASES:
            fighting |= where == phase

        asking = ~fighting

        if not bool(asking.any()):
            return (action, logp, 0, 0, action,
                    torch.zeros_like(logp))

        seen, ids, paid, over = self.vec.peek_moves(
            offered.cpu().numpy().astype(np.uintp),
            asking.cpu().numpy().astype(np.uint8))
        rows, wide = offered.shape

        worth = torch.as_tensor(paid, device=self.device).float()
        ended = torch.as_tensor(over, device=self.device).bool()

        _, after, _ = self.net.forward(
            torch.as_tensor(seen.reshape(rows * wide, -1),
                            device=self.device).float(),
            torch.as_tensor(ids.reshape(rows * wide, -1),
                            device=self.device).long())

        # A climb that ended there is worth its last payment and nothing
        # after it.
        worth = worth + torch.where(ended, torch.zeros_like(worth),
                                    self.args.gamma *
                                    after.reshape(rows, wide))

        kept = offered.gather(1, worth.argmax(dim=1, keepdim=True)).squeeze(1)
        kept = torch.where(asking, kept, action)
        logps = torch.distributions.Categorical(logits=scores).log_prob(kept)

        # And a move the policy all but rules out is left alone, for the
        # reason spelled out in reranked(): the ratio is exp(new - old), and
        # a move played at a probability of 1e-22 puts an astronomical number
        # in it the moment the policy warms to it.
        take = logps > LOGP_FLOOR
        differs = kept != action
        moved = int((differs & take).sum())
        barred = int((differs & ~take).sum())

        # What the looking wanted is handed back whether or not the floor let
        # it be played, because a move that cannot safely be played can still
        # be taught.
        return (torch.where(take, kept, action),
                torch.where(take, logps, logp), moved, barred,
                kept, asking.float())

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

        # What the looking wanted at each step, and where it had an
        # opinion at all. Kept apart from what was played, because the two
        # come apart wherever the floor turned the looking away - and those
        # are the moves worth teaching.
        wanted = torch.zeros((steps, envs), dtype=torch.long,
                             device=self.device)
        taught = torch.zeros((steps, envs), device=self.device)

        # And which moves were the climber's own. A move the looking put
        # there is not something to correct the policy towards through the
        # ratio: the ratio is exp(new - old), the looking picks moves the
        # policy rates at a thousandth or less, and every one of those pushes
        # at the clip's full strength the moment it comes out well. That is
        # distillation wearing PPO's coat, and distillation was measured to
        # make this climber worse at every strength tried.
        own = torch.ones((steps, envs), device=self.device)

        # What the trunk is asked to foresee, and the two raw counts
        # the targets are worked out from.
        seen = torch.zeros((steps, envs, len(FORESIGHTS)), device=self.device)
        hurt = torch.zeros((steps, envs), device=self.device)
        floors = torch.zeros((steps, envs), device=self.device)

        finished = []

        for step in range(steps):
            legal = np.asarray(self.mask, dtype=np.uint8)
            state = torch.as_tensor(np.asarray(self.obs),
                                    device=self.device).float()
            named = torch.as_tensor(np.asarray(self.ids),
                                    device=self.device).long()
            allowed = torch.as_tensor(legal, device=self.device).bool()

            with torch.no_grad():
                action, logp, _, value, scores = self.net.act(
                    state, named, allowed)

                # A fight looked into before the move is made. The policy
                # offers its best few and the engine plays each one's fight
                # out; whichever comes out best is what actually happens.
                if self.args.peek > 0:
                    action, logp = self.reranked(state, named, allowed,
                                                 action, logp)

                # And out of a fight, where a fight cannot be played out at
                # all, the same question asked of the value head instead.
                if self.args.look > 1:
                    mine = action
                    action, logp, moved, barred, said, had = self.lookedAhead(
                        state, allowed, action, logp, scores)
                    self.looked += envs
                    self.overruled += moved
                    self.barred += barred
                    wanted[step] = said
                    taught[step] = had
                    own[step] = (action == mine).float()

            obs[step] = state
            ids[step] = named
            masks[step] = allowed
            actions[step] = action
            logps[step] = logp
            values[step] = value

            # Read before the step, to difference against after it.
            wasState = np.asarray(self.obs)
            before = wasState[:, HEALTH_AT].copy()
            was = wasState[:, FLOOR_AT].copy()

            picks = action.cpu().numpy()
            self.obs, self.ids, self.mask, reward, done, info = self.vec.step(
                picks)

            rewards[step] = torch.as_tensor(np.asarray(reward),
                                            device=self.device)
            dones[step] = torch.as_tensor(np.asarray(done, dtype=np.float32),
                                          device=self.device)

            # The two raw counts the foresight targets are read from. Health
            # is a share of the maximum in the state, so the drop between one
            # step and the next is already the right scale; a floor is one
            # floor. A climb that ended is skipped, because the state now
            # belongs to the next one.
            after = np.asarray(self.obs)
            alive = 1.0 - np.asarray(done, dtype=np.float32)
            hurt[step] = torch.as_tensor(
                np.maximum(0.0, before - after[:, HEALTH_AT]) * alive,
                device=self.device).float()
            floors[step] = torch.as_tensor(
                np.maximum(0.0, after[:, FLOOR_AT] - was) * alive,
                device=self.device).float()

            self.steps += envs

            if done.any():
                counts = vec_summaries(self.vec, last=True)

                # Only the ones that ended on this tick: the list holds the
                # whole batch, so its length is not what just happened.
                for i, ended in enumerate(done):
                    if not ended:
                        continue

                    # A climb picked up part-way up is played and learned
                    # from like any other. It is not one of the climbs the
                    # run is judged on: it walked fewer floors to reach
                    # wherever it reached and met one act's dangers rather
                    # than three, so putting it in the average would move
                    # every number the run is read by without the climber
                    # having changed at all.
                    if counts[i]["started_deep"]:
                        self.deep += 1
                        continue

                    finished.append((float(info["returns"][i]), counts[i]))
                    self.episodes += 1

        with torch.no_grad():
            state = torch.as_tensor(np.asarray(self.obs),
                                    device=self.device).float()
            named = torch.as_tensor(np.asarray(self.ids),
                                    device=self.device).long()
            _, last, _ = self.net.forward(state, named)

        # What the trunk was asked to see, filled in now that the batch is
        # whole - each of these is a number from later in the same climb, so
        # nothing had to be labelled.
        #
        # How much health went in the step after, whether the fight was over
        # within eight steps, and how many more floors the climb managed.
        for step in range(steps):
            seen[step, :, 0] = torch.clamp(hurt[step], 0.0, 1.0)

            ahead = min(steps, step + 8)
            seen[step, :, 1] = (dones[step:ahead].sum(dim=0) > 0).float()

            # Floors left, out of a spire's worth, so the number sits beside
            # the others rather than dwarfing them.
            gained = floors[step:].sum(dim=0) - floors[step]
            seen[step, :, 2] = torch.clamp(gained / 60.0, 0.0, 1.0)

        # `finished` stays last, because the loop above reads it as batch[-1].
        return (obs, ids, masks, actions, logps, values, rewards, dones,
                last, seen, wanted, taught, own, finished)

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
         seen, wanted, taught, own, _) = batch

        adv, returns = self.advantages(rewards, values, dones, last)

        flat = lambda one: one.reshape((-1,) + one.shape[2:])
        obs, ids, masks = flat(obs), flat(ids), flat(masks)
        actions, logps = flat(actions), flat(logps)
        adv, returns = flat(adv), flat(returns)
        seen = flat(seen)
        wanted, taught, own = flat(wanted), flat(taught), flat(own)

        adv = (adv - adv.mean()) / (adv.std() + 1e-8)

        total = obs.shape[0]
        size = max(1, total // self.args.minibatches)
        losses = []

        # How undecided the policy still is, against how undecided it could
        # be. The entropy on its own says nothing, because the number of
        # legal moves changes from one state to the next: two nats is nearly
        # everything when three moves are legal and nearly nothing when forty
        # are. What is comparable across a run is the share of the most it
        # could have, which is the log of the count of legal moves.
        spread = []

        for _ in range(self.args.epochs):
            order = torch.randperm(total, device=self.device)

            for at in range(0, total, size):
                cut = order[at:at + size]
                judged = self.net.judge(obs[cut], ids[cut], masks[cut],
                                        actions[cut])
                newLogp, entropy, value = judged[:3]
                told = judged[4]

                with torch.no_grad():
                    legal = masks[cut].sum(dim=-1).clamp(min=1.0)
                    spread.append(float((entropy /
                                         legal.log().clamp(min=1e-6)).mean()))

                # The card net also says what it foresees; the flat one
                # has no such head and leaves that slot empty.
                foresight = judged[3]

                ratio = (newLogp - logps[cut]).exp()
                clipped = torch.clamp(ratio, 1.0 - self.args.clip,
                                      1.0 + self.args.clip)
                # Only over the moves the climber chose itself. What the
                # looking put there still counts everywhere else it counts -
                # it is in the trajectory, so the value head is fitted
                # against what it led to and every later state is one it
                # reached - but the policy is not pushed towards a move it
                # did not make.
                mine = torch.min(ratio * adv[cut], clipped * adv[cut])
                policyLoss = -(mine * own[cut]).sum() / own[cut].sum().clamp(
                    min=1.0)
                valueLoss = functional.mse_loss(value, returns[cut])
                loss = (policyLoss + self.args.value * valueLoss -
                        self.pressure * entropy.mean())

                # And what the trunk was asked to see. Nothing reads these at
                # play: they are here so the trunk has to hold the damage
                # coming and the shape of the fight, which every deck
                # decision is then made on top of.
                if foresight is not None and self.args.foresight > 0.0:
                    loss = loss + self.args.foresight * functional.mse_loss(
                        foresight, seen[cut])

                # And what the looking wanted, whether or not it got to play
                # it. Two thirds of what the looking picks is turned away at
                # the log-probability floor, because handing PPO a move the
                # policy all but rules out puts an astronomical ratio in the
                # loss. Teaching it the move directly is what that floor
                # leaves room for: no ratio, no blow-up, and the moves it
                # most needs to learn are exactly the ones it rates lowest
                # and so never plays.
                if self.args.distil > 0.0 and bool(taught[cut].any()):
                    want = functional.cross_entropy(
                        told, wanted[cut], reduction="none")
                    loss = loss + self.args.distil * (
                        want * taught[cut]).sum() / taught[cut].sum()

                self.opt.zero_grad(set_to_none=True)
                loss.backward()
                nn.utils.clip_grad_norm_(self.net.parameters(),
                                         self.args.clip_grad)
                self.opt.step()
                losses.append(float(loss.detach()))

        self.spread = sum(spread) / len(spread) if spread else 0.0

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

            # What it has been doing to the deck itself. Nothing in the
            # reward speaks of a deck, so whether sharpening and tearing up
            # are being learnt at all can only be read off these.
            # The count of rests goes with them: climbing further passes more
            # fires, so a rise in the sharpening alone says nothing until it
            # is read against the chances it had.
            deck = {name: np.array([one[1][name] for one in recent])
                    for name in ("cards_taken", "cards_removed",
                                 "cards_upgraded", "cards_transformed",
                                 "rests", "curses_chosen",
                                 "curses_refused", "deepest_act")}

            # How far up the spire it is getting. Neither of the numbers
            # above can say it once an act limit is in play: half the climbs
            # put the first boss down, so boss_rate sits at a half and stops
            # moving, and the floors stop at the boss too. What is actually
            # changing after that is the act, and how often more than one
            # boss goes down in the same climb.
            deepest = float(deck["deepest_act"].mean())
            through = float((bosses > 1).mean())

            # Of the times a room put a curse on the table, how often it
            # walked away. This is the one number that says whether the
            # danger is being seen: a curse is handed over rather than
            # offered, so what lands in the deck cannot tell a refusal from
            # a room it never met.
            offers = deck["curses_chosen"] + deck["curses_refused"]
            refusal = (float(deck["curses_refused"].sum() / offers.sum())
                       if offers.sum() > 0 else 0.0)

            line = ("return %7.1f  floors %5.2f  fights %5.2f  boss %5.1f%%"
                    "  win %5.1f%%  act %4.2f  through %4.1f%%"
                    "  sharp %4.2f  torn %4.2f  nocurse %4.0f%%"
                    % (returns.mean(), floors.mean(), fights.mean(),
                       100.0 * (bosses > 0).mean(), 100.0 * wins.mean(),
                       deepest, 100.0 * through,
                       deck["cards_upgraded"].mean(),
                       deck["cards_removed"].mean(), 100.0 * refusal))
            self.noteBest(returns.mean(), floors.mean())
            self.hold()

            row = [self.updates, self.steps, self.episodes,
                   float(returns.mean()), float(floors.mean()),
                   float(fights.mean()), float((bosses > 0).mean()),
                   float(wins.mean()), loss,
                   float(deck["cards_taken"].mean()),
                   float(deck["cards_removed"].mean()),
                   float(deck["cards_upgraded"].mean()),
                   float(deck["cards_transformed"].mean()),
                   float(deck["rests"].mean()),
                   float(deck["curses_chosen"].mean()),
                   float(deck["curses_refused"].mean()),
                   refusal, deepest, through]
        else:
            line = "no climb has ended yet"
            row = [self.updates, self.steps, self.episodes, 0, 0, 0, 0, 0,
                   loss]
            row += [0.0] * (len(CURVE_COLUMNS) - len(row))

        # What is on the shelves, so that a run asked to practise the later
        # acts can be seen to be doing it rather than only to have been told
        # to.
        shelves = ""

        if self.args.deep > 0.0:
            held = [self.vec.deep_held(act) for act in (2, 3)]
            shelves = "  deep %d (%d/%d held)" % (self.deep, held[0], held[1])

        if self.args.look > 1 and self.looked:
            shelves += "  look %.1f%% (%.0f%% barred)" % (
                100.0 * self.overruled / self.looked,
                100.0 * self.barred / max(1, self.overruled + self.barred))
            self.looked = 0
            self.overruled = 0
            self.barred = 0

        print("update %-6d %-11s %s  loss %7.3f  spread %4.2f  "
              "push %5.3f%s  %5.0f moves/s" %
              (self.updates, "(%d climbs)" % self.episodes, line, loss,
               self.spread, self.pressure, shelves,
               (self.steps - self.startSteps) / spent))

        with open(os.path.join(self.folder, "curve.csv"), "a",
                  newline="") as handle:
            csv.writer(handle).writerow(row)

        if self.board is not None:
            # Named from the curve's own columns, so that a column added to
            # one cannot go missing from the other.
            for name, value in zip(CURVE_COLUMNS[3:], row[3:]):
                self.board.add_scalar(name, value, self.steps)

            self.board.flush()

        # What it has been choosing lately, and how those climbs went. The
        # table is cleared afterwards, so every one of these is a window on
        # what the policy is doing now rather than everything since the start.
        # How many climbs the table has counted so far. It is only emptied
        # once it has seen a window's worth of them, so that a relic offered
        # once or twice a climb still has enough behind it to mean anything.
        counted = totals_of(self.vec)
        # Every row, however thinly attended. The page sinks a rate that
        # rests on far less than the rest of its table to the bottom and
        # marks it, so a row seen twice is no longer misleading there - and
        # dropping it outright meant the third act's bosses were missing from
        # the page altogether rather than shown as the two sightings they
        # are. What pick_least still gates is the tensorboard scalars below,
        # where a rate off two climbs is a spike nobody can read past.
        picks = table_of(self.vec, least=1)

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
    parser.add_argument("--entropy", type=float, default=0.01,
                        help="the least the policy is pushed to stay "
                             "undecided by; it is pushed harder than this "
                             "whenever the spread falls under --spread")
    parser.add_argument("--deep", type=float, default=0.0,
                        help="start this share of the climbs part-way up "
                             "rather than at the bottom, from copies the "
                             "climber leaves behind whenever it comes up "
                             "into a new act. Every climb starts on the "
                             "first floor, so the acts it loses in are the "
                             "ones it practises least; these climbs are "
                             "learned from and left out of every table")
    parser.add_argument("--spread", type=float, default=0.12,
                        help="how undecided the policy should stay, as a "
                             "share of the most it could be. A trained "
                             "climber sits near 0.10 and stops trying "
                             "things; 0.12 is what ten times the floor "
                             "actually buys, measured, so the pressure "
                             "settles under its ceiling instead of pinning "
                             "against it")
    parser.add_argument("--patience", type=int, default=4000,
                        help="how many updates without a new best, while the "
                             "spread is up to what was asked, before the "
                             "rate comes down; 0 to leave the rate alone")
    parser.add_argument("--decay", type=float, default=0.5,
                        help="what the rate is multiplied by when it does")
    parser.add_argument("--lr-floor", type=float, default=5e-5,
                        dest="lr_floor",
                        help="and how far down it is allowed to go")
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
                        help="how many climbs a thing has to turn up in "
                             "before its floors and win rate go to "
                             "tensorboard; the page and the csv keep every "
                             "row and mark the thin ones instead")
    parser.add_argument("--max-hp-weight", type=float, default=-1.0,
                        dest="max_hp_weight",
                        help="what a point of the health ceiling is worth, "
                             "charged when a room lowers it and paid when a "
                             "room raises it; below zero keeps the one the "
                             "engine holds")
    parser.add_argument("--curse-penalty", type=float, default=-1.0,
                        dest="curse_penalty",
                        help="what a curse in the deck costs for every floor "
                             "walked with it, against a floor being worth "
                             "one; below zero keeps the engine's own")
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
    parser.add_argument("--foresight", type=float, default=0.1,
                        help="how hard the trunk is held to what it foresees "
                             "- the damage coming, whether the fight is "
                             "nearly over, how far the climb gets. Nothing "
                             "reads these at play; 0 turns the head off")
    parser.add_argument("--distil", type=float, default=0.0,
                        help="how hard to teach the policy the move the "
                             "looking wanted, whether or not the floor let "
                             "it play it. Two thirds of what the looking "
                             "picks is a move the policy rates too low for "
                             "PPO to be handed safely, and this is the way "
                             "those are learned at all")
    parser.add_argument("--look", type=int, default=0,
                        help="out of a fight, walk this many of the policy's "
                             "best moves one step and keep whichever the "
                             "value head thinks most of; 0 or 1 to just "
                             "play. 2 is what measured best - at eight the "
                             "head's largest error is what gets chosen")
    parser.add_argument("--peek", type=int, default=0,
                        help="look this many of the policy's best moves a "
                             "fight ahead before making one, and take "
                             "whichever comes out best; 0 to just play")
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
