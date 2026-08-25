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
    //! What it asks in energy, and nothing else. Bloodletting and Offering
    //! are nought energy cards that charge in health, and rolling the health
    //! into this made them read as costing one and two: the number a fight
    //! needs - can this be afforded right now - stopped being in here at all.
    int cost = 0;

    //! What it charges in health.
    int health = 0;
    int damage = 0;
    int block = 0;

    //! Cards drawn, and energy handed over. Apart from the rest because they
    //! are the two things that buy anything at all in this game - a card
    //! drawn is any card, an energy is any card played - and lumped in with
    //! everything else they could not be told from a buff. Battle Trance
    //! draws three and Flex gives two strength for one turn; both came out
    //! at power 4, the same number for the strong card and the weak one.
    int draw = 0;
    int energy = 0;

    //! Everything else it hands over once: a buff put on, a heal, a card
    //! upgraded, an orb channelled.
    int power = 0;

    //! What it hands over again every turn, or on every trigger, for the
    //! rest of the fight. Apart from the power because the two are not the
    //! same size at all: Inflame gives two Strength once and Demon Form
    //! gives two every turn, and both came out at power two - the same three
    //! numbers for a common card and a rare one, and the policy took Demon
    //! Form from a pile six times in a hundred.
    //!
    //! A power that changes the rules rather than handing something over -
    //! Corruption making every skill free, Barricade keeping the block -
    //! counts here too, at a stand-in worth, because the alternative is
    //! calling it one.
    int lasting = 0;

    //! How many of the climber's own cards it throws away - the hand it
    //! exhausts, the cards it discards, and itself if it exhausts.
    //!
    //! Stated rather than judged. Thrown away is a price when the cards were
    //! worth playing and a payment when they were not, it thins what is left
    //! to draw from, and a deck holding Dark Embrace or Feel No Pain is paid
    //! for every card that goes. Which of those it is depends on the deck,
    //! which is beside this in the state, so the number is left as a count
    //! and the sign is left to whoever reads it.
    int exhausts = 0;

    //! What holding it costs, near enough, for every turn it sits in the
    //! hand. A curse deals no damage and blocks nothing, so every figure
    //! above it is nought - the same nought a card that does nothing has -
    //! and the only thing telling a Regret from a blank was its name.
    //!
    //! Where the harm is written down in the fight rather than on the card,
    //! this mirrors it. Battle::EndOfTurnCurses is the other half.
    int harm = 0;

    //! Whether it cannot be played at all. An unplayable card is a draw
    //! wasted every time it turns up, and its cost is a sentinel rather than
    //! a price: written into the state as it stands, a curse read as cheaper
    //! than a Strike.
    int unplayable = 0;

    //! How far up the rarities it is, from a basic card at nought to a rare
    //! at three. What is worth tearing out of a deck is mostly what sits at
    //! the bottom of this.
    int rarity = 0;

    //! Whether any of these figures is read off the table rather than
    //! written on the card. Fiend Fire hits for seven a card exhausted;
    //! seven is what the damage says, and this says the seven is a rate.
    int scales = 0;

    //! Whether it is always in the opening hand. What Brutality buys by being
    //! sharpened is exactly this and nothing else, so a table without it says
    //! the sharpening changes nothing.
    int innate = 0;

    //! Whether it burns itself at the end of a turn it was not played in. An
    //! Apparition is sharpened out of this, and again the sharpening changes
    //! none of the other figures.
    int ethereal = 0;

    //! Whether it lands on everything standing rather than one thing. A Blind
    //! and a Trip are sharpened from one to all, and neither the damage nor
    //! anything else on the card moves - the width of the swing is the whole
    //! of what is bought.
    int hitsAll = 0;
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
