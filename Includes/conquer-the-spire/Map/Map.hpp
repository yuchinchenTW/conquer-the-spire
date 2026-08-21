// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_MAP_HPP
#define CONQUER_THE_SPIRE_MAP_HPP

#include <conquer-the-spire/Map/MapNode.hpp>

#include <random>
#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief Map class.
//!
//! One act of the spire: fifteen rows of seven columns with a boss on top.
//! Only the places a path runs through exist, and every node says which
//! columns of the next row it leads to.
//!
class Map
{
 public:
    //! How many rows of nodes an act holds, not counting the boss.
    static constexpr int ROWS = 15;

    //! How wide an act is.
    static constexpr int COLUMNS = 7;

    //! The row that always holds the treasure.
    static constexpr int TREASURE_ROW = 8;

    //! The row that always holds a rest site, the one before the boss.
    static constexpr int REST_ROW = ROWS - 1;

    //! The first row an elite or a rest site may turn up on.
    static constexpr int FIRST_HARD_ROW = 5;

    Map();

    //! How many rows this act actually uses. The last act is only a few
    //! rooms deep, while the others use the whole grid.
    int GetRows() const;
    void SetRows(int rows);

    MapNode& GetNode(int row, int column);
    const MapNode& GetNode(int row, int column) const;

    //! Returns the boss waiting above the last row.
    const MapNode& GetBossNode() const;

    //! Returns true when \p row and \p column are on the map at all.
    static bool IsInside(int row, int column);

    //! Returns the columns of \p row a path runs through.
    std::vector<int> GetColumns(int row) const;

    //! Returns the columns of row zero a run may start from.
    std::vector<int> GetStartingColumns() const;

    //! Returns the columns of \p row that lead to \p column of the row above.
    std::vector<int> GetParents(int row, int column) const;

    //! Notes that a path runs through \p row and \p column.
    void MarkNode(int row, int column);

    //! Notes that \p fromColumn of \p row leads to \p toColumn of the row
    //! above it.
    void Connect(int row, int fromColumn, int toColumn);

    //! Sets what waits at \p row and \p column.
    void SetType(int row, int column, MapNodeType type);

    //! Returns how many nodes of \p type the whole act holds.
    int CountType(MapNodeType type) const;

 private:
    int m_rows = ROWS;

    std::vector<MapNode> m_nodes;
    MapNode m_boss;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_MAP_HPP
