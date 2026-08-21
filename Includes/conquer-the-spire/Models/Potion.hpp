// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_POTION_HPP
#define CONQUER_THE_SPIRE_POTION_HPP

#include <conquer-the-spire/Enums/PotionId.hpp>
#include <conquer-the-spire/Models/Card.hpp>

#include <string>
#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief Potion class.
//!
//! A one shot the player carries in a belt slot. What a potion does is the
//! same list of effects a card uses, so drinking one goes through the battle
//! the same way playing a card does.
//!
class Potion
{
 public:
    Potion() = default;
    Potion(PotionId id, std::string name, PotionRarity rarity,
           CardTarget target, std::vector<CardEffect> effects,
           PotionUse use = PotionUse::IN_BATTLE);

    //! Returns the id the registry builds this potion from.
    PotionId GetId() const;

    //! Returns the display name.
    const std::string& GetName() const;

    //! Returns how often this potion turns up.
    PotionRarity GetRarity() const;

    //! Returns what this potion needs to be drunk at.
    CardTarget GetTarget() const;

    //! Returns what this potion does.
    const std::vector<CardEffect>& GetEffects() const;

    //! Returns when this potion may be drunk.
    PotionUse GetUse() const;

    //! Returns true when this potion may be drunk in a fight, and when it may
    //! be drunk anywhere else. The one that waits for a climber to die may be
    //! drunk nowhere: it drinks itself.
    bool IsUsableInBattle() const;
    bool IsUsableOutside() const;

 private:
    PotionId m_id = PotionId::INVALID;
    std::string m_name;
    PotionRarity m_rarity = PotionRarity::INVALID;
    CardTarget m_target = CardTarget::SELF;
    std::vector<CardEffect> m_effects;
    PotionUse m_use = PotionUse::IN_BATTLE;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_POTION_HPP
