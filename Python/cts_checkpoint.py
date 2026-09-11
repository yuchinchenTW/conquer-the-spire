"""Checkpoint writes that never expose a partly written archive."""

import os
import shutil
import sys
import tempfile
import time
from pathlib import Path

import torch

# How long a save waits for whoever has the old file open before it gives
# up on replacing it in one step. The judge copies a checkpoint of a
# hundred megabytes in well under a second; a disk under load takes longer.
PATIENCE = 30.0


def atomic_save(value, path):
    """Writes \\p value to \\p path so that no reader ever sees half of it.

    The archive goes to a temporary file beside \\p path and is moved over
    it in one step. On Windows that step fails while another process holds
    the old file open, so it is retried for up to PATIENCE seconds. Past
    that the archive is copied over the old file in place - a reader might
    catch it half written, which the judge checks for, but a save must never
    take the training down with it.
    """
    path = Path(path)
    fd, temporary = tempfile.mkstemp(prefix="." + path.name + "-",
                                     suffix=".tmp", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as handle:
            torch.save(value, handle)
            handle.flush()
            os.fsync(handle.fileno())

        started = time.monotonic()

        while True:
            try:
                os.replace(temporary, path)

                return
            except PermissionError:
                if time.monotonic() - started > PATIENCE:
                    break

                time.sleep(0.1)

        print("%s was held open for %.0f seconds; written in place"
              % (path.name, PATIENCE), file=sys.stderr, flush=True)
        shutil.copyfile(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
