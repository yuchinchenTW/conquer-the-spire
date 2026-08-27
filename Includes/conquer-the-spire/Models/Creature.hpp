// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_CREATURE_HPP
#define CONQUER_THE_SPIRE_CREATURE_HPP

#include <conquer-the-spire/Enums/BattleEnums.hpp>

#include <map>
#include <string>

namespace ConquerTheSpire
{
//!
//! \brief Creature class.
//!
//! Anything that can be hit: the player and every monster. It owns health,
//! block and the buffs/debuffs, and it knows how those modify damage.
//!
class Creature
{
 public:
    Creature() = default;
    Creature(std::string name, int maxHealth);

    virtual ~Creature() = default;

    Creature(const Creature& rhs) = default;
    Creature(Creature&& rhs) = default;
    Creature& operator=(const Creature& rhs) = default;
    Creature& operator=(Creature&& rhs) = default;

    //! Returns the display name.
    const std::string& GetName() const;

    //! Returns the current health.
    int GetHealth() const;

    //! Returns the health this creature started the battle with.
    int GetMaxHealth() const;

    //! Returns the block that is currently soaking damage.
    int GetBlock() const;

    //! Returns true if health has dropped to zero.
    bool IsDead() const;

    //! Returns the amount of \p type, or zero when it is not applied.
    int GetPower(PowerType type) const;

    //! Returns every power that is currently applied.
    const std::map<PowerType, int>& GetPowers() const;

    //! Adds \p amount stacks of \p type. A negative amount reduces the power
    //! and drops it entirely once it reaches zero.
    void AddPower(PowerType type, int amount);

    //! Removes \p type regardless of how many stacks are left.
    void RemovePower(PowerType type);

    //! Adds \p amount of block. Modifiers are not applied here; pass the value
    //! through CalculateBlockGain() first.
    void AddBlock(int amount);

    //! Drops all block. Happens at the start of the owner's turn.
    void ClearBlock();

    //! Applies \p amount of damage, block first. Returns the health lost.
    //! Takes \p amount, block first. \p soften comes off whatever is left
    //! after the block rather than off the blow itself, which is where a
    //! tungsten rod sits: it makes the health lost one less and does nothing
    //! to what the block soaked.
    int TakeDamage(int amount, int soften = 0);

    //! Loses \p amount of health, ignoring block (poison, self damage).
    void LoseHealth(int amount);

    //! Restores \p amount of health, capped at the maximum.
    void Heal(int amount);

    //! Raises the maximum health by \p amount and heals for the same, which is
    //! what Feed does.
    void IncreaseMaxHealth(int amount);

    //! Sets the health outright, which is how a run takes back what a battle
    //! left of it.
    void SetHealth(int amount);

    //! Sets the maximum health outright.
    void SetMaxHealth(int amount);

    //! Returns \p base after this creature's own attack modifiers.
    int CalculateDamageDealt(int base) const;

    //! Returns \p incoming after this creature's damage taken modifiers.
    int CalculateDamageTaken(int incoming) const;

    //! Returns \p base after this creature's block modifiers.
    int CalculateBlockGain(int base) const;

 protected:
    std::string m_name;
    int m_maxHealth = 0;
    int m_health = 0;
    int m_block = 0;
    std::map<PowerType, int> m_powers;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_CREATURE_HPP
