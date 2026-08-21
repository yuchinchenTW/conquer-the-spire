"""A gym-shaped wrapper around the Conquer the Spire engine.

Nothing but ctypes and the standard library: the engine is reached through the
shared library the build puts in ``build/bin`` (or ``build/lib``), and every
call is a plain C call.

    from cts_env import SpireEnv

    env = SpireEnv()
    obs = env.reset(character="ironclad", seed=1)

    while not env.done:
        mask = env.action_mask()
        action = pick_one_of(mask)          # your policy goes here
        obs, reward, done, info = env.step(action)

Run this file on its own to play a few climbs with a die, which is also the
quickest way to see that the library is being found.
"""

import ctypes
import os
import random
import sys

# The characters, as the engine numbers them.
IRONCLAD = 1
SILENT = 2
DEFECT = 3

CHARACTERS = {"ironclad": IRONCLAD, "silent": SILENT, "defect": DEFECT}

# The phases a climb walks through. The numbers match EnvPhase.
PHASES = [
    "invalid",
    "map",
    "battle",
    "reward",
    "event",
    "shop",
    "rest",
    "boss",
    "act_done",
    "over",
]

_LIBRARY_NAMES = [
    "libconquer-the-spire.dll",
    "conquer-the-spire.dll",
    "libconquer-the-spire.so",
    "libconquer-the-spire.dylib",
]


def _find_library(explicit=None):
    """Returns the path of the shared library, or raises with what was tried."""
    if explicit:
        return explicit

    from_env = os.environ.get("CTS_LIBRARY")

    if from_env:
        return from_env

    here = os.path.dirname(os.path.abspath(__file__))
    roots = [
        os.path.join(here, os.pardir, "build", "bin"),
        os.path.join(here, os.pardir, "build", "lib"),
        os.path.join(here, os.pardir, "build"),
        here,
    ]

    tried = []

    for root in roots:
        for name in _LIBRARY_NAMES:
            path = os.path.normpath(os.path.join(root, name))

            tried.append(path)

            if os.path.exists(path):
                return path

    raise OSError(
        "the engine library was not found; build it first, or point "
        "CTS_LIBRARY at it. Tried:\n  " + "\n  ".join(tried)
    )


class _Api(object):
    """The C entry points, typed up once."""

    def __init__(self, path):
        self.path = path
        self.lib = ctypes.CDLL(path)

        c = self.lib
        c.cts_create.restype = ctypes.c_void_p
        c.cts_destroy.argtypes = [ctypes.c_void_p]
        c.cts_reset.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_uint]
        c.cts_observation_size.restype = ctypes.c_size_t
        c.cts_action_count.restype = ctypes.c_size_t
        c.cts_layout.argtypes = [ctypes.POINTER(ctypes.c_size_t)]
        c.cts_observe.argtypes = [ctypes.c_void_p,
                                  ctypes.POINTER(ctypes.c_float)]
        c.cts_id_count.restype = ctypes.c_size_t
        c.cts_id_layout.argtypes = [ctypes.POINTER(ctypes.c_size_t)]
        c.cts_observe_ids.argtypes = [ctypes.c_void_p,
                                      ctypes.POINTER(ctypes.c_int)]
        c.cts_action_mask.argtypes = [ctypes.c_void_p,
                                      ctypes.POINTER(ctypes.c_ubyte)]
        c.cts_action_table.argtypes = [ctypes.POINTER(ctypes.c_int),
                                       ctypes.POINTER(ctypes.c_int),
                                       ctypes.POINTER(ctypes.c_int)]
        c.cts_step.argtypes = [
            ctypes.c_void_p,
            ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_int),
            ctypes.POINTER(ctypes.c_int),
        ]
        c.cts_step.restype = ctypes.c_float

        for name in [
            "cts_phase",
            "cts_done",
            "cts_act",
            "cts_floor",
            "cts_total_floors",
            "cts_gold",
            "cts_health",
            "cts_max_health",
            "cts_deck_size",
        ]:
            fn = getattr(c, name)
            fn.argtypes = [ctypes.c_void_p]
            fn.restype = ctypes.c_int

        c.cts_save.argtypes = [ctypes.c_void_p, ctypes.c_char_p,
                               ctypes.c_size_t]
        c.cts_save.restype = ctypes.c_size_t
        c.cts_load.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        c.cts_load.restype = ctypes.c_int


_API = None


def _api(library=None):
    global _API

    if _API is None:
        _API = _Api(_find_library(library))

    return _API


class SpireEnv(object):
    """One climb, stepped one move at a time.

    The action space is a fixed head of ``action_count`` slots; ``action_mask``
    says which of them are legal just now. The observation is a flat list of
    floats of length ``observation_size``, and ``layout`` says where each part
    of it begins.
    """

    def __init__(self, library=None):
        self._api = _api(library)
        self._env = self._api.lib.cts_create()

        if not self._env:
            raise MemoryError("the engine would not start")

        self.observation_size = int(self._api.lib.cts_observation_size())
        self.action_count = int(self._api.lib.cts_action_count())

        names = [
            "phase",
            "run",
            "deck",
            "relics",
            "battle",
            "powers",
            "monsters",
            "hand",
            "piles",
            "total",
            "monster_stride",
            "hand_stride",
            "pile_stride",
            "rewards",
            "shop",
            "event",
            "potions",
            "moves",
            "map",
            "deck_cards",
            "reward_stride",
            "event_stride",
            "deck_stride",
        ]
        # The engine says how many it writes, so that this list cannot fall
        # out of step with it without saying so.
        self._api.lib.cts_layout_slots.restype = ctypes.c_size_t
        self._api.lib.cts_id_layout_slots.restype = ctypes.c_size_t

        wanted = int(self._api.lib.cts_layout_slots())

        if wanted != len(names):
            raise RuntimeError(
                "the engine writes %d numbers of layout and this knows %d "
                "names for them" % (wanted, len(names)))

        raw = (ctypes.c_size_t * len(names))()

        self._api.lib.cts_layout(raw)
        self.layout = dict(zip(names, [int(value) for value in raw]))

        id_names = [
            "hand",
            "potions",
            "relics",
            "reward_kinds",
            "reward_options",
            "reward_option_kinds",
            "shop_cards",
            "shop_relics",
            "shop_potions",
            "event",
            "monsters",
            "deck",
            "total",
        ]

        wanted_ids = int(self._api.lib.cts_id_layout_slots())

        if wanted_ids != len(id_names):
            raise RuntimeError(
                "the engine writes %d ids of layout and this knows %d names "
                "for them" % (wanted_ids, len(id_names)))

        raw_ids = (ctypes.c_size_t * len(id_names))()

        self._api.lib.cts_id_layout(raw_ids)
        self.id_layout = dict(zip(id_names,
                                  [int(value) for value in raw_ids]))
        self.id_count = int(self._api.lib.cts_id_count())

        self._obs = (ctypes.c_float * self.observation_size)()
        self._mask = (ctypes.c_ubyte * self.action_count)()
        self._ids = (ctypes.c_int * self.id_count)()

    def __del__(self):
        env = getattr(self, "_env", None)

        if env:
            self._api.lib.cts_destroy(env)
            self._env = None

    # ---------------------------------------------------------------- gym
    def set_health_weight(self, weight):
        """What a point of health taken off costs, a floor being worth one."""
        lib = self._api.lib

        lib.cts_set_health_weight.argtypes = [ctypes.c_void_p, ctypes.c_float]
        lib.cts_set_health_weight(self._env, ctypes.c_float(float(weight)))

    def set_act_limit(self, acts):
        """Ends a climb once ``acts`` of the spire are cleared, 0 for all.

        The engine has to be the one that knows: taking the move that walks
        on to the next act off the table from out here would leave the climb
        standing at the top of the act with a move it may not make.
        """
        lib = self._api.lib

        lib.cts_set_act_limit.argtypes = [ctypes.c_void_p, ctypes.c_int]
        lib.cts_set_act_limit(self._env, int(acts))

    def reset(self, character="ironclad", seed=0):
        """Starts a climb and returns the first observation."""
        if isinstance(character, str):
            character = CHARACTERS[character.lower()]

        self._api.lib.cts_reset(self._env, int(character),
                                ctypes.c_uint(int(seed)))

        return self.observe()

    def step(self, action):
        """Takes the move at ``action``.

        Returns ``(observation, reward, done, info)``, where ``info`` says
        whether the move was legal at all.
        """
        taken = ctypes.c_int(0)
        done = ctypes.c_int(0)
        reward = float(
            self._api.lib.cts_step(self._env, ctypes.c_size_t(int(action)),
                                   ctypes.byref(taken), ctypes.byref(done))
        )

        return (
            self.observe(),
            reward,
            bool(done.value),
            {"taken": bool(taken.value), "phase": self.phase},
        )

    def observe(self):
        self._api.lib.cts_observe(self._env, self._obs)

        return list(self._obs)

    def observe_ids(self):
        """Returns which card, relic, potion, room and monster each id slot is
        about. Zero means the slot is empty; the rest are engine ids, meant for
        a lookup table or an embedding."""
        self._api.lib.cts_observe_ids(self._env, self._ids)

        return list(self._ids)

    def action_mask(self):
        self._api.lib.cts_action_mask(self._env, self._mask)

        return list(self._mask)

    def legal_actions(self):
        return [i for i, ok in enumerate(self.action_mask()) if ok]

    # ------------------------------------------------------------- state
    @property
    def phase(self):
        index = int(self._api.lib.cts_phase(self._env))

        return PHASES[index] if index < len(PHASES) else "invalid"

    @property
    def done(self):
        return bool(self._api.lib.cts_done(self._env))

    @property
    def act(self):
        return int(self._api.lib.cts_act(self._env))

    @property
    def floor(self):
        return int(self._api.lib.cts_floor(self._env))

    @property
    def total_floors(self):
        return int(self._api.lib.cts_total_floors(self._env))

    @property
    def gold(self):
        return int(self._api.lib.cts_gold(self._env))

    @property
    def health(self):
        return int(self._api.lib.cts_health(self._env))

    @property
    def max_health(self):
        return int(self._api.lib.cts_max_health(self._env))

    @property
    def deck_size(self):
        return int(self._api.lib.cts_deck_size(self._env))

    # -------------------------------------------------------------- saves
    def save(self):
        """Returns the climb as bytes, or None in the middle of a fight."""
        size = int(self._api.lib.cts_save(self._env, None, 0))

        if size == 0:
            return None

        buffer = ctypes.create_string_buffer(size)

        self._api.lib.cts_save(self._env, buffer, ctypes.c_size_t(size))

        return buffer.value

    def load(self, text):
        if isinstance(text, str):
            text = text.encode("utf-8")

        return bool(self._api.lib.cts_load(self._env, text))


# The kinds of move, in the order of ActionKind.
ACTION_KINDS = [
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

_TABLE = None


def action_table(library=None):
    """Returns what every slot of the fixed head is: ``(kinds, a, b)``.

    ``kinds`` holds the name of each move; ``a`` and ``b`` hold what it works
    on. The head never changes, so this is asked for once.
    """
    global _TABLE

    if _TABLE is None:
        api = _api(library)
        count = int(api.lib.cts_action_count())
        kinds = (ctypes.c_int * count)()
        a = (ctypes.c_int * count)()
        b = (ctypes.c_int * count)()

        api.lib.cts_action_table(kinds, a, b)

        names = [ACTION_KINDS[k] if 0 <= k < len(ACTION_KINDS) else "invalid"
                 for k in kinds]

        _TABLE = (names, list(a), list(b))

    return _TABLE


def rollout(env, rng, limit=5000):
    """Plays a whole climb with a die and returns what happened."""
    total = 0.0
    steps = 0

    while not env.done and steps < limit:
        legal = env.legal_actions()

        if not legal:
            break

        _, reward, _, info = env.step(rng.choice(legal))

        if not info["taken"]:
            raise RuntimeError("a legal move was turned down: " + info["phase"])

        total += reward
        steps += 1

    return {
        "steps": steps,
        "reward": total,
        "floors": env.total_floors,
        "act": env.act,
        "health": env.health,
        "deck": env.deck_size,
        "phase": env.phase,
    }


def main(argv):
    runs = int(argv[1]) if len(argv) > 1 else 5
    env = SpireEnv()

    print("library    :", env._api.path)
    print("observation:", env.observation_size, "floats", env.layout)
    print("ids        :", env.id_count, "slots", env.id_layout)
    print("actions    :", env.action_count, "slots")

    rng = random.Random(7)
    deepest = 0

    for seed in range(1, runs + 1):
        env.reset("ironclad", seed)
        out = rollout(env, rng)
        deepest = max(deepest, out["floors"])
        print(
            "seed %-3d floors=%-3d steps=%-4d reward=%-8.1f act=%d hp=%d"
            % (seed, out["floors"], out["steps"], out["reward"], out["act"],
               out["health"])
        )

    print("deepest:", deepest, "floors")

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
