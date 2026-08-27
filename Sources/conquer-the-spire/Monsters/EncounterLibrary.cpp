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
                std::vector<MonsterId> monsters)
{
    Encounter encounter;
    encounter.name = name;
    encounter.type = type;
    encounter.monsters = std::move(monsters);

    return encounter;
}
}  // namespace

const std::vector<Encounter>& EncounterLibrary::GetAct1Weak()
{
    static const std::vector<Encounter> groups = {
        Group("Cultist", MonsterType::NORMAL, { MonsterId::CULTIST }),
        Group("Jaw Worm", MonsterType::NORMAL, { MonsterId::JAW_WORM }),
        Group("2 Louses", MonsterType::NORMAL,
              { MonsterId::RED_LOUSE, MonsterId::GREEN_LOUSE }),
        Group("Small Slimes", MonsterType::NORMAL,
              { MonsterId::SPIKE_SLIME_S, MonsterId::ACID_SLIME_M })
    };

    return groups;
}

const std::vector<Encounter>& EncounterLibrary::GetAct1Strong()
{
    static const std::vector<Encounter> groups = {
        Group("Gremlin Gang", MonsterType::NORMAL,
              { MonsterId::MAD_GREMLIN, MonsterId::SNEAKY_GREMLIN,
                MonsterId::FAT_GREMLIN, MonsterId::SHIELD_GREMLIN,
                MonsterId::GREMLIN_WIZARD }),
        Group("Large Acid Slime", MonsterType::NORMAL,
              { MonsterId::ACID_SLIME_L }),
        Group("Large Spike Slime", MonsterType::NORMAL,
              { MonsterId::SPIKE_SLIME_L }),
        Group("Blue Slaver", MonsterType::NORMAL,
              { MonsterId::BLUE_SLAVER }),
        Group("Red Slaver", MonsterType::NORMAL, { MonsterId::RED_SLAVER }),
        Group("3 Louses", MonsterType::NORMAL,
              { MonsterId::RED_LOUSE, MonsterId::GREEN_LOUSE,
                MonsterId::RED_LOUSE }),
        Group("2 Fungi Beasts", MonsterType::NORMAL,
              { MonsterId::FUNGI_BEAST, MonsterId::FUNGI_BEAST }),
        Group("Exordium Thugs", MonsterType::NORMAL,
              { MonsterId::LOOTER, MonsterId::BLUE_SLAVER }),
        Group("Exordium Wildlife", MonsterType::NORMAL,
              { MonsterId::FUNGI_BEAST, MonsterId::JAW_WORM }),
        Group("Looter", MonsterType::NORMAL, { MonsterId::LOOTER }),
        Group("Lots of Slimes", MonsterType::NORMAL,
              { MonsterId::SPIKE_SLIME_S, MonsterId::SPIKE_SLIME_S,
                MonsterId::ACID_SLIME_S, MonsterId::ACID_SLIME_S,
                MonsterId::ACID_SLIME_S })
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

    // The two just had are out of the draw, so that no three fights running
    // hold two alike. Elites and bosses come from pools of a few and the rule
    // is written about monster rooms, so it is only asked there.
    const bool ruled = node == MapNodeType::MONSTER;
    const auto barred = [&lately, ruled](const std::string& name) {
        if (!ruled)
        {
            return false;
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
