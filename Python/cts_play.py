"""Playing a trained climber, with the fights looked into before moving.

The trainer plays flat out: one look at the state, one move, no simulation,
because a search costs more than half the throughput and a run needs
millions of climbs. Playing is the other way round - there is one climb to
get right and time to think about it - so this looks each candidate move a
whole fight ahead and takes whichever comes out best.

Measured on a climber at 5.3M climbs, over 600 climbs each way:

                        floors   one boss   two bosses
    flat out             21.88      85.8%         5.0%
    looking ahead        22.48      79.3%        10.3%

It is a trade, not a free gain. The rule of thumb the search plays out with
blocks only what is coming and spends everything else on damage, so it
prefers fights that end quickly: slightly reckless in act one, where the
climber could have afforded to be careful, and necessary in act two, where
a long fight is a lost one. Twice as many climbs come out the top of act
two for it, at six points off the act-one boss.

    python cts_play.py runs/ironclad              # a hundred climbs, counted
    python cts_play.py runs/ironclad 500          # five hundred
    python cts_play.py runs/ironclad 100 --flat   # without the search

The search only ever re-orders moves the policy itself rates highly - it
changes the answer on about two fights in five - so it is the climber's own
play sharpened rather than a different player. Training through it makes
things worse: the policy leans on the search and goes soft, and judged with
the search on it came out 21.79 floors against 23.86 for one trained flat
out. So this belongs at play and nowhere else.
"""

import os
import sys

import numpy as np

try:
    import torch
except ImportError:  # pragma: no cover
    print("this needs torch: pip install torch")
    raise

from cts_env import SpireEnv, action_table
from cts_log import vec_summaries
from cts_net import CardPolicy
from cts_vec import VecSpireEnv

# How many of the policy's own best moves the search is allowed to weigh.
# Six was enough to show the whole effect; more costs time for little.
CONSIDER = 6


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


def play(net, kept, device, climbs, envs, search):
    """Plays \\p climbs and returns how they went."""
    vec = VecSpireEnv(envs)
    vec.set_act_limit(kept["acts"])
    obs, ids, mask = vec.reset(kept["character"], 0)

    done = []

    while len(done) < climbs:
        legal = np.asarray(mask, dtype=np.uint8)

        with torch.no_grad():
            scores, _, _ = net.forward(
                torch.as_tensor(np.asarray(obs), device=device).float(),
                torch.as_tensor(np.asarray(ids), device=device).long())
            allowed = torch.as_tensor(legal, device=device).bool()
            scores = scores.masked_fill(~allowed, -1e9)
            picks = scores.argmax(dim=1)

            if search:
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
    search = "--flat" not in argv

    for one in argv[2:]:
        if one.isdigit():
            climbs = int(one)

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    net, kept = load(folder, device)

    print("%s at update %d (%s climbs trained), act limit %d"
          % (kept["character"], kept["updates"],
             "{:,}".format(kept["episodes"]), kept["acts"]))
    print("playing %d climbs %s"
          % (climbs, "with the fights looked into" if search else "flat out"))

    got = play(net, kept, device, climbs, envs, search)

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
