"""Asks what one kind of choice is worth, by taking it away from the climber.

A pick rate says what the climber does. It does not say whether doing it was
right, and the two come apart badly: a climber that turns down a card at every
pile looks decisive and is throwing floors away, and a climber that never
sharpens anything may be right to, because a fire is thirty health as well as
a whetstone.

So this plays the same climbs twice over. The policy makes every move except
the one being asked about; that one is overruled the same way every time. The
seeds are the same in both columns, so what is left between them is the
question.

    python Python/cts_ask.py runs/ironclad --ask fire
    python Python/cts_ask.py runs/ironclad --ask draft --climbs 800
    python Python/cts_ask.py runs/ironclad --ask removal

Read the answers against the error beside them. A gap smaller than that is not
a small difference, it is no difference: the choice does not matter, which is
worth knowing before anything is built to make the climber better at it.
"""

import argparse
import os
import sys

import numpy as np
import torch

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from cts_env import SpireEnv, action_table
from cts_log import vec_summaries
from cts_net import CardPolicy
from cts_vec import VecSpireEnv

#! The two cards a climber starts with five and four of.
STRIKE = 1
DEFEND = 2


class Table:
    """What the moves of the fixed head are, looked up by kind."""

    def __init__(self):
        self.names, self.a, self.b = action_table()

    def of(self, *kinds):
        """The slots of every move of \\p kinds."""
        return [i for i, kind in enumerate(self.names) if kind in kinds]

    def kind(self, at):
        return self.names[at]


def _best(scores, row, among):
    """The one of \\p among the policy thinks most of."""
    return max(among, key=lambda i: scores[row, i]) if among else None


def wantsKind(*kinds):
    """Overrules a move to be of \\p kinds, wherever one is on offer."""
    def asked(table, legal, scores, ids, obs, row):
        return _best(scores, row, [i for i in table.of(*kinds)
                                   if legal[row, i]])

    return asked


def wantsCard(card, deckAt):
    """Overrules a move that names a deck slot to name \\p card.

    Only where the policy was already minded to take a card out, so that the
    question is which card rather than whether to bother - otherwise the two
    columns take a different number of cards out and the difference is not
    about the card at all.
    """
    outs = ("buy_removal", "toke", "choose_option")

    def asked(table, legal, scores, ids, obs, row):
        if table.kind(int(np.argmax(np.where(legal[row], scores[row],
                                             -1e9)))) not in outs:
            return None

        among = []

        for at, kind in enumerate(table.names):
            if kind not in outs or not legal[row, at]:
                continue

            slot = table.a[at] if kind != "choose_option" else table.b[at]

            if slot >= 0 and ids[row, deckAt + slot] == card:
                among.append(at)

        return _best(scores, row, among)

    return asked


def wantsAnyOffer(seed):
    """Overrules which of the cards on offer is taken, not whether to take one.

    A pile that holds more than one card is on the head as one move per card,
    all of them naming the same pile. So the cards on offer are the moves that
    name the pile the policy had already settled on, and overruling among
    those and no others asks which card while leaving whether to take a card
    alone - the two questions come apart, and the coarser one is already a
    column of its own.

    A die rather than a fixed slot: the first card on a pile is not a card,
    it is whatever the pile happened to put there, so always taking it would
    be a third policy rather than the absence of one.
    """
    die = np.random.RandomState(seed)

    def asked(table, legal, scores, ids, obs, row):
        pick = int(np.argmax(np.where(legal[row], scores[row], -1e9)))

        if table.kind(pick) != "claim_reward":
            return None

        pile = table.a[pick]
        among = [at for at, kind in enumerate(table.names)
                 if kind == "claim_reward" and table.a[at] == pile and
                 table.b[at] >= 0 and legal[row, at]]

        # One card on the pile is not a choice, and neither is gold or a
        # relic, which come up as a single move naming no card at all.
        return int(die.choice(among)) if len(among) > 1 else None

    return asked


def drinksUnder(mark, healthAt):
    """Reaches for the belt whenever health is under \\p mark and there is
    something in it.

    Which potion is left to the climber: among the ones it may drink, the one
    it thinks most of. What is being taken away is only *when*, because the
    when is what the climbs that died on the road got wrong - they walked into
    the fight that killed them at a third of their health with a third of them
    still carrying something.
    """
    def asked(table, legal, scores, ids, obs, row):
        if obs[row, healthAt] >= mark:
            return None

        return _best(scores, row, [i for i in table.of("use_potion")
                                   if legal[row, i]])

    return asked


def neverDrinks():
    """Never reaches for the belt at all.

    The far end of the same question, and the one that says how much of the
    answer is even there to be had: if a climber forbidden its potions is
    barely worse than one left alone, then when it drinks cannot be worth much
    either, whatever the other columns say.
    """
    def asked(table, legal, scores, ids, obs, row):
        dry = [i for i, kind in enumerate(table.names)
               if kind != "use_potion" and legal[row, i]]

        if not dry:
            return None

        wanted = int(np.argmax(np.where(legal[row], scores[row], -1e9)))

        if table.kind(wanted) != "use_potion":
            return None

        return _best(scores, row, dry)

    return asked


def wandersOff(seed):
    """Takes whichever way up is nearest to hand rather than the chosen one.

    The last thing on the road that is a choice rather than a fight. A climber
    bleeds out through the second act, and if the bleeding is in where it
    walks then throwing the walking away costs a great deal; if it is not,
    this column reads the same as being left alone and the road is not where
    the answer is.
    """
    die = np.random.RandomState(seed)

    def asked(table, legal, scores, ids, obs, row):
        ways = [i for i in table.of("travel") if legal[row, i]]

        # One way on is not a fork, and a step that is not a step at all is
        # not this question.
        return int(die.choice(ways)) if len(ways) > 1 else None

    return asked


#! What can be asked, and what each column overrules.
def questions(deckAt, healthAt=0):
    return {
        "fire": ("what a fire is worth",
                 [("as it likes", None),
                  ("rest, always", wantsKind("rest")),
                  ("sharpen, always", wantsKind("smith"))]),
        "draft": ("what drafting is worth",
                  [("as it likes", None),
                   ("any of the offer", wantsAnyOffer(11)),
                   ("take every card", wantsKind("claim_reward")),
                   ("take none of them", wantsKind("skip_reward"))]),
        "drink": ("when the belt is worth reaching for",
                  [("as it likes", None),
                   ("under half, always", drinksUnder(0.5, healthAt)),
                   ("under a third, always", drinksUnder(1 / 3.0, healthAt)),
                   ("never at all", neverDrinks())]),
        "path": ("what choosing the way up is worth",
                 [("as it likes", None),
                  ("any way up", wandersOff(23))]),
        "removal": ("which card is worth tearing out",
                    [("as it likes", None),
                     ("a Strike, always", wantsCard(STRIKE, deckAt)),
                     ("a Defend, always", wantsCard(DEFEND, deckAt))]),
    }


def load(folder, device):
    """The climber saved in \\p folder, if it was built for this engine."""
    path = os.path.join(folder, "checkpoint.pt")

    if not os.path.exists(path):
        raise SystemExit("no climber in %s" % folder)

    kept = torch.load(path, map_location=device, weights_only=False)
    plan = SpireEnv()

    for name, mine in [("floats", plan.observation_size),
                       ("ids", plan.id_count),
                       ("actions", plan.action_count)]:
        if kept.get(name) != mine:
            raise SystemExit(
                "the checkpoint was made for %s=%s and this engine is %s"
                % (name, kept.get(name), mine))

    net = CardPolicy(plan.layout, plan.id_layout, action_table(),
                     width=kept["width"]).to(device)
    net.load_state_dict(kept["net"])
    net.eval()

    return net, kept, plan


def played(net, kept, plan, device, overrule, climbs, rows, seed):
    """Plays \\p climbs with \\p overrule having the last word."""
    table = Table()
    vec = VecSpireEnv(rows)

    vec.set_act_limit(kept["acts"])

    obs, ids, mask = vec.reset(kept["character"], seed)
    floors = []
    bosses = []
    wins = []
    forced = 0

    while len(floors) < climbs:
        legal = np.asarray(mask, dtype=np.uint8)
        named = np.asarray(ids)

        with torch.no_grad():
            logits, _, _ = net.forward(
                torch.as_tensor(np.asarray(obs), device=device).float(),
                torch.as_tensor(named, device=device).long())

        scores = logits.cpu().numpy()
        scores[legal == 0] = -1e9
        picks = scores.argmax(axis=1)
        flat = np.asarray(obs, dtype=np.float32).reshape(rows, -1)

        if overrule is not None:
            for row in range(rows):
                said = overrule(table, legal, scores, named, flat, row)

                if said is not None and said != picks[row]:
                    picks[row] = said
                    forced += 1

        obs, ids, mask, _, dones, _ = vec.step(
            np.asarray(picks, dtype=np.int64))

        if not any(dones):
            continue

        for row, summary in enumerate(vec_summaries(vec, last=True)):
            if dones[row]:
                floors.append(float(summary["floors"]))
                bosses.append(float(summary["bosses_won"]))
                wins.append(float(summary["won_the_spire"]))

    return (np.array(floors[:climbs]), np.array(bosses[:climbs]),
            np.array(wins[:climbs]), forced)


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Ask what one kind of choice is worth.")
    parser.add_argument("folder", nargs="?", default="runs/ironclad")
    parser.add_argument("--ask", default="fire",
                        choices=sorted(questions(0, 0)),
                        help="which choice to take away")
    parser.add_argument("--climbs", type=int, default=500)
    parser.add_argument("--envs", type=int, default=64)
    parser.add_argument("--seed", type=int, default=5)
    args = parser.parse_args(argv)

    device = "cuda" if torch.cuda.is_available() else "cpu"
    net, kept, plan = load(args.folder, device)
    # Health is a share of the ceiling, four along the block that says where
    # the climb has got to.
    title, columns = questions(plan.id_layout["deck"],
                               plan.layout["run"] + 4)[args.ask]

    print("%s, over %d climbs each, the same seeds" % (title, args.climbs))
    print()
    print("%-22s %8s %8s %8s %8s %8s"
          % ("overruled", "floors", "+-", "bosses", "won", "forced"))

    for label, overrule in columns:
        floors, bosses, wins, forced = played(net, kept, plan, device,
                                              overrule, args.climbs,
                                              args.envs, args.seed)

        print("%-22s %8.2f %8.2f %8.3f %7.1f%% %8d"
              % (label, floors.mean(),
                 floors.std() / max(1.0, len(floors) ** 0.5),
                 bosses.mean(), 100.0 * wins.mean(), forced))

    return 0


def _check():
    """Asks whether an overruling actually overrules.

    Without this the columns could quietly be the same climb three times over,
    and three identical numbers read as "the choice does not matter" rather
    than as "nothing was taken away".
    """
    table = Table()
    vec = VecSpireEnv(8)
    plan = SpireEnv()

    obs, ids, mask = vec.reset("ironclad", 3)
    rng = np.random.RandomState(5)
    asked = wantsKind("claim_reward")
    tookOne = 0
    skipped = 0

    # And whether asking which card ever has more than one to choose between.
    # If it never did, the column would be the policy playing itself and the
    # two identical numbers would read as "which card does not matter".
    among = wantsAnyOffer(11)
    offered = 0
    moved = 0

    # And whether reaching for the belt is ever a thing that can be done. A
    # column that never fires is three identical numbers reading as "when it
    # drinks does not matter".
    thirsty = drinksUnder(0.5, plan.layout["run"] + 4)
    drank = 0

    for _ in range(1200):
        legal = np.asarray(mask, dtype=np.uint8)
        scores = rng.rand(legal.shape[0], legal.shape[1])
        scores[legal == 0] = -1e9
        picks = scores.argmax(axis=1)
        flat = np.asarray(obs, dtype=np.float32).reshape(legal.shape[0], -1)

        for row in range(legal.shape[0]):
            said = asked(table, legal, scores, np.asarray(ids), flat, row)

            if said is not None:
                picks[row] = said

        for row in range(legal.shape[0]):
            said = among(table, legal, scores, np.asarray(ids), flat, row)

            if said is not None:
                offered += 1
                moved += 1 if said != int(np.argmax(
                    np.where(legal[row], scores[row], -1e9))) else 0

            told = thirsty(table, legal, scores, np.asarray(ids), flat, row)

            if told is not None:
                drank += 1
                assert table.kind(told) == "use_potion", (
                    "being told to drink named a %s" % table.kind(told))

        for pick in picks:
            tookOne += 1 if table.kind(pick) == "claim_reward" else 0
            skipped += 1 if table.kind(pick) == "skip_reward" else 0

        obs, ids, mask, _, _, _ = vec.step(np.asarray(picks, dtype=np.int64))

    assert tookOne > 0, "no pile was ever taken, so nothing was overruled"
    assert skipped == 0, "a pile was turned down while being overruled to take"
    assert offered > 0, ("no pile ever held two cards, so which card was "
                        "never asked")
    assert moved > 0, "the die never named a card other than the one wanted"

    print("taking every pile took %d and turned down %d" % (tookOne, skipped))
    print("which card was asked %d times and moved the answer %d"
          % (offered, moved))
    print("the belt was reached for %d times under half health" % drank)

    assert drank > 0, "nothing was ever thirsty, so when it drinks is unasked"
    print("a deck slot is named at %d" % plan.id_layout["deck"])


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--check":
        _check()
    else:
        sys.exit(main())
