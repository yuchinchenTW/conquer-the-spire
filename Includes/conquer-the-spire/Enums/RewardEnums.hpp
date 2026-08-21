// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_REWARD_ENUMS_HPP
#define CONQUER_THE_SPIRE_REWARD_ENUMS_HPP

namespace ConquerTheSpire
{
//! What a reward hands over.
enum class RewardKind
{
    INVALID = 0,
    GOLD,           //!< A pile of gold, taken as it is.
    CARD_CHOICE,    //!< One of a few cards, or nothing.
    RELIC_CHOICE,   //!< One relic, or one of the three a boss offers.
    POTION,         //!< A potion for the belt.
    MAX_HEALTH,     //!< Maximum health, which is what a skipped card can give.
    CURSE           //!< A curse, which a Cursed Key brings along.
};

//! How big a chest is, which decides what it holds.
//! The three keys the door of the last act wants.
enum class KeyType
{
    INVALID = 0,
    RUBY,
    EMERALD,
    SAPPHIRE
};

enum class ChestSize
{
    INVALID = 0,
    SMALL,
    MEDIUM,
    LARGE
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_REWARD_ENUMS_HPP
