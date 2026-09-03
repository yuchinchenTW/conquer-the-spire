"""Playing a trained climber, with a move looked at before it is made.

The policy names a move by guessing what it comes to. Out of a fight, this
walks its best two moves one step each on a copy of the climb and keeps
whichever the value head thinks more of, with what the move paid added to
what it left. In a fight it plays the policy's move as named.

Both halves of that were measured rather than chosen. Over 800 climbs on the
same seeds, on the climber at 7.1M climbs:

                               floors     won
    as it likes                 32.08    15.2%
    looks at 2, in a fight      32.55    13.6%
    looks at 2, out of one      35.50    28.0%
    looks at 2, everywhere      36.17    28.5%

So the looking pays outside the fights and not in them - a step into a
fight is followed by cards nobody has drawn yet, and the head cannot tell
one move from another through that - and it pays a great deal: nearly
double the wins. Two moves and not more, because at eight the largest of
eight noisy readings is mostly the largest mistake and the climb loses half
its floors.

    python cts_play.py runs/ironclad              # a hundred climbs, counted
    python cts_play.py runs/ironclad 500          # five hundred
    python cts_play.py runs/ironclad 100 --flat   # the policy as named

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

import os
import sys

import numpy as np

try:
    import torch
except ImportError:  # pragma: no cover
    print("this needs torch: pip install torch")
    raise

from cts_ask import looksAhead
from cts_env import PHASES, SpireEnv, action_table
from cts_log import vec_summaries
from cts_net import CardPolicy
from cts_vec import VecSpireEnv

# How many of the policy's own best moves the old fight search weighs.
# Six was enough to show the whole effect; more costs time for little.
CONSIDER = 6

# How many of the policy's best moves are walked a step out of a fight. Two
# is what measured best; at eight the value head's largest error is what
# gets chosen and the climb loses half its floors.
LOOKS = 2

#! Where a fight is, in the block of the state that says where the climber
#! stands.
FIGHTING = (PHASES.index("battle"), PHASES.index("boss"))


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
    net.load_state_dict(kept["net"])
    net.eval()

    return net, kept


def play(net, kept, device, climbs, envs, looks, fights):
    """Plays \\p climbs and returns how they went.

    \\p looks is how many moves are walked a step out of a fight, 0 for none;
    \\p fights turns on the older whole-fight search inside one.
    """
    vec = VecSpireEnv(envs)
    vec.set_act_limit(kept["acts"])
    obs, ids, mask = vec.reset(kept["character"], 0)

    plan = SpireEnv()
    phaseAt = plan.layout["phase"]
    outside = tuple(i for i in range(len(PHASES)) if i not in FIGHTING)
    looking = looksAhead(net, device, outside, looks) if looks > 1 else None
    done = []

    while len(done) < climbs:
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
                # Out of a fight, the best two walked a step each and the
                # one worth more kept. The rows in a fight get their own
                # move back untouched: there the walking is worth nothing
                # and costs a copy of the fight for every move.
                where = flat[:, phaseAt:phaseAt + len(PHASES)].argmax(axis=1)
                said = looking(vec, flat, named, legal, scores.cpu().numpy(),
                               where)
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

        obs, ids, mask, reward, ended, info = vec.step(
            picks.cpu().numpy().astype(np.int32))

        if not ended.any():
            continue

        counts = vec_summaries(vec, last=True)

        for row in range(envs):
            if ended[row]:
                done.append(counts[row])

    return done[:climbs]


def main(argv):
    folder = argv[1] if len(argv) > 1 else "runs/ironclad"
    climbs = 100
    envs = 64
    looks = 0 if "--flat" in argv else LOOKS
    fights = "--fights" in argv

    for at, one in enumerate(argv[2:], start=2):
        if one.isdigit():
            climbs = int(one)
        elif one == "--looks" and at + 1 < len(argv):
            looks = int(argv[at + 1])

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    net, kept = load(folder, device)

    how = ("flat out" if looks < 2
           else "looking at %d moves out of a fight" % looks)

    if fights:
        how += ", and the fights looked into"

    print("%s at update %d (%s climbs trained), act limit %d"
          % (kept["character"], kept["updates"],
             "{:,}".format(kept["episodes"]), kept["acts"]))
    print("playing %d climbs %s" % (climbs, how))

    got = play(net, kept, device, climbs, envs, looks, fights)

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
