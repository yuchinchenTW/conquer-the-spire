"""What came of the choices, over as many climbs as were played.

The engine keeps a row for every card, relic, potion and room that was ever
chosen: how often it was chosen, how many climbs it was in, how many of those
were won, how many died, and how far they got. This reads that table out with
the names filled in.

    from cts_vec import VecSpireEnv
    from cts_stats import table_of, report

    vec = VecSpireEnv(64)
    ... play a few thousand ticks ...

    print(report(vec))                       # the top picks of each kind
    rows = table_of(vec)["card_taken"]        # or the table itself
    for row in rows[:10]:
        print(row["name"], row["picks"], row["win_rate"], row["avg_floors"])

A word of warning about reading these: a card that only turns up in decks that
were already winning will read well without having done anything. Over enough
climbs the rows are still the quickest way to see what a learner has settled
on, but they are counts, not causes.
"""

import ctypes

from cts_env import _api
from cts_log import (SOURCES, card_name, card_type, event_name,
                     event_option_name, map_node_name,
                     monster_name, potion_name, relic_name)

# What a row is about, in the order of StatKind.
KINDS = [
    "invalid",
    "card_taken",
    "card_bought",
    "card_removed",
    "card_upgraded",
    "card_transformed",
    "relic_taken",
    "relic_bought",
    "potion_taken",
    "potion_bought",
    "potion_drunk",
    "room_entered",
    "room_answered",
    "node_walked",
    "curse_option",
    "fight_fought",
    "elite_fought",
    "boss_fought",
]

# The kinds whose ids are cards, so that a curse can be told from a strike.
CARD_KINDS = [
    "card_taken",
    "card_bought",
    "card_removed",
    "card_upgraded",
    "card_transformed",
]

# The kinds that put a card into the deck, as against tearing one out of it
# or sharpening one already there.
GAIN_KINDS = [
    "card_taken",
    "card_bought",
    "card_transformed",
]

# Which table a kind's ids are looked up in.
_NAMERS = {
    "card_taken": card_name,
    "card_bought": card_name,
    "card_removed": card_name,
    "card_upgraded": card_name,
    "card_transformed": card_name,
    "relic_taken": relic_name,
    "relic_bought": relic_name,
    "potion_taken": potion_name,
    "potion_bought": potion_name,
    "potion_drunk": potion_name,
    "room_entered": event_name,
    "node_walked": map_node_name,
    "curse_option": event_name,
    "fight_fought": monster_name,
    "elite_fought": monster_name,
    "boss_fought": monster_name,
}

_DECLARED = False


def _declare(lib):
    global _DECLARED

    if _DECLARED:
        return

    lib.cts_stat_slots.restype = ctypes.c_size_t
    lib.cts_stat_rows.argtypes = [ctypes.c_void_p]
    lib.cts_stat_rows.restype = ctypes.c_size_t
    lib.cts_stats.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_int)]
    lib.cts_stat_totals.argtypes = [ctypes.c_void_p,
                                    ctypes.POINTER(ctypes.c_int)]
    lib.cts_stats_clear.argtypes = [ctypes.c_void_p]

    lib.cts_vec_stat_rows.argtypes = [ctypes.c_void_p]
    lib.cts_vec_stat_rows.restype = ctypes.c_size_t
    lib.cts_vec_stats.argtypes = [ctypes.c_void_p,
                                  ctypes.POINTER(ctypes.c_int)]
    lib.cts_vec_stat_totals.argtypes = [ctypes.c_void_p,
                                        ctypes.POINTER(ctypes.c_int)]
    lib.cts_vec_stats_clear.argtypes = [ctypes.c_void_p]
    lib.cts_vec_roll_random.argtypes = [
        ctypes.c_void_p,
        ctypes.c_int,
        ctypes.c_uint,
        ctypes.c_size_t,
        ctypes.POINTER(ctypes.c_float),
        ctypes.POINTER(ctypes.c_int),
        ctypes.POINTER(ctypes.c_int),
    ]

    _DECLARED = True


def _handle(source):
    """Returns the handle and whether it is a row of climbs."""
    if hasattr(source, "_vec"):
        return source._vec, True

    return source._env, False


def _name_of(kind, id_):
    if kind == "room_answered":
        room = id_ // 100
        option = id_ % 100
        label = event_option_name(room, 0, option)

        return "%s: %s" % (event_name(room),
                           label or ("option %d" % option))

    namer = _NAMERS.get(kind)

    return namer(id_) if namer else str(id_)


def totals_of(source):
    """Returns how many climbs were counted, and how they went."""
    api = _api()

    _declare(api.lib)

    handle, row = _handle(source)
    raw = (ctypes.c_int * 4)()

    if row:
        api.lib.cts_vec_stat_totals(handle, raw)
    else:
        api.lib.cts_stat_totals(handle, raw)

    runs, wins, deaths, floors = [int(value) for value in raw]

    return {
        "runs": runs,
        "wins": wins,
        "deaths": deaths,
        "floors": floors,
        "win_rate": wins / runs if runs else 0.0,
        "avg_floors": floors / runs if runs else 0.0,
    }


def table_of(source, least=1):
    """Returns the table as ``{kind: [row, ...]}``, best picked first.

    ``least`` drops the rows offered in fewer than that many climbs, which is
    what keeps the one-off picks out of the way.

    Each row carries ``picks`` (how often it was taken), ``passes`` (how often
    it was offered and left), ``pick_rate`` (the first over the two together),
    ``runs`` (climbs it was taken in) and ``win_rate`` and ``avg_floors`` over
    those climbs.
    """
    api = _api()

    _declare(api.lib)

    handle, row = _handle(source)
    slots = int(api.lib.cts_stat_slots())
    count = int(api.lib.cts_vec_stat_rows(handle) if row
                else api.lib.cts_stat_rows(handle))

    if count == 0:
        return {}

    raw = (ctypes.c_int * (count * slots))()

    if row:
        api.lib.cts_vec_stats(handle, raw)
    else:
        api.lib.cts_stats(handle, raw)

    out = {}

    for i in range(count):
        (kind, id_, picks, passes, runs, wins, deaths,
         floors) = raw[i * slots:(i + 1) * slots]

        if max(runs, picks + passes) < least:
            continue

        name = KINDS[kind] if kind < len(KINDS) else "invalid"

        out.setdefault(name, []).append(
            {
                "id": int(id_),
                "name": _name_of(name, int(id_)),
                "type": card_type(int(id_)) if name in CARD_KINDS else "",
                "picks": int(picks),
                "passes": int(passes),
                "offered": int(picks + passes),
                "runs": int(runs),
                "wins": int(wins),
                "deaths": int(deaths),
                "pick_rate": picks / (picks + passes)
                if picks + passes else 0.0,
                "win_rate": wins / runs if runs else 0.0,
                "avg_floors": floors / runs if runs else 0.0,
            }
        )

    for rows in out.values():
        rows.sort(key=lambda one: (-one["offered"], -one["picks"],
                                   one["name"]))

    return out


def curses_of(source, least=1):
    """Returns the curses that were taken, worst first.

    Curses are never offered as one of a reward's cards, so these all come
    from rooms, from chests opened with the Cursed Key, and from the two
    relics that hand one over. A row here means the learner walked into it.
    """
    table = table_of(source, least)
    rows = [row for kind in GAIN_KINDS for row in table.get(kind, [])
            if row["type"] == "curse"]

    rows.sort(key=lambda one: (-one["picks"], one["name"]))

    return rows


def clear(source):
    """Forgets everything counted so far."""
    api = _api()

    _declare(api.lib)

    handle, row = _handle(source)

    if row:
        api.lib.cts_vec_stats_clear(handle)
    else:
        api.lib.cts_stats_clear(handle)


def roll_random_into(vec, character=1, seed=1, runs=100):
    """Plays whole climbs with a die on the engine side, counted into ``vec``.

    Returns ``(returns, floors, steps)`` as lists.
    """
    api = _api()

    _declare(api.lib)

    if isinstance(character, str):
        from cts_env import CHARACTERS

        character = CHARACTERS[character.lower()]

    out = (ctypes.c_float * runs)()
    floors = (ctypes.c_int * runs)()
    steps = (ctypes.c_int * runs)()

    api.lib.cts_vec_roll_random(vec._vec, int(character),
                                ctypes.c_uint(int(seed)),
                                ctypes.c_size_t(runs), out, floors, steps)

    return list(out), list(floors), list(steps)


def report(source, top=8, least=2, kinds=None):
    """Returns the table as a printable block, a few rows of each kind."""
    return render(totals_of(source), table_of(source, least),
                  curses_of(source, 1), top, kinds)


def render(counts, table, hurt=None, top=8, kinds=None):
    """The same block, out of tables already read.

    Worth having apart: the trainer forgets the table every time it reports,
    so the one it printed is the only copy left of it.
    """
    lines = [
        "%d climbs counted: %d won (%.1f%%), %d died, %.1f floors on average"
        % (counts["runs"], counts["wins"], 100.0 * counts["win_rate"],
           counts["deaths"], counts["avg_floors"])
    ]

    if hurt and not kinds:
        lines.append("")
        lines.append("curses picked up  (%d different, %d in all)"
                     % (len(hurt), sum(row["picks"] for row in hurt)))
        lines.append("  %-24s %6s %6s %7s" % ("", "taken", "win %", "floors"))

        for row in hurt[:top]:
            lines.append("  %-24s %6d %5.1f%% %7.1f" %
                         (row["name"][:24], row["picks"],
                          100.0 * row["win_rate"], row["avg_floors"]))

    for kind in (kinds or KINDS):
        rows = table.get(kind)

        if not rows:
            continue

        lines.append("")
        lines.append("%s  (%d different)" % (kind, len(rows)))
        lines.append("  %-24s %6s %6s %7s %6s %7s %7s" %
                     ("", "picks", "seen", "pick %", "runs", "win %",
                      "floors"))

        for row in rows[:top]:
            lines.append("  %-24s %6d %6d %6.1f%% %6d %6.1f%% %7.1f" %
                         (row["name"][:24], row["picks"], row["offered"],
                          100.0 * row["pick_rate"], row["runs"],
                          100.0 * row["win_rate"], row["avg_floors"]))

    return "\n".join(lines)
