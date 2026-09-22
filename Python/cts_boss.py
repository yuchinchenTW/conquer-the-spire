"""The same act 3 boss fight, given to several climbers.

A climber that wins less often may be losing the fight, or may be walking
into it with less to fight with. Replaying whole climbs cannot tell the
two apart, because every climber walks its own road. This saves the state
at the moment the last boss is met - one reference climber makes the road
- and then hands that exact state to each climber in turn.

    python Python/cts_boss.py runs/ironclad/well.pt runs/ironclad/fell.pt
    python Python/cts_boss.py --fights 40 --reference well.pt a.pt b.pt

The first climber named is the reference unless --reference says otherwise.
Every climber then fights the same saved fights, so the only thing that
differs is what it does once the fight has started.
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

from cts_env import PHASES, SpireEnv, action_table
from cts_net import CardPolicy, load_weights

#! Where the last boss of the act is waiting to be fought - the room, not
#! the fight. The state is saved here, before the first card is drawn, so
#! that every climber draws its own opening hand from the same deck.
AT_THE_BOSS = PHASES.index("boss")

#! Where a fight is, a card being chosen mid-fight included.
FIGHTING = (PHASES.index("battle"), PHASES.index("boss"),
            PHASES.index("choosing"))

#! What the engine pays for winning the spire and for dying, which is how
#! a fight's outcome is read off the step.
WON = 50.0
DIED = -10.0


def load(path, device):
    """A climber and what it was trained with."""
    kept = torch.load(path, map_location=device, weights_only=False)
    plan = SpireEnv()
    net = CardPolicy(plan.layout, plan.id_layout, action_table(),
                     width=kept["width"]).to(device)
    load_weights(net, kept["net"])
    net.eval()

    return net, kept


def where(env):
    """What the engine is showing now: the state, the ids and the mask."""
    return env.observe(), env.observe_ids(), env.action_mask()


def decide(net, obs, ids, mask, device):
    """The move this climber names, its own best."""
    with torch.no_grad():
        scores, _, _ = net(
            torch.as_tensor(np.asarray(obs, dtype=np.float32)
                            .reshape(1, -1), device=device),
            torch.as_tensor(np.asarray(ids).reshape(1, -1),
                            device=device).long())

    allowed = torch.as_tensor(np.asarray(mask, dtype=np.uint8)
                              .reshape(1, -1).astype(bool), device=device)

    return int(scores.masked_fill(~allowed, -1e9).argmax(1).item())


def collect(net, env, device, acts, fights, seed, hp):
    """Plays until \\p fights act-3 boss rooms have been reached, and saves
    the state at each of them.

    The states come with what the climber walked in with, because that is
    the thing being held still: two climbers given the same saved state
    have the same health, deck, relics and potions, and differ only in how
    they fight.
    """
    saved = []
    at = seed

    while len(saved) < fights:
        env.reset("ironclad", seed=at)
        obs, ids, mask = where(env)
        at += 1
        phaseAt = env.layout["phase"]
        runAt = env.layout["run"]

        for step in range(4000):
            flat = np.asarray(obs, dtype=np.float32)
            phase = int(flat[phaseAt:phaseAt + len(PHASES)].argmax())
            act = flat[runAt] * 4.0

            if phase == AT_THE_BOSS and act >= 2.5:
                # The engine will not save inside a fight, which is why
                # this is the boss room and not the first turn of it.
                state = env.save()

                if state is None:
                    raise SystemExit("the engine would not save the boss "
                                     "room; it saves out of fights only")

                saved.append((at - 1, state, float(flat[runAt + 4]),
                              float(flat[runAt + 6])))
                print("  %d of %d: seed %d, health %.0f%%, %.0f potions"
                      % (len(saved), fights, at - 1, 100 * saved[-1][2],
                         saved[-1][3] * 5), flush=True)
                break

            move = decide(net, obs, ids, mask, device)
            _, reward, done, _ = env.step(move)
            obs, ids, mask = where(env)

            if done:
                break

    return saved


def fight(net, env, device, state):
    """Loads \\p state and fights it out. Returns whether it was won."""
    if not env.load(state):
        raise SystemExit("the engine would not take a saved fight back")

    obs, ids, mask = where(env)
    phaseAt = env.layout["phase"]

    for step in range(4000):
        move = decide(net, obs, ids, mask, device)
        _, reward, done, _ = env.step(move)
        obs, ids, mask = where(env)

        if reward > WON:
            return True, step

        if reward < DIED or done:
            return False, step

        flat = np.asarray(obs, dtype=np.float32)
        phase = int(flat[phaseAt:phaseAt + len(PHASES)].argmax())

        # Out of the fight without either: the act was cleared and the
        # climb goes on, which with an act limit of 3 cannot happen after
        # the last boss - but a lower limit ends the climb here.
        if phase not in FIGHTING:
            return True, step

    return False, 4000


def main(argv):
    parser = argparse.ArgumentParser(
        description="Give several climbers the same act 3 boss fights.")
    parser.add_argument("climbers", nargs="+",
                        help="checkpoints to compare")
    parser.add_argument("--reference",
                        help="who makes the road to the boss; the first "
                             "climber named by default")
    parser.add_argument("--fights", type=int, default=40)
    parser.add_argument("--seed", type=int, default=90000)
    parser.add_argument("--hp-weight", type=float, default=0.01,
                        dest="hp_weight")
    args = parser.parse_args(argv[1:])

    device = torch.device("cpu")
    torch.set_num_threads(4)
    env = SpireEnv()
    env.set_health_weight(args.hp_weight)

    for path in [args.reference] + args.climbers:
        if path is not None and not os.path.exists(path):
            raise SystemExit("no climber at %s" % path)

    guide = args.reference or args.climbers[0]
    net, kept = load(guide, device)
    env.set_act_limit(kept["acts"])

    print("the road to the boss is walked by %s (update %d)"
          % (guide, kept["updates"]))
    saved = collect(net, env, device, kept["acts"], args.fights, args.seed,
                    args.hp_weight)
    health = np.mean([s[2] for s in saved])
    print("%d fights saved; the reference arrives with %.0f%% health"
          % (len(saved), 100 * health))
    print()
    print("%-42s %8s %8s %8s" % ("climber", "update", "won", "steps"))

    for path in args.climbers:
        net, kept = load(path, device)
        env.set_act_limit(kept["acts"])
        won = 0
        steps = []

        for seed, state, _, _ in saved:
            beat, took = fight(net, env, device, state)
            won += int(beat)
            steps.append(took)

        print("%-42s %8d %7.1f%% %8.1f"
              % (os.path.basename(path), kept["updates"],
                 100.0 * won / len(saved), float(np.mean(steps))))

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
