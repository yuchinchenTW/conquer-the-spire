// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Map/MapNode.hpp>

#include <algorithm>

namespace ConquerTheSpire
{
bool MapNode::Exists() const
{
    return exists;
}

bool MapNode::LeadsTo(int wanted) const
{
    return std::find(nextColumns.begin(), nextColumns.end(), wanted) !=
           nextColumns.end();
}
}  // namespace ConquerTheSpire
