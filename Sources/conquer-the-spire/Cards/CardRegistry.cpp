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
//! Adds up what a card hands over: damage, block, and one number for
//! everything else it does.
//!
//! The everything-else used to cover five kinds of effect out of fifty-two,
//! so a card built from any of the rest came out as damage 0, block 0,
//! power 0 - the same three numbers a Slimed reads as. Worse, the state puts
//! the difference between a card's worth and its sharpened worth beside it,
//! and nought minus nought says sharpening buys nothing. Entrench, Limit
//! Break, Havoc, Second Wind and Exhume were all worth nothing to look at,
//! and were drafted accordingly - Limit Break offered 2690 times and taken
//! none of them.
//!
//! The ones that multiply what is already there cannot be given a number
//! that is true in every state; they are counted as a modest something,
//! which is nearer than nothing.
CardWorth WorthOf(const Card& card)
{
    // What a card that doubles or multiplies is called worth. Better than
    // zero and honest about being a stand-in.
    constexpr int MULTIPLIER = 5;

    // How much health reads as one point of cost. Three keeps the cost of
    // the health-for-energy cards inside the nought-to-three range the state
    // scales cost against: Bloodletting asks one, Offering two.
    constexpr int HEALTH_A_COST = 3;

    CardWorth worth;

    worth.cost = card.GetCost();

    for (const auto& effect : card.GetEffects())
    {
        const int times = std::max(1, effect.times);
        const int value = std::max(1, effect.value) * times;

        switch (effect.type)
        {
            case EffectType::DEAL_DAMAGE:
                worth.damage += effect.value * times;
                break;

            case EffectType::GAIN_BLOCK:
                worth.block += effect.value * times;
                break;

            case EffectType::INCREASE_SELF_DAMAGE:
            case EffectType::INCREASE_CLAW_DAMAGE:
                worth.damage += value;
                break;

            case EffectType::INCREASE_SELF_BLOCK:
                worth.block += value;
                break;

            // Doubling what is already on the table.
            case EffectType::DOUBLE_BLOCK:
                worth.block += MULTIPLIER;
                break;

            case EffectType::DOUBLE_STRENGTH:
            case EffectType::MULTIPLY_TARGET_POWER:
            case EffectType::DOUBLE_ENERGY:
                worth.power += MULTIPLIER;
                break;

            case EffectType::APPLY_POWER:
            case EffectType::GAIN_ENERGY:
            case EffectType::DRAW_CARD:
            case EffectType::HEAL:
            case EffectType::INCREASE_MAX_HEALTH:
            case EffectType::HEAL_PERCENT:
            case EffectType::DRAW_UNTIL:
            case EffectType::UPGRADE_HAND_CARD:
            case EffectType::RETURN_FROM_EXHAUST:
            case EffectType::RETURN_FROM_DISCARD:
            case EffectType::DRAW_TO_HAND_FROM_TOP:
            case EffectType::PLAY_TOP_CARD:
            case EffectType::COPY_HAND_CARD:
            case EffectType::COPY_SELF_TO_DISCARD:
            case EffectType::DISCARD_TO_DRAW_TOP:
            case EffectType::HAND_TO_DRAW_TOP:
            case EffectType::EXHAUST_FOR_ENERGY:
            case EffectType::REDUCE_SELF_COST:
            case EffectType::SET_HAND_COST:
            case EffectType::REMOVE_BLOCK:
            case EffectType::CHANNEL_ORB:
            case EffectType::EVOKE_ORB:
            case EffectType::EVOKE_ALL_ORBS:
            case EffectType::ADD_ORB_SLOTS:
            case EffectType::TRIGGER_DARK_ORBS:
            case EffectType::OBTAIN_POTION:
            case EffectType::SETUP_CARD:
            case EffectType::REMEMBER_CARD:
            case EffectType::TAKE_FROM_DRAW_BY_TYPE:
            case EffectType::ADD_RANDOM_ATTACK:
            case EffectType::ADD_RANDOM_SKILL:
            case EffectType::ADD_RANDOM_POWER:
            case EffectType::ADD_RANDOM_COMMON:
            case EffectType::ADD_RANDOM_CARD:
            case EffectType::EXHAUST_HAND:
            case EffectType::EXHAUST_HAND_CARD:
            case EffectType::DISCARD_CARDS:
            case EffectType::DISCARD_HAND:
            case EffectType::RESHUFFLE_ALL:
            case EffectType::REMOVE_ALL_ORBS:
                worth.power += value;
                break;

            // What a card charges for itself goes to the cost, not against
            // the power, because netting the two hides the card.
            //
            // Bloodletting is two energy for three health and Offering is
            // two energy and three cards for six: taken off the power both
            // came out at exactly -1, the same three numbers, and the policy
            // took Offering from a reward pile 28% of the time and
            // Bloodletting 0% of 2629 - it could only be telling them apart
            // by name. Now the power is what a card hands over and the cost
            // is what it asks, and they read as different cards.
            case EffectType::LOSE_HEALTH:
                worth.cost += (value + HEALTH_A_COST - 1) / HEALTH_A_COST;
                break;

            case EffectType::ADD_CARD:
                // A card handed into a pile. A Burn or a Wound is a price;
                // a Shiv is not - Blade Dance hands over three of them and
                // came out asking four when its energy is one. So the kind
                // of card decides which way it counts.
                if (effect.cardId != CardId::INVALID &&
                    CardRegistry::Get(effect.cardId, 0).IsPlayable())
                {
                    worth.power += value;
                }
                else
                {
                    worth.cost += value;
                }

                break;

            case EffectType::INVALID:
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
