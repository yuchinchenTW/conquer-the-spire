# Independent Checkpoint Evaluation

Run beside training, using the Python installation with torch:

```bat
python Python/cts_judge.py runs/ironclad
```

The judge uses CPU only, with two torch threads and 16 environments. It waits
one hour after a checkpoint's evaluation completes before checking again.
It skips checkpoints already evaluated under the same protocol. Training
continues independently; the judge can still compete for CPU, RAM and disk.

Each frozen checkpoint is evaluated four times:

| Mode | Selection | Reporting |
|---|---|---|
| Flat | seeds 200005-200404 | seeds 300005-300404 |
| Two candidates, everywhere | seeds 200005-200404 | seeds 300005-300404 |

Every listed seed finishes. Only the selection win count decides whether to
replace `best-flat.pt` or `best-look2.pt`. Ties retain the current winner.
Reporting wins never enter that decision. `judged.csv` records update, source
hash, mode, split, seed range, wins, floors, boss rates and evaluation time.
Each winner includes the original training checkpoint and its evaluation
metadata, including both scores.

The first evaluated checkpoint establishes the incumbent; older `best.pt`
weights do not enter the competition automatically. This selects the best
among evaluated snapshots, not among every update produced by training.

The default health weight comes from the checkpoint. For a legacy checkpoint
without it, explicitly pass `--hp-weight 0.01`. The judge refuses to silently
use the engine's different default. Gamma is read from new checkpoints and
defaults to this run's historical 0.999 for old files.

Useful options:

```bat
python Python/cts_judge.py runs/ironclad --once
python Python/cts_judge.py runs/ironclad --threads 1 --envs 8
python Python/cts_judge.py runs/ironclad --out runs/ironclad-judge-experiment ^
    --seed 400005 --report-seed 500005
```

Changing the evaluation code, engine, seeds or scoring settings requires a
separate output directory. Existing winners are not overwritten using scores
from another protocol. The file lock permits only one judge per output folder.
The trainer writes checkpoints through an atomic replacement. The judge also
checks size, modification metadata and zip CRCs when copying older saves that
were written in place. A source replaced during evaluation cannot change the
snapshot being scored.

Stop the judge before starting training with `--fresh`. Fresh training archives
the two judged winners and `judged.csv` alongside the old training results.

Playing and comparing weights:

```bat
python Python/cts_play.py runs/ironclad 400
python Python/cts_play.py runs/ironclad 400 --flat
python Python/cts_play.py runs/ironclad/checkpoint.pt 400 --seed 1005
python Python/cts_play.py runs/ironclad/checkpoint.pt 400 --seed 1005 --flat
```

A directory selects `best-look2.pt` by default and `best-flat.pt` for flat
play, falling back to `best.pt` then `checkpoint.pt` before judged winners
exist. Pass an explicit checkpoint file when comparing modes on the same
weights. The selected path and update are printed.

Repeated inspection of the reporting set can influence later human tuning.
Use another untouched seed set for a final claim of exceeding 50% wins, and
report sampling uncertainty. A validation winner is not proof of improvement
on all seeds.
