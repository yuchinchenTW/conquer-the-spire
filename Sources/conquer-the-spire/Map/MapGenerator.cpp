// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Map/MapGenerator.hpp>

#include <algorithm>

namespace ConquerTheSpire
{
namespace
{
//! Returns a column of the row above that \p column may step to: straight on,
//! or one to either side.
int PickNextColumn(std::mt19937& rng, int column)
{
    std::vector<int> choices;

    for (int step = -1; step <= 1; ++step)
    {
        const int candidate = column + step;

        if (candidate >= 0 && candidate < Map::COLUMNS)
        {
            choices.emplace_back(candidate);
        }
    }

    std::uniform_int_distribution<std::size_t> pick(0, choices.size() - 1);

    return choices[pick(rng)];
}

//! Returns true when \p type may not follow itself along a path.
bool RunsOnce(MapNodeType type)
{
    return type == MapNodeType::REST || type == MapNodeType::ELITE ||
           type == MapNodeType::MERCHANT || type == MapNodeType::EVENT;
}

//! Returns true when \p type may sit at \p row, and not straight after one of
//! the same kind.
bool Allowed(const Map& map, int row, int column, MapNodeType type)
{
    if (type == MapNodeType::ELITE || type == MapNodeType::REST)
    {
        // Neither turns up in the first stretch of the act.
        if (row < Map::FIRST_HARD_ROW)
        {
            return false;
        }
    }

    if (type == MapNodeType::REST && row == Map::ROWS - 2)
    {
        // A rest site never sits right below the one before the boss.
        return false;
    }

    if (!RunsOnce(type))
    {
        return true;
    }

    for (const int parent : map.GetParents(row, column))
    {
        if (map.GetNode(row - 1, parent).type == type)
        {
            return false;
        }
    }

    return true;
}
}  // namespace

Map MapGenerator::Generate(std::mt19937& rng, int pathCount)
{
    Map map;

    CarvePaths(map, rng, pathCount);
    AssignTypes(map, rng);

    return map;
}

Map MapGenerator::GenerateFinalAct()
{
    Map map;

    // Three rooms in a line, and then whatever is behind them.
    const MapNodeType rooms[] = { MapNodeType::REST, MapNodeType::MERCHANT,
                                  MapNodeType::ELITE };
    const int column = Map::COLUMNS / 2;

    map.SetRows(3);

    for (int row = 0; row < 3; ++row)
    {
        map.MarkNode(row, column);
        map.SetType(row, column, rooms[row]);

        if (row > 0)
        {
            map.Connect(row - 1, column, column);
        }
    }

    return map;
}

void MapGenerator::CarvePaths(Map& map, std::mt19937& rng, int pathCount)
{
    std::uniform_int_distribution<int> startPick(0, Map::COLUMNS - 1);
    int firstStart = -1;

    for (int path = 0; path < pathCount; ++path)
    {
        int column = startPick(rng);

        if (path == 0)
        {
            firstStart = column;
        }
        else if (path == 1)
        {
            // The second path always opens somewhere else, so a run is never
            // down to a single way up.
            while (column == firstStart)
            {
                column = startPick(rng);
            }
        }

        map.MarkNode(0, column);

        for (int row = 0; row < Map::ROWS - 1; ++row)
        {
            const int next = PickNextColumn(rng, column);

            map.Connect(row, column, next);
            map.MarkNode(row + 1, next);

            column = next;
        }
    }

    // Everything in the last row leads to the boss.
    for (const int column : map.GetColumns(Map::ROWS - 1))
    {
        map.GetNode(Map::ROWS - 1, column)
            .nextColumns.emplace_back(map.GetBossNode().column);
    }
}

void MapGenerator::AssignTypes(Map& map, std::mt19937& rng)
{
    // What the middle of an act is made of, out of a hundred.
    const struct
    {
        MapNodeType type;
        int weight;
    } table[] = { { MapNodeType::MONSTER, 45 },
                  { MapNodeType::EVENT, 22 },
                  { MapNodeType::ELITE, 16 },
                  { MapNodeType::REST, 12 },
                  { MapNodeType::MERCHANT, 5 } };

    std::uniform_int_distribution<int> roll(1, 100);

    for (int row = 0; row < Map::ROWS; ++row)
    {
        for (const int column : map.GetColumns(row))
        {
            if (row == 0)
            {
                // An act always opens on a fight.
                map.SetType(row, column, MapNodeType::MONSTER);
                continue;
            }

            if (row == Map::TREASURE_ROW)
            {
                map.SetType(row, column, MapNodeType::TREASURE);
                continue;
            }

            if (row == Map::REST_ROW)
            {
                map.SetType(row, column, MapNodeType::REST);
                continue;
            }

            MapNodeType chosen = MapNodeType::MONSTER;

            // Roll until something is allowed here. A monster always is, so
            // this cannot run away.
            for (int attempt = 0; attempt < 20; ++attempt)
            {
                int score = roll(rng);
                MapNodeType candidate = MapNodeType::MONSTER;

                for (const auto& entry : table)
                {
                    score -= entry.weight;

                    if (score <= 0)
                    {
                        candidate = entry.type;
                        break;
                    }
                }

                if (Allowed(map, row, column, candidate))
                {
                    chosen = candidate;
                    break;
                }
            }

            map.SetType(row, column, chosen);
        }
    }
}
}  // namespace ConquerTheSpire
