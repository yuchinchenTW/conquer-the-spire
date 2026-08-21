// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_PLAYER_HPP
#define CONQUER_THE_SPIRE_PLAYER_HPP

#include <conquer-the-spire/Models/Card.hpp>
#include <conquer-the-spire/Models/Creature.hpp>
#include <conquer-the-spire/Models/Orb.hpp>
#include <conquer-the-spire/Models/Potion.hpp>
#include <conquer-the-spire/Models/Relic.hpp>

#include <cstddef>
#include <random>
#include <string>
#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief Player class.
//!
//! The character the battle is fought with: a creature that also owns energy
//! and the four card piles.
//!
class Player : public Creature
{
 public:
    //! Cards drawn beyond this many are left in the draw pile.
    static constexpr std::size_t MAX_HAND_SIZE = 10;

    Player() = default;
    Player(std::string name, int maxHealth, int maxEnergy = 3,
           int cardsPerTurn = 5);

    //! Returns the energy left this turn.
    int GetEnergy() const;

    //! Returns the energy this player refills to each turn.
    int GetMaxEnergy() const;

    //! Returns how many cards are drawn at the start of a turn.
    int GetCardsPerTurn() const;

    //! Refills the energy to \p amount.
    void SetEnergy(int amount);

    //! Adds \p amount of energy for this turn only.
    void GainEnergy(int amount);

    //! Spends \p amount of energy, never dropping below zero.
    void UseEnergy(int amount);

    //! Adds \p card to the deck this player brings into battles.
    void AddCardToDeck(Card card);

    //! Returns the deck this player brings into battles.
    std::vector<Card>& GetDeck();
    const std::vector<Card>& GetDeck() const;

    std::vector<Card>& GetDrawPile();
    const std::vector<Card>& GetDrawPile() const;

    std::vector<Card>& GetHand();
    const std::vector<Card>& GetHand() const;

    std::vector<Card>& GetDiscardPile();
    const std::vector<Card>& GetDiscardPile() const;

    std::vector<Card>& GetExhaustPile();
    const std::vector<Card>& GetExhaustPile() const;

    //! Clears every pile and shuffles the deck into the draw pile.
    void InitializePiles(std::mt19937& rng);

    //! Draws \p count cards, reshuffling the discard pile once the draw pile
    //! runs out. Stops at MAX_HAND_SIZE or when no cards are left at all.
    void Draw(int count, std::mt19937& rng);

    //! Draws a single card and returns true when one was actually drawn. The
    //! battle draws one at a time so that it can react to each card.
    bool DrawOne(std::mt19937& rng);

    //! Puts \p card into \p pile. DRAW_SHUFFLED inserts it at a random spot,
    //! DRAW_TOP where it will be drawn next.
    void AddCardToPile(Card card, CardPile pile, std::mt19937& rng);

    //! Returns how many copies of \p id are anywhere in the battle: hand, draw
    //! pile, discard pile and exhaust pile.
    int CountCards(CardId id) const;

    //! Moves every card with the innate flag from the draw pile to the hand.
    void DrawInnateCards();

    //! Moves the whole hand to the discard pile, keeping the cards that are
    //! retained.
    void DiscardHand();

    //! Returns the character this player is, which is what the cards a potion
    //! or a relic hands over are drawn from.
    CardColor GetColor() const;

    //! Sets the character this player is.
    void SetColor(CardColor color);

    //! Hands \p relic to the player for the rest of the run.
    void AddRelic(Relic relic);

    //! Puts \p potion in the belt and returns true when there was room. Sozu
    //! turns every potion away.
    bool AddPotion(Potion potion);

    //! Returns how many potions can be carried at once.
    int GetPotionSlots() const;

    std::vector<Potion>& GetPotions();
    const std::vector<Potion>& GetPotions() const;

    //! The cards a bottle holds, which are in hand from the first turn of
    //! every fight.
    const std::vector<CardId>& GetBottledCards() const;
    void BottleCard(CardId id);

    //! How much strength lifting at rest sites has put on, which every fight
    //! opens with.
    int GetLiftedStrength() const;
    void Lift(int amount);

    //! How much energy the next fight opens with over the usual, which is
    //! what a tea set leaves behind.
    int GetBonusEnergy() const;
    void SetBonusEnergy(int amount);

    //! Returns true when the player carries \p id.
    bool HasRelic(RelicId id) const;

    std::vector<Relic>& GetRelics();
    const std::vector<Relic>& GetRelics() const;

    //! Returns how many times the discard pile has been shuffled back into
    //! the draw pile, which is what Sundial and The Abacus count.
    int GetShuffleCount() const;

    std::vector<Orb>& GetOrbs();
    const std::vector<Orb>& GetOrbs() const;

    //! Returns how many orbs can be held at once.
    int GetOrbSlots() const;

    //! Adds \p amount to the orb slots, never dropping below zero. Orbs past
    //! the new limit fall out of orbit.
    void AddOrbSlots(int amount);

 private:
    int m_energy = 0;
    int m_maxEnergy = 3;
    int m_cardsPerTurn = 5;
    std::vector<Card> m_deck;
    std::vector<Card> m_drawPile;
    std::vector<Card> m_hand;
    std::vector<Card> m_discardPile;
    std::vector<Card> m_exhaustPile;
    std::vector<Orb> m_orbs;
    std::vector<Relic> m_relics;
    std::vector<Potion> m_potions;
    std::vector<CardId> m_bottled;
    int m_liftedStrength = 0;
    int m_bonusEnergy = 0;
    CardColor m_color = CardColor::COLORLESS;
    int m_orbSlots = 3;
    int m_shuffleCount = 0;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_PLAYER_HPP
