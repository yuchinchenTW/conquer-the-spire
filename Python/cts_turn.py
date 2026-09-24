"""Playing a whole turn on a copy before playing it for real.

The looking that cts_play does walks one move on a copy and keeps the one
the value head thinks more of. That pays out of a fight and pays little in
one, and the measurement says why: a turn is a sequence. Block only counts
once the monsters have swung, and they swing inside the move that ends the
turn, so the first card of a turn says almost nothing about what the turn
comes to.

This searches the turn instead. From the state at hand it grows sequences
a move at a time, keeps the best few by what the value head says, and
stops when the turn ends or the budget runs out. What a sequence is worth
is what it paid along the way, discounted, plus what the head says about
where it left the climb - and because the end of the turn is a move like
any other, a sequence that ends the turn is scored after the monsters have
answered.

    from cts_turn import turnSearch
    search = turnSearch(net, device, width=4, budget=120)
    move = search(vec, row, obs, ids, mask, scores)

Three things it is built to avoid, all of them learnt the hard way:

  * Enumerating the turn. Five cards with targets is thousands of orders,
    and most of them differ by nothing. The budget is nodes walked, not
    depth, so a hand with one sensible line spends nothing.
  * Trusting the head too far. Looking at 4 candidates one move ahead beat
    looking at 2 nowhere and lost floors in a fight (Notes/look-trained-
    head-2026-09-11.txt) - the widest search finds the head's largest
    error. The width here is small on purpose and the score leans on what
    was actually paid wherever it can.
  * Searching where there is nothing to find. Out of a fight the existing
    one-move looking already wins; this only runs inside one.
"""

import numpy as np

try:
    import torch
except ImportError:  # pragma: no cover
    print("this needs torch: pip install torch")
    raise

from cts_env import PHASES, SpireEnv, action_table

#! How many sequences are carried forward at each depth. Four, because the
#! measured failure of wider looking is that it finds the value head's
#! largest mistake rather than the board's best line.
WIDTH = 4

#! How many walked sequences a turn may cost. A turn is three to six moves,
#! so this allows a handful of lines to their end and stops a pathological
#! hand - a dozen zero-cost cards - from taking the afternoon.
BUDGET = 120

#! How many of the policy's moves are considered at each depth. The policy
#! is a strong prior: its fifth choice in a fight is rarely the answer, and
#! every one considered is a sequence walked.
BRANCH = 4

#! What a step of the future is worth against a step of now, as in training.
GAMMA = 0.999


def turnSearch(net, device, width=WIDTH, budget=BUDGET, branch=BRANCH,
               gamma=GAMMA):
    """A function that picks the next move by searching the turn.

    \\p width sequences are carried at each depth, \\p branch moves are
    considered from each, and no more than \\p budget sequences are walked
    for one decision.
    """
    plan = SpireEnv()
    kinds, _, _ = action_table()
    kinds = np.asarray(kinds)
    endsTurn = np.flatnonzero(kinds == "end_turn")
    endTurn = int(endsTurn[0]) if endsTurn.size else -1
    phaseAt = plan.layout["phase"]
    fighting = (PHASES.index("battle"), PHASES.index("boss"),
                PHASES.index("choosing"))

    def worth(states, named):
        """What the value head says about a batch of states."""
        with torch.no_grad():
            _, value, _ = net(
                torch.as_tensor(np.asarray(states, dtype=np.float32),
                                device=device),
                torch.as_tensor(np.asarray(named), device=device).long())

        return value.cpu().numpy()

    def order(states, named, masks):
        """The policy's best few legal moves for each state."""
        with torch.no_grad():
            scores, _, _ = net(
                torch.as_tensor(np.asarray(states, dtype=np.float32),
                                device=device),
                torch.as_tensor(np.asarray(named), device=device).long())

        allowed = torch.as_tensor(np.asarray(masks, dtype=np.uint8)
                                  .astype(bool), device=device)
        scores = scores.masked_fill(~allowed, -1e9)
        take = min(branch, scores.shape[1])

        return torch.topk(scores, take, dim=1).indices.cpu().numpy()

    def search(vec, row, obs, ids, mask, scores=None):
        """The move to make now, having played the turn out on a copy.

        \\p obs, \\p ids and \\p mask are this row's own; \\p scores is the
        policy's reading of them when the caller already has it.
        """
        flat = np.asarray(obs, dtype=np.float32)
        where = int(flat[phaseAt:phaseAt + len(PHASES)].argmax())

        if where not in fighting:
            return None

        legal = np.asarray(mask, dtype=np.uint8)

        if legal.sum() <= 1:
            # Nothing to choose between.
            return int(np.argmax(legal)) if legal.any() else None

        # A live sequence: the moves in it, what they paid, and the state
        # it left the copy in.
        live = [{"moves": [], "paid": [], "obs": flat,
                 "ids": np.asarray(ids), "mask": legal, "done": False}]
        finished = []
        spent = 0

        while live and spent < budget:
            # What the policy would consider from each live sequence.
            fronts = order([one["obs"] for one in live],
                           [one["ids"] for one in live],
                           [one["mask"] for one in live])
            grown = []

            for one, front in zip(live, fronts):
                for move in front:
                    if spent >= budget:
                        break

                    if one["mask"][int(move)] == 0:
                        continue

                    moves = one["moves"] + [int(move)]
                    got = vec.walk(row, moves)
                    spent += 1
                    obsOne, idsOne, maskOne, paid, over, taken = got

                    if taken < len(moves):
                        # The sequence stopped being legal on the way; the
                        # copy is not where this line says it is.
                        continue

                    step = {"moves": moves, "paid": list(paid),
                            "obs": np.asarray(obsOne, dtype=np.float32),
                            "ids": np.asarray(idsOne),
                            "mask": np.asarray(maskOne, dtype=np.uint8),
                            "done": bool(over)}

                    # The turn is over once the ending move has been taken
                    # or the climb has: either way the monsters have had
                    # their answer and there is nothing more to add.
                    stood = int(step["obs"][phaseAt:phaseAt + len(PHASES)]
                                .argmax())

                    if (int(move) == endTurn or step["done"]
                            or stood not in fighting):
                        finished.append(step)
                    else:
                        grown.append(step)

            if not grown:
                break

            # Keep the best few by what the head says about where they
            # stand, so the search spends its budget on lines that are
            # going somewhere.
            said = worth([one["obs"] for one in grown],
                         [one["ids"] for one in grown])
            best = np.argsort(-said)[:width]
            live = [grown[int(at)] for at in best]

        candidates = finished + live

        if not candidates:
            return None

        # What a sequence is worth: what it paid, discounted by when, plus
        # what the head says about where it left the climb, discounted by
        # how long the sequence was. A climb that ended pays nothing after
        # its last step.
        ends = [one for one in candidates if not one["done"]]
        said = (worth([one["obs"] for one in ends],
                      [one["ids"] for one in ends])
                if ends else np.zeros(0, dtype=np.float32))
        after = {id(one): float(value) for one, value in zip(ends, said)}
        best = None
        bestWorth = None

        for one in candidates:
            total = 0.0

            for at, pay in enumerate(one["paid"]):
                total += float(pay) * (gamma ** at)

            total += (after.get(id(one), 0.0)
                      * (gamma ** len(one["moves"])))

            if bestWorth is None or total > bestWorth:
                bestWorth = total
                best = one

        return int(best["moves"][0]) if best["moves"] else None

    return search
