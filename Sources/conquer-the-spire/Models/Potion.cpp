// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Models/Potion.hpp>

#include <utility>

namespace ConquerTheSpire
{
Potion::Potion(PotionId id, std::string name, PotionRarity rarity,
               CardTarget target, std::vector<CardEffect> effects,
               PotionUse use)
    : m_id(id),
      m_name(std::move(name)),
      m_rarity(rarity),
      m_target(target),
      m_effects(std::move(effects)),
      m_use(use)
{
    // Do nothing
}

PotionId Potion::GetId() const
{
    return m_id;
}

const std::string& Potion::GetName() const
{
    return m_name;
}

PotionRarity Potion::GetRarity() const
{
    return m_rarity;
}

CardTarget Potion::GetTarget() const
{
    return m_target;
}

const std::vector<CardEffect>& Potion::GetEffects() const
{
    return m_effects;
}

PotionUse Potion::GetUse() const
{
    return m_use;
}

bool Potion::IsUsableInBattle() const
{
    return m_use != PotionUse::PASSIVE;
}

bool Potion::IsUsableOutside() const
{
    return m_use == PotionUse::ANYWHERE;
}
}  // namespace ConquerTheSpire
