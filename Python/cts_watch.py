"""One climb, played the way play.bat plays, written down.

cts_play counts two hundred climbs and tells you how many were won. This
plays one and says what it did: every card taken and passed over, every
relic, every room answered, every fight, floor by floor. It plays exactly
as cts_play does with --turn - the turn searched inside a fight, the best
two moves walked outside one - so what is written down is what the counted
climbs were doing.

    python cts_watch.py runs/ironclad/best-look2.pt
    python cts_watch.py runs/ironclad/best-look2.pt --seed 500007
    python cts_watch.py runs/ironclad/best-look2.pt --until-won

``--until-won`` plays seeds one after another until one wins and writes
that one down, which is usually the interesting one to read.
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

from cts_ask import looksAhead
from cts_env import PHASES, SpireEnv, action_table
from cts_log import describe, lines_of, summary_of
from cts_net import CardPolicy, load_weights
from cts_turn import turnSearch
from cts_vec import VecSpireEnv

#! Where a fight is - a card being chosen mid-fight included.
FIGHTING = (PHASES.index("battle"), PHASES.index("boss"),
            PHASES.index("choosing"))

#! What is written down by default: the things the climber chose, and the
#! things that happened to it because of them. A climb writes about a
#! thousand lines and nearly half are one per monster per turn, which
#! drowns the twenty or thirty choices that decide it.
CHOICES = (
    "card_taken", "card_passed", "card_bought", "card_removed",
    "card_upgraded", "card_transformed", "relic_taken", "relic_bought",
    "relic_lost", "potion_taken", "potion_bought", "potion_drunk",
    "potion_thrown", "room_entered", "room_answered", "rested",
    "gold_spent", "floor_walked", "fight_won", "act_started", "died",
    "spire_done",
)

#! And with --cards, every card it played as well.
IN_A_FIGHT = ("card_played",)


def written(env, kinds):
    """The climb's record, as lines, keeping only \\p kinds."""
    return [describe(line) for line in lines_of(env)
            if kinds is None or line["entry"] in kinds]


def load(path, device):
    """A climber and what it was trained with."""
    if os.path.isdir(path):
        for name in ("best-look2.pt", "best.pt", "checkpoint.pt"):
            if os.path.exists(os.path.join(path, name)):
                path = os.path.join(path, name)
                break

    if not os.path.exists(path):
        raise SystemExit("no climber at %s" % path)

    kept = torch.load(path, map_location=device, weights_only=False)
    plan = SpireEnv()
    net = CardPolicy(plan.layout, plan.id_layout, action_table(),
                     width=kept["width"]).to(device)
    load_weights(net, kept["net"])
    net.eval()

    return net, kept, path


def climb(net, kept, device, seed, turn=True, hp=0.01, width=4, budget=120):
    """Plays one climb and returns the engine it was played in.

    The engine is a batch of one, because walking a turn on a copy lives
    on the batch. What comes back still holds the log of the climb.
    """
    plan = SpireEnv()
    phaseAt = plan.layout["phase"]
    vec = VecSpireEnv(1)
    vec.set_act_limit(kept["acts"])
    vec.set_health_weight(hp)
    vec.set_auto_reset(False)

    searching = (turnSearch(net, device, width=width, budget=budget)
                 if turn else None)
    outside = tuple(i for i in range(len(PHASES)) if i not in FIGHTING)
    looking = looksAhead(net, device, outside, 2,
                         gamma=kept.get("gamma", 0.999))

    obs, ids, mask = vec.reset(kept["character"], seed=seed)

    for step in range(8000):
        flat = np.asarray(obs, dtype=np.float32).reshape(1, -1)
        named = np.asarray(ids).reshape(1, -1)
        legal = np.asarray(mask, dtype=np.uint8).reshape(1, -1)

        if not legal.any():
            break

        with torch.no_grad():
            scores, _, _ = net(torch.as_tensor(flat, device=device),
                               torch.as_tensor(named, device=device).long())

        allowed = torch.as_tensor(legal.astype(bool), device=device)
        scores = scores.masked_fill(~allowed, -1e9)
        move = int(scores.argmax(dim=1).item())

        # Out of a fight, the best two walked a step each.
        stood = flat[:, phaseAt:phaseAt + len(PHASES)].argmax(axis=1)
        said = looking(vec, flat, named, legal, scores.cpu().numpy(), stood)
        move = int(said[0])

        # In one, the whole turn played out on a copy.
        if searching is not None:
            asked = searching(vec, 0, flat[0], named[0], legal[0])

            if asked is not None:
                move = int(asked)

        obs, ids, mask, _, dones, _ = vec.step(np.array([move],
                                                        dtype=np.int64))

        if np.asarray(dones).reshape(-1)[0]:
            break

    return vec


def main(argv):
    parser = argparse.ArgumentParser(
        description="Play one climb and write down what it did.")
    parser.add_argument("climber", nargs="?", default="runs/ironclad")
    parser.add_argument("--seed", type=int, default=500005)
    parser.add_argument("--until-won", action="store_true",
                        dest="untilWon",
                        help="play seeds in turn until one is won")
    parser.add_argument("--tries", type=int, default=20,
                        help="how many seeds --until-won may spend")
    parser.add_argument("--cards", action="store_true",
                        help="every card it played in a fight as well")
    parser.add_argument("--all", action="store_true", dest="everything",
                        help="the whole record, a line a monster a turn")
    parser.add_argument("--flat", action="store_true",
                        help="the policy's own moves, no turn searched")
    parser.add_argument("--width", type=int, default=4)
    parser.add_argument("--budget", type=int, default=120)
    parser.add_argument("--hp-weight", type=float, default=0.01,
                        dest="hp_weight")
    args = parser.parse_args(argv[1:])

    device = torch.device("cpu")
    torch.set_num_threads(4)
    net, kept, path = load(args.climber, device)

    print("%s at update %d, act limit %d" % (path, kept["updates"],
                                             kept["acts"]))
    print("playing %s" % ("its own moves" if args.flat
                          else "the turn searched in fights, two moves "
                               "walked outside one"))
    print()

    seed = args.seed
    tries = args.tries if args.untilWon else 1

    for at in range(tries):
        vec = climb(net, kept, device, seed + at, turn=not args.flat,
                    hp=args.hp_weight, width=args.width,
                    budget=args.budget)
        counts = summary_of(vec.at(0))

        if not args.untilWon or counts["won_the_spire"]:
            kinds = None if args.everything else (
                CHOICES + IN_A_FIGHT if args.cards else CHOICES)

            print("=== seed %d ===" % (seed + at))

            for line in written(vec.at(0), kinds):
                print(line)

            print()
            print("floors %d, act %d, fights won %d (%d elite, %d boss)"
                  % (counts["floors"], counts["act"], counts["fights_won"],
                     counts["elites_won"], counts["bosses_won"]))
            passed = len([1 for one in lines_of(vec.at(0))
                           if one["entry"] == "card_passed"])

            print("cards: %d taken, %d passed on, %d torn up, %d sharpened"
                  % (counts["cards_taken"], passed,
                     counts["cards_removed"], counts["cards_upgraded"]))
            print("relics %d, potions %d drunk, rests %d, gold %d earned"
                  % (counts["relics_taken"], counts["potions_drunk"],
                     counts["rests"], counts["gold_earned"]))
            print("%s" % ("won the spire" if counts["won_the_spire"]
                          else "died on floor %d" % counts["floors"]))

            return 0

        print("seed %d: %d floors, died" % (seed + at, counts["floors"]))

    print()
    print("none of those %d won; try --tries with a larger number" % tries)

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
