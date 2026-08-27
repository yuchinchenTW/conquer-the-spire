"""Watching a training run as it goes.

Three ways, and they all read the same csv the trainer writes, so any of them
can be opened and closed while the training carries on:

    python cts_plot.py ironclad             a window that redraws itself
    python cts_plot.py ironclad --once      one picture, saved as a png
    tensorboard --logdir runs               the trainer writes events too

The trainer also writes ``runs/<character>/progress.html`` every time it
reports, which is a page with the curves drawn into it and nothing else - no
libraries, no server. Open it in a browser and it refreshes itself.
"""

import argparse
import csv
import os
import sys
import time

# What the trainer writes, in order.
COLUMNS = ["updates", "steps", "episodes", "return", "floors", "fights",
           "boss_rate", "win_rate", "loss",
           "cards_taken", "cards_removed", "cards_upgraded",
           "cards_transformed", "rests",
           "curses_chosen", "curses_refused", "curse_refusal",
           "deepest_act", "through"]

# How wide a row was before the deck counts were added to the end of it. A
# curve written by the older trainer is still worth drawing, so a row of that
# width is read with the counts left at zero rather than thrown away.
LEGACY_COLUMNS = 9

# What is worth looking at, and how to draw it. The order is the order the
# window lays them out in: the first four across the top, the rest below.
PANELS = [
    ("floors", "floors reached", "#4c9f70"),
    ("boss_rate", "climbs that put a boss down", "#c76b4a"),
    ("fights", "fights won a climb", "#6a8f4a"),
    ("curse_refusal", "curse offers turned down", "#9a4a7a"),
    ("return", "reward a climb", "#4a6fc7"),
    ("loss", "loss", "#8a8a8a"),
    ("cards_upgraded", "cards upgraded a climb", "#c9a227"),
    ("cards_removed", "cards removed a climb", "#4a9a9a"),
    ("deepest_act", "deepest act reached", "#7a5ac9"),
    ("through", "climbs that put two bosses down", "#c94a6f"),
]


FIRM = 30
"""How few sightings make a share not worth reading on its own."""

THINNER_THAN = 20
"""And how far below the best-seen row of its own table a row may fall before
its share stops being comparable with the others. A quarter of forty and a
quarter of twelve thousand are not the same fact, and sorting them together
ranks them as though they were: the second act's bosses are met a few dozen
times because the climber dies in the first act, so their shares rest on
three hundred times less than the first act's do and would otherwise sit
interleaved with them looking broken."""


def readPicks(folder):
    """Reads the choices of the last window out of ``picks.csv``.

    The trainer writes one batch of rows every time it reports, each stamped
    with the update it belongs to, so the last stamp is what the policy has
    been doing lately.
    """
    path = os.path.join(folder, "picks.csv")

    if not os.path.exists(path):
        return {}

    rows = []

    with open(path, "r", newline="") as handle:
        for row in csv.DictReader(handle):
            try:
                row["updates"] = int(row["updates"])
                row["picks"] = int(row["picks"])
                row["seen"] = int(row["seen"])
                row["runs"] = int(row["runs"])
                row["pick_rate"] = float(row["pick_rate"])
                row["win_rate"] = float(row["win_rate"])
                row["avg_floors"] = float(row["avg_floors"])
            except (KeyError, TypeError, ValueError):
                continue

            rows.append(row)

    if not rows:
        return {}

    last = max(row["updates"] for row in rows)
    out = {}

    for row in rows:
        if row["updates"] == last:
            out.setdefault(row["kind"], []).append(row)

    for kept in out.values():
        kept.sort(key=lambda one: (-one["pick_rate"], -one["seen"]))

    return out


def read(folder):
    """Reads the curve, oldest first. Rows that are still being written are
    skipped rather than guessed at."""
    path = os.path.join(folder, "curve.csv")

    if not os.path.exists(path):
        return []

    out = []

    with open(path, "r", newline="") as handle:
        for row in csv.reader(handle):
            # Anything from the oldest shape up to the current one is read,
            # so that a curve started before a column was added carries on
            # being drawn instead of vanishing from the window.
            if not LEGACY_COLUMNS <= len(row) <= len(COLUMNS):
                continue

            try:
                values = [float(one) for one in row]
            except ValueError:
                continue

            # A row from an older trainer stops early; the columns it never
            # wrote read as nothing rather than as a gap in the line.
            values += [0.0] * (len(COLUMNS) - len(values))

            out.append(dict(zip(COLUMNS, values)))

    return out


def smooth(values, window=9):
    """A moving average, for reading the shape through the noise.

    The window is kept short while there is little to average, so that an
    early curve is not buried under its own lag.
    """
    window = max(1, min(window, len(values) // 4))

    if window <= 1 or len(values) < 2:
        return list(values)

    out = []
    total = 0.0
    kept = []

    for value in values:
        kept.append(value)
        total += value

        if len(kept) > window:
            total -= kept.pop(0)

        out.append(total / len(kept))

    return out


# ------------------------------------------------------------------- a page
def _svg(rows, key, title, colour, width=560, height=180):
    """One panel of the page: a polyline with a floor and a ceiling on it."""
    values = [row[key] for row in rows]
    steps = [row["steps"] for row in rows]

    if not values:
        return "<p>nothing yet</p>"

    low = min(values)
    high = max(values)
    span = (high - low) or 1.0
    left = 46
    bottom = 24
    plotW = width - left - 10
    plotH = height - bottom - 18

    def place(step, value):
        x = left + plotW * (step - steps[0]) / ((steps[-1] - steps[0]) or 1)
        y = 12 + plotH - plotH * (value - low) / span

        return "%.1f,%.1f" % (x, y)

    raw = " ".join(place(s, v) for s, v in zip(steps, values))
    mean = " ".join(place(s, v)
                    for s, v in zip(steps, smooth(values)))

    return (
        '<figure><figcaption>%s <b>%.3g</b></figcaption>'
        '<svg viewBox="0 0 %d %d" role="img">'
        '<line x1="%d" y1="%d" x2="%d" y2="%d" class="axis"/>'
        '<line x1="%d" y1="%d" x2="%d" y2="%d" class="axis"/>'
        '<text x="4" y="16" class="tick">%.3g</text>'
        '<text x="4" y="%d" class="tick">%.3g</text>'
        '<polyline points="%s" fill="none" stroke="%s" stroke-width="1" '
        'opacity="0.35"/>'
        '<polyline points="%s" fill="none" stroke="%s" stroke-width="2"/>'
        '</svg></figure>'
        % (title, values[-1], width, height,
           left, 12, left, 12 + plotH,
           left, 12 + plotH, left + plotW, 12 + plotH,
           high, 12 + plotH, low,
           raw, colour, mean, colour)
    )


# The tables of choices the page shows, and what to call them.
# The tables the page shows, what to call them, and how many rows of each
# to draw. Nought means every one there is: which card it drafts and which it
# leaves is the whole question, and the answer is not in the top ten.
PICK_TABLES = [
    ("card_taken", "cards taken", 0),
    ("card_removed", "cards removed", 0),
    ("card_upgraded", "cards upgraded", 0),
    ("relic_taken", "relics taken", 0),
    ("potion_taken", "potions taken", 0),
    ("room_answered", "rooms answered", 0),
    ("node_walked", "paths taken", 0),
    ("curse_option", "rooms that offered a curse", 0),

    # For a fight the share is the share of them it won, not the share of
    # offers it took, so these read as win rates.
    ("boss_fought", "bosses, and how often it won", 0),
    ("elite_fought", "elites, and how often it won", 0),
    ("fight_fought", "fights, and how often it won", 0),
]

# Curses get a panel of their own: they are never offered beside a real card,
# so they would otherwise sit unnoticed in the middle of the cards taken.
CURSE_TITLE = "curses picked up"


def _spared(rows):
    """The same rows without the curses in them.

    A curse is never turned down, so it reads as picked every time it turns
    up and would sit at the top of the cards for ever. They have a panel of
    their own instead.
    """
    from cts_log import card_type

    out = []

    for row in rows:
        kind = row.get("type")

        if kind is None:
            try:
                kind = card_type(int(row["id"]))
            except (KeyError, TypeError, ValueError):
                kind = ""

        if kind != "curse":
            out.append(row)

    return out


def _curses(picks):
    """The curse rows, worst first.

    The trainer writes them under a kind of their own, which is what the
    file has; a table read straight off the engine has them among the cards
    with a type on them instead.
    """
    from cts_stats import GAIN_KINDS

    rows = list((picks or {}).get("curse_taken", []))

    if not rows:
        rows = [row for kind in GAIN_KINDS
                for row in (picks or {}).get(kind, [])
                if row.get("type") == "curse"]

    rows.sort(key=lambda one: (-one["picks"], one["name"]))

    return rows


# Where a picture of a card is looked for. The page is opened off the disk,
# so a path relative to the folder it sits in is all it takes: a page in
# runs/ironclad reaches Assets/cards two folders up.
ART = os.path.join("Assets", "cards")
ART_FROM_PAGE = "../../Assets/cards"

# What a kind of card is drawn in when there is no picture of it: the colours
# the game itself uses for the four corners of a deck.
KIND_COLOURS = {
    "attack": "#b4453c",
    "skill": "#3f6f9c",
    "power": "#b58a2e",
    "status": "#7d7d85",
    "curse": "#4b3a5c",
    "relic": "#8a6f3c",
    "potion": "#3f8a6f",
}


def _slug(name):
    """The file a card would be kept under: lower case, no spaces."""
    kept = [one.lower() if one.isalnum() else "_" for one in name]

    return "".join(kept).strip("_")


# Which of the tables are about a thing there is a picture of, and what that
# picture is filed under.
PICTURED = {
    "card_taken": "", "card_bought": "", "card_removed": "",
    "card_upgraded": "", "card_transformed": "", "card_played": "",
    "curse_taken": "",
    "relic_taken": "relic_", "relic_bought": "relic_",
    "potion_taken": "potion_", "potion_bought": "potion_",
    "potion_drunk": "potion_",
}


def _art(row, kind):
    """A picture of the thing, or one drawn from what it is.

    A folder of pictures is the better answer and needs nothing but the
    files; without one, the kind and the first letters still tell a Strike
    from a Bash at a glance down the column. A row about a room or a place on
    the map is not a thing with a picture, and gets nothing.
    """
    if kind not in PICTURED:
        return ""

    name = row.get("name", "")
    slug = PICTURED[kind] + _slug(name)
    picture = os.path.join(ART, slug + ".png")

    if os.path.exists(picture):
        return ('<img class="art" src="%s/%s.png" alt="" loading="lazy">'
                % (ART_FROM_PAGE, slug))

    # Nothing on the disk for it: a tile in the colour of what it is.
    what = row.get("type") or ""

    if not what and kind.startswith(("card", "curse")):
        try:
            from cts_log import card_type

            what = card_type(int(row["id"]))
        except (ImportError, KeyError, TypeError, ValueError, OSError):
            what = ""

    if kind.startswith("relic"):
        what = "relic"
    elif kind.startswith("potion"):
        what = "potion"

    colour = KIND_COLOURS.get(what, "#6f757c")
    short = "".join(one[0] for one in name.split()[:2]).upper() or "?"

    return ('<span class="art drawn" style="background:%s" title="%s">%s'
            "</span>" % (colour, name, short))


def _table(rows, title, top=10, count=False, kind=""):
    """One table of choices. ``top`` of nought draws every row there is."""
    """One table of choices: a bar for how often it is picked when it turns
    up, and the numbers beside it."""
    if not rows:
        return ""

    top = len(rows) if top <= 0 else top

    # Sorted by the column the table is about: how often it was picked when
    # it turned up. What the engine hands over is ordered by how often a
    # thing was seen, which buries a card taken every time it appeared under
    # the Strikes it was offered beside. A count of curses has no rate to
    # sort by, so those go by how many were taken.

    # The best-attended row of this table, which is what the thin ones are
    # thin against. Taken before the sort, because the sort asks.
    firmest = max([int(one.get("offered", one.get("seen", 0)))
                   for one in rows] + [0])

    def thinly(row):
        seen = int(row.get("offered", row.get("seen", 0)))

        return seen < FIRM or seen * THINNER_THAN < firmest

    def order(row):
        seen = int(row.get("offered", row.get("seen", 0)))

        if count:
            return (-int(row["picks"]), -seen, row.get("name", ""))

        # A share that rests on far less than the rest of its table goes to
        # the bottom, however high it happens to be.
        return (1 if thinly(row) else 0,
                -float(row.get("pick_rate", 0.0)), -seen,
                row.get("name", ""))

    rows = sorted(rows, key=order)
    lines = []

    # Counted rather than shared: the bar is how many were taken against the
    # worst of them. At least one, because a curse can be turned down now and
    # a window in which every one of them was refused is all zeroes - which
    # divided by its own largest is what this used to do.
    most = max([row["picks"] for row in rows[:top]] + [1]) if count else 0

    # A row read out of the engine says offered; one read back out of the
    # file says seen. They are the same number.
    def offered(row):
        return int(row.get("offered", row.get("seen", 0)))

    for row in rows[:top]:
        share = (100.0 * row["picks"] / most if count
                 else 100.0 * row["pick_rate"])

        # Said out loud rather than left to be worked out from the count
        # column: this row's share is not worth reading yet.
        thin = not count and thinly(row)

        lines.append(
            ('<tr class="thin">' if thin else '<tr>') +
            '<td class="name">%s%s</td>'
            '<td class="bar"><span style="width:%.1f%%"></span></td>'
            '<td class="num">%s</td><td class="num">%d</td>'
            '<td class="num">%.1f</td><td class="num">%.0f%%</td></tr>'
            % (_art(row, kind), row["name"][:26], share,
               ("%d" % row["picks"]) if count
               else ("%.0f%%" % (100.0 * row["pick_rate"])),
               offered(row), row["avg_floors"],
               100.0 * row["win_rate"]))

    return (
        '<section><h2>%s <span class="count">%d different</span></h2>'
        '<div class="rows"><table class="picks"><thead><tr><th></th>'
        '<th>picked</th>'
        '<th>%%</th><th>seen</th><th>floors</th><th>win</th></tr></thead>'
        '<tbody>%s</tbody></table></div></section>'
        % (title, len(rows), "".join(lines)))


def write_html(folder, character, rows, seconds=15, picks=None,
               curses=None, climbs=0):
    """Writes ``progress.html`` into \\p folder: the curves and the numbers,
    with nothing else in it. The page reloads itself."""
    if not rows:
        return

    last = rows[-1]
    panels = "".join(_svg(rows, key, title, colour)
                     for key, title, colour in PANELS)
    chosen = "".join(
        _table(_spared((picks or {}).get(kind, []))
               if kind in ("card_taken", "card_bought")
               else (picks or {}).get(kind, []), title, top=top, kind=kind)
        for kind, title, top in PICK_TABLES)
    chosen += _table(curses if curses is not None else _curses(picks),
                     CURSE_TITLE, count=True, kind="curse_taken")

    if chosen:
        over = ("the last %s climbs" % "{:,}".format(int(climbs))
                if climbs > 0 else "the climbs counted so far")
        chosen = ('<h1 class="second">what it has been choosing</h1>'
                  '<p class="now">of everything offered over %s: how often '
                  'it was taken, and how those climbs went</p>'
                  '<div class="tables">' % over + chosen + '</div>')
    page = """<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta http-equiv="refresh" content="%d">
<title>%s - training</title>
<style>
:root { color-scheme: light dark; }
body { font: 14px/1.5 system-ui, sans-serif; margin: 24px;
       background: #14161a; color: #e8e8e8; }
h1 { font-size: 18px; margin: 0 0 4px; }
.now { color: #9aa0a6; margin-bottom: 18px; }
.grid { display: grid; gap: 18px;
        grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); }
figure { margin: 0; background: #1c1f24; border-radius: 8px; padding: 10px; }
figcaption { color: #9aa0a6; font-size: 12px; margin-bottom: 4px; }
figcaption b { color: #e8e8e8; font-size: 14px; }
svg { width: 100%%; height: auto; }
.axis { stroke: #3a3f46; stroke-width: 1; }
.tick { fill: #6f757c; font-size: 9px; }
table { border-collapse: collapse; margin-top: 20px; font-size: 13px; }
td { padding: 2px 14px 2px 0; }
td.key { color: #9aa0a6; }
h1.second { font-size: 16px; margin: 34px 0 4px; }
.tables { display: grid; gap: 18px;
          grid-template-columns: repeat(auto-fit, minmax(340px, 1fr)); }
section { background: #1c1f24; border-radius: 8px; padding: 10px 12px; }
section h2 { font-size: 13px; margin: 0 0 6px; font-weight: 600; }
section h2 .count { color: #6f757c; font-weight: 400; }
.rows { max-height: 430px; overflow-y: auto; }
table.picks { margin: 0; width: 100%%; font-size: 12px; }
table.picks thead th { position: sticky; top: 0; background: #1c1f24;
                       z-index: 1; }
table.picks th { color: #6f757c; font-weight: 400; text-align: right;
                 padding: 0 0 4px 8px; font-size: 11px; }
table.picks th:first-child, table.picks td.name { text-align: left; }
table.picks td { padding: 1px 0 1px 8px; }
table.picks td.name { color: #e8e8e8; padding-left: 0;
                     white-space: nowrap; }
.art { display: inline-block; width: 22px; height: 30px; margin-right: 7px;
       vertical-align: -9px; border-radius: 2px; object-fit: cover;
       background: #2a2d33; }
.art.drawn { font: 600 9px/30px "IBM Plex Mono", monospace; color: #fff;
             text-align: center; letter-spacing: 0.02em; }
table.picks td.num { text-align: right; color: #9aa0a6;
                     font-variant-numeric: tabular-nums; }
table.picks tr.thin td { opacity: 0.45; }
table.picks tr.thin td.num:nth-of-type(2)::after { content: " ?"; }
table.picks td.bar { width: 96px; }
table.picks td.bar span { display: block; height: 8px; border-radius: 4px;
                          background: #4c9f70; min-width: 1px; }
</style></head><body>
<h1>%s</h1>
<p class="now">update %d &middot; %d climbs &middot; %s moves &middot;
this page reloads every %d seconds</p>
<div class="grid">%s</div>
<table>
<tr><td class="key">floors a climb</td><td>%.2f</td></tr>
<tr><td class="key">fights won a climb</td><td>%.2f</td></tr>
<tr><td class="key">climbs with a boss down</td><td>%.1f%%</td></tr>
<tr><td class="key">climbs that finished the spire</td><td>%.1f%%</td></tr>
<tr><td class="key">reward a climb</td><td>%.1f</td></tr>
</table>
%s
</body></html>
""" % (seconds, character, character, int(last["updates"]),
       int(last["episodes"]), "{:,}".format(int(last["steps"])), seconds,
       panels, last["floors"], last["fights"], 100.0 * last["boss_rate"],
       100.0 * last["win_rate"], last["return"], chosen)

    path = os.path.join(folder, "progress.html")

    with open(path, "w", encoding="utf-8") as handle:
        handle.write(page)


# ----------------------------------------------------------------- a window
def window(folder, character, once=False, every=5.0):
    """Draws the curves in a window that keeps up with the training."""
    try:
        import matplotlib
        import matplotlib.pyplot as plt
    except ImportError:
        print("this needs matplotlib: pip install matplotlib")

        return 1

    if once:
        matplotlib.use("Agg")

    # Constrained rather than tight: nine columns of panels, several with a
    # card or a monster name down the side, is more than tight_layout will
    # fit - it gives up and warns about it on every redraw. This one works
    # the spacing out from the room it actually has.
    figure = plt.figure(figsize=(24, 7), layout="constrained")
    figure.canvas.manager.set_window_title("%s - training" % character)
    grid = figure.add_gridspec(2, 9)

    # Four curves across the top and four below, in the order of PANELS.
    flat = [figure.add_subplot(grid[0, column]) for column in range(4)]
    flat += [figure.add_subplot(grid[1, column]) for column in range(4)]
    bars = [figure.add_subplot(grid[:, 4]), figure.add_subplot(grid[:, 5]),
            figure.add_subplot(grid[0, 6]), figure.add_subplot(grid[1, 6]),
            figure.add_subplot(grid[0, 7:]), figure.add_subplot(grid[1, 7:])]

    def drawBars(panel, rows, title, colour, top=12, count=False,
                 label=None):
        panel.clear()
        panel.set_title(title, fontsize=10)

        if not rows:
            panel.text(0.5, 0.5, "nothing seen often enough yet",
                       ha="center", va="center", fontsize=8, color="grey")
            panel.set_xticks([])
            panel.set_yticks([])

            return

        kept = rows[:top][::-1]
        places = range(len(kept))

        # A curse is handed over rather than offered, so counting them says
        # more than a share of the times they turned up, which is always all
        # of them.
        widths = ([float(one["picks"]) for one in kept] if count
                  else [100.0 * one["pick_rate"] for one in kept])
        room = max(widths or [1.0]) * 1.35 or 1.0

        panel.barh(list(places), widths, color=colour)
        panel.set_yticks(list(places))
        panel.set_yticklabels(["%s" % one["name"][:22] for one in kept],
                              fontsize=8)
        panel.set_xlabel(label if label is not None else
                         ("taken, over the climbs counted" if count
                          else "picked, % of the times it turned up"),
                         fontsize=8)
        panel.set_xlim(0, room)
        panel.grid(axis="x", alpha=0.2)

        # The numbers go beside the bar rather than inside it, where a short
        # bar would cut them off.
        for place, one, width in zip(places, kept, widths):
            panel.text(width + room * 0.015, place,
                       ("%d taken  %.1f floors" % (one["picks"],
                                                   one["avg_floors"]))
                       if count else
                       ("%d seen  %.1f floors" % (one["seen"],
                                                  one["avg_floors"])),
                       va="center", fontsize=7, color="#666666")

    def draw():
        rows = read(folder)
        picks = readPicks(folder)

        for panel, (key, title, colour) in zip(flat, PANELS):
            panel.clear()
            panel.set_title(title, fontsize=10)
            panel.grid(alpha=0.2)

            if not rows:
                continue

            steps = [row["steps"] for row in rows]
            values = [row[key] for row in rows]

            panel.plot(steps, values, color=colour, alpha=0.3, linewidth=1)
            panel.plot(steps, smooth(values), color=colour, linewidth=2)
            panel.set_xlabel("moves", fontsize=8)

        drawBars(bars[0], _spared(picks.get("card_taken", [])),
                 "cards taken", "#4c9f70")
        drawBars(bars[1],
                 picks.get("relic_taken", []) + picks.get("potion_taken", []),
                 "relics and potions taken", "#c76b4a")
        drawBars(bars[2], picks.get("curse_taken", []),
                 "curses picked up (rooms, not rewards)", "#9a4a7a", top=7,
                 count=True)
        drawBars(bars[3], picks.get("node_walked", []),
                 "which place it walks onto", "#4a6fc7", top=7)
        drawBars(bars[4], picks.get("boss_fought", []),
                 "bosses, and how often it won", "#a03030", top=12,
                 label="won, % of the times it was fought")
        drawBars(bars[5], picks.get("elite_fought", []),
                 "elites, and how often it won", "#b07030", top=8,
                 label="won, % of the times it was fought")

        if rows:
            last = rows[-1]
            figure.suptitle(
                "%s   update %d   %d climbs   %s moves   "
                "floors %.2f   boss %.1f%%" %
                (character, last["updates"], last["episodes"],
                 "{:,}".format(int(last["steps"])), last["floors"],
                 100.0 * last["boss_rate"]),
                fontsize=11)

        return bool(rows)

    if once:
        draw()

        path = os.path.join(folder, "progress.png")

        try:
            figure.savefig(path, dpi=110)
        except OSError as trouble:
            # Windows will not let a picture be written over while something
            # else has it open.
            print("could not write %s: %s" % (path, trouble))
            print("close whatever is showing it and try again")

            return 1

        print("saved %s" % path)

        return 0

    plt.ion()
    plt.show(block=False)

    print("watching %s; close the window to stop" % folder)

    while plt.fignum_exists(figure.number):
        draw()
        figure.canvas.draw_idle()
        plt.pause(every)

    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Watch a training run.")

    parser.add_argument("character", nargs="?", default="ironclad")
    parser.add_argument("--out", default="runs")
    parser.add_argument("--once", action="store_true",
                        help="save one png instead of watching")
    parser.add_argument("--html", action="store_true",
                        help="write progress.html and stop")
    parser.add_argument("--every", type=float, default=5.0)

    args = parser.parse_args(argv)
    folder = os.path.join(args.out, args.character)

    if not os.path.isdir(folder):
        print("there is nothing in %s yet; start the training first" % folder)

        return 1

    if args.html:
        write_html(folder, args.character, read(folder),
                   picks=readPicks(folder))
        print("wrote %s" % os.path.join(folder, "progress.html"))

        return 0

    return window(folder, args.character, args.once, args.every)


if __name__ == "__main__":
    sys.exit(main())
