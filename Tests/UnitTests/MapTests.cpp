#include "doctest.h"

#include <conquer-the-spire/Map/MapGenerator.hpp>
#include <conquer-the-spire/Run/Run.hpp>

#include <algorithm>
#include <cstddef>
#include <random>
#include <vector>

using namespace ConquerTheSpire;

namespace
{
//! Lays out an act from \p seed.
Map MakeMap(unsigned int seed)
{
    std::mt19937 rng(seed);

    return MapGenerator::Generate(rng);
}

//! Returns true when every node of the act can be reached from the bottom row.
bool EveryNodeIsReachable(const Map& map)
{
    std::vector<int> reachable = map.GetStartingColumns();

    for (int row = 0; row < Map::ROWS; ++row)
    {
        const std::vector<int> here = map.GetColumns(row);

        // Every place a path runs through must be one of the places the row
        // below leads to.
        for (const int column : here)
        {
            if (std::find(reachable.begin(), reachable.end(), column) ==
                reachable.end())
            {
                return false;
            }
        }

        std::vector<int> next;

        for (const int column : here)
        {
            for (const int ahead : map.GetNode(row, column).nextColumns)
            {
                if (std::find(next.begin(), next.end(), ahead) == next.end())
                {
                    next.emplace_back(ahead);
                }
            }
        }

        reachable = next;
    }

    return true;
}
}  // namespace

TEST_CASE("An act is fifteen rows tall with a boss above it")
{
    const Map map = MakeMap(1);

    CHECK(Map::ROWS == 15);
    CHECK(Map::COLUMNS == 7);
    CHECK(map.GetBossNode().type == MapNodeType::BOSS);
    CHECK(map.GetBossNode().row == Map::ROWS);
}

TEST_CASE("The same seed lays out the same act")
{
    const Map first = MakeMap(42);
    const Map second = MakeMap(42);
    const Map other = MakeMap(43);

    bool same = true;
    bool differs = false;

    for (int row = 0; row < Map::ROWS; ++row)
    {
        for (int column = 0; column < Map::COLUMNS; ++column)
        {
            const MapNode& a = first.GetNode(row, column);
            const MapNode& b = second.GetNode(row, column);
            const MapNode& c = other.GetNode(row, column);

            if (a.exists != b.exists || a.type != b.type ||
                a.nextColumns != b.nextColumns)
            {
                same = false;
            }

            if (a.exists != c.exists || a.type != c.type)
            {
                differs = true;
            }
        }
    }

    CHECK(same == true);
    CHECK(differs == true);
}

TEST_CASE("Every row a path runs through has somewhere to go")
{
    for (unsigned int seed = 1; seed <= 40; ++seed)
    {
        const Map map = MakeMap(seed);

        for (int row = 0; row < Map::ROWS; ++row)
        {
            const std::vector<int> columns = map.GetColumns(row);

            REQUIRE(columns.empty() == false);

            for (const int column : columns)
            {
                const MapNode& node = map.GetNode(row, column);

                CHECK(node.nextColumns.empty() == false);

                if (row == Map::ROWS - 1)
                {
                    // The last row reaches the boss from anywhere.
                    continue;
                }

                for (const int ahead : node.nextColumns)
                {
                    // A step goes straight on or one column to either side.
                    CHECK(std::abs(ahead - column) <= 1);
                    CHECK(map.GetNode(row + 1, ahead).Exists() == true);
                }
            }
        }
    }
}

TEST_CASE("Every place on the act can be walked to")
{
    for (unsigned int seed = 1; seed <= 40; ++seed)
    {
        CHECK(EveryNodeIsReachable(MakeMap(seed)) == true);
    }
}

TEST_CASE("An act opens on more than one way up")
{
    for (unsigned int seed = 1; seed <= 40; ++seed)
    {
        const Map map = MakeMap(seed);

        CHECK(map.GetStartingColumns().size() >= 2u);
    }
}

TEST_CASE("The fixed rows hold what they always hold")
{
    for (unsigned int seed = 1; seed <= 40; ++seed)
    {
        const Map map = MakeMap(seed);

        for (const int column : map.GetColumns(0))
        {
            CHECK(map.GetNode(0, column).type == MapNodeType::MONSTER);
        }

        for (const int column : map.GetColumns(Map::TREASURE_ROW))
        {
            CHECK(map.GetNode(Map::TREASURE_ROW, column).type ==
                  MapNodeType::TREASURE);
        }

        for (const int column : map.GetColumns(Map::REST_ROW))
        {
            CHECK(map.GetNode(Map::REST_ROW, column).type ==
                  MapNodeType::REST);
        }
    }
}

TEST_CASE("Elites and rest sites keep out of the opening rows")
{
    for (unsigned int seed = 1; seed <= 40; ++seed)
    {
        const Map map = MakeMap(seed);

        for (int row = 0; row < Map::FIRST_HARD_ROW; ++row)
        {
            for (const int column : map.GetColumns(row))
            {
                const MapNodeType type = map.GetNode(row, column).type;

                CHECK(type != MapNodeType::ELITE);
                CHECK(type != MapNodeType::REST);
            }
        }

        // Nor does a rest site sit right below the one before the boss.
        for (const int column : map.GetColumns(Map::ROWS - 2))
        {
            CHECK(map.GetNode(Map::ROWS - 2, column).type !=
                  MapNodeType::REST);
        }
    }
}

TEST_CASE("A rest site, an elite or a merchant never follows its own kind")
{
    for (unsigned int seed = 1; seed <= 40; ++seed)
    {
        const Map map = MakeMap(seed);

        for (int row = 1; row < Map::ROWS; ++row)
        {
            for (const int column : map.GetColumns(row))
            {
                const MapNodeType type = map.GetNode(row, column).type;

                if (type != MapNodeType::REST && type != MapNodeType::ELITE &&
                    type != MapNodeType::MERCHANT)
                {
                    continue;
                }

                // The rows that are fixed are allowed to repeat.
                if (row == Map::REST_ROW || row == Map::TREASURE_ROW)
                {
                    continue;
                }

                for (const int parent : map.GetParents(row, column))
                {
                    CHECK(map.GetNode(row - 1, parent).type != type);
                }
            }
        }
    }
}

TEST_CASE("An act holds a mix of things to do")
{
    int elites = 0;
    int rests = 0;
    int events = 0;
    int monsters = 0;

    for (unsigned int seed = 1; seed <= 40; ++seed)
    {
        const Map map = MakeMap(seed);

        elites += map.CountType(MapNodeType::ELITE);
        rests += map.CountType(MapNodeType::REST);
        events += map.CountType(MapNodeType::EVENT);
        monsters += map.CountType(MapNodeType::MONSTER);

        CHECK(map.CountType(MapNodeType::TREASURE) > 0);
    }

    CHECK(elites > 0);
    CHECK(rests > 0);
    CHECK(events > 0);
    CHECK(monsters > 0);
}

TEST_CASE("The last row leads to the boss")
{
    const Map map = MakeMap(7);

    for (const int column : map.GetColumns(Map::REST_ROW))
    {
        CHECK(map.GetNode(Map::REST_ROW, column)
                  .LeadsTo(map.GetBossNode().column) == true);
    }
}

TEST_CASE("A question mark turns out to be one of the four things")
{
    std::map<MapNodeType, int> tally;

    for (unsigned int seed = 1; seed < 400; ++seed)
    {
        Run run(CardColor::RED, seed);

        // Walk until a question mark is stood on, and see what it became.
        while (!run.GetAvailableColumns().empty())
        {
            const int before = run.GetFloor();

            run.Travel(run.GetAvailableColumns().front());

            if (before + 1 != run.GetFloor())
            {
                break;
            }

            const MapNodeType here = run.GetCurrentNodeType();

            if (run.GetMap().GetNode(run.GetFloor() - 1, run.GetColumn())
                    .type == here &&
                (here == MapNodeType::MONSTER ||
                 here == MapNodeType::MERCHANT ||
                 here == MapNodeType::TREASURE ||
                 here == MapNodeType::EVENT))
            {
                ++tally[here];
            }
        }
    }

    // All four turn up over four hundred climbs.
    CHECK(tally[MapNodeType::MONSTER] > 0);
    CHECK(tally[MapNodeType::MERCHANT] > 0);
    CHECK(tally[MapNodeType::TREASURE] > 0);
    CHECK(tally[MapNodeType::EVENT] > 0);
}

TEST_CASE("What a question mark is not gets likelier, and what it is resets")
{
    Run run(CardColor::RED, 5);

    CHECK(run.GetUnknownOdds().monster == 10);
    CHECK(run.GetUnknownOdds().shop == 3);
    CHECK(run.GetUnknownOdds().treasure == 2);

    // A bracelet keeps the fights out, so their share climbs and climbs.
    Run quiet(CardColor::RED, 9);

    quiet.AddRelic(RelicId::JUZU_BRACELET);

    int marks = 0;

    while (!quiet.GetAvailableColumns().empty())
    {
        const MapNodeType was = quiet.GetCurrentNodeType();

        static_cast<void>(was);

        if (!quiet.Travel(quiet.GetAvailableColumns().front()))
        {
            break;
        }

        // Nothing a bracelet holder walks into is a plain fight out of a
        // question mark: the odds climb instead.
        if (quiet.GetUnknownOdds().monster > 10)
        {
            ++marks;
        }
    }

    CHECK(marks > 0);

    // And a new act puts them all back.
    quiet.BeginAct(2);

    CHECK(quiet.GetUnknownOdds().monster == 10);
    CHECK(quiet.GetUnknownOdds().shop == 3);
    CHECK(quiet.GetUnknownOdds().treasure == 2);
}
