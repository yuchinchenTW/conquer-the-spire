// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Monsters/EncounterLibrary.hpp>

#include <utility>

namespace ConquerTheSpire
{
namespace
{
Encounter Group(const char* name, MonsterType type,
                std::vector<MonsterId> monsters, int weight = 1)
{
    Encounter encounter;
    encounter.name = name;
    encounter.type = type;
    encounter.monsters = std::move(monsters);
    encounter.weight = weight;

    return encounter;
}
}  // namespace

const std::vector<Encounter>& EncounterLibrary::GetAct1Weak()
{
    static const std::vector<Encounter> groups = {
        // Two apiece, so the four come up alike. The louses are a colour
        // each, drawn as the room is built rather than one of each every
        // time.
        Group("Cultist", MonsterType::NORMAL, { MonsterId::CULTIST }, 2),
        Group("Jaw Worm", MonsterType::NORMAL, { MonsterId::JAW_WORM }, 2),
        Group("2 Louses", MonsterType::NORMAL,
              { MonsterId::RANDOM_LOUSE, MonsterId::RANDOM_LOUSE }, 2),

        // A medium of one kind beside a small of the other, either way
        // round. It was a small spike beside a medium acid, always.
        Group("Small Slimes", MonsterType::NORMAL,
              { MonsterId::SPIKE_SLIME_M, MonsterId::ACID_SLIME_S }, 2)
    };

    return groups;
}

const std::vector<Encounter>& EncounterLibrary::GetAct1Strong()
{
    static const std::vector<Encounter> groups = {
        // The weights doubled, because two of the eleven are worth one and
        // a half against the others and there are no halves here. Two of the
        // rooms had been split in two - a large acid slime and a large spike
        // slime standing as separate rooms - which handed that one room twice
        // the share the spire gives it.
        Group("Gremlin Gang", MonsterType::NORMAL,
              { MonsterId::RANDOM_GREMLIN, MonsterId::RANDOM_GREMLIN,
                MonsterId::RANDOM_GREMLIN, MonsterId::RANDOM_GREMLIN }, 2),
        Group("Large Slime", MonsterType::NORMAL,
              { MonsterId::RANDOM_LARGE_SLIME }, 4),
        Group("Blue Slaver", MonsterType::NORMAL,
              { MonsterId::BLUE_SLAVER }, 4),
        Group("Red Slaver", MonsterType::NORMAL,
              { MonsterId::RED_SLAVER }, 2),
        Group("3 Louses", MonsterType::NORMAL,
              { MonsterId::RANDOM_LOUSE, MonsterId::RANDOM_LOUSE,
                MonsterId::RANDOM_LOUSE }, 4),
        Group("2 Fungi Beasts", MonsterType::NORMAL,
              { MonsterId::FUNGI_BEAST, MonsterId::FUNGI_BEAST }, 4),

        // A louse or a medium slime, and then a slaver, a cultist or a
        // looter. It was a looter beside a blue slaver, always.
        Group("Exordium Thugs", MonsterType::NORMAL,
              { MonsterId::RANDOM_LOUSE, MonsterId::LOOTER }, 3),

        // A fungi beast or a jaw worm, and then a louse or a medium slime.
        Group("Exordium Wildlife", MonsterType::NORMAL,
              { MonsterId::FUNGI_BEAST, MonsterId::RANDOM_LOUSE }, 3),
        Group("Looter", MonsterType::NORMAL, { MonsterId::LOOTER }, 4),

        // Three spike and two acid, which was the other way round.
        Group("Lots of Slimes", MonsterType::NORMAL,
              { MonsterId::SPIKE_SLIME_S, MonsterId::SPIKE_SLIME_S,
                MonsterId::SPIKE_SLIME_S, MonsterId::ACID_SLIME_S,
                MonsterId::ACID_SLIME_S }, 2)
    };

    return groups;
}

const std::vector<Encounter>& EncounterLibrary::GetAct1Elites()
{
    static const std::vector<Encounter> groups = {
        Group("Gremlin Nob", MonsterType::ELITE, { MonsterId::GREMLIN_NOB }),
        Group("Lagavulin", MonsterType::ELITE, { MonsterId::LAGAVULIN }),
        Group("3 Sentries", MonsterType::ELITE,
              { MonsterId::SENTRY, MonsterId::SENTRY, MonsterId::SENTRY })
    };

    return groups;
}

const std::vector<Encounter>& EncounterLibrary::GetAct1Bosses()
{
    static const std::vector<Encounter> groups = {
        Group("The Guardian", MonsterType::BOSS,
              { MonsterId::THE_GUARDIAN }),
        Group("Hexaghost", MonsterType::BOSS, { MonsterId::HEXAGHOST }),
        Group("Slime Boss", MonsterType::BOSS, { MonsterId::SLIME_BOSS })
    };

    return groups;
}

const std::vector<Encounter>& EncounterLibrary::GetAct2Weak()
{
    static const std::vector<Encounter> groups = {
        { "Spheric Guardian",
          MonsterType::NORMAL,
          { MonsterId::SPHERIC_GUARDIAN } },
        { "Chosen", MonsterType::NORMAL, { MonsterId::CHOSEN } },
        { "Shelled Parasite",
          MonsterType::NORMAL,
          { MonsterId::SHELLED_PARASITE } },
        { "3 Byrds",
          MonsterType::NORMAL,
          { MonsterId::BYRD, MonsterId::BYRD, MonsterId::BYRD } },
        // A looter and a mugger, not two muggers: the looter runs off with
        // what it has taken and the mugger stays, so which of the two is
        // which changes what the fight is about.
        { "2 Thieves",
          MonsterType::NORMAL,
          { MonsterId::LOOTER, MonsterId::MUGGER } }
    };

    return groups;
}

const std::vector<Encounter>& EncounterLibrary::GetAct2Strong()
{
    // The weights are published after all, for this act at least: a snake
    // plant or a centurion turns up three times as often as a chosen with a
    // byrd. Every group carrying the same one had the learner meeting the rare
    // rooms half again as often as the spire hands them out, and the common
    // ones half as often.
    //
    // Written as the ratios rather than as the percentages the page prints,
    // because the percentages are those ratios rounded: two, three, two, six,
    // four, six, three, three out of twenty-nine comes to seven, ten, seven,
    // twenty-one, fourteen, twenty-one, ten, ten.
    static const std::vector<Encounter> groups = {
        // The byrd acts first. The spire moves its monsters in the order
        // they stand in, and having the chosen go first let it make the
        // climber vulnerable and the byrd swoop into it on the same turn -
        // which the game itself went back and fixed.
        { "Chosen and Byrd",
          MonsterType::NORMAL,
          { MonsterId::BYRD, MonsterId::CHOSEN },
          2 },
        { "Sentry and Sphere",
          MonsterType::NORMAL,
          { MonsterId::SENTRY, MonsterId::SPHERIC_GUARDIAN },
          2 },
        { "Snake Plant", MonsterType::NORMAL, { MonsterId::SNAKE_PLANT },
          6 },
        { "Snecko", MonsterType::NORMAL, { MonsterId::SNECKO }, 4 },
        { "Centurion and Healer",
          MonsterType::NORMAL,
          { MonsterId::CENTURION, MonsterId::MYSTIC },
          6 },
        { "Cultist and Chosen",
          MonsterType::NORMAL,
          { MonsterId::CULTIST, MonsterId::CHOSEN },
          3 },
        { "3 Cultists",
          MonsterType::NORMAL,
          { MonsterId::CULTIST, MonsterId::CULTIST, MonsterId::CULTIST },
          3 },
        { "Shelled Parasite and Fungi",
          MonsterType::NORMAL,
          { MonsterId::SHELLED_PARASITE, MonsterId::FUNGI_BEAST },
          3 }
    };

    return groups;
}

const std::vector<Encounter>& EncounterLibrary::GetAct2Elites()
{
    static const std::vector<Encounter> groups = {
        // It does not stand alone: two gremlins are already there, whichever
        // two they happen to be.
        { "Gremlin Leader",
          MonsterType::ELITE,
          { MonsterId::GREMLIN_LEADER, MonsterId::RANDOM_GREMLIN,
            MonsterId::RANDOM_GREMLIN } },
        { "Slavers",
          MonsterType::ELITE,
          // Blue, taskmaster, red, left to right. Where they stand is not
          // written down on either wiki; this is the order the project's own
          // reading of the game gives, and the order matters because the
          // vector is the line they stand in - it decides which of them a
          // target index names and what the state reads out first.
          { MonsterId::BLUE_SLAVER, MonsterId::TASKMASTER,
            MonsterId::RED_SLAVER } },
        { "Book of Stabbing",
          MonsterType::ELITE,
          { MonsterId::BOOK_OF_STABBING } }
    };

    return groups;
}

const std::vector<Encounter>& EncounterLibrary::GetAct2Bosses()
{
    static const std::vector<Encounter> groups = {
        { "Bronze Automaton",
          MonsterType::BOSS,
          { MonsterId::BRONZE_AUTOMATON } },
        { "The Champ", MonsterType::BOSS, { MonsterId::THE_CHAMP } },
        { "The Collector",
          MonsterType::BOSS,
          { MonsterId::THE_COLLECTOR } }
    };

    return groups;
}

const std::vector<Encounter>& EncounterLibrary::GetAct3Weak()
{
    static const std::vector<Encounter> groups = {
        { "3 Darklings",
          MonsterType::NORMAL,
          { MonsterId::DARKLING, MonsterId::DARKLING, MonsterId::DARKLING } },
        { "Orb Walker", MonsterType::NORMAL, { MonsterId::ORB_WALKER } },
        // Three of the shapes, whichever three, and never three alike.
        { "3 Shapes",
          MonsterType::NORMAL,
          { MonsterId::RANDOM_SHAPE, MonsterId::RANDOM_SHAPE,
            MonsterId::RANDOM_SHAPE } }
    };

    return groups;
}

const std::vector<Encounter>& EncounterLibrary::GetAct3Strong()
{
    static const std::vector<Encounter> groups = {
        { "Spire Growth", MonsterType::NORMAL, { MonsterId::SPIRE_GROWTH } },
        { "Transient", MonsterType::NORMAL, { MonsterId::TRANSIENT } },
        { "4 Shapes",
          MonsterType::NORMAL,
          { MonsterId::RANDOM_SHAPE, MonsterId::RANDOM_SHAPE,
            MonsterId::RANDOM_SHAPE, MonsterId::RANDOM_SHAPE } },
        { "Maw", MonsterType::NORMAL, { MonsterId::THE_MAW } },
        { "Sphere and 2 Shapes",
          MonsterType::NORMAL,
          { MonsterId::SPHERIC_GUARDIAN, MonsterId::RANDOM_SHAPE,
            MonsterId::RANDOM_SHAPE } },
        // The third act's worms, which have already bellowed once when the
        // fight starts. Three of the first act's worms is a much softer room
        // than the one the spire puts here.
        { "Jaw Worm Horde",
          MonsterType::NORMAL,
          { MonsterId::JAW_WORM_HARD, MonsterId::JAW_WORM_HARD,
            MonsterId::JAW_WORM_HARD } },
        { "Writhing Mass",
          MonsterType::NORMAL,
          { MonsterId::WRITHING_MASS } },
        { "3 Darklings",
          MonsterType::NORMAL,
          { MonsterId::DARKLING, MonsterId::DARKLING, MonsterId::DARKLING } },
        { "Orb Walker", MonsterType::NORMAL, { MonsterId::ORB_WALKER } }
    };

    return groups;
}

const std::vector<Encounter>& EncounterLibrary::GetAct3Elites()
{
    static const std::vector<Encounter> groups = {
        { "Giant Head", MonsterType::ELITE, { MonsterId::GIANT_HEAD } },
        { "Nemesis", MonsterType::ELITE, { MonsterId::NEMESIS } },
        // It starts the fight with two daggers already beside it, and
        // spawns more on top of those. Standing alone, its opening summon was
        // the whole of the threat rather than the second wave of it.
        { "Reptomancer",
          MonsterType::ELITE,
          { MonsterId::REPTOMANCER, MonsterId::DAGGER, MonsterId::DAGGER } }
    };

    return groups;
}

const std::vector<Encounter>& EncounterLibrary::GetAct3Bosses()
{
    static const std::vector<Encounter> groups = {
        { "Awakened One",
          MonsterType::BOSS,
          { MonsterId::CULTIST, MonsterId::AWAKENED_ONE,
            MonsterId::CULTIST } },
        { "Time Eater", MonsterType::BOSS, { MonsterId::TIME_EATER } },
        // Deca stands on the left and so moves first, which is the user's
        // call: neither page says which of the two is where. The one hint
        // either of them gives is a patch note - "Swapped Donu & Deca's
        // positions so the attack order is left -> right" - which says the
        // order follows the standing and not which way round the standing
        // ended up.
        //
        // It matters: Donu's circle gives everything three of strength, so
        // Donu first would have Deca's beam land the harder on the very
        // first turn.
        { "Donu and Deca",
          MonsterType::BOSS,
          { MonsterId::DECA, MonsterId::DONU } }
    };

    return groups;
}

const std::vector<Encounter>& EncounterLibrary::GetAct4Elites()
{
    static const std::vector<Encounter> groups = { { "Shield and Spear",
                                                     MonsterType::ELITE,
                                                     { MonsterId::SPIRE_SHIELD,
                                                       MonsterId::
                                                           SPIRE_SPEAR } } };

    return groups;
}

const std::vector<Encounter>& EncounterLibrary::GetAct4Bosses()
{
    static const std::vector<Encounter> groups = {
        { "Corrupt Heart", MonsterType::BOSS, { MonsterId::CORRUPT_HEART } }
    };

    return groups;
}

int EncounterLibrary::WeakFightsOf(int act)
{
    return act <= 1 ? WEAK_FIGHTS : LATER_WEAK_FIGHTS;
}

Encounter EncounterLibrary::Pick(int act, MapNodeType node, int fightsSoFar,
                                 std::mt19937& rng,
                                 const std::vector<std::string>& lately)
{
    const std::vector<Encounter>* weak = &GetAct1Weak();
    const std::vector<Encounter>* strong = &GetAct1Strong();
    const std::vector<Encounter>* elites = &GetAct1Elites();
    const std::vector<Encounter>* bosses = &GetAct1Bosses();

    if (act == 2)
    {
        weak = &GetAct2Weak();
        strong = &GetAct2Strong();
        elites = &GetAct2Elites();
        bosses = &GetAct2Bosses();
    }
    else if (act == 3)
    {
        weak = &GetAct3Weak();
        strong = &GetAct3Strong();
        elites = &GetAct3Elites();
        bosses = &GetAct3Bosses();
    }
    else if (act >= 4)
    {
        // The last act holds nothing but the pair at the door and what is
        // behind it.
        weak = &GetAct4Elites();
        strong = &GetAct4Elites();
        elites = &GetAct4Elites();
        bosses = &GetAct4Bosses();
    }

    const std::vector<Encounter>* pool = strong;

    switch (node)
    {
        case MapNodeType::ELITE:
            pool = elites;
            break;

        case MapNodeType::BOSS:
            pool = bosses;
            break;

        case MapNodeType::MONSTER:
            pool = fightsSoFar < WeakFightsOf(act) ? weak : strong;
            break;

        default:
            // Nothing waits at the other places, so a fight there is a plain
            // one.
            pool = weak;
            break;
    }

    // The first fight out of the harder list cannot be certain rooms, going
    // by the last of the easy ones. Written as the page writes it:
    //
    //   3 Louses after 2 Louses
    //   Lots of Slimes or Large Slime after Small Slimes
    //   Exordium Thugs after Looter
    //   Red Slaver or Exordium Thugs after Blue Slaver
    //
    // The last two clauses cannot come up in the first act, because a looter
    // and a blue slaver are rooms of the harder list and so can never be the
    // easy fight before it. They are kept because the page keeps them, and
    // they cost nothing standing there.
    const bool firstHard =
        node == MapNodeType::MONSTER && fightsSoFar == WeakFightsOf(act);
    const std::string before = lately.empty() ? std::string() : lately.front();
    const auto barredAfter = [&before](const std::string& name) {
        if (before == "2 Louses")
        {
            return name == "3 Louses";
        }

        if (before == "Small Slimes")
        {
            return name == "Lots of Slimes" || name == "Large Slime";
        }

        if (before == "Looter")
        {
            return name == "Exordium Thugs";
        }

        if (before == "Blue Slaver")
        {
            return name == "Red Slaver" || name == "Exordium Thugs";
        }

        return false;
    };

    // The two just had are out of the draw, so that no three fights running
    // hold two alike. Elites and bosses come from pools of a few and the rule
    // is written about monster rooms, so it is only asked there.
    const bool ruled = node == MapNodeType::MONSTER;
    const auto barred = [&lately, ruled, firstHard,
                        &barredAfter](const std::string& name) {
        if (!ruled)
        {
            return false;
        }

        if (firstHard && barredAfter(name))
        {
            return true;
        }

        const std::size_t deep = lately.size() < 2u ? lately.size() : 2u;

        for (std::size_t at = 0; at < deep; ++at)
        {
            if (lately[at] == name)
            {
                return true;
            }
        }

        return false;
    };

    // Twice over, because a pool that has nothing left once the barred ones
    // are out - a short list, or a run of the same fight - must still hand
    // back a fight. The second pass bars nothing.
    for (int pass = 0; pass < 2; ++pass)
    {
        const bool barring = pass == 0;
        int total = 0;

        for (const auto& group : *pool)
        {
            if (barring && barred(group.name))
            {
                continue;
            }

            total += group.weight > 0 ? group.weight : 1;
        }

        if (total <= 0)
        {
            continue;
        }

        std::uniform_int_distribution<int> roll(1, total);
        int score = roll(rng);

        for (const auto& group : *pool)
        {
            if (barring && barred(group.name))
            {
                continue;
            }

            score -= group.weight > 0 ? group.weight : 1;

            if (score <= 0)
            {
                return group;
            }
        }
    }

    return pool->back();
}

std::vector<Monster> EncounterLibrary::Build(const Encounter& encounter,
                                             std::mt19937& rng)
{
    std::vector<Monster> monsters;
    monsters.reserve(encounter.monsters.size());

    // No more than two shapes alike in a room, which the draw for each of
    // them cannot know on its own. Counted as they are made, and a third of a
    // kind is drawn again until it is not a third of that kind.
    std::map<int, int> shapes;

    for (const MonsterId id : encounter.monsters)
    {
        if (id == MonsterId::RANDOM_SHAPE)
        {
            // Drawn from the kinds that are still allowed rather than drawn
            // over and over until an allowed one turns up. Drawing again is
            // only nearly a guarantee, and the rule is a rule.
            std::vector<MonsterId> open;

            for (const MonsterId kind : { MonsterId::REPULSOR,
                                          MonsterId::EXPLODER,
                                          MonsterId::SPIKER })
            {
                if (shapes[static_cast<int>(kind)] < 2)
                {
                    open.emplace_back(kind);
                }
            }

            std::uniform_int_distribution<std::size_t> pick(
                0, open.size() - 1);
            const MonsterId kind = open[pick(rng)];

            ++shapes[static_cast<int>(kind)];
            monsters.emplace_back(MonsterRoster::Make(kind, rng));
            monsters.back().SetMonsterType(encounter.type);
            continue;
        }

        monsters.emplace_back(MonsterRoster::Make(id, rng));

        // What the room is, not what the monster usually is. A Sentry is an
        // elite where three of them are the room and a plain monster where one
        // stands beside a Spheric Guardian; carrying elite about with it had a
        // preserved insect taking a quarter off it in a plain fight, a
        // slaver's collar paying out in one, and the whole room counted as an
        // elite by everything that asks.
        //
        // A minion called in later keeps whatever the roster gave it, which
        // for the ones that matter is plain.
        monsters.back().SetMonsterType(encounter.type);
    }

    // The rooms whose parts are drawn together rather than one at a time.
    // A small slimes room is a medium of one kind beside a small of the
    // other, either way round, which is one draw and not two; a gremlin gang
    // is four out of a bag holding two fats, two sneakies, two mads, one
    // shield and one wizard, so what has already been taken changes what is
    // left; and the two mixed rooms draw each place from its own list.
    if (encounter.name == "Small Slimes" && monsters.size() == 2u)
    {
        std::uniform_int_distribution<int> coin(0, 1);

        if (coin(rng) == 0)
        {
            monsters[0] = MonsterRoster::Make(MonsterId::ACID_SLIME_M, rng);
            monsters[1] = MonsterRoster::Make(MonsterId::SPIKE_SLIME_S, rng);
            monsters[0].SetMonsterType(encounter.type);
            monsters[1].SetMonsterType(encounter.type);
        }
    }

    if (encounter.name == "Gremlin Gang" && monsters.size() == 4u)
    {
        std::vector<MonsterId> bag = {
            MonsterId::FAT_GREMLIN,    MonsterId::FAT_GREMLIN,
            MonsterId::SNEAKY_GREMLIN, MonsterId::SNEAKY_GREMLIN,
            MonsterId::MAD_GREMLIN,    MonsterId::MAD_GREMLIN,
            MonsterId::SHIELD_GREMLIN, MonsterId::GREMLIN_WIZARD
        };

        for (std::size_t at = 0; at < monsters.size(); ++at)
        {
            std::uniform_int_distribution<std::size_t> pick(
                0, bag.size() - 1);
            const std::size_t which = pick(rng);

            monsters[at] = MonsterRoster::Make(bag[which], rng);
            monsters[at].SetMonsterType(encounter.type);
            bag.erase(bag.begin() + static_cast<std::ptrdiff_t>(which));
        }
    }

    if (encounter.name == "Exordium Thugs" && monsters.size() == 2u)
    {
        std::uniform_int_distribution<int> coin(0, 1);
        std::uniform_int_distribution<int> three(0, 2);
        const MonsterId second[] = { MonsterId::BLUE_SLAVER,
                                     MonsterId::RED_SLAVER,
                                     MonsterId::CULTIST,
                                     MonsterId::LOOTER };

        monsters[0] = MonsterRoster::Make(
            coin(rng) == 0 ? MonsterId::RANDOM_LOUSE
                           : MonsterId::RANDOM_MEDIUM_SLIME,
            rng);

        // A slaver of either colour counts as one of the three.
        const int which = three(rng);

        monsters[1] = MonsterRoster::Make(
            which == 0 ? second[coin(rng)] : second[which + 1], rng);

        monsters[0].SetMonsterType(encounter.type);
        monsters[1].SetMonsterType(encounter.type);
    }

    if (encounter.name == "Exordium Wildlife" && monsters.size() == 2u)
    {
        std::uniform_int_distribution<int> coin(0, 1);

        monsters[0] = MonsterRoster::Make(coin(rng) == 0
                                              ? MonsterId::FUNGI_BEAST
                                              : MonsterId::JAW_WORM,
                                          rng);
        monsters[1] = MonsterRoster::Make(
            coin(rng) == 0 ? MonsterId::RANDOM_LOUSE
                           : MonsterId::RANDOM_MEDIUM_SLIME,
            rng);

        monsters[0].SetMonsterType(encounter.type);
        monsters[1].SetMonsterType(encounter.type);
    }

    // The middle darkling of three cannot chomp. It is the same monster as
    // the two beside it in every other way, so it is that entry with the one
    // move taken out rather than a second kind of darkling.
    if (encounter.name == "3 Darklings" && monsters.size() == 3u)
    {
        monsters[1].DropMove("Chomp");
    }

    // In the elite fight the middle Sentry opens the other way round.
    if (encounter.name == "3 Sentries" && monsters.size() == 3u)
    {
        monsters[1].ForceMove("Beam");
    }

    return monsters;
}
}  // namespace ConquerTheSpire
