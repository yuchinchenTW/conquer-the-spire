// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Cards/CardBuilders.hpp>
#include <conquer-the-spire/Cards/CardRegistry.hpp>

#include <map>

namespace ConquerTheSpire
{
namespace
{
//! The last id the registry walks when it collects the pools. Ids are
//! contiguous, so this is the whole table.
constexpr CardId LAST_CARD_ID = CardId::WRITHE;

//! Returns every known id grouped by colour, built once on first use.
const std::map<CardColor, std::vector<CardId>>& AllPools()
{
    static const std::map<CardColor, std::vector<CardId>> pools = [] {
        std::map<CardColor, std::vector<CardId>> built;

        for (int i = 1; i <= static_cast<int>(LAST_CARD_ID); ++i)
        {
            const CardId id = static_cast<CardId>(i);
            const Card card = CardRegistry::Get(id);

            if (card.GetId() == CardId::INVALID)
            {
                continue;
            }

            built[card.GetColor()].emplace_back(id);
        }

        return built;
    }();

    return pools;
}
}  // namespace

std::size_t CardRegistry::IdCount()
{
    // The last card of the list is a curse; a test makes sure every card of
    // every pool fits under this.
    return static_cast<std::size_t>(CardId::WRITHE) + 1u;
}

namespace
{
//! Adds up what a card hands over, the crude way: every effect that deals
//! damage, blocks or puts a power on somebody, counted once.
CardWorth WorthOf(const Card& card)
{
    CardWorth worth;

    worth.cost = card.GetCost();

    for (const auto& effect : card.GetEffects())
    {
        const int times = std::max(1, effect.times);

        switch (effect.type)
        {
            case EffectType::DEAL_DAMAGE:
                worth.damage += effect.value * times;
                break;

            case EffectType::GAIN_BLOCK:
                worth.block += effect.value * times;
                break;

            case EffectType::APPLY_POWER:
            case EffectType::GAIN_ENERGY:
            case EffectType::DRAW_CARD:
            case EffectType::HEAL:
            case EffectType::INCREASE_MAX_HEALTH:
                worth.power += effect.value * times;
                break;

            default:
                break;
        }
    }

    return worth;
}

//! How many sharpenings are worth telling apart. Past the second the only
//! card still changing is a Searing Blow, and one more of those is the same
//! shape of answer as the last.
constexpr int WORTH_UPGRADES = 15;
}  // namespace

const CardWorth& CardRegistry::Worth(CardId id, int upgradeCount)
{
    static std::map<std::pair<int, int>, CardWorth> known;
    static const CardWorth nothing;

    const int at = std::max(0, std::min(upgradeCount, WORTH_UPGRADES));
    const auto key = std::make_pair(static_cast<int>(id), at);
    const auto found = known.find(key);

    if (found != known.end())
    {
        return found->second;
    }

    const Card card = Get(id, at);

    if (card.GetId() == CardId::INVALID)
    {
        return nothing;
    }

    return known.emplace(key, WorthOf(card)).first->second;
}

bool CardRegistry::CanUpgrade(CardId id, int upgradeCount)
{
    const Card card = Get(id, std::max(0, upgradeCount));

    if (card.GetId() == CardId::INVALID ||
        card.GetCardType() == CardType::CURSE ||
        card.GetCardType() == CardType::STATUS)
    {
        return false;
    }

    const CardWorth& now = Worth(id, upgradeCount);
    const CardWorth& next = Worth(id, upgradeCount + 1);

    return now.cost != next.cost || now.damage != next.damage ||
           now.block != next.block || now.power != next.power;
}

Card CardRegistry::Get(CardId id, int upgradeCount)
{
    Card card = Detail::MakeIroncladCard(id, upgradeCount);

    if (card.GetId() == CardId::INVALID)
    {
        card = Detail::MakeSilentCard(id, upgradeCount);
    }

    if (card.GetId() == CardId::INVALID)
    {
        card = Detail::MakeDefectCard(id, upgradeCount);
    }

    if (card.GetId() == CardId::INVALID)
    {
        card = Detail::MakeColorlessCard(id, upgradeCount);
    }

    if (card.GetId() == CardId::INVALID)
    {
        card = Detail::MakeStatusCard(id, upgradeCount);
    }

    if (card.GetId() == CardId::INVALID)
    {
        card = Detail::MakeCurseCard(id, upgradeCount);
    }

    // Statuses and curses have no upgraded form, so they keep their name.
    if (card.GetColor() != CardColor::STATUS &&
        card.GetColor() != CardColor::CURSE)
    {
        card.MarkUpgraded(upgradeCount);
    }

    return card;
}

const std::vector<CardId>& CardRegistry::GetPool(CardColor color)
{
    static const std::vector<CardId> empty;

    const auto& pools = AllPools();
    const auto iter = pools.find(color);

    return iter == pools.end() ? empty : iter->second;
}

std::vector<CardId> CardRegistry::GetPool(CardColor color, CardRarity rarity)
{
    std::vector<CardId> matching;

    for (const CardId id : GetPool(color))
    {
        if (Get(id).GetRarity() == rarity)
        {
            matching.emplace_back(id);
        }
    }

    return matching;
}

std::vector<CardId> CardRegistry::GetPoolByType(CardColor color,
                                                CardType type)
{
    std::vector<CardId> matching;

    for (const CardId id : GetPool(color))
    {
        const Card card = Get(id);
        const CardRarity rarity = card.GetRarity();

        // Basics are not in the reward pool, and neither are the cards other
        // cards make.
        if (card.GetCardType() != type || rarity == CardRarity::BASIC ||
            rarity == CardRarity::SPECIAL)
        {
            continue;
        }

        matching.emplace_back(id);
    }

    return matching;
}

std::vector<CardId> CardRegistry::GetPoolByRarity(CardColor color,
                                                  CardRarity rarity)
{
    return GetPool(color, rarity);
}

std::vector<CardId> CardRegistry::GetAttackPool(CardColor color)
{
    return GetPoolByType(color, CardType::ATTACK);
}

std::vector<Card> CardRegistry::MakeStarterDeck(CardColor color)
{
    std::vector<Card> deck;

    deck.reserve(10);

    if (color == CardColor::RED)
    {
        for (int i = 0; i < 5; ++i)
        {
            deck.emplace_back(Get(CardId::STRIKE_RED));
        }

        for (int i = 0; i < 4; ++i)
        {
            deck.emplace_back(Get(CardId::DEFEND_RED));
        }

        deck.emplace_back(Get(CardId::BASH));
    }
    else if (color == CardColor::BLUE)
    {
        for (int i = 0; i < 4; ++i)
        {
            deck.emplace_back(Get(CardId::STRIKE_BLUE));
        }

        for (int i = 0; i < 4; ++i)
        {
            deck.emplace_back(Get(CardId::DEFEND_BLUE));
        }

        deck.emplace_back(Get(CardId::ZAP));
        deck.emplace_back(Get(CardId::DUALCAST));
    }
    else if (color == CardColor::GREEN)
    {
        for (int i = 0; i < 5; ++i)
        {
            deck.emplace_back(Get(CardId::STRIKE_GREEN));
        }

        for (int i = 0; i < 5; ++i)
        {
            deck.emplace_back(Get(CardId::DEFEND_GREEN));
        }

        deck.emplace_back(Get(CardId::NEUTRALIZE));
        deck.emplace_back(Get(CardId::SURVIVOR));
    }

    return deck;
}
}  // namespace ConquerTheSpire
