// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Models/Player.hpp>

#include <algorithm>
#include <utility>

namespace ConquerTheSpire
{
Player::Player(std::string name, int maxHealth, int maxEnergy, int cardsPerTurn)
    : Creature(std::move(name), maxHealth),
      m_maxEnergy(maxEnergy),
      m_cardsPerTurn(cardsPerTurn)
{
    // Do nothing
}

int Player::GetEnergy() const
{
    return m_energy;
}

int Player::GetMaxEnergy() const
{
    return m_maxEnergy;
}

int Player::GetCardsPerTurn() const
{
    return m_cardsPerTurn;
}

void Player::SetEnergy(int amount)
{
    m_energy = amount < 0 ? 0 : amount;
}

void Player::GainEnergy(int amount)
{
    if (amount > 0)
    {
        m_energy += amount;
    }
}

void Player::UseEnergy(int amount)
{
    m_energy -= amount;

    if (m_energy < 0)
    {
        m_energy = 0;
    }
}

void Player::AddCardToDeck(Card card)
{
    m_deck.emplace_back(std::move(card));
}

std::vector<Card>& Player::GetDeck()
{
    return m_deck;
}

const std::vector<Card>& Player::GetDeck() const
{
    return m_deck;
}

std::vector<Card>& Player::GetDrawPile()
{
    return m_drawPile;
}

const std::vector<Card>& Player::GetDrawPile() const
{
    return m_drawPile;
}

std::vector<Card>& Player::GetHand()
{
    return m_hand;
}

const std::vector<Card>& Player::GetHand() const
{
    return m_hand;
}

std::vector<Card>& Player::GetDiscardPile()
{
    return m_discardPile;
}

const std::vector<Card>& Player::GetDiscardPile() const
{
    return m_discardPile;
}

std::vector<Card>& Player::GetExhaustPile()
{
    return m_exhaustPile;
}

const std::vector<Card>& Player::GetExhaustPile() const
{
    return m_exhaustPile;
}

void Player::InitializePiles(std::mt19937& rng)
{
    m_hand.clear();
    m_discardPile.clear();
    m_exhaustPile.clear();
    m_drawPile = m_deck;

    std::shuffle(m_drawPile.begin(), m_drawPile.end(), rng);
}

void Player::Draw(int count, std::mt19937& rng)
{
    for (int i = 0; i < count; ++i)
    {
        if (!DrawOne(rng))
        {
            return;
        }
    }
}

bool Player::DrawOne(std::mt19937& rng)
{
    if (m_hand.size() >= MAX_HAND_SIZE)
    {
        return false;
    }

    if (m_drawPile.empty())
    {
        if (m_discardPile.empty())
        {
            return false;
        }

        m_drawPile = std::move(m_discardPile);
        m_discardPile.clear();

        std::shuffle(m_drawPile.begin(), m_drawPile.end(), rng);
        ++m_shuffleCount;
    }

    m_hand.emplace_back(std::move(m_drawPile.back()));
    m_drawPile.pop_back();

    return true;
}

void Player::AddCardToPile(Card card, CardPile pile, std::mt19937& rng)
{
    switch (pile)
    {
        case CardPile::HAND:
            if (m_hand.size() < MAX_HAND_SIZE)
            {
                m_hand.emplace_back(std::move(card));
            }
            else
            {
                // A hand that is already full sends the card away instead.
                m_discardPile.emplace_back(std::move(card));
            }
            break;

        case CardPile::DRAW_TOP:
            m_drawPile.emplace_back(std::move(card));
            break;

        case CardPile::DRAW_BOTTOM:
            m_drawPile.insert(m_drawPile.begin(), std::move(card));
            break;

        case CardPile::DRAW_SHUFFLED:
        {
            std::uniform_int_distribution<std::size_t> where(0,
                                                             m_drawPile.size());
            const std::size_t index = where(rng);
            m_drawPile.insert(
                m_drawPile.begin() + static_cast<std::ptrdiff_t>(index),
                std::move(card));
            break;
        }

        case CardPile::EXHAUST:
            m_exhaustPile.emplace_back(std::move(card));
            break;

        case CardPile::DISCARD:
        case CardPile::INVALID:
            m_discardPile.emplace_back(std::move(card));
            break;
    }
}

int Player::CountCards(CardId id) const
{
    int count = 0;

    for (const std::vector<Card>* pile :
         { &m_hand, &m_drawPile, &m_discardPile, &m_exhaustPile })
    {
        for (const auto& card : *pile)
        {
            if (card.GetId() == id)
            {
                ++count;
            }
        }
    }

    return count;
}

void Player::DrawInnateCards()
{
    for (std::size_t i = m_drawPile.size(); i > 0; --i)
    {
        const std::size_t index = i - 1;

        if (!m_drawPile[index].Has(CardFlag::INNATE))
        {
            continue;
        }

        if (m_hand.size() >= MAX_HAND_SIZE)
        {
            return;
        }

        m_hand.emplace_back(std::move(m_drawPile[index]));
        m_drawPile.erase(m_drawPile.begin() +
                         static_cast<std::ptrdiff_t>(index));
    }
}

CardColor Player::GetColor() const
{
    return m_color;
}

void Player::SetColor(CardColor color)
{
    m_color = color;
}

bool Player::AddPotion(Potion potion)
{
    if (HasRelic(RelicId::SOZU))
    {
        // Sozu turns every potion away.
        return false;
    }

    if (static_cast<int>(m_potions.size()) >= GetPotionSlots())
    {
        return false;
    }

    m_potions.emplace_back(std::move(potion));

    return true;
}

int Player::GetPotionSlots() const
{
    return HasRelic(RelicId::POTION_BELT) ? 5 : 3;
}

std::vector<Potion>& Player::GetPotions()
{
    return m_potions;
}

const std::vector<Potion>& Player::GetPotions() const
{
    return m_potions;
}

void Player::AddRelic(Relic relic)
{
    m_relics.emplace_back(std::move(relic));
}

const std::vector<CardId>& Player::GetBottledCards() const
{
    return m_bottled;
}

void Player::BottleCard(CardId id)
{
    if (id != CardId::INVALID)
    {
        m_bottled.emplace_back(id);
    }
}

int Player::GetLiftedStrength() const
{
    return m_liftedStrength;
}

void Player::Lift(int amount)
{
    if (amount > 0)
    {
        m_liftedStrength += amount;
    }
}

int Player::GetBonusEnergy() const
{
    return m_bonusEnergy;
}

void Player::SetBonusEnergy(int amount)
{
    m_bonusEnergy = amount < 0 ? 0 : amount;
}

bool Player::HasRelic(RelicId id) const
{
    for (const auto& relic : m_relics)
    {
        if (relic.GetId() == id)
        {
            return true;
        }
    }

    return false;
}

std::vector<Relic>& Player::GetRelics()
{
    return m_relics;
}

const std::vector<Relic>& Player::GetRelics() const
{
    return m_relics;
}

int Player::GetShuffleCount() const
{
    return m_shuffleCount;
}

std::vector<Orb>& Player::GetOrbs()
{
    return m_orbs;
}

const std::vector<Orb>& Player::GetOrbs() const
{
    return m_orbs;
}

int Player::GetOrbSlots() const
{
    return m_orbSlots;
}

void Player::AddOrbSlots(int amount)
{
    m_orbSlots += amount;

    if (m_orbSlots < 0)
    {
        m_orbSlots = 0;
    }

    while (m_orbs.size() > static_cast<std::size_t>(m_orbSlots))
    {
        m_orbs.pop_back();
    }
}

void Player::DiscardHand()
{
    std::vector<Card> retained;

    for (auto& card : m_hand)
    {
        if (card.Has(CardFlag::RETAIN))
        {
            retained.emplace_back(std::move(card));
        }
        else
        {
            m_discardPile.emplace_back(std::move(card));
        }
    }

    m_hand = std::move(retained);
}
}  // namespace ConquerTheSpire
