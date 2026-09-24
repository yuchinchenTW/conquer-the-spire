"""One climb as a page: every choice it made, floor by floor, in pictures.

cts_watch writes a climb down as lines of text. This writes the same climb
as a page - the cards it took and the cards it passed over side by side,
the relics, the potions, the rooms, act by act and floor by floor - so
that what the climber is drafting can be read at a glance rather than
spelled out.

The pictures are the ones Scripts/get_card_art.py fetches into
Assets/cards; a card with no picture is drawn as a tile in the colour of
its kind, the way the dashboard does it.

    python cts_story.py                          the best weights, until won
    python cts_story.py --seed 500007            that climb, won or not
    python cts_story.py --died                   the first climb that dies

It plays exactly as play.bat plays - the turn searched inside a fight, the
best two moves walked outside one - so the page is what the counted climbs
were doing.
"""

import argparse
import html
import os
import sys
import webbrowser

try:
    import torch
except ImportError:  # pragma: no cover
    print("this needs torch: pip install torch")
    raise

from cts_log import card_name, event_name, event_option_name
from cts_log import lines_of, potion_name, relic_name, summary_of
from cts_plot import ART, ART_FROM_PAGE, KIND_COLOURS, _slug
from cts_watch import climb, load

#! Where the page is written, beside the run it is about.
PAGE = "climb.html"

#! What a line of the log is about, and what to call it on the page. The
#! order is the order the columns of a floor appear in.
KINDS = {
    "card_taken": ("took", "", "good"),
    "card_bought": ("bought", "", "good"),
    "card_passed": ("passed over", "", "faded"),
    "card_removed": ("tore up", "", "gone"),
    "card_upgraded": ("sharpened", "", "sharp"),
    "card_transformed": ("turned", "", "sharp"),
    "relic_taken": ("took", "relic_", "good"),
    "relic_bought": ("bought", "relic_", "good"),
    "relic_passed": ("passed over", "relic_", "faded"),
    "potion_taken": ("took", "potion_", "good"),
    "potion_bought": ("bought", "potion_", "good"),
    "potion_passed": ("passed over", "potion_", "faded"),
    "potion_drunk": ("drank", "potion_", "used"),
    "potion_thrown": ("threw away", "potion_", "gone"),
}

#! What each kind of room is called where the page says where it walked.
ROOM_NAMES = ["nothing", "a fight", "an elite", "an event", "a fire",
              "a shop", "a chest", "the boss"]


def nameOf(entry, id_):
    """What the thing on this line is called."""
    if entry.startswith("relic"):
        return str(relic_name(id_))

    if entry.startswith("potion"):
        return str(potion_name(id_))

    return str(card_name(id_))


def kindOf(name, entry):
    """Which colour a thing with no picture is drawn in."""
    if entry.startswith("relic"):
        return "relic"

    if entry.startswith("potion"):
        return "potion"

    return "skill"


def picture(name, prefix, entry):
    """The tag for one thing: its picture, or a tile in its colour."""
    slug = prefix + _slug(name)
    here = os.path.join(os.path.dirname(os.path.dirname(
        os.path.abspath(__file__))), ART, slug + ".png")
    label = html.escape(name)

    if os.path.exists(here):
        return ('<img src="%s/%s.png" alt="%s" title="%s">'
                % (ART_FROM_PAGE, html.escape(slug), label, label))

    colour = KIND_COLOURS.get(kindOf(name, entry), "#555")

    return ('<span class="tile" style="background:%s" title="%s">%s</span>'
            % (colour, label, html.escape(name[:12])))


def floors(lines):
    """The log cut into floors, in order, each with what happened on it."""
    out = []
    at = None

    for line in lines:
        where = (line["act"], line["floor"])

        if at is None or at["where"] != where:
            at = {"where": where, "room": None, "things": [], "notes": []}
            out.append(at)

        entry = line["entry"]

        if entry == "floor_walked":
            at["room"] = (ROOM_NAMES[line["id"]]
                          if 0 <= line["id"] < len(ROOM_NAMES) else "?")
        elif entry in KINDS:
            what, prefix, mood = KINDS[entry]
            at["things"].append((what, mood, nameOf(entry, line["id"]),
                                 prefix, entry))
        elif entry == "room_entered":
            at["notes"].append("at %s" % event_name(line["id"]))
        elif entry == "room_answered":
            said = event_option_name(line["id"], line.get("stage", 0),
                                     line["extra"])
            at["notes"].append("chose %s" % (said or
                                             "option %d" % line["extra"]))
        elif entry == "rested":
            at["notes"].append("rested for %d" % line["extra"])
        elif entry == "fight_won":
            at["notes"].append("won the fight")
        elif entry == "gold_spent":
            at["notes"].append("spent %d gold" % line["extra"])
        elif entry == "died":
            at["notes"].append("died here")
        elif entry == "spire_done":
            at["notes"].append("out the top of the spire")
        elif entry == "act_started":
            at["notes"].append("act %d begins" % line["id"])

    return [one for one in out if one["things"] or one["notes"]
            or one["room"]]


STYLE = """
body { background: #14131a; color: #ddd9d0; margin: 0;
       font: 14px/1.5 "Segoe UI", system-ui, sans-serif; }
main { max-width: 1100px; margin: 0 auto; padding: 24px 20px 60px; }
h1 { font-size: 20px; font-weight: 600; margin: 0 0 4px; }
.sub { color: #8d8879; margin: 0 0 24px; }
.act { margin: 28px 0 10px; font-size: 15px; letter-spacing: .08em;
       text-transform: uppercase; color: #c8a15a;
       border-bottom: 1px solid #2c2a33; padding-bottom: 6px; }
.floor { display: grid; grid-template-columns: 92px 1fr; gap: 14px;
         padding: 9px 0; border-bottom: 1px solid #1e1d25; }
.at { color: #6f6a5e; font-variant-numeric: tabular-nums; padding-top: 3px; }
.at b { color: #a9a396; font-weight: 600; display: block; }
.what { display: flex; flex-wrap: wrap; gap: 14px; align-items: flex-start; }
.group { display: flex; flex-direction: column; gap: 3px; }
.group .label { font-size: 11px; letter-spacing: .06em;
                text-transform: uppercase; color: #6f6a5e; }
.row { display: flex; flex-wrap: wrap; gap: 4px; }
img, .tile { width: 46px; height: 46px; object-fit: cover; border-radius: 3px;
             display: block; }
.tile { font-size: 9px; line-height: 1.1; padding: 3px; color: #f2eee6;
        box-sizing: border-box; overflow: hidden; }
.good img, .good .tile { outline: 2px solid #6f9c5a; }
.faded img, .faded .tile { opacity: .3; filter: grayscale(.7); }
.gone img, .gone .tile { outline: 2px solid #9c5a5a; opacity: .55; }
.sharp img, .sharp .tile { outline: 2px solid #c8a15a; }
.used img, .used .tile { outline: 2px solid #5a7f9c; }
.notes { color: #8d8879; font-size: 13px; padding-top: 3px; }
.tally { margin: 30px 0 0; padding: 14px 16px; background: #1a1922;
         border-radius: 4px; display: flex; flex-wrap: wrap; gap: 26px; }
.tally div { font-variant-numeric: tabular-nums; }
.tally b { display: block; color: #c8a15a; font-size: 18px; }
.won { color: #8fbf6a; } .lost { color: #bf6a6a; }
"""


def page(lines, counts, title, subtitle):
    """The whole climb as one html document."""
    out = ["<!doctype html><meta charset=utf-8>",
           "<title>%s</title>" % html.escape(title),
           "<style>%s</style>" % STYLE, "<main>",
           "<h1>%s</h1>" % html.escape(title),
           '<p class="sub">%s</p>' % html.escape(subtitle)]
    act = None

    for floor in floors(lines):
        where = floor["where"]

        if where[0] != act:
            act = where[0]
            out.append('<div class="act">act %d</div>' % act)

        out.append('<div class="floor"><div class="at"><b>floor %d</b>%s'
                   '</div><div class="what">'
                   % (where[1], html.escape(floor["room"] or "")))

        # Things of a kind go together: everything taken, then everything
        # passed over, so the choice reads as a choice.
        seen = []

        for what, mood, name, prefix, entry in floor["things"]:
            if not seen or seen[-1][0] != (what, mood):
                seen.append(((what, mood), []))

            seen[-1][1].append(picture(name, prefix, entry))

        for (what, mood), tags in seen:
            out.append('<div class="group %s"><span class="label">%s</span>'
                       '<div class="row">%s</div></div>'
                       % (mood, html.escape(what), "".join(tags)))

        if floor["notes"]:
            out.append('<div class="notes">%s</div>'
                       % html.escape(" - ".join(floor["notes"])))

        out.append("</div></div>")

    won = counts["won_the_spire"]
    passed = len([1 for one in lines if one["entry"] == "card_passed"])
    out.append('<div class="tally">')

    for label, value in (("floors", counts["floors"]),
                         ("fights won", counts["fights_won"]),
                         ("elites", counts["elites_won"]),
                         ("bosses", counts["bosses_won"]),
                         ("cards taken", counts["cards_taken"]),
                         ("passed over", passed),
                         ("torn up", counts["cards_removed"]),
                         ("sharpened", counts["cards_upgraded"]),
                         ("relics", counts["relics_taken"]),
                         ("potions drunk", counts["potions_drunk"]),
                         ("gold earned", counts["gold_earned"])):
        out.append("<div><b>%d</b>%s</div>" % (value, html.escape(label)))

    out.append('<div class="%s"><b>%s</b>%s</div>'
               % ("won" if won else "lost", "won" if won else "died",
                  "the spire" if won else "on floor %d" % counts["floors"]))
    out.append("</div></main>")

    return "\n".join(out)


def main(argv):
    parser = argparse.ArgumentParser(
        description="Write one climb out as a page, in pictures.")
    parser.add_argument("climber", nargs="?", default="runs/ironclad")
    parser.add_argument("--seed", type=int, default=None,
                        help="one climb, won or not; the default hunts for "
                             "a win")
    parser.add_argument("--died", action="store_true",
                        help="hunt for a climb that dies instead")
    parser.add_argument("--tries", type=int, default=20)
    parser.add_argument("--out", default=None,
                        help="where the page goes; beside the weights by "
                             "default")
    parser.add_argument("--flat", action="store_true",
                        help="the policy's own moves, no turn searched")
    parser.add_argument("--width", type=int, default=4)
    parser.add_argument("--budget", type=int, default=120)
    parser.add_argument("--hp-weight", type=float, default=0.01,
                        dest="hp_weight")
    parser.add_argument("--quiet", action="store_true",
                        help="do not open the page when it is written")
    args = parser.parse_args(argv[1:])

    device = torch.device("cpu")
    torch.set_num_threads(4)
    net, kept, path = load(args.climber, device)
    hunting = args.seed is None
    seed = args.seed if args.seed is not None else 500005
    tries = args.tries if hunting else 1

    print("%s at update %d" % (path, kept["updates"]))

    for at in range(tries):
        vec = climb(net, kept, device, seed + at, turn=not args.flat,
                    hp=args.hp_weight, width=args.width,
                    budget=args.budget)
        counts = summary_of(vec.at(0))
        won = bool(counts["won_the_spire"])
        wanted = (not hunting) or (won != args.died)
        print("  seed %d: %d floors, %s%s"
              % (seed + at, counts["floors"], "won" if won else "died",
                 "" if wanted else " - looking for another"))

        if wanted:
            lines = lines_of(vec.at(0))
            where = args.out or os.path.join(
                os.path.dirname(path) if os.path.isfile(path) else path,
                PAGE)
            title = "%s, seed %d" % ("a climb that won" if won
                                     else "a climb that died", seed + at)
            subtitle = ("%s at update %d, the turn searched in fights - "
                        "%d floors, %d cards taken of %d offered"
                        % (os.path.basename(path), kept["updates"],
                           counts["floors"], counts["cards_taken"],
                           counts["cards_taken"] + len(
                               [1 for one in lines
                                if one["entry"] == "card_passed"])))

            with open(where, "w", encoding="utf-8") as handle:
                handle.write(page(lines, counts, title, subtitle))

            print("written to %s" % where)

            if not args.quiet:
                webbrowser.open("file:///" + os.path.abspath(where)
                                .replace("\\", "/"))

            return 0

    print("none of those %d were what was asked for" % tries)

    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
