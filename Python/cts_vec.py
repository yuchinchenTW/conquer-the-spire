"""A row of climbs stepped together, for feeding a learner.

The single environment in ``cts_env`` crosses the ctypes wall once a move,
which costs about as much as the move itself. This one crosses it once a tick
for the whole row, and hands back numpy views onto buffers that are written in
place - no copying, no per-move call.

    from cts_vec import VecSpireEnv

    vec = VecSpireEnv(64)
    obs, ids, mask = vec.reset("ironclad", seed=1)

    while True:
        actions = policy(obs, ids, mask)              # one index a climb
        obs, ids, mask, reward, done, info = vec.step(actions)

        for r, l in zip(info["returns"][done], info["lengths"][done]):
            print("a climb ended:", r, "over", l, "moves")

A climb that ends starts another one on its own with the next seed, so the row
never goes quiet; the tick it ended on reports what it was worth altogether.

Run this file on its own to see how fast the row goes.
"""

import ctypes
import sys
import time

from cts_env import CHARACTERS, PHASES, _api

try:
    import numpy as np
except ImportError:  # pragma: no cover - numpy is only a convenience here
    np = None


class VecSpireEnv(object):
    """``count`` climbs, stepped together."""

    def __init__(self, count=8, library=None):
        self._api = _api(library)
        lib = self._api.lib

        self._declare(lib)

        self.count = int(count)
        self._vec = lib.cts_vec_create(ctypes.c_size_t(self.count))

        if not self._vec:
            raise MemoryError("the row would not start")

        self.observation_size = int(lib.cts_observation_size())
        self.id_count = int(lib.cts_id_count())
        self.action_count = int(lib.cts_action_count())

        # One buffer each, written in place every tick.
        self._obs = (ctypes.c_float * (self.count * self.observation_size))()
        self._ids = (ctypes.c_int * (self.count * self.id_count))()
        self._mask = (ctypes.c_ubyte * (self.count * self.action_count))()
        self._actions = (ctypes.c_size_t * self.count)()
        self._rewards = (ctypes.c_float * self.count)()
        self._dones = (ctypes.c_ubyte * self.count)()
        self._taken = (ctypes.c_ubyte * self.count)()
        self._returns = (ctypes.c_float * self.count)()
        self._lengths = (ctypes.c_int * self.count)()

        if np is not None:
            self.obs = np.ctypeslib.as_array(self._obs).reshape(
                self.count, self.observation_size)
            self.ids = np.ctypeslib.as_array(self._ids).reshape(
                self.count, self.id_count)
            self.mask = np.ctypeslib.as_array(self._mask).reshape(
                self.count, self.action_count)
            self.rewards = np.ctypeslib.as_array(self._rewards)
            self.dones = np.ctypeslib.as_array(self._dones).view(np.bool_)
            self.taken = np.ctypeslib.as_array(self._taken).view(np.bool_)
            self.returns = np.ctypeslib.as_array(self._returns)
            self.lengths = np.ctypeslib.as_array(self._lengths)
        else:
            self.obs = self._obs
            self.ids = self._ids
            self.mask = self._mask
            self.rewards = self._rewards
            self.dones = self._dones
            self.taken = self._taken
            self.returns = self._returns
            self.lengths = self._lengths

    @staticmethod
    def _declare(lib):
        if getattr(lib, "_cts_vec_declared", False):
            return

        lib.cts_vec_create.argtypes = [ctypes.c_size_t]
        lib.cts_vec_create.restype = ctypes.c_void_p
        lib.cts_vec_destroy.argtypes = [ctypes.c_void_p]
        lib.cts_vec_count.argtypes = [ctypes.c_void_p]
        lib.cts_vec_count.restype = ctypes.c_size_t
        lib.cts_vec_reset.argtypes = [ctypes.c_void_p, ctypes.c_int,
                                      ctypes.c_uint]
        lib.cts_vec_reset_one.argtypes = [ctypes.c_void_p, ctypes.c_size_t,
                                          ctypes.c_int, ctypes.c_uint]
        lib.cts_vec_set_auto_reset.argtypes = [ctypes.c_void_p, ctypes.c_int]
        lib.cts_vec_observe.argtypes = [ctypes.c_void_p,
                                        ctypes.POINTER(ctypes.c_float)]
        lib.cts_vec_observe_ids.argtypes = [ctypes.c_void_p,
                                            ctypes.POINTER(ctypes.c_int)]
        lib.cts_vec_action_mask.argtypes = [ctypes.c_void_p,
                                            ctypes.POINTER(ctypes.c_ubyte)]
        lib.cts_vec_step.argtypes = [
            ctypes.c_void_p,
            ctypes.POINTER(ctypes.c_size_t),
            ctypes.POINTER(ctypes.c_float),
            ctypes.POINTER(ctypes.c_ubyte),
            ctypes.POINTER(ctypes.c_ubyte),
            ctypes.POINTER(ctypes.c_float),
            ctypes.POINTER(ctypes.c_int),
        ]
        lib.cts_vec_rank.argtypes = [
            ctypes.c_void_p,
            ctypes.POINTER(ctypes.c_size_t),
            ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_size_t),
        ]
        lib.cts_vec_phase.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
        lib.cts_vec_phase.restype = ctypes.c_int
        lib.cts_vec_total_floors.argtypes = [ctypes.c_void_p,
                                             ctypes.c_size_t]
        lib.cts_vec_total_floors.restype = ctypes.c_int
        lib.cts_roll_random.argtypes = [
            ctypes.c_int,
            ctypes.c_uint,
            ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_float),
            ctypes.POINTER(ctypes.c_int),
            ctypes.POINTER(ctypes.c_int),
        ]

        lib._cts_vec_declared = True

    def __del__(self):
        vec = getattr(self, "_vec", None)

        if vec:
            self._api.lib.cts_vec_destroy(vec)
            self._vec = None

    # ---------------------------------------------------------------- gym
    def set_health_weight(self, weight):
        """What a point of health taken off costs every climb of the row."""
        lib = self._api.lib

        lib.cts_vec_set_health_weight.argtypes = [ctypes.c_void_p,
                                                  ctypes.c_float]
        lib.cts_vec_set_health_weight(self._vec,
                                      ctypes.c_float(float(weight)))

    def set_max_health_weight(self, weight):
        """What a point of the health ceiling is worth, for the whole row."""
        lib = self._api.lib

        lib.cts_vec_set_max_health_weight.argtypes = [ctypes.c_void_p,
                                                      ctypes.c_float]
        lib.cts_vec_set_max_health_weight(self._vec,
                                          ctypes.c_float(float(weight)))

    def set_curse_penalty(self, penalty):
        """What a curse costs every climb of the row, a floor at a time."""
        lib = self._api.lib

        lib.cts_vec_set_curse_penalty.argtypes = [ctypes.c_void_p,
                                                  ctypes.c_float]
        lib.cts_vec_set_curse_penalty(self._vec,
                                      ctypes.c_float(float(penalty)))

    def set_act_limit(self, acts):
        """Ends every climb of the row once ``acts`` are cleared, 0 for all."""
        lib = self._api.lib

        lib.cts_vec_set_act_limit.argtypes = [ctypes.c_void_p, ctypes.c_int]
        lib.cts_vec_set_act_limit(self._vec, int(acts))

    def reset(self, character="ironclad", seed=0):
        """Starts every climb and returns ``(obs, ids, mask)``."""
        if isinstance(character, str):
            character = CHARACTERS[character.lower()]

        self._api.lib.cts_vec_reset(self._vec, int(character),
                                    ctypes.c_uint(int(seed)))

        return self._look()

    def set_auto_reset(self, on=True):
        self._api.lib.cts_vec_set_auto_reset(self._vec, 1 if on else 0)

    def step(self, actions, observe=True):
        """Takes one move in every climb.

        ``actions`` is one action index a climb. Returns
        ``(obs, ids, mask, rewards, dones, info)``; ``info`` carries the
        returns and lengths of the climbs that ended on this tick, and
        whether each move was legal.

        With ``observe=False`` the state buffers are left as they were, which
        is worth doing when a tick's state is not going to be looked at.
        """
        for i in range(self.count):
            self._actions[i] = int(actions[i])

        self._api.lib.cts_vec_step(self._vec, self._actions, self._rewards,
                                   self._dones, self._taken, self._returns,
                                   self._lengths)

        if observe:
            obs, ids, mask = self._look()
        else:
            obs, ids, mask = self.obs, self.ids, self.mask

        return (
            obs,
            ids,
            mask,
            self.rewards,
            self.dones,
            {
                "taken": self.taken,
                "returns": self.returns,
                "lengths": self.lengths,
            },
        )

    def _look(self):
        lib = self._api.lib

        lib.cts_vec_observe(self._vec, self._obs)
        lib.cts_vec_observe_ids(self._vec, self._ids)
        lib.cts_vec_action_mask(self._vec, self._mask)

        return self.obs, self.ids, self.mask

    # ------------------------------------------------- looking a fight ahead
    def rank(self, candidates):
        """Of the moves offered per climb, which comes out of the fight best.

        ``candidates`` is ``[count, width]`` of move indices, best-first by
        whatever offered them; an index at or past the action count is an
        empty slot. Returns one index a climb: the candidate whose fight is
        won, else costs least health, else leaves the monsters worst off. A
        climb not in a fight gets its first candidate back.

        The whole batch crosses in one call because the crossing costs more
        than the work - a simulated fight is about five microseconds.
        """
        rows = np.ascontiguousarray(candidates, dtype=np.uintp)

        if rows.ndim != 2 or rows.shape[0] != self.count:
            raise ValueError("candidates must be [count, width]")

        out = np.empty(self.count, dtype=np.uintp)
        self._api.lib.cts_vec_rank(
            self._vec,
            rows.ctypes.data_as(ctypes.POINTER(ctypes.c_size_t)),
            rows.shape[1],
            out.ctypes.data_as(ctypes.POINTER(ctypes.c_size_t)))

        return out.astype(np.int64)

    # ------------------------------------------------------------- a look
    def phase(self, index):
        got = int(self._api.lib.cts_vec_phase(self._vec, index))

        return PHASES[got] if got < len(PHASES) else "invalid"

    def total_floors(self, index):
        return int(self._api.lib.cts_vec_total_floors(self._vec, index))


def roll_random(character="ironclad", seed=1, runs=100, library=None):
    """Plays whole climbs with a die on the engine side, and reports each.

    Returns ``(returns, floors, steps)``. Nothing crosses the wall but the one
    call, so this is what the engine can do at its fastest.
    """
    api = _api(library)

    VecSpireEnv._declare(api.lib)

    if isinstance(character, str):
        character = CHARACTERS[character.lower()]

    returns = (ctypes.c_float * runs)()
    floors = (ctypes.c_int * runs)()
    steps = (ctypes.c_int * runs)()

    api.lib.cts_roll_random(int(character), ctypes.c_uint(int(seed)),
                            ctypes.c_size_t(runs), returns, floors, steps)

    if np is not None:
        return (np.ctypeslib.as_array(returns).copy(),
                np.ctypeslib.as_array(floors).copy(),
                np.ctypeslib.as_array(steps).copy())

    return list(returns), list(floors), list(steps)


def _pick_random(mask, rng):
    """One legal move a climb, chosen with a die."""
    if np is not None:
        picked = []

        for row in mask:
            legal = np.flatnonzero(row)
            picked.append(int(rng.choice(legal)) if legal.size else 0)

        return picked

    return [0] * len(mask)


def main(argv):
    count = int(argv[1]) if len(argv) > 1 else 32
    ticks = int(argv[2]) if len(argv) > 2 else 2000

    if np is None:
        print("numpy is not here, so this demo has nothing to pick with")
        return 1

    rng = np.random.default_rng(7)
    vec = VecSpireEnv(count)

    print("row        :", vec.count, "climbs")
    print("observation:", vec.observation_size, "floats a climb")
    print("ids        :", vec.id_count, "a climb")
    print("actions    :", vec.action_count, "slots")

    obs, ids, mask = vec.reset("ironclad", seed=1)

    ended = 0
    total = 0.0
    start = time.perf_counter()

    for _ in range(ticks):
        actions = _pick_random(mask, rng)
        obs, ids, mask, rewards, dones, info = vec.step(actions)

        if dones.any():
            ended += int(dones.sum())
            total += float(info["returns"][dones].sum())

    spent = time.perf_counter() - start
    steps = ticks * count

    print("%d ticks x %d climbs = %d moves in %.2f s" % (ticks, count, steps,
                                                          spent))
    print("  %.0f moves/s  %.0f ticks/s" % (steps / spent, ticks / spent))
    print("  climbs ended: %d, average return %.1f" %
          (ended, total / ended if ended else 0.0))

    start = time.perf_counter()
    returns, floors, _ = roll_random("ironclad", 1, 400)
    spent = time.perf_counter() - start

    print("engine on its own: 400 climbs in %.2f s (%.0f climbs/s)" %
          (spent, 400 / spent))
    print("  average floors %.1f, deepest %d, average return %.1f" %
          (float(floors.mean()), int(floors.max()), float(returns.mean())))

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
