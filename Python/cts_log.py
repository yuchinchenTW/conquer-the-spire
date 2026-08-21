"""Reading back what a climb did.

The engine keeps a line for everything worth knowing afterwards - every card
taken, torn up or sharpened, every relic and potion, every fight won, every
floor walked - along with the counts that go with it. This turns those lines
into names, and the counts into a dict.

    from cts_env import SpireEnv
    from cts_log import summary_of, lines_of, describe

    env = SpireEnv()
    env.reset("ironclad", 1)
    ... play ...

    print(summary_of(env))                 # the counts
    for line in lines_of(env):
        print(describe(line))              # "floor 3: took Cleave (reward)"

The names come out of the engine itself, so nothing here has to be kept in
step with the card lists by hand.
"""

import ctypes

from cts_env import _api

# The kinds of line, in the order of LogEntry.
# What a line is about, in the order of LogEntry. Filled in from the engine
# the first time it is needed, so that adding a kind of line on that side
# cannot leave this one quietly mislabelling everything after it. The list
# below is only what to fall back on if the engine is too old to be asked.
ENTRIES = [
    "invalid",
    "card_taken",
    "card_passed",
    "relic_passed",
    "potion_passed",
    "card_removed",
    "card_upgraded",
    "card_transformed",
    "relic_taken",
    "relic_lost",
    "potion_taken",
    "potion_drunk",
    "potion_thrown",
    "gold_earned",
    "gold_spent",
    "fight_won",
    "floor_walked",
    "act_started",
    "room_entered",
    "room_answered",
    "rested",
    "died",
    "spire_done",
    "room_passed",
]

# Where a line came from, in the order of LogSource.
SOURCES = [
    "",
    "reward",
    "shop",
    "room",
    "relic",
    "rest",
    "chest",
    "fight",
    "boss",
]

# What the counts are called, in the order the engine writes them.
SUMMARY_FIELDS = [
    "floors",
    "act",
    "deepest_act",
    "fights_won",
    "elites_won",
    "bosses_won",
    "cards_taken",
    "cards_passed",
    "cards_bought",
    "cards_removed",
    "cards_upgraded",
    "cards_transformed",
    "relics_taken",
    "relics_bought",
    "potions_taken",
    "potions_bought",
    "potions_drunk",
    "gold_earned",
    "gold_spent",
    "rooms_entered",
    "rests",
    "died",
    "won_the_spire",
    "curses_chosen",
    "curses_refused",
]

# The rooms of the map, in the order of MapNodeType.
ROOMS = ["empty", "monster", "elite", "room", "rest", "shop", "chest", "boss"]

# The kinds of fight, in the order of MonsterType.
FIGHTS = ["", "normal", "elite", "boss"]

# What a card is, in the order of CardType and CardRarity.
CARD_TYPES = ["", "attack", "skill", "power", "status", "curse"]
CARD_RARITIES = ["", "basic", "common", "uncommon", "rare", "special"]

_DECLARED = False
_NAMES = {}


def _declare(lib):
    global _DECLARED

    if _DECLARED:
        return

    lib.cts_summary_slots.restype = ctypes.c_size_t
    lib.cts_log_lines.argtypes = [ctypes.c_void_p]
    lib.cts_log_lines.restype = ctypes.c_size_t
    lib.cts_summary.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_int)]
    lib.cts_log.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_int)]

    for name in ["cts_card_name", "cts_relic_name", "cts_potion_name",
                 "cts_monster_name", "cts_event_name"]:
        fn = getattr(lib, name)
        fn.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_size_t]
        fn.restype = ctypes.c_size_t

    lib.cts_event_option_name.argtypes = [ctypes.c_int, ctypes.c_int,
                                          ctypes.c_int, ctypes.c_char_p,
                                          ctypes.c_size_t]
    lib.cts_event_option_name.restype = ctypes.c_size_t

    lib.cts_map_node_name.argtypes = [ctypes.c_int, ctypes.c_char_p,
                                      ctypes.c_size_t]
    lib.cts_map_node_name.restype = ctypes.c_size_t

    lib.cts_log_entry_count.restype = ctypes.c_size_t
    lib.cts_log_entry_name.argtypes = [ctypes.c_int, ctypes.c_char_p,
                                       ctypes.c_size_t]
    lib.cts_log_entry_name.restype = ctypes.c_size_t

    lib.cts_card_type.argtypes = [ctypes.c_int]
    lib.cts_card_type.restype = ctypes.c_int
    lib.cts_card_rarity.argtypes = [ctypes.c_int]
    lib.cts_card_rarity.restype = ctypes.c_int

    lib.cts_vec_summary.argtypes = [ctypes.c_void_p,
                                    ctypes.POINTER(ctypes.c_int)]
    lib.cts_vec_last_summary.argtypes = [ctypes.c_void_p,
                                         ctypes.POINTER(ctypes.c_int)]

    _DECLARED = True


def _name(kind, id_):
    """Asks the engine what something is called, and remembers the answer."""
    if id_ <= 0:
        return ""

    key = (kind, id_)

    if key in _NAMES:
        return _NAMES[key]

    api = _api()

    _declare(api.lib)

    fn = getattr(api.lib, "cts_" + kind + "_name")
    buffer = ctypes.create_string_buffer(128)

    fn(int(id_), buffer, ctypes.c_size_t(len(buffer)))
    _NAMES[key] = buffer.value.decode("utf-8", "replace")

    return _NAMES[key]


def card_name(id_):
    return _name("card", id_)


def relic_name(id_):
    return _name("relic", id_)


def potion_name(id_):
    return _name("potion", id_)


def monster_name(id_):
    return _name("monster", id_)


def event_name(id_):
    return _name("event", id_)


def event_option_name(id_, stage, option):
    """What an option of a room is called."""
    if id_ <= 0:
        return ""

    api = _api()

    _declare(api.lib)

    buffer = ctypes.create_string_buffer(64)

    api.lib.cts_event_option_name(int(id_), int(stage), int(option), buffer,
                                  ctypes.c_size_t(len(buffer)))

    return buffer.value.decode("utf-8", "replace")


_CARD_KINDS = {}


def card_type(id_):
    """What kind of card an id is: attack, skill, power, status or curse."""
    if id_ <= 0:
        return ""

    if id_ not in _CARD_KINDS:
        api = _api()

        _declare(api.lib)

        got = int(api.lib.cts_card_type(int(id_)))
        _CARD_KINDS[id_] = (CARD_TYPES[got] if 0 <= got < len(CARD_TYPES)
                            else "")

    return _CARD_KINDS[id_]


def card_rarity(id_):
    """How rare a card is: basic, common, uncommon, rare or special."""
    if id_ <= 0:
        return ""

    api = _api()

    _declare(api.lib)

    got = int(api.lib.cts_card_rarity(int(id_)))

    return CARD_RARITIES[got] if 0 <= got < len(CARD_RARITIES) else ""


_ASKED = []


def _entry_name(entry):
    """What a kind of line is called, asked of the engine once.

    The engine is the one that knows: a list kept here would go on
    mislabelling every line after any kind that was added on that side.
    """
    global _ASKED

    if not _ASKED:
        api = _api()

        _declare(api.lib)

        count = int(api.lib.cts_log_entry_count())
        buffer = ctypes.create_string_buffer(64)

        for index in range(count):
            api.lib.cts_log_entry_name(index, buffer,
                                       ctypes.c_size_t(len(buffer)))
            _ASKED.append(buffer.value.decode("utf-8", "replace"))

    entry = int(entry)

    return _ASKED[entry] if 0 <= entry < len(_ASKED) else "invalid"


def map_node_name(type_):
    """What kind of place on the map a number means: fight, elite, room,
    campfire, shop, chest or boss."""
    api = _api()

    _declare(api.lib)

    buffer = ctypes.create_string_buffer(32)

    api.lib.cts_map_node_name(int(type_), buffer,
                              ctypes.c_size_t(len(buffer)))

    return buffer.value.decode("utf-8", "replace")


def summary_slots():
    api = _api()

    _declare(api.lib)

    return int(api.lib.cts_summary_slots())


def summary_of(env):
    """Returns the counts of a climb as a dict."""
    api = _api()

    _declare(api.lib)

    raw = (ctypes.c_int * summary_slots())()

    api.lib.cts_summary(env._env, raw)

    return dict(zip(SUMMARY_FIELDS, [int(value) for value in raw]))


def lines_of(env):
    """Returns the log as a list of dicts, oldest first."""
    api = _api()

    _declare(api.lib)

    count = int(api.lib.cts_log_lines(env._env))

    if count == 0:
        return []

    raw = (ctypes.c_int * (count * 7))()

    api.lib.cts_log(env._env, raw)

    out = []

    for i in range(count):
        entry, source, id_, extra, act, floor, stage = raw[i * 7:i * 7 + 7]
        out.append(
            {
                "entry": _entry_name(entry),
                "source": SOURCES[source] if source < len(SOURCES) else "",
                "id": int(id_),
                "extra": int(extra),
                "act": int(act),
                "floor": int(floor),
                "stage": int(stage),
            }
        )

    return out


def describe(line):
    """Turns one line of the log into a sentence."""
    where = "act %d floor %-2d" % (line["act"], line["floor"])
    entry = line["entry"]
    id_ = line["id"]
    extra = line["extra"]
    source = (" (%s)" % line["source"]) if line["source"] else ""

    if entry == "card_taken":
        what = "took %s" % card_name(id_)
    elif entry == "card_removed":
        what = "tore up %s" % card_name(id_)
    elif entry == "card_upgraded":
        what = "sharpened %s" % card_name(id_)
    elif entry == "card_transformed":
        what = "turned %s into %s" % (card_name(id_), card_name(extra))
    elif entry == "relic_taken":
        what = "took %s" % relic_name(id_)
    elif entry == "relic_lost":
        what = "gave up %s" % relic_name(id_)
    elif entry == "potion_taken":
        what = "took %s" % potion_name(id_)
    elif entry == "potion_drunk":
        what = "drank %s" % potion_name(id_)
    elif entry == "potion_thrown":
        what = "threw away %s" % potion_name(id_)
    elif entry == "gold_earned":
        what = "gained %d gold" % extra
    elif entry == "gold_spent":
        what = "spent %d gold" % extra
    elif entry == "fight_won":
        kind = FIGHTS[id_] if 0 <= id_ < len(FIGHTS) else ""
        what = "won a %s fight" % kind if kind else "won a fight"
    elif entry == "floor_walked":
        room = ROOMS[id_] if 0 <= id_ < len(ROOMS) else "?"
        what = "walked into a %s" % room
    elif entry == "act_started":
        what = "started act %d" % id_
    elif entry == "room_entered":
        what = "entered %s" % event_name(id_)
    elif entry == "room_answered":
        label = event_option_name(id_, line.get("stage", 0), extra)
        what = "chose %s at %s" % (label or ("option %d" % extra),
                                   event_name(id_))
    elif entry == "rested":
        what = "rested for %d" % extra
    elif entry == "died":
        what = "died"
    elif entry == "spire_done":
        what = "came out the top of the spire"
    else:
        what = entry

    return "%s: %s%s" % (where, what, source)


def report(env):
    """Returns the whole climb as one printable block."""
    counts = summary_of(env)
    out = [describe(line) for line in lines_of(env)]

    out.append("")
    out.append("floors %d, act %d (deepest %d), fights %d (%d elite, %d boss)"
               % (counts["floors"], counts["act"], counts["deepest_act"],
                  counts["fights_won"], counts["elites_won"],
                  counts["bosses_won"]))
    out.append("cards: %d taken (%d bought), %d torn up, %d sharpened, "
               "%d turned" % (counts["cards_taken"], counts["cards_bought"],
                              counts["cards_removed"],
                              counts["cards_upgraded"],
                              counts["cards_transformed"]))
    out.append("relics: %d (%d bought)   potions: %d (%d bought, %d drunk)"
               % (counts["relics_taken"], counts["relics_bought"],
                  counts["potions_taken"], counts["potions_bought"],
                  counts["potions_drunk"]))
    out.append("gold: %d earned, %d spent   rests: %d   died: %d   won: %d"
               % (counts["gold_earned"], counts["gold_spent"],
                  counts["rests"], counts["died"], counts["won_the_spire"]))

    return "\n".join(out)


def vec_summaries(vec, last=False):
    """Returns one dict a climb of a row.

    With ``last=True`` these are the climbs that ended on the last tick; a row
    whose climb has not ended reads as zeroes.
    """
    api = _api()

    _declare(api.lib)

    slots = summary_slots()
    raw = (ctypes.c_int * (vec.count * slots))()

    if last:
        api.lib.cts_vec_last_summary(vec._vec, raw)
    else:
        api.lib.cts_vec_summary(vec._vec, raw)

    out = []

    for i in range(vec.count):
        values = [int(value) for value in raw[i * slots:(i + 1) * slots]]
        out.append(dict(zip(SUMMARY_FIELDS, values)))

    return out
