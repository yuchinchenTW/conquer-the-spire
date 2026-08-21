// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Map/Map.hpp>

#include <algorithm>
#include <cstddef>

namespace ConquerTheSpire
{
const char* NameOf(MapNodeType type)
{
    // In the order of the enum, one for every value of it.
    static const char* names[] = { "empty",  "fight",    "elite",
                                   "room",   "campfire", "shop",
                                   "chest",  "boss" };

    static_assert(sizeof(names) / sizeof(names[0]) ==
                      static_cast<std::size_t>(MapNodeType::COUNT),
                  "a kind of place was added without a name");

    const auto at = static_cast<std::size_t>(type);

    return at < static_cast<std::size_t>(MapNodeType::COUNT) ? names[at]
                                                             : names[0];
}

Map::Map()
{
    m_nodes.resize(static_cast<std::size_t>(ROWS * COLUMNS));

    for (int row = 0; row < ROWS; ++row)
    {
        for (int column = 0; column < COLUMNS; ++column)
        {
            MapNode& node = GetNode(row, column);
            node.row = row;
            node.column = column;
        }
    }

    m_boss.type = MapNodeType::BOSS;
    m_boss.row = ROWS;
    m_boss.column = COLUMNS / 2;
    m_boss.exists = true;
}

int Map::GetRows() const
{
    return m_rows;
}

void Map::SetRows(int rows)
{
    m_rows = rows > 0 && rows <= ROWS ? rows : ROWS;
}

MapNode& Map::GetNode(int row, int column)
{
    return m_nodes[static_cast<std::size_t>(row * COLUMNS + column)];
}

const MapNode& Map::GetNode(int row, int column) const
{
    return m_nodes[static_cast<std::size_t>(row * COLUMNS + column)];
}

const MapNode& Map::GetBossNode() const
{
    return m_boss;
}

bool Map::IsInside(int row, int column)
{
    return row >= 0 && row < ROWS && column >= 0 && column < COLUMNS;
}

std::vector<int> Map::GetColumns(int row) const
{
    std::vector<int> columns;

    if (row < 0 || row >= ROWS)
    {
        return columns;
    }

    for (int column = 0; column < COLUMNS; ++column)
    {
        if (GetNode(row, column).Exists())
        {
            columns.emplace_back(column);
        }
    }

    return columns;
}

std::vector<int> Map::GetStartingColumns() const
{
    return GetColumns(0);
}

std::vector<int> Map::GetParents(int row, int column) const
{
    std::vector<int> parents;

    if (row <= 0 || row >= ROWS)
    {
        return parents;
    }

    for (int candidate = 0; candidate < COLUMNS; ++candidate)
    {
        const MapNode& node = GetNode(row - 1, candidate);

        if (node.Exists() && node.LeadsTo(column))
        {
            parents.emplace_back(candidate);
        }
    }

    return parents;
}

void Map::MarkNode(int row, int column)
{
    if (!IsInside(row, column))
    {
        return;
    }

    GetNode(row, column).exists = true;
}

void Map::Connect(int row, int fromColumn, int toColumn)
{
    if (!IsInside(row, fromColumn) || !IsInside(row + 1, toColumn))
    {
        return;
    }

    MapNode& node = GetNode(row, fromColumn);

    if (!node.LeadsTo(toColumn))
    {
        node.nextColumns.emplace_back(toColumn);
        std::sort(node.nextColumns.begin(), node.nextColumns.end());
    }
}

void Map::SetType(int row, int column, MapNodeType type)
{
    if (!IsInside(row, column))
    {
        return;
    }

    GetNode(row, column).type = type;
}

int Map::CountType(MapNodeType type) const
{
    int count = 0;

    for (const auto& node : m_nodes)
    {
        if (node.Exists() && node.type == type)
        {
            ++count;
        }
    }

    return count;
}
}  // namespace ConquerTheSpire
