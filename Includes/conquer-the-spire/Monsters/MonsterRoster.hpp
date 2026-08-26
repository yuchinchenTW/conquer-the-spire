// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_MONSTER_ROSTER_HPP
#define CONQUER_THE_SPIRE_MONSTER_ROSTER_HPP

#include <conquer-the-spire/Models/Monster.hpp>

#include <random>
#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief MonsterRoster class.
//!
//! Builds the enemies of the spire. Health is rolled from the range the enemy
//! has, and a few of them roll a number for the whole fight - the damage a
//! Louse bites for - so building one needs the battle's generator.
//!
class MonsterRoster
{
 public:
    //! Builds the monster \p id, rolling its health from its own range.
    //! \p healthOverride sets the health outright, which is what a split
    //! needs to hand its leftovers to the two that step in.
    static Monster Make(MonsterId id, std::mt19937& rng,
                        int healthOverride = 0);

    //! Returns every id the roster knows.
    //! Returns what \p id is by nature, before any room says otherwise. A
    //! Sentry is an elite by nature and a plain monster in the room where one
    //! stands beside a Spheric Guardian, and telling which of a room's
    //! monsters is the one it is named for wants the nature rather than the
    //! standing.
    static MonsterType NatureOf(MonsterId id);

    static const std::vector<MonsterId>& GetAll();

    //! Returns every id of \p type.
    static std::vector<MonsterId> GetPool(MonsterType type);
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_MONSTER_ROSTER_HPP
