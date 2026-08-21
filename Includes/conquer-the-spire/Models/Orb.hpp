// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_ORB_HPP
#define CONQUER_THE_SPIRE_ORB_HPP

#include <conquer-the-spire/Enums/BattleEnums.hpp>

namespace ConquerTheSpire
{
//!
//! \brief Orb struct.
//!
//! One of the orbs the Defect keeps in orbit. Every orb does something small
//! each turn while it sits there - its passive - and something bigger when it
//! is evoked. Focus raises both, and a Dark orb carries the damage it has
//! built up in \p amount.
//!
struct Orb
{
    Orb() = default;
    explicit Orb(OrbType wanted, int startingAmount = 0);

    OrbType type = OrbType::INVALID;
    int amount = 0;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_ORB_HPP
