"""Select flat/look2 checkpoints on fixed seeds, report on separate seeds.

    python Python/cts_judge.py runs/ironclad --once
    python Python/cts_judge.py runs/ironclad --interval 3600

CPU only. Each mode completes 400 selection climbs and 400 reporting climbs
by default. Only selection wins choose best-flat.pt and best-look2.pt;
ties keep the incumbent. judged.csv records both splits and the exact update.
Changing the seeds, engine or evaluation code requires a separate --out folder
so scores from different protocols cannot silently compete.
"""

import argparse
import csv
import hashlib
import json
import math
import os
import shutil
import sys
import tempfile
import time
import zipfile
from contextlib import contextmanager
from datetime import datetime, timezone
from pathlib import Path

import torch

from cts_checkpoint import atomic_save
from cts_env import SpireEnv
from cts_play import load, play


FIELDS = ["time", "protocol", "checkpoint_sha256", "update", "steps",
          "mode", "split", "seed", "climbs", "wins", "win_rate",
          "floors", "boss_rate", "through", "seconds", "promoted"]
MODES = ("flat", "look2")


class SnapshotBusy(RuntimeError):
    pass


def digest_file(path):
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def stamp(info):
    return info.st_ino, info.st_size, info.st_mtime_ns


@contextmanager
def snapshot(source, directory):
    """Copy a stable, CRC-checked archive, including older in-place saves."""
    temporary = None
    for attempt in range(3):
        fd, name = tempfile.mkstemp(prefix=".judge-snapshot-", suffix=".pt",
                                    dir=directory)
        temporary = Path(name)
        try:
            with os.fdopen(fd, "wb") as target, open(source, "rb") as handle:
                before = stamp(os.fstat(handle.fileno()))
                shutil.copyfileobj(handle, target, length=1024 * 1024)
                after = stamp(os.fstat(handle.fileno()))
            if (before != after or after != stamp(Path(source).stat()) or
                    temporary.stat().st_size != before[1]):
                raise SnapshotBusy("checkpoint changed while copying")
            # Same size does not establish completeness for a torch zip file.
            with zipfile.ZipFile(temporary) as archive:
                if archive.testzip() is not None:
                    raise SnapshotBusy("checkpoint CRC mismatch")
            fingerprint = digest_file(temporary)
            break
        except (OSError, EOFError, zipfile.BadZipFile, SnapshotBusy) as error:
            temporary.unlink(missing_ok=True)
            if attempt == 2:
                raise SnapshotBusy("no complete snapshot: %s"
                                   % error) from error
            time.sleep(0.2)
    try:
        yield temporary, fingerprint
    finally:
        temporary.unlink(missing_ok=True)


@contextmanager
def judge_lock(folder):
    """One writer per output folder; the OS releases the lock on a crash."""
    with open(Path(folder) / ".judge.lock", "a+b") as handle:
        if handle.tell() == 0:
            handle.write(b"0")
            handle.flush()
        handle.seek(0)
        try:
            if os.name == "nt":
                import msvcrt
                msvcrt.locking(handle.fileno(), msvcrt.LK_NBLCK, 1)
            else:
                import fcntl
                fcntl.flock(handle.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        except OSError as error:
            raise RuntimeError("another judge is using %s" % folder) from error
        try:
            yield
        finally:
            if os.name == "nt":
                handle.seek(0)
                msvcrt.locking(handle.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                fcntl.flock(handle.fileno(), fcntl.LOCK_UN)


def runtime_identity():
    root = Path(__file__).resolve().parent
    files = ("cts_judge.py", "cts_play.py", "cts_ask.py", "cts_net.py",
             "cts_env.py", "cts_vec.py", "cts_log.py")
    return {"code": {name: digest_file(root / name) for name in files},
            "engine": digest_file(SpireEnv()._api.path),
            "torch": torch.__version__}


def protocol_of(args, kept, runtime):
    hp = args.hp_weight if args.hp_weight is not None else kept.get("hp_weight")
    if hp is None or not math.isfinite(hp) or hp < 0:
        raise ValueError("checkpoint has no hp_weight; pass --hp-weight 0.01 "
                         "to match this run")
    gamma = kept.get("gamma", 0.999)
    if not math.isfinite(gamma) or not 0 <= gamma <= 1:
        raise ValueError("invalid checkpoint gamma")
    config = dict(runtime, character=kept["character"], acts=kept["acts"],
                  floats=kept["floats"], ids=kept["ids"],
                  actions=kept["actions"], hp_weight=hp, gamma=gamma,
                  max_hp_weight=kept.get("max_hp_weight"),
                  curse_penalty=kept.get("curse_penalty"),
                  seed=args.seed, climbs=args.climbs,
                  report_seed=args.report_seed,
                  report_climbs=args.report_climbs,
                  envs=args.envs, threads=args.threads)
    encoded = json.dumps(config, sort_keys=True).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest(), config


def read_history(path):
    if not path.exists():
        return []
    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames != FIELDS:
            raise ValueError("unexpected judged.csv columns; use another --out")
        return list(reader)


def write_history(path, rows):
    fd, temporary = tempfile.mkstemp(prefix=".judged-", suffix=".csv",
                                     dir=path.parent)
    try:
        with os.fdopen(fd, "w", newline="", encoding="utf-8") as handle:
            writer = csv.DictWriter(handle, fieldnames=FIELDS)
            writer.writeheader()
            writer.writerows(rows)
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def evaluate(net, kept, args, mode, split, config):
    reporting = split == "report"
    seed = args.report_seed if reporting else args.seed
    climbs = args.report_climbs if reporting else args.climbs
    started = time.monotonic()
    got = play(net, kept, torch.device("cpu"), climbs, args.envs,
               looks=0 if mode == "flat" else 2, fights=False,
               hp=config["hp_weight"], seed=seed, gamma=config["gamma"])
    if len(got) != climbs:
        raise RuntimeError("evaluation did not complete the fixed seed list")
    wins = sum(int(one["won_the_spire"]) for one in got)
    return {"seed": seed, "climbs": climbs, "wins": wins,
            "win_rate": wins / climbs,
            "floors": sum(one["floors"] for one in got) / climbs,
            "boss_rate": sum(one["bosses_won"] > 0 for one in got) / climbs,
            "through": sum(one["bosses_won"] > 1 for one in got) / climbs,
            "seconds": round(time.monotonic() - started, 3)}


def incumbent(path, protocol):
    if not path.exists():
        return None
    kept = torch.load(path, map_location="cpu", weights_only=False)
    judged = kept.get("judged", {})
    if judged.get("protocol") != protocol:
        raise ValueError("%s belongs to a different evaluation protocol; "
                         "use another --out folder" % path)
    return judged


def judge_once(args, runtime):
    output = Path(args.out or args.folder)
    source = Path(args.folder) / "checkpoint.pt"
    history_path = output / "judged.csv"
    with snapshot(source, output) as (frozen, fingerprint):
        torch.manual_seed(0)
        net, kept = load(str(frozen), torch.device("cpu"))
        protocol, config = protocol_of(args, kept, runtime)
        rows = read_history(history_path)
        if any(row["protocol"] != protocol for row in rows):
            raise ValueError("judged.csv uses another protocol; "
                             "use another --out")
        for mode in MODES:
            best_path = output / ("best-%s.pt" % mode)
            best = incumbent(best_path, protocol)
            completed = {r["split"] for r in rows
                         if r["checkpoint_sha256"] == fingerprint and
                         r["mode"] == mode}
            if completed == {"selection", "report"} and best is not None:
                print("update %d %s already judged" % (kept["updates"], mode),
                      flush=True)
                continue
            print("update %d %s: selection seeds %d..%d" %
                  (kept["updates"], mode, args.seed,
                   args.seed + args.climbs - 1), flush=True)
            selected = evaluate(net, kept, args, mode, "selection", config)
            # Decide before looking at reporting outcomes; ties do not promote.
            promote = (best is None
                       or selected["wins"] > best["selection"]["wins"])
            print("  selection %.2f%%; reporting seeds %d..%d" %
                  (100 * selected["win_rate"], args.report_seed,
                   args.report_seed + args.report_climbs - 1), flush=True)
            reported = evaluate(net, kept, args, mode, "report", config)
            at = datetime.now(timezone.utc).isoformat(timespec="seconds")
            if promote:
                winner = dict(kept)
                winner["hp_weight"] = config["hp_weight"]
                winner["gamma"] = config["gamma"]
                winner["judged"] = dict(protocol=protocol, config=config,
                                        checkpoint_sha256=fingerprint,
                                        mode=mode, selection=selected,
                                        report=reported, time=at)
                atomic_save(winner, best_path)
            for split, result in (("selection", selected),
                                  ("report", reported)):
                rows.append(dict(result, time=at, protocol=protocol,
                                 checkpoint_sha256=fingerprint,
                                 update=kept["updates"], steps=kept["steps"],
                                 mode=mode, split=split,
                                 promoted=int(promote)))
            write_history(history_path, rows)
            print("  report %.2f%%, %.2f floors%s" %
                  (100 * reported["win_rate"], reported["floors"],
                   "; kept " + best_path.name if promote
                   else "; incumbent kept"),
                  flush=True)


def arguments(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("folder", nargs="?", default="runs/ironclad")
    parser.add_argument("--out", help="output folder, default: the run folder")
    parser.add_argument("--once", action="store_true")
    parser.add_argument("--interval", type=float, default=3600,
                        help="seconds to wait after finishing each checkpoint")
    parser.add_argument("--climbs", type=int, default=400)
    parser.add_argument("--report-climbs", type=int, default=400)
    parser.add_argument("--seed", type=int, default=200005)
    parser.add_argument("--report-seed", type=int, default=300005)
    parser.add_argument("--envs", type=int, default=16)
    parser.add_argument("--threads", type=int, default=2)
    parser.add_argument("--hp-weight", type=float, default=None)
    args = parser.parse_args(argv)
    if min(args.climbs, args.report_climbs, args.envs, args.threads) <= 0:
        parser.error("climbs, report-climbs, envs and threads must be positive")
    if not math.isfinite(args.interval) or args.interval <= 0:
        parser.error("interval must be positive and finite")
    for seed, count in ((args.seed, args.climbs),
                        (args.report_seed, args.report_climbs)):
        if seed < 0 or seed + count > 2**32:
            parser.error("seed ranges must fit unsigned 32-bit integers")
    if max(args.seed, args.report_seed) < min(
            args.seed + args.climbs,
            args.report_seed + args.report_climbs):
        parser.error("selection and reporting seeds must not overlap")
    return args


def main(argv=None):
    args = arguments(argv)
    torch.set_num_threads(args.threads)
    torch.set_num_interop_threads(1)
    output = Path(args.out or args.folder)
    output.mkdir(parents=True, exist_ok=True)
    runtime = runtime_identity()
    print("CPU judge: %d threads, %d envs; selection %d + reporting %d "
          "climbs per mode" % (args.threads, args.envs, args.climbs,
                                args.report_climbs), flush=True)
    try:
        with judge_lock(output):
            while True:
                try:
                    judge_once(args, runtime)
                except SnapshotBusy as error:
                    print(error, flush=True)
                    if args.once:
                        return 1
                if args.once:
                    return 0
                time.sleep(args.interval)
    except KeyboardInterrupt:
        print("judge stopped; training was not touched", flush=True)
        return 0
    except (OSError, ValueError, RuntimeError) as error:
        print("judge: %s" % error, file=sys.stderr, flush=True)
        return 1


if __name__ == "__main__":
    sys.exit(main())
