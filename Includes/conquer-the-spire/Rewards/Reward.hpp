// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_REWARD_HPP
#define CONQUER_THE_SPIRE_REWARD_HPP

#include <conquer-the-spire/Enums/CardId.hpp>
#include <conquer-the-spire/Enums/PotionId.hpp>
#include <conquer-the-spire/Enums/RelicId.hpp>
#include <conquer-the-spire/Enums/RewardEnums.hpp>

#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief Reward struct.
//!
//! One line of the screen that comes up after a fight or out of a chest. A
//! reward that offers a choice carries the options; the run claims it by
//! naming which one it wants.
//!
struct Reward
{
    //! A pile of gold.
    static Reward Gold(int amount);

    //! A choice between \p cards, which may also be turned down.
    static Reward CardChoice(std::vector<CardId> cards);

    //! A choice between \p relics. A plain relic reward offers just the one.
    static Reward RelicChoice(std::vector<RelicId> relics);

    //! A potion for the belt.
    static Reward Potion(PotionId id);

    //! Maximum health, which is what turning a card down can give.
    static Reward MaxHealth(int amount);

    //! A curse, which a Cursed Key brings along with the chest.
    static Reward Curse(CardId id);

    RewardKind kind = RewardKind::INVALID;
    int amount = 0;
    std::vector<CardId> cards;
    std::vector<RelicId> relics;
    PotionId potion = PotionId::INVALID;
    bool claimed = false;

    //! Whether this was found in a chest rather than left by a fight, which
    //! is what the record of a climb tells apart.
    bool fromChest = false;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_REWARD_HPP
