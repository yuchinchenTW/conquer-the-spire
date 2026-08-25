"""A policy that knows what a card is, rather than which slot it sits in.

The first net this project had read the state as one long row of numbers and
answered with one weight for every move. That makes a card into a place: what
it learns about Bash in the third slot of the hand it has to learn again for
the seventh, and what it learns about Bash in hand says nothing about Bash on
a reward pile. With six hundred moves and a hundred cards, most of those
weights never see enough of anything.

This one is built the other way round. Everything a move can be about - a
card in hand, a card in the deck, a potion, a monster, a shelf, a place on the
map - is a token: an embedding shared everywhere that thing turns up, plus
the few numbers the state says about it here and now. A move's score is then
a dot product between the token it is about and a question asked of the whole
state:

    play a card at a monster  ->  <hand, question> + <hand, monster>
    take a card off a pile    ->  <the card offered, question>
    sharpen a card            ->  <deck slot, question>

So Bash is Bash wherever it stands, and what the learner works out about it in
one place it knows in the other. Moves that are not about anything in
particular - ending a turn, leaving a shop - keep a weight of their own.

    from cts_net import CardPolicy

    net = CardPolicy(env.layout, env.id_layout, action_table())

It answers the same way the flat one does - ``act`` and ``judge`` over a mask
of the fixed head - so the trainer does not care which of the two it holds.

Everything is laid out so that a forward pass is a handful of big operations
rather than one small one per family: the tokens are gathered in a single read
of the state, projected by a single pair of matrix multiplies, and every move
is scored at once.

Running this file on its own checks the wiring: that the same card scores the
same in two different slots, that another card scores differently, that which
card lies on a reward pile reaches the move that takes it, and that every move
of the head is accounted for exactly once.
"""

import math

import torch
import torch.nn as nn

# How many ids there could be. Cards, relics, potions, rooms and monsters all
# number from one in their own tables, so this is the widest of them with room
# to spare.
ID_VOCAB = 512

# How wide a token is, and how many kinds a thing on offer can be.
TOKEN = 64
ITEM_KINDS = 8


class Family(object):
    """One kind of thing a move can be about.

    ``count`` slots of it, each with ``width`` numbers in the state at ``at``
    every ``stride``, an id of its own at ``ids``, a kind at ``kinds``, and
    one id at ``shared`` that belongs to all of them at once.
    """

    def __init__(self, name, count, at=0, stride=0, width=0, ids=None,
                 kinds=None, shared=None):
        self.name = name
        self.count = count
        self.at = at
        self.stride = stride
        self.width = width
        self.ids = ids
        self.kinds = kinds
        self.shared = shared


def families(layout, ids):
    """The tokens to build, given where everything sits in the state."""
    return [
        Family("hand", 10, layout["hand"], layout["hand_stride"],
               layout["hand_stride"], ids=ids["hand"]),
        Family("deck", 40, layout["deck_cards"], layout["deck_stride"],
               layout["deck_stride"], ids=ids["deck"]),
        Family("potion", 5, layout["potions"], 1, 1, ids=ids["potions"]),
        Family("monster", 8, layout["monsters"], layout["monster_stride"],
               layout["monster_stride"], ids=ids["monsters"]),

        # A reward slot says what kind of pile it is and how much is on it;
        # each of the four things it holds out is a card of its own, with
        # what it asks and what it is worth beside its id.
        Family("reward", 6, layout["rewards"], layout["reward_stride"],
               layout["reward_stride"], ids=ids["reward_kinds"]),
        Family("offer", 24, layout["offers"], layout["offer_stride"],
               layout["offer_stride"], ids=ids["reward_options"],
               kinds=ids["reward_option_kinds"]),

        # The shelf: its cards carry the same figures, behind the price.
        Family("shopcard", 7, layout["shop_cards"],
               layout["shop_card_stride"], layout["shop_card_stride"],
               ids=ids["shop_cards"]),
        Family("shoprelic", 3, layout["shop"], 2, 2,
               ids=ids["shop_relics"]),
        Family("shoppotion", 3, layout["shop"] + 6, 2, 2,
               ids=ids["shop_potions"]),

        # A room option carries what taking it would do to the climber - the
        # seventeen signals the engine writes beside it - and which room is
        # asking.
        Family("option", 6, layout["event"] + 2, layout["event_stride"],
               layout["event_stride"], shared=ids["event"]),

        # A place on the map: whether a path leads there, and what waits.
        Family("column", 7, layout["map"], 8, 8),
    ]


# Which token each kind of move is about: (question, family, other end).
RULES = {
    "play_card": ("play", "hand", "monster"),
    "use_potion": ("potion", "potion", "monster"),
    "discard_potion": ("throw", "potion", None),
    "travel": ("travel", "column", None),
    "claim_reward": ("claim", "reward", "offer"),
    "skip_reward": ("skip", "reward", None),
    "choose_option": ("option", "option", "deck"),
    "buy_card": ("buycard", "shopcard", None),
    "buy_relic": ("buyrelic", "shoprelic", None),
    "buy_potion": ("buypotion", "shoppotion", None),
    "buy_removal": ("remove", "deck", None),
    "smith": ("smith", "deck", None),
    "toke": ("toke", "deck", None),
}

QUESTIONS = sorted({rule[0] for rule in RULES.values()})

# What the trunk is asked to see, beside choosing a move. Each is a number
# read off the state a moment later, so the target comes free - no labelling,
# no extra rollouts. They were picked as the things the fights actually turn
# on, which the measurements pointed at: damage taken, whether the fight is
# nearly over, and how deep the climb gets.
FORESIGHTS = [
    "hurt_this_turn",     # health about to be lost, as a share of the most
    "fight_over_soon",    # whether this fight ends within a few turns
    "climb_floors",       # how many more floors this climb manages
]


class CardPolicy(nn.Module):
    """The masked policy, with a value head beside it."""

    def __init__(self, layout, id_layout, table, width=512, embed=TOKEN):
        super().__init__()

        kinds, firsts, seconds = table

        self.actions = len(kinds)
        self.floats = layout["total"]
        self.idCount = id_layout["total"]
        self.token = embed
        self.families = families(layout, id_layout)
        self.tokenCount = sum(one.count for one in self.families)
        self.widest = max(one.width for one in self.families)

        self.readState()
        self.readHead(kinds, firsts, seconds)

        # One embedding for every id there is, shared by every family: a card
        # is the same card in hand, in the deck and on a pile.
        self.embed = nn.Embedding(ID_VOCAB, embed)
        self.kindEmbed = nn.Embedding(ITEM_KINDS, embed)

        # A slot still gets a little of its own: the first card in hand is not
        # quite the fifth, however much they share.
        self.place = nn.Parameter(torch.zeros(self.tokenCount, embed))

        self.project = nn.Sequential(
            nn.Linear(embed + self.widest, embed),
            nn.GELU(),
            nn.Linear(embed, embed),
        )

        # The whole state, with a summary of every family folded in so that
        # what the deck holds is not only knowable one slot at a time.
        self.trunk = nn.Sequential(
            nn.Linear(self.floats + embed * len(self.families), width),
            nn.GELU(),
            nn.Linear(width, width),
            nn.GELU(),
        )

        # Every question at once, then split apart.
        self.ask = nn.Linear(width, embed * len(QUESTIONS))

        # The moves that are not about anything in particular.
        self.singles = nn.Linear(width, self.actions)
        self.value = nn.Linear(width, 1)

        # What the trunk is asked to work out besides which move to make.
        # Nothing reads these at play - they are there to make the trunk
        # represent the things a fight turns on, because a head cannot
        # predict the damage coming without the trunk holding it somewhere.
        # The measurements said tactics were fine and the deck was the wall;
        # what this leans on is whether the trunk sees the fight clearly
        # enough for the deck decisions to be made on top of it.
        self.foresee = nn.Linear(width, len(FORESIGHTS))

        nn.init.orthogonal_(self.foresee.weight, gain=0.1)
        nn.init.zeros_(self.foresee.bias)

        # A quiet head to start with keeps the first climbs close to even.
        nn.init.orthogonal_(self.singles.weight, gain=0.01)
        nn.init.zeros_(self.singles.bias)
        nn.init.orthogonal_(self.ask.weight, gain=0.01)
        nn.init.zeros_(self.ask.bias)

    # ---------------------------------------------------------- the wiring
    def readState(self):
        """Works out, once, where every token reads itself from.

        One row a token: which id names it, which kind it is, which id it
        shares, and which numbers of the state sit beside it. Gathering by
        these is a single read rather than one for every family.
        """
        idAt = []
        kindAt = []
        sharedAt = []
        featAt = []
        featOn = []
        hasId = []
        hasKind = []
        hasShared = []

        for one in self.families:
            for slot in range(one.count):
                idAt.append(one.ids + slot if one.ids is not None else 0)
                kindAt.append(one.kinds + slot if one.kinds is not None else 0)
                sharedAt.append(one.shared if one.shared is not None else 0)
                hasId.append(1.0 if one.ids is not None else 0.0)
                hasKind.append(1.0 if one.kinds is not None else 0.0)
                hasShared.append(1.0 if one.shared is not None else 0.0)

                start = one.at + slot * one.stride

                featAt.append([start + i if i < one.width else 0
                               for i in range(self.widest)])
                featOn.append([1.0 if i < one.width else 0.0
                               for i in range(self.widest)])

        self.register_buffer("idAt", torch.tensor(idAt, dtype=torch.long))
        self.register_buffer("featAt", torch.tensor(featAt, dtype=torch.long))
        self.register_buffer("featOn", torch.tensor(featOn))
        self.register_buffer("hasId", torch.tensor(hasId).unsqueeze(1))

        # The few tokens that carry a kind, or an id belonging to all of
        # them at once. Adding those to every row instead would be another
        # pass over the whole grid for the sake of thirty of its rows.
        kinded = [at for at, one in enumerate(hasKind) if one > 0.0]
        shared = [at for at, one in enumerate(hasShared) if one > 0.0]

        self.register_buffer("kinded", torch.tensor(kinded, dtype=torch.long))
        self.register_buffer("shared", torch.tensor(shared, dtype=torch.long))
        self.register_buffer(
            "kindAt", torch.tensor([kindAt[at] for at in kinded],
                                   dtype=torch.long))
        self.register_buffer(
            "sharedAt", torch.tensor([sharedAt[at] for at in shared],
                                     dtype=torch.long))

        # Which tokens each family owns, for the summary fed to the trunk.
        self.spans = []
        first = 0

        for one in self.families:
            self.spans.append((first, first + one.count))
            first += one.count

    def readHead(self, kinds, firsts, seconds):
        """Works out, once, what every move of the fixed head is about.

        A kind of move is a grid: every slot of one family against every slot
        of another, or against nothing at all. Scoring it is then one small
        matrix multiply for the whole kind, and the grid is dealt back out
        into the fixed head by a list of places worked out here. Scoring it
        move by move instead means reading a token for each of six hundred
        moves, which at a few thousand climbs a batch is gigabytes of
        shuffling for nothing.
        """
        counts = {one.name: one.count for one in self.families}
        self.plans = []
        alone = torch.ones(self.actions)

        for kind, rule in RULES.items():
            question, family, target = rule
            where = [i for i, one in enumerate(kinds) if one == kind]

            if not where:
                continue

            # How wide the other end is, and where each move sits in the grid.
            if target is None:
                wide = 1
                marks = [0 for _ in where]
            elif target == "offer":
                # Four of the things a reward offers have an id of their own;
                # the rest are the reward slot and nothing more, which is the
                # last column.
                wide = 5
                marks = [seconds[i] if seconds[i] < 4 else 4 for i in where]
            else:
                # The first column is for having handed nothing over.
                wide = counts[target] + 1
                marks = [seconds[i] + 1 for i in where]

            spot = [firsts[i] * wide + marks[at]
                    for at, i in enumerate(where)]
            at = len(self.plans)

            self.plans.append({"question": QUESTIONS.index(question),
                               "family": family, "target": target,
                               "wide": wide, "at": at})
            self.register_buffer("where%d" % at,
                                 torch.tensor(where, dtype=torch.long))
            self.register_buffer("spot%d" % at,
                                 torch.tensor(spot, dtype=torch.long))

            alone[torch.tensor(where, dtype=torch.long)] = 0.0

        self.register_buffer("alone", alone)

    # --------------------------------------------------------- the forward
    def byFamily(self, tokens):
        """The tokens cut up per family, which is how they are scored."""
        return {one.name: tokens[:, low:high, :]
                for one, (low, high) in zip(self.families, self.spans)}

    def tokens(self, obs, ids):
        """Every token there is, as ``[batch, tokens, width]``."""
        named = self.place + self.embed(
            ids[:, self.idAt].clamp(0, ID_VOCAB - 1)) * self.hasId

        if self.kinded.numel() > 0:
            named = named.index_add(
                1, self.kinded,
                self.kindEmbed(ids[:, self.kindAt].clamp(0,
                                                         ITEM_KINDS - 1)))

        if self.shared.numel() > 0:
            named = named.index_add(
                1, self.shared,
                self.embed(ids[:, self.sharedAt].clamp(0, ID_VOCAB - 1)))

        beside = obs[:, self.featAt] * self.featOn

        return self.project(torch.cat([named, beside], dim=2))

    def forward(self, obs, ids):
        tokens = self.tokens(obs, ids)
        held = self.byFamily(tokens)

        # A pile carries what is lying on it. Turning a pile down is a move
        # about the pile, so without this the one thing it could not be
        # weighed against is the cards being turned down.
        held["reward"] = held["reward"] + held["offer"].view(
            tokens.shape[0], held["reward"].shape[1], -1,
            self.token).mean(dim=2)

        # The state, with what every family holds folded into it.
        summary = torch.cat([one.mean(dim=1) for one in held.values()], dim=1)
        hidden = self.trunk(torch.cat([obs, summary], dim=1))
        questions = self.ask(hidden).view(-1, len(QUESTIONS), self.token)
        scale = 1.0 / math.sqrt(self.token)

        # Moves that stand alone keep a weight of their own; the rest are
        # scored from the token they are about.
        logits = self.singles(hidden) * self.alone
        scored = torch.zeros_like(logits)

        for plan in self.plans:
            about = held[plan["family"]]
            asked = questions[:, plan["question"], :]

            # What the state is asking of this thing, whatever it is aimed at.
            score = torch.einsum("bcd,bd->bc", about, asked) * scale

            if plan["wide"] > 1:
                if plan["target"] == "offer":
                    # Every reward against the four things it offers.
                    other = held["offer"].view(about.shape[0],
                                               about.shape[1], 4, -1)
                    pair = torch.einsum("bcd,bcod->bco", about, other) * scale
                else:
                    pair = torch.einsum("bcd,btd->bct", about,
                                        held[plan["target"]]) * scale

                # A column for having aimed at nothing at all.
                blank = torch.zeros(pair.shape[0], pair.shape[1], 1,
                                    device=pair.device, dtype=pair.dtype)
                grid = torch.cat([pair, blank], dim=2) \
                    if plan["target"] == "offer" \
                    else torch.cat([blank, pair], dim=2)

                score = score.unsqueeze(2) + grid

            flat = score.reshape(score.shape[0], -1)
            spot = getattr(self, "spot%d" % plan["at"])
            where = getattr(self, "where%d" % plan["at"])

            scored = scored.index_copy(1, where, flat.index_select(1, spot))

        return (logits + scored, self.value(hidden).squeeze(-1),
                self.foresee(hidden))

    # -------------------------------------------------- the same two answers
    def act(self, obs, ids, mask):
        logits, value, _ = self.forward(obs, ids)
        logits = logits.masked_fill(mask == 0, -1e9)
        dist = torch.distributions.Categorical(logits=logits)
        action = dist.sample()

        return action, dist.log_prob(action), dist.entropy(), value

    def judge(self, obs, ids, mask, action):
        """Adds what the trunk foresees, which the loss holds it to."""
        logits, value, foresight = self.forward(obs, ids)
        logits = logits.masked_fill(mask == 0, -1e9)
        dist = torch.distributions.Categorical(logits=logits)

        return dist.log_prob(action), dist.entropy(), value, foresight


def _check():
    """Makes sure a card is scored by what it is, not by where it sits."""
    from cts_env import SpireEnv, action_table

    plan = SpireEnv()
    kinds, firsts, seconds = action_table()
    net = CardPolicy(plan.layout, plan.id_layout, (kinds, firsts, seconds))

    net.eval()
    torch.manual_seed(4)

    obs = torch.rand(1, plan.observation_size) * 0.1
    ids = torch.zeros(1, plan.id_layout["total"], dtype=torch.long)
    hand = plan.id_layout["hand"]
    stride = plan.layout["hand_stride"]
    at = plan.layout["hand"]

    # The same card in two slots of the hand, with the same numbers beside it.
    ids[0, hand + 0] = 20
    ids[0, hand + 5] = 20

    for slot in (0, 5):
        for i in range(stride):
            obs[0, at + slot * stride + i] = obs[0, at + i]

    def find(kind, slot, mark):
        for i, one in enumerate(kinds):
            if one == kind and firsts[i] == slot and seconds[i] == mark:
                return i

        raise AssertionError("no %s move for %d, %d" % (kind, slot, mark))

    with torch.no_grad():
        logits, _, _ = net(obs, ids)

    play = find("play_card", 0, 0)
    other = find("play_card", 5, 0)
    first = float(logits[0, play])
    second = float(logits[0, other])

    assert abs(first - second) < 1e-5, \
        "the same card scored %.6f in one slot and %.6f in another" \
        % (first, second)

    # A different card in that slot scores differently.
    ids[0, hand + 5] = 21

    with torch.no_grad():
        moved, _, _ = net(obs, ids)

    assert abs(float(moved[0, other]) - second) > 1e-6, \
        "the card in the slot made no odds"

    # And which card lies on a reward pile reaches the move that takes it.
    offers = plan.id_layout["reward_options"]
    claim = find("claim_reward", 0, 0)

    ids[0, offers] = 30

    with torch.no_grad():
        before, _, _ = net(obs, ids)

    ids[0, offers] = 31

    with torch.no_grad():
        after, _, _ = net(obs, ids)

    assert abs(float(before[0, claim]) - float(after[0, claim])) > 1e-6, \
        "which card is on the pile made no odds to taking it"

    # Every move of the head is either scored from a token or has a weight of
    # its own, and none is both.
    scored = sum(getattr(net, "where%d" % plan["at"]).numel()
                 for plan in net.plans)
    counted = int(net.alone.sum()) + scored

    assert counted == net.actions, \
        "%d moves of %d are unaccounted for" % (net.actions - counted,
                                                net.actions)

    print("a card is scored by what it is: %.4f in both slots, %.4f as "
          "another card" % (first, float(moved[0, other])))
    print("%d moves: %d scored from a token, %d standing alone"
          % (net.actions, scored, int(net.alone.sum())))
    print("%.2fM parameters"
          % (sum(one.numel() for one in net.parameters()) / 1e6))

    return 0


if __name__ == "__main__":
    import sys

    sys.exit(_check())
