// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Cards/CardBuilders.hpp>

#include <utility>

namespace ConquerTheSpire::Detail
{
namespace
{
using CE = CardEffect;

//! Most statuses and curses only clog the hand. What the ones that do
//! something do happens while they sit in hand, which the battle drives from
//! their id at the end of the turn, so their effect list stays empty here.
Card Clutter(CardId id, const char* name, CardColor color, CardType type,
             CardFlag flags)
{
    return Card(id, name, color, type, CardRarity::SPECIAL, CardTarget::NONE,
                -2, {}, flags | CardFlag::UNPLAYABLE);
}
}  // namespace

Card MakeStatusCard(CardId id, int upgradeCount)
{
    static_cast<void>(upgradeCount);  // Statuses cannot be upgraded.

    switch (id)
    {
        case CardId::BURN:
            // Deals 2 damage at the end of the turn while it is in hand.
            return Clutter(id, "Burn", CardColor::STATUS, CardType::STATUS,
                           CardFlag::NONE);

        case CardId::DAZED:
            return Clutter(id, "Dazed", CardColor::STATUS, CardType::STATUS,
                           CardFlag::ETHEREAL);

        case CardId::SLIMED:
            // The one status that can be played, to get rid of it.
            return Card(id, "Slimed", CardColor::STATUS, CardType::STATUS,
                        CardRarity::SPECIAL, CardTarget::SELF, 1, {},
                        CardFlag::EXHAUST);

        case CardId::VOID:
            return Clutter(id, "Void", CardColor::STATUS, CardType::STATUS,
                           CardFlag::LOSE_ENERGY_ON_DRAW |
                               CardFlag::ETHEREAL);

        case CardId::WOUND:
            return Clutter(id, "Wound", CardColor::STATUS, CardType::STATUS,
                           CardFlag::NONE);

        default:
            return Card();
    }
}

Card MakeCurseCard(CardId id, int upgradeCount)
{
    static_cast<void>(upgradeCount);  // Curses cannot be upgraded.

    switch (id)
    {
        case CardId::ASCENDERS_BANE:
            return Clutter(id, "Ascender's Bane", CardColor::CURSE,
                           CardType::CURSE, CardFlag::ETHEREAL);

        case CardId::CLUMSY:
            return Clutter(id, "Clumsy", CardColor::CURSE, CardType::CURSE,
                           CardFlag::ETHEREAL);

        case CardId::CURSE_OF_THE_BELL:
            return Clutter(id, "Curse of the Bell", CardColor::CURSE,
                           CardType::CURSE, CardFlag::NONE);

        case CardId::DECAY:
            // Deals 2 damage at the end of the turn.
            return Clutter(id, "Decay", CardColor::CURSE, CardType::CURSE,
                           CardFlag::NONE);

        case CardId::DOUBT:
            // Applies 1 Weak at the end of the turn.
            return Clutter(id, "Doubt", CardColor::CURSE, CardType::CURSE,
                           CardFlag::NONE);

        case CardId::INJURY:
            return Clutter(id, "Injury", CardColor::CURSE, CardType::CURSE,
                           CardFlag::NONE);

        case CardId::NECRONOMICURSE:
            // Comes back to the hand whenever it is exhausted.
            return Clutter(id, "Necronomicurse", CardColor::CURSE,
                           CardType::CURSE, CardFlag::NONE);

        case CardId::NORMALITY:
            // Blocks the fourth card played in a turn.
            return Clutter(id, "Normality", CardColor::CURSE, CardType::CURSE,
                           CardFlag::NONE);

        case CardId::PAIN:
            // Costs 1 health whenever another card is played.
            return Clutter(id, "Pain", CardColor::CURSE, CardType::CURSE,
                           CardFlag::NONE);

        case CardId::PARASITE:
            return Clutter(id, "Parasite", CardColor::CURSE, CardType::CURSE,
                           CardFlag::NONE);

        case CardId::PRIDE:
            // Playable: it exhausts and leaves a copy on top of the draw pile.
            return Card(id, "Pride", CardColor::CURSE, CardType::CURSE,
                        CardRarity::SPECIAL, CardTarget::SELF, 1,
                        { CE::AddCard(CardId::PRIDE, CardPile::DRAW_TOP) },
                        CardFlag::EXHAUST | CardFlag::INNATE);

        case CardId::REGRET:
            // Costs health equal to the cards in hand at the end of the turn.
            return Clutter(id, "Regret", CardColor::CURSE, CardType::CURSE,
                           CardFlag::NONE);

        case CardId::SHAME:
            // Applies 1 Frail at the end of the turn.
            return Clutter(id, "Shame", CardColor::CURSE, CardType::CURSE,
                           CardFlag::NONE);

        case CardId::WRITHE:
            return Clutter(id, "Writhe", CardColor::CURSE, CardType::CURSE,
                           CardFlag::INNATE);

        default:
            return Card();
    }
}
}  // namespace ConquerTheSpire::Detail
