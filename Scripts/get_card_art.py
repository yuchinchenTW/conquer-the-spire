"""Fetches a picture of every card from the wiki, for the progress page.

The dashboard lists what the learner has been drafting, tearing up and
playing, and a column of names is slower to read than a column of cards. The
pictures are the game's own art, kept in Assets/cards and out of the
repository - this only puts them on the disk of whoever runs it.

    python Scripts/get_card_art.py            every card the engine knows
    python Scripts/get_card_art.py --force    fetch again, over the top

One request a card, a little apart, and nothing fetched twice: a card
already on the disk is left alone. The pictures are asked for at a hundred
pixels wide, which is what a page showing them at twenty-two needs and a
fraction of what the full art weighs.
"""

import argparse
import ctypes
import os
import re
import sys
import time
import urllib.request

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ART = os.path.join(HERE, "Assets", "cards")
# The pages the pictures are named on. The cards carry a prefix for the deck
# they belong to; a relic and a potion are filed under their own name.
PAGES = {
    "card": "https://slaythespire.wiki.gg/wiki/Cards_List",
    "relic": "https://slaythespire.wiki.gg/wiki/Relics",
    "potion": "https://slaythespire.wiki.gg/wiki/Potions",
}
SITE = "https://slaythespire.wiki.gg"
AGENT = "conquer-the-spire card art (local dashboard, one request a card)"

# How wide a picture is asked for. The page draws them at twenty-two across.
WIDE = 100

# Which prefix a card's picture is filed under, by the colour the engine
# gives it.
PREFIXES = {
    1: ("Red",),
    2: ("Green",),
    3: ("Blue",),
    4: ("Colorless",),
    5: ("Colorless", "Status"),
    6: ("Curse",),
}


def bare(name):
    """The wiki files a card under its name with the spaces taken out."""
    return re.sub(r"[^A-Za-z0-9]", "", name)


def slug(name):
    """What the page looks for: lower case, one word."""
    kept = [one.lower() if one.isalnum() else "_" for one in name]

    return "".join(kept).strip("_")


def listing(where):
    """Every picture a list page names, by its file name."""
    ask = urllib.request.Request(where, headers={"User-Agent": AGENT})

    with urllib.request.urlopen(ask, timeout=30) as answer:
        page = answer.read().decode("utf-8", "replace")

    return dict(re.findall(r'<img alt="([^"]+\.png)" src="([^"]+)"', page))


def cards():
    """Every card the engine knows, as (name, colour).

    Asked of the engine rather than kept in a list here, so that a card added
    to one is not missing from the other. A library too old to say which
    colour a card is - one loaded by a training run that started before the
    question could be asked - leaves the colour at nought, and every prefix
    is tried in turn instead.
    """
    sys.path.insert(0, os.path.join(HERE, "Python"))

    from cts_env import _api
    from cts_log import card_name

    lib = _api().lib
    ask = None

    try:
        ask = lib.cts_card_color
        ask.argtypes = [ctypes.c_int]
        ask.restype = ctypes.c_int
    except AttributeError:
        print("the library cannot say which deck a card is from; every "
              "prefix will be tried")

    out = []

    for id_ in range(1, 400):
        name = card_name(id_)

        if name:
            out.append((name, int(ask(id_)) if ask else 0))

    return out


def others():
    """Every relic and potion the engine knows, by kind and name."""
    sys.path.insert(0, os.path.join(HERE, "Python"))

    from cts_log import potion_name, relic_name

    out = []

    for id_ in range(1, 250):
        name = relic_name(id_)

        if name:
            out.append(("relic", name))

    for id_ in range(1, 80):
        name = potion_name(id_)

        if name:
            out.append(("potion", name))

    return out


# The order to try when the colour is unknown. Red first: the Ironclad is
# what this project trains, and a Strike is a Strike in three of the decks.
EVERY = ("Red", "Colorless", "Curse", "Status", "Green", "Blue")


def smaller(url):
    """The same picture, asked for at the width the page draws it."""
    if "/thumb/" in url:
        url = re.sub(r"/\d+px-", "/%dpx-" % WIDE, url)

    return SITE + url if url.startswith("/") else url


def wanted(name, colour, have):
    """The picture for one card, at the width the page wants."""
    for prefix in PREFIXES.get(colour, EVERY):
        key = "%s-%s.png" % (prefix, bare(name))

        if key not in have:
            continue

        # A thumbnail url carries its width; asking for another is a matter
        # of writing it in. A file with no thumbnail is taken whole.
        return smaller(have[key])

    return ""


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Fetch a picture of every card for the progress page.")
    parser.add_argument("--force", action="store_true",
                        help="fetch again over the pictures already there")
    parser.add_argument("--wait", type=float, default=0.35,
                        help="seconds between requests")
    args = parser.parse_args(argv)

    if not os.path.isdir(ART):
        os.makedirs(ART)

    named = {}

    for kind, where in PAGES.items():
        print("reading %s" % where)
        named[kind] = listing(where)
        print("  %d pictures named there" % len(named[kind]))

    # A card, then everything else the dashboard lists beside them.
    known = [("card", name, colour) for name, colour in cards()]
    known += [(kind, name, 0) for kind, name in others()]

    got = 0
    kept = 0
    lost = []

    for kind, name, colour in known:
        path = os.path.join(ART, kind + "_" + slug(name) + ".png"
                            if kind != "card" else slug(name) + ".png")

        if os.path.exists(path) and not args.force:
            kept += 1
            continue

        if kind == "card":
            url = wanted(name, colour, named["card"])
        else:
            key = bare(name) + ".png"
            url = smaller(named[kind][key]) if key in named[kind] else ""

        if not url:
            lost.append(name)
            continue

        ask = urllib.request.Request(url, headers={"User-Agent": AGENT})

        try:
            with urllib.request.urlopen(ask, timeout=30) as answer:
                picture = answer.read()
        except OSError as trouble:
            print("  %-24s %s" % (name, trouble))
            lost.append(name)
            continue

        with open(path, "wb") as handle:
            handle.write(picture)

        got += 1

        if got % 25 == 0:
            print("  %d fetched" % got)

        time.sleep(args.wait)

    print("")
    print("%d cards, relics and potions: %d fetched, %d already there, "
          "%d without a picture" % (len(known), got, kept, len(lost)))

    if lost:
        print("  no picture for: %s" % ", ".join(lost))

    print("kept in %s, which the repository ignores" % ART)

    return 0


if __name__ == "__main__":
    sys.exit(main())
