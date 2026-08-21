// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_RELIC_HPP
#define CONQUER_THE_SPIRE_RELIC_HPP

#include <conquer-the-spire/Enums/RelicEnums.hpp>
#include <conquer-the-spire/Enums/RelicId.hpp>
#include <conquer-the-spire/Models/Card.hpp>

#include <string>
#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief RelicTrigger struct.
//!
//! One thing a relic does, and when. The effects are the same list a card
//! uses, so a relic that gains block or applies a power needs no code of its
//! own.
//!
struct RelicTrigger
{
    //! Fires whenever \p hook happens.
    static RelicTrigger On(RelicHook hook, std::vector<CardEffect> effects);

    //! Fires on every \p every occurrence of \p hook, which is how Kunai
    //! counts attacks and Happy Flower counts turns.
    static RelicTrigger Every(RelicHook hook, int every,
                              std::vector<CardEffect> effects);

    //! Fires on every \p every occurrence of \p hook within one turn: the
    //! count goes back to zero when the turn does, which is what Kunai and
    //! Letter Opener need.
    static RelicTrigger EveryInTurn(RelicHook hook, int every,
                                    std::vector<CardEffect> effects);

    //! Fires only at the start of turn \p turn, which is what Horn Cleat
    //! waits for.
    static RelicTrigger OnTurn(int turn, std::vector<CardEffect> effects);

    //! Fires only at the end of turn \p turn, which is when Stone Calendar
    //! goes off.
    static RelicTrigger OnTurnEnd(int turn, std::vector<CardEffect> effects);

    RelicHook hook = RelicHook::NONE;
    int every = 1;
    int onTurn = 0;
    bool perTurn = false;
    std::vector<CardEffect> effects;
};

//!
//! \brief Relic class.
//!
//! Something the player carries for the whole run. A relic either reacts to
//! one of the hooks, or changes a number the battle reads - the energy it
//! refills to, the cards it draws - or works outside a battle entirely, in
//! which case it carries no trigger at all.
//!
class Relic
{
 public:
    Relic() = default;
    Relic(RelicId id, std::string name, RelicTier tier,
          std::vector<RelicTrigger> triggers = {});

    //! Returns the id the registry builds this relic from.
    RelicId GetId() const;

    //! Returns the display name.
    const std::string& GetName() const;

    //! Returns how this relic is come by.
    RelicTier GetTier() const;

    //! Returns what this relic reacts to.
    const std::vector<RelicTrigger>& GetTriggers() const;

    //! Returns how many times the counted hook has come round, for the relics
    //! that only fire every so often.
    int GetCounter() const;

    //! Adds one to the count and returns true when it has come round far
    //! enough for \p every, resetting it when it has.
    bool CountUp(int every);

    //! Puts the count back to zero, which happens at the start of a battle
    //! and, for the relics that count within a turn, at the start of each one.
    void ResetCounter();

    //! Returns true when any of this relic's triggers only count within a
    //! single turn.
    bool CountsPerTurn() const;

    //! Returns true once this relic has been spent, which is what Lizard Tail
    //! and Necronomicon need to remember.
    bool IsUsed() const;

    //! Marks this relic as spent.
    void MarkUsed();

 private:
    RelicId m_id = RelicId::INVALID;
    std::string m_name;
    RelicTier m_tier = RelicTier::INVALID;
    std::vector<RelicTrigger> m_triggers;
    int m_counter = 0;
    bool m_used = false;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_RELIC_HPP
