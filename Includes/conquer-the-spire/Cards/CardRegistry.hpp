// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_CARD_REGISTRY_HPP
#define CONQUER_THE_SPIRE_CARD_REGISTRY_HPP

#include <conquer-the-spire/Enums/CardId.hpp>
#include <conquer-the-spire/Models/Card.hpp>

#include <cstddef>
#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief CardRegistry class.
//!
//! Builds cards from their id. Cards refer to each other by id, so this is
//! what lets Armaments upgrade a card, Dual Wield copy one and Infernal Blade
//! pull a random attack out of the pool.
//!
//!
//! \brief CardWorth struct.
//!
//! What a card does, in numbers: what it costs and how much damage, block and
//! power it hands over. Rough on purpose - it is not a simulation of the card,
//! only enough of one to tell a Strike from a Bash, and a Strike from a
//! Strike sharpened.
//!
struct CardWorth
{
    int cost = 0;
    int damage = 0;
    int block = 0;
    int power = 0;
};

class CardRegistry
{
 public:
    //! Returns what \p id is worth after \p upgradeCount sharpenings.
    static const CardWorth& Worth(CardId id, int upgradeCount);

    //! Returns whether sharpening \p id again would change anything at all.
    //! Statuses and curses never change, and most cards only take one: only
    //! a Searing Blow keeps growing.
    static bool CanUpgrade(CardId id, int upgradeCount);

    //! Returns the card \p id, upgraded \p upgradeCount times. Most cards only
    //! read whether the count is zero; Searing Blow keeps scaling.
    static Card Get(CardId id, int upgradeCount = 0);

    //! Returns one past the largest card id there is, which is how wide a
    //! vector with a slot per card has to be.
    static std::size_t IdCount();

    //! Returns every id of \p color, ignoring statuses and curses unless that
    //! is the colour asked for.
    static const std::vector<CardId>& GetPool(CardColor color);

    //! Returns every id of \p color with \p rarity.
    static std::vector<CardId> GetPool(CardColor color, CardRarity rarity);

    //! Returns the ids of \p color with \p rarity that can show up in a run.
    static std::vector<CardId> GetPoolByRarity(CardColor color,
                                               CardRarity rarity);

    //! Returns the ids of \p color of \p type that can show up in a run,
    //! which is what Infernal Blade and Distraction draw from.
    static std::vector<CardId> GetPoolByType(CardColor color, CardType type);

    //! Returns the ids of \p color that are attacks and can show up in a run.
    static std::vector<CardId> GetAttackPool(CardColor color);

    //! Returns the deck a character starts a run with.
    static std::vector<Card> MakeStarterDeck(CardColor color);
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_CARD_REGISTRY_HPP
