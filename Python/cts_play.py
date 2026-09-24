"""Playing a trained climber, with what it is about to do played out first.

The policy names a move by guessing what it comes to. Two things are done
with that guess, and which one pays depends entirely on where the climber
is standing.

Out of a fight, the best two moves are each walked one step on a copy of
the climb and whichever the value head thinks more of is kept. In a fight
that is nearly worthless, because a turn is a sequence: block only counts
once the monsters have swung, and they swing inside the move that ends
the turn, so the first card of a turn says almost nothing about what the
turn comes to. So in a fight ``--turn`` plays the whole turn out instead -
sequences grown a move at a time, four carried, scored by what they paid
plus where they left the climb, and a sequence that ends the turn scored
after the monsters have answered. See cts_turn.py.

Two hundred climbs a reading, every one played to its end, on the weights
of update 387360:

    how it played                   seeds 300005    seeds 500005
                                    won   floors    won   floors
    the policy names its move      29.0%   35.78   25.5%   35.92
    looks at 2, everywhere         36.0%   36.91   39.0%   38.54
    looks at 2 out, turn searched  71.5%   44.24   72.0%   44.11

The second seed set had never been measured on. Twice the wins of the
same weights naming their own moves, and the floors, the bosses and the
wins all agree with each other. Notes/the-turn-search-on-whole-climbs-
2026-09-24.txt has the rest, including an older checkpoint doing better
still (74.5%) with the same search in front of it.

It is a flag and not the default because of what it costs: about 30ms a
decision in a fight, so a climb takes nine seconds rather than a tenth of
one.

    python cts_play.py runs/ironclad --turn       # the strongest, and slow
    python cts_play.py runs/ironclad              # looks 2 everywhere
    python cts_play.py runs/ironclad 500          # five hundred climbs
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
from cts_turn import turnSearch
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


def checkpoint_path(folder, mode="look2"):
    """An explicit file, or the winner for the requested playing mode."""
    if os.path.isfile(folder):
        return os.fspath(folder)
    for name in ("best-%s.pt" % mode, "best.pt", "checkpoint.pt"):
        path = os.path.join(folder, name)
        if os.path.isfile(path):
            return path
    raise SystemExit("no climber in %s" % folder)


def load(folder, device, mode="look2"):
    """The climber saved in \\p folder, ready to play."""
    # Prefer the winner for this playing mode, then the legacy return winner.
    # An explicit file bypasses selection for same-checkpoint comparisons.
    path = checkpoint_path(folder, mode)

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
         outside=False, seed=0, gamma=None, turn=False, width=4,
         budget=120):
    """Plays \\p climbs and returns how they went.

    \\p looks is how many moves are walked a step before one is made, 0 for
    none; \\p outside keeps that out of the fights, the way it was until
    2026-09-11; \\p fights turns on the older whole-fight search inside one.
    The climbs start at \\p seed, every one played to its end, so
    two runs of this with different settings are the same climbs compared.
    A point of health costs what it cost in training, read from the
    checkpoint or \\p hp.
    """
    if climbs <= 0 or envs <= 0 or seed < 0 or seed + climbs > 2**32:
        raise ValueError("positive climbs/envs and unsigned 32-bit seeds "
                         "required")
    vec = VecSpireEnv(min(envs, climbs))
    envs = vec.count
    vec.set_act_limit(kept["acts"])
    setHealthWeight(vec, kept, hp)

    plan = SpireEnv()
    phaseAt = plan.layout["phase"]

    # With the turn search on, the one-move looking keeps out of fights:
    # the search is what answers there, and the two would be asking the
    # same question with the worse tool winning ties.
    where = tuple(i for i in range(len(PHASES))
                  if not (outside or turn) or i not in FIGHTING)
    searching = (turnSearch(net, device, width=width, budget=budget)
                 if turn else None)
    discount = kept.get("gamma", 0.999) if gamma is None else gamma
    looking = (looksAhead(net, device, where, looks, gamma=discount)
               if looks > 1 else None)

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
                # kept. With --outside or --turn the rows in a fight get
                # their own move back untouched.
                stood = flat[:, phaseAt:phaseAt + len(PHASES)].argmax(axis=1)
                said = looking(vec, flat, named, legal, scores.cpu().numpy(),
                               stood)
                picks = torch.as_tensor(said, device=device).long()

            if searching is not None:
                # A row in a fight plays its turn out on a copy first. The
                # search answers None where there is nothing to search -
                # out of a fight, or with one move on offer - and the row
                # keeps what it had.
                chosen = picks.cpu().numpy()

                for row in range(envs):
                    if not legal[row].any():
                        continue

                    move = searching(vec, row, flat[row], named[row],
                                     legal[row])

                    if move is not None:
                        chosen[row] = move

                picks = torch.as_tensor(chosen, device=device).long()

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

    return exactly(vec, kept["character"], list(range(seed, seed + climbs)),
                   decide)


def main(argv):
    # argparse and not a walk over the words: the old walk took every bare
    # number for the climb count, so "800 --looks 3" played three climbs.
    parser = argparse.ArgumentParser(
        description="Play a trained climber, a move looked at before it is "
                    "made.")
    parser.add_argument("folder", nargs="?", default="runs/ironclad")
    parser.add_argument("climbs", nargs="?", type=int, default=100,
                        help="how many consecutive seeds, every "
                             "one played to its end")
    parser.add_argument("--envs", type=int, default=64)
    parser.add_argument("--seed", type=int, default=0,
                        help="first seed; every seed in the range is played "
                             "to its end")
    parser.add_argument("--flat", action="store_true",
                        help="the policy as named, no looking")
    parser.add_argument("--looks", type=int, default=LOOKS,
                        help="how many of the policy's best moves to walk a "
                             "step out of a fight")
    parser.add_argument("--outside", action="store_true",
                        help="look only out of a fight, as before 2026-09-11")
    parser.add_argument("--turn", action="store_true",
                        help="search the whole turn inside a fight")
    parser.add_argument("--width", type=int, default=4,
                        help="how many sequences the turn search carries")
    parser.add_argument("--budget", type=int, default=120,
                        help="how many sequences a decision may walk")
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
    net, kept = load(args.folder, device,
                     mode="flat" if looks < 2 else "look2")

    how = ("flat out" if looks < 2
           else "looking at %d moves %s" % (
               looks, "out of a fight" if (outside or args.turn)
               else "everywhere"))

    if args.turn:
        how += ", and the turn searched in one (%d wide, %d a decision)" % (
            args.width, args.budget)

    if fights:
        how += ", and the fights looked into"

    print("%s at update %d (%s climbs trained), act limit %d"
          % (kept["character"], kept["updates"],
             "{:,}".format(kept["episodes"]), kept["acts"]))
    print("playing %d climbs %s" % (climbs, how))

    got = play(net, kept, device, climbs, envs, looks, fights, hp, outside,
               seed=args.seed, turn=args.turn, width=args.width,
               budget=args.budget)

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
