// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_MONSTER_LIBRARY_HPP
#define CONQUER_THE_SPIRE_MONSTER_LIBRARY_HPP

#include <conquer-the-spire/Models/Monster.hpp>

namespace ConquerTheSpire::Monsters
{
//! 42 health. Cycles Chomp, Thrash and Bellow.
Monster JawWorm();

//! 50 health. Buffs itself once with Incantation, then attacks every turn.
Monster Cultist();

//! 11 health. Bites every turn.
Monster RedLouse();

//! 8 health. Alternates a small attack and a Weak debuff.
Monster AcidSlimeS();

//! A monster that never acts, for testing damage and block on its own.
Monster TrainingDummy(int health);
}  // namespace ConquerTheSpire::Monsters

#endif  // CONQUER_THE_SPIRE_MONSTER_LIBRARY_HPP
