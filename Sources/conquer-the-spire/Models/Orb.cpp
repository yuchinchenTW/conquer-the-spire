// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Models/Orb.hpp>

namespace ConquerTheSpire
{
Orb::Orb(OrbType wanted, int startingAmount)
    : type(wanted), amount(startingAmount)
{
    // Do nothing
}
}  // namespace ConquerTheSpire
