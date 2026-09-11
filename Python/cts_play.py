"""Playing a trained climber, with a move looked at before it is made.

The policy names a move by guessing what it comes to. This walks its best
two moves one step each on a copy of the climb and keeps whichever the
value head thinks more of, with what the move paid added to what it left.
Everywhere: in a fight as well as out of one.

That was measured rather than chosen, on fixed seeds with every climb
played to its end and a point of health at 0.01 - twice, once on the 800
seeds the setting was picked on and once on 800 it had never seen, with
the climber of update 245290:

                               seeds 5..804      seeds 1005..1804
                               floors     won     floors     won
    as it likes                 32.99   18.0%      33.50   16.8%
    looks at 2, in a fight      34.09   19.6%      33.80   18.1%
    looks at 2, out of one      35.47   30.1%      35.98   29.8%
    looks at 2, everywhere      37.71   36.1%      37.08   33.9%

So the looking pays most out of a fight, where the next state is known,
and a little in one, where a step is followed by cards nobody has drawn
yet; and the two together pay more than either alone. Two moves and not
more: at three the wins fall back to 31% and at four to 26%, because the
largest of several noisy readings is mostly the largest mistake. The runs
are in Notes/look-trained-head-2026-09-11.txt.

    python cts_play.py runs/ironclad              # a hundred climbs, counted
    python cts_play.py runs/ironclad 500          # five hundred
    python cts_play.py runs/ironclad 100 --flat   # the policy as named
    python cts_play.py runs/ironclad --outside    # no looking in a fight

Older: ``--fights`` turns on the search this file used to be about, which
plays each candidate a whole fight ahead by a rule of thumb. Asked the same
way over 800 climbs it came out level with playing flat, and laid over the
looking above it takes most of the gain back - 300 climbs here came out at
14.7% won with it against 26.0% without - so it is off.

This belongs at play. Trained through, the looking is safe only when the
policy is judged on its own moves and never taught what the looking picked
- both of which the trainer now does - and even then it is a third of the
speed. Here there is one climb to get right and time to think about it.
"""

import argparse
import os
import sys

import numpy as np

try:
    import torch
except ImportError:  # pragma: no cover
    print("this needs torch: pip install torch")
    raise

from cts_ask import exactly, looksAhead, setHealthWeight
from cts_env import PHASES, SpireEnv, action_table
from cts_net import CardPolicy, load_weights
from cts_vec import VecSpireEnv

# How many of the policy's own best moves the old fight search weighs.
# Six was enough to show the whole effect; more costs time for little.
CONSIDER = 6

# How many of the policy's best moves are walked a step out of a fight. Two
# is what measured best; at eight the value head's largest error is what
# gets chosen and the climb loses half its floors.
LOOKS = 2

#! Where a fight is, in the block of the state that says where the climber
#! stands - a card being chosen mid-fight included.
FIGHTING = (PHASES.index("battle"), PHASES.index("boss"),
            PHASES.index("choosing"))


def load(folder, device):
    """The climber saved in \\p folder, ready to play."""
    # A folder may hold two: the working weights, which are whatever
    # the run was doing when it last saved, and the best it ever had.
    # A run that has since got worse writes over the first and not the
    # second, so the best is what anybody watching would want to see.
    best = os.path.join(folder, "best.pt")
    path = os.path.join(folder, "checkpoint.pt")

    if os.path.exists(best):
        path = best

    if not os.path.exists(path):
        raise SystemExit("no climber in %s" % folder)

    print("playing %s" % path)

    kept = torch.load(path, map_location=device, weights_only=False)

    if kept.get("kind") != "card":
        raise SystemExit("this plays the card net; that one is %s"
                         % kept.get("kind"))

    plan = SpireEnv()

    for name, mine in [("floats", plan.observation_size),
                       ("ids", plan.id_count),
                       ("actions", plan.action_count)]:
        if kept.get(name) != mine:
            raise SystemExit(
                "the checkpoint was made for %s=%s and this engine is %s" %
                (name, kept.get(name), mine))

    net = CardPolicy(plan.layout, plan.id_layout, action_table(),
                     width=kept["width"]).to(device)
    load_weights(net, kept["net"])
    net.eval()

    return net, kept


def play(net, kept, device, climbs, envs, looks, fights, hp=None,
         outside=False):
    """Plays \\p climbs and returns how they went.

    \\p looks is how many moves are walked a step before one is made, 0 for
    none; \\p outside keeps that out of the fights, the way it was until
    2026-09-11; \\p fights turns on the older whole-fight search inside one.
    The climbs are seeds 0 to \\p climbs - 1, every one played to its end, so
    two runs of this with different settings are the same climbs compared.
    A point of health costs what it cost in training, read from the
    checkpoint or \\p hp.
    """
    vec = VecSpireEnv(envs)
    vec.set_act_limit(kept["acts"])
    setHealthWeight(vec, kept, hp)

    plan = SpireEnv()
    phaseAt = plan.layout["phase"]
    where = tuple(i for i in range(len(PHASES))
                  if not outside or i not in FIGHTING)
    looking = looksAhead(net, device, where, looks) if looks > 1 else None

    def decide(obs, ids, mask):
        legal = np.asarray(mask, dtype=np.uint8)
        flat = np.asarray(obs, dtype=np.float32).reshape(envs, -1)
        named = np.asarray(ids)

        with torch.no_grad():
            scores, _, _ = net.forward(
                torch.as_tensor(flat, device=device).float(),
                torch.as_tensor(named, device=device).long())
            allowed = torch.as_tensor(legal, device=device).bool()
            scores = scores.masked_fill(~allowed, -1e9)
            picks = scores.argmax(dim=1)

            if looking is not None:
                # The best two walked a step each and the one worth more
                # kept. With --outside the rows in a fight get their own
                # move back untouched.
                stood = flat[:, phaseAt:phaseAt + len(PHASES)].argmax(axis=1)
                said = looking(vec, flat, named, legal, scores.cpu().numpy(),
                               stood)
                picks = torch.as_tensor(said, device=device).long()

            if fights:
                # The policy's best few, in its own order, and the engine
                # says which of them the fight comes out of best. A slot the
                # mask does not allow reads as empty on the other side.
                top = torch.topk(scores, min(CONSIDER, scores.shape[1]),
                                 dim=1).indices
                offered = torch.where(allowed.gather(1, top), top,
                                      torch.full_like(top, vec.action_count))
                ranked = vec.rank(offered.cpu().numpy().astype(np.uintp))
                ranked = torch.as_tensor(ranked, device=device).long()

                # Only a move that is really there and really legal.
                safe = ranked.clamp(max=vec.action_count - 1)
                usable = ((ranked < vec.action_count)
                          & allowed.gather(1, safe[:, None]).squeeze(1))
                picks = torch.where(usable, safe, picks)

        return picks.cpu().numpy()

    return exactly(vec, kept["character"], list(range(climbs)), decide)


def main(argv):
    # argparse and not a walk over the words: the old walk took every bare
    # number for the climb count, so "800 --looks 3" played three climbs.
    parser = argparse.ArgumentParser(
        description="Play a trained climber, a move looked at before it is "
                    "made.")
    parser.add_argument("folder", nargs="?", default="runs/ironclad")
    parser.add_argument("climbs", nargs="?", type=int, default=100,
                        help="how many climbs, seeds 0 to climbs-1, every "
                             "one played to its end")
    parser.add_argument("--envs", type=int, default=64)
    parser.add_argument("--flat", action="store_true",
                        help="the policy as named, no looking")
    parser.add_argument("--looks", type=int, default=LOOKS,
                        help="how many of the policy's best moves to walk a "
                             "step out of a fight")
    parser.add_argument("--outside", action="store_true",
                        help="look only out of a fight, as before 2026-09-11")
    parser.add_argument("--fights", action="store_true",
                        help="the older whole-fight search inside a fight")
    parser.add_argument("--hp-weight", type=float, default=None,
                        dest="hp_weight",
                        help="what a point of health cost in training, when "
                             "the checkpoint does not say")
    args = parser.parse_args(argv[1:])

    climbs = args.climbs
    envs = args.envs
    looks = 0 if args.flat else args.looks
    outside = args.outside
    fights = args.fights
    hp = args.hp_weight

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    net, kept = load(args.folder, device)

    how = ("flat out" if looks < 2
           else "looking at %d moves %s" % (
               looks, "out of a fight" if outside else "everywhere"))

    if fights:
        how += ", and the fights looked into"

    print("%s at update %d (%s climbs trained), act limit %d"
          % (kept["character"], kept["updates"],
             "{:,}".format(kept["episodes"]), kept["acts"]))
    print("playing %d climbs %s" % (climbs, how))

    got = play(net, kept, device, climbs, envs, looks, fights, hp, outside)

    floors = np.array([one["floors"] for one in got])
    bosses = np.array([one["bosses_won"] for one in got])
    print()
    print("  floors, on average    : %.2f" % floors.mean())
    print("  deepest               : %d" % floors.max())
    print("  put a boss down       : %.1f%%" % (100.0 * (bosses > 0).mean()))
    print("  put two down          : %.1f%%" % (100.0 * (bosses > 1).mean()))
    print("  won the spire         : %.1f%%"
          % (100.0 * np.mean([one["won_the_spire"] for one in got])))

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
