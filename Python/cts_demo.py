"""A baseline bot, and the record of what it did.

Nothing here learns anything: it is a handful of rules to sit under a learner
as the line to beat, and a way to see that the record of a climb reads the way
it should.

    python cts_demo.py            # one climb, printed line by line
    python cts_demo.py 200        # two hundred climbs, counted up
"""

import sys

from cts_env import SpireEnv
from cts_log import report, summary_of

# The kinds of move, in the order of ActionKind.
KINDS = [
    "invalid",
    "travel",
    "play_card",
    "end_turn",
    "use_potion",
    "discard_potion",
    "claim_reward",
    "skip_reward",
    "leave_rewards",
    "choose_option",
    "buy_card",
    "buy_relic",
    "buy_potion",
    "buy_removal",
    "leave_shop",
    "rest",
    "smith",
    "toke",
    "dig",
    "lift",
    "leave_rest",
    "fight_boss",
    "next_act",
]

# What the bot reaches for first, wherever it is standing. Anything not named
# here it will only do when there is nothing better.
WANTED = [
    "play_card",
    "claim_reward",
    "buy_relic",
    "buy_card",
    "buy_potion",
    "smith",
    "rest",
    "fight_boss",
    "next_act",
    "travel",
    "choose_option",
    "use_potion",
    "end_turn",
    "leave_rewards",
    "leave_shop",
    "leave_rest",
]


def _kinds_of(env):
    """Returns the legal moves as ``(index, kind)`` pairs."""
    out = []

    for index in env.legal_actions():
        # The engine numbers the kinds; the wrapper only hands over indices,
        # so ask it what each one is by walking the table.
        out.append((index, _kind_of(index, env)))

    return out


_KIND_CACHE = {}


def _kind_of(index, env):
    """Which kind of move a slot of the fixed head is."""
    if not _KIND_CACHE:
        # The head is laid out in blocks; work out the block of every slot
        # once by asking the engine for one climb's worth of masks.
        _fill_kind_cache(env)

    return _KIND_CACHE.get(index, "invalid")


def _fill_kind_cache(env):
    """Works out which block of the head each slot belongs to.

    The layout is fixed, so this is done once: the blocks are in the order the
    engine builds them, and their sizes come from the numbers it reports.
    """
    counts = [
        ("end_turn", 1),
        ("play_card", 10 * 8),
        ("use_potion", 5 * 8),
        ("discard_potion", 5),
        ("travel", 7),
        ("claim_reward", 6 * 20),
        ("skip_reward", 6),
        ("leave_rewards", 1),
        ("choose_option", 6 * 41),
        ("buy_card", 7),
        ("buy_relic", 3),
        ("buy_potion", 3),
        ("buy_removal", 40),
        ("leave_shop", 1),
        ("rest", 1),
        ("smith", 40),
        ("toke", 40),
        ("dig", 1),
        ("lift", 1),
        ("leave_rest", 1),
        ("fight_boss", 1),
        ("next_act", 1),
    ]

    at = 0

    for kind, size in counts:
        for slot in range(at, at + size):
            _KIND_CACHE[slot] = kind

        at += size

    if at != env.action_count:
        raise RuntimeError(
            "the head is %d slots but this adds up to %d; the blocks have "
            "moved" % (env.action_count, at)
        )


def act(env):
    """Picks a move: the first thing on the wanted list that is legal."""
    legal = _kinds_of(env)

    if not legal:
        return None

    by_kind = {}

    for index, kind in legal:
        by_kind.setdefault(kind, []).append(index)

    for kind in WANTED:
        if kind in by_kind:
            return by_kind[kind][0]

    return legal[0][0]


def play(env, character="ironclad", seed=1, limit=20000):
    """Plays one climb and returns its counts."""
    env.reset(character, seed)

    steps = 0
    total = 0.0

    while not env.done and steps < limit:
        move = act(env)

        if move is None:
            break

        _, reward, _, info = env.step(move)

        if not info["taken"]:
            # The rules said it could be done and it could not: that is worth
            # knowing about rather than looping.
            raise RuntimeError("a legal move was turned down in " +
                               info["phase"])

        total += reward
        steps += 1

    counts = summary_of(env)
    counts["reward"] = total
    counts["steps"] = steps

    return counts


def main(argv):
    runs = int(argv[1]) if len(argv) > 1 else 1
    env = SpireEnv()

    if runs == 1:
        play(env, "ironclad", 4)
        print(report(env))

        return 0

    totals = {}
    deepest = 0
    wins = 0

    for seed in range(1, runs + 1):
        counts = play(env, "ironclad", seed)

        for key, value in counts.items():
            totals[key] = totals.get(key, 0) + value

        deepest = max(deepest, counts["floors"])
        wins += counts["won_the_spire"]

    print("%d climbs with the rules bot" % runs)
    print("  floors      : %.1f on average, deepest %d" %
          (totals["floors"] / runs, deepest))
    print("  acts reached: %.2f" % (totals["deepest_act"] / runs))
    print("  fights won  : %.1f (%.2f elite, %.2f boss)" %
          (totals["fights_won"] / runs, totals["elites_won"] / runs,
           totals["bosses_won"] / runs))
    print("  cards       : %.1f taken (%.2f bought), %.2f torn up, "
          "%.2f sharpened" %
          (totals["cards_taken"] / runs, totals["cards_bought"] / runs,
           totals["cards_removed"] / runs, totals["cards_upgraded"] / runs))
    print("  relics      : %.2f (%.2f bought)" %
          (totals["relics_taken"] / runs, totals["relics_bought"] / runs))
    print("  potions     : %.2f taken, %.2f drunk" %
          (totals["potions_taken"] / runs, totals["potions_drunk"] / runs))
    print("  gold        : %.0f earned, %.0f spent" %
          (totals["gold_earned"] / runs, totals["gold_spent"] / runs))
    print("  reward      : %.1f    spire done: %d" %
          (totals["reward"] / runs, wins))

    # And what came of the choices, over all of them.
    from cts_stats import report

    print()
    print(report(env, top=6, least=max(2, runs // 25),
                 kinds=["card_taken", "card_bought", "card_removed",
                        "card_upgraded", "relic_taken", "potion_taken",
                        "room_answered"]))

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
