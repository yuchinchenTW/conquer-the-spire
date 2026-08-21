// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_MAP_NODE_HPP
#define CONQUER_THE_SPIRE_MAP_NODE_HPP

#include <conquer-the-spire/Enums/MapEnums.hpp>

#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief MapNode struct.
//!
//! One place on the map. A node knows which columns of the row above it leads
//! to, which is what the player may walk to next.
//!
struct MapNode
{
    //! Returns true when a path runs through here.
    bool Exists() const;

    //! Returns true when this node leads to \p column in the row above.
    bool LeadsTo(int column) const;

    MapNodeType type = MapNodeType::EMPTY;
    int row = 0;
    int column = 0;
    bool exists = false;
    std::vector<int> nextColumns;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_MAP_NODE_HPP
