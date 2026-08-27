// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Models/Creature.hpp>

#include <utility>

namespace ConquerTheSpire
{
Creature::Creature(std::string name, int maxHealth)
    : m_name(std::move(name)), m_maxHealth(maxHealth), m_health(maxHealth)
{
    // Do nothing
}

const std::string& Creature::GetName() const
{
    return m_name;
}

int Creature::GetHealth() const
{
    return m_health;
}

int Creature::GetMaxHealth() const
{
    return m_maxHealth;
}

int Creature::GetBlock() const
{
    return m_block;
}

bool Creature::IsDead() const
{
    return m_health <= 0;
}

int Creature::GetPower(PowerType type) const
{
    const auto iter = m_powers.find(type);
    return iter == m_powers.end() ? 0 : iter->second;
}

const std::map<PowerType, int>& Creature::GetPowers() const
{
    return m_powers;
}

void Creature::AddPower(PowerType type, int amount)
{
    if (amount == 0)
    {
        return;
    }

    // Strength and Dexterity can be pushed below zero, which is what Disarm,
    // Piercing Wail and Wraith Form rely on. Everything else stops at zero and
    // is dropped.
    const bool signedPower = type == PowerType::STRENGTH ||
                             type == PowerType::DEXTERITY ||
                             type == PowerType::FOCUS;

    int stacks = GetPower(type) + amount;

    if (!signedPower && stacks < 0)
    {
        stacks = 0;
    }

    if (stacks == 0)
    {
        RemovePower(type);
    }
    else
    {
        m_powers[type] = stacks;
    }
}

void Creature::RemovePower(PowerType type)
{
    m_powers.erase(type);
}

void Creature::AddBlock(int amount)
{
    if (amount > 0)
    {
        m_block += amount;
    }
}

void Creature::ClearBlock()
{
    m_block = 0;
}

int Creature::TakeDamage(int amount, int soften)
{
    if (amount <= 0)
    {
        return 0;
    }

    if (m_block >= amount)
    {
        m_block -= amount;
        return 0;
    }

    int healthLost = amount - m_block;
    m_block = 0;

    // Softened after the block and not before it: a rod takes a point off
    // what actually reaches the climber, and a blow that the block swallowed
    // whole was never going to reach them at all.
    if (soften > 0)
    {
        healthLost = healthLost > soften ? healthLost - soften : 0;
    }

    m_health -= healthLost;

    return healthLost;
}

void Creature::LoseHealth(int amount)
{
    if (amount > 0)
    {
        m_health -= amount;
    }
}

void Creature::Heal(int amount)
{
    if (amount <= 0)
    {
        return;
    }

    m_health += amount;

    if (m_health > m_maxHealth)
    {
        m_health = m_maxHealth;
    }
}

void Creature::IncreaseMaxHealth(int amount)
{
    if (amount <= 0)
    {
        return;
    }

    m_maxHealth += amount;
    m_health += amount;
}

void Creature::SetHealth(int amount)
{
    m_health = amount > m_maxHealth ? m_maxHealth : amount;
}

void Creature::SetMaxHealth(int amount)
{
    if (amount > 0)
    {
        m_maxHealth = amount;
    }
}

int Creature::CalculateDamageDealt(int base) const
{
    int damage = base + GetPower(PowerType::STRENGTH);

    if (damage <= 0)
    {
        return 0;
    }

    // Weak rounds down, so plain integer division matches the game.
    if (GetPower(PowerType::WEAK) > 0)
    {
        damage = damage * 3 / 4;
    }

    return damage;
}

int Creature::CalculateDamageTaken(int incoming) const
{
    if (incoming <= 0)
    {
        return 0;
    }

    if (GetPower(PowerType::VULNERABLE) > 0)
    {
        return incoming + incoming / 2;
    }

    return incoming;
}

int Creature::CalculateBlockGain(int base) const
{
    int block = base + GetPower(PowerType::DEXTERITY);

    if (block <= 0)
    {
        return 0;
    }

    if (GetPower(PowerType::FRAIL) > 0)
    {
        block = block * 3 / 4;
    }

    return block;
}
}  // namespace ConquerTheSpire
