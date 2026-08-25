// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Events/Event.hpp>

#include <utility>

namespace ConquerTheSpire
{
EventEffect EventEffect::Gold(int amount)
{
    EventEffect effect;
    effect.type = EventEffectType::GAIN_GOLD;
    effect.amount = amount;
    effect.high = amount;

    return effect;
}

EventEffect EventEffect::GoldRange(int low, int high)
{
    EventEffect effect;
    effect.type = EventEffectType::GAIN_GOLD;
    effect.amount = low;
    effect.high = high;

    return effect;
}

EventEffect EventEffect::LoseGold(int low, int high)
{
    EventEffect effect;
    effect.type = EventEffectType::LOSE_GOLD;
    effect.amount = low;
    effect.high = high;

    return effect;
}

EventEffect EventEffect::LoseAllGold()
{
    EventEffect effect;
    effect.type = EventEffectType::LOSE_ALL_GOLD;

    return effect;
}

EventEffect EventEffect::Heal(int amount)
{
    EventEffect effect;
    effect.type = EventEffectType::HEAL;
    effect.amount = amount;

    return effect;
}

EventEffect EventEffect::HealPercent(int percent)
{
    EventEffect effect;
    effect.type = EventEffectType::HEAL_PERCENT;
    effect.percent = percent;

    return effect;
}

EventEffect EventEffect::HealFull()
{
    EventEffect effect;
    effect.type = EventEffectType::HEAL_FULL;

    return effect;
}

EventEffect EventEffect::Damage(int amount)
{
    EventEffect effect;
    effect.type = EventEffectType::LOSE_HEALTH;
    effect.amount = amount;

    return effect;
}

EventEffect EventEffect::DamagePercent(int percent)
{
    EventEffect effect;
    effect.type = EventEffectType::LOSE_HEALTH_PERCENT;
    effect.percent = percent;

    return effect;
}

EventEffect EventEffect::GainMaxHealth(int amount)
{
    EventEffect effect;
    effect.type = EventEffectType::GAIN_MAX_HEALTH;
    effect.amount = amount;

    return effect;
}

EventEffect EventEffect::LoseMaxHealth(int amount)
{
    EventEffect effect;
    effect.type = EventEffectType::LOSE_MAX_HEALTH;
    effect.amount = amount;

    return effect;
}

EventEffect EventEffect::LoseMaxHealthPercent(int percent)
{
    EventEffect effect;
    effect.type = EventEffectType::LOSE_MAX_HEALTH_PERCENT;
    effect.percent = percent;

    return effect;
}

EventEffect EventEffect::DamageCurrentPercent(int percent)
{
    EventEffect effect;
    effect.type = EventEffectType::LOSE_HEALTH_PERCENT_CURRENT;
    effect.percent = percent;

    return effect;
}

EventEffect EventEffect::CardReward(int count, CardColor color,
                                    CardRarity rarity)
{
    EventEffect effect;
    effect.type = EventEffectType::CARD_REWARD;
    effect.count = count;
    effect.color = color;
    effect.rarity = rarity;

    return effect;
}

EventEffect EventEffect::GainRelic(RelicId id)
{
    EventEffect effect;
    effect.type = EventEffectType::GAIN_RELIC;
    effect.relic = id;

    return effect;
}

EventEffect EventEffect::RandomRelic()
{
    EventEffect effect;
    effect.type = EventEffectType::GAIN_RANDOM_RELIC;

    return effect;
}

EventEffect EventEffect::RandomRelic(RelicTier tier)
{
    EventEffect effect;
    effect.type = EventEffectType::GAIN_RANDOM_RELIC;
    effect.tier = tier;

    return effect;
}

EventEffect EventEffect::BossSwap()
{
    EventEffect effect;
    effect.type = EventEffectType::BOSS_RELIC_SWAP;

    return effect;
}

EventEffect EventEffect::GainCurse(CardId id)
{
    EventEffect effect;
    effect.type = EventEffectType::GAIN_CURSE;
    effect.card = id;

    return effect;
}

EventEffect EventEffect::RandomCurse()
{
    EventEffect effect;
    effect.type = EventEffectType::GAIN_RANDOM_CURSE;

    return effect;
}

EventEffect EventEffect::GainCard(CardId id)
{
    EventEffect effect;
    effect.type = EventEffectType::GAIN_CARD;
    effect.card = id;

    return effect;
}

EventEffect EventEffect::GainCards(CardId id, int count)
{
    EventEffect effect;
    effect.type = EventEffectType::GAIN_CARD;
    effect.card = id;
    effect.count = count;

    return effect;
}

EventEffect EventEffect::MaybeCurse(CardId id, int chance)
{
    EventEffect effect;
    effect.type = EventEffectType::GAIN_CURSE;
    effect.card = id;
    effect.percent = chance;

    return effect;
}

EventEffect EventEffect::RandomCards(int count, CardColor color,
                                     CardRarity rarity)
{
    EventEffect effect;
    effect.type = EventEffectType::GAIN_RANDOM_CARDS;
    effect.count = count;
    effect.color = color;
    effect.rarity = rarity;

    return effect;
}

EventEffect EventEffect::Potions(int count)
{
    EventEffect effect;
    effect.type = EventEffectType::GAIN_POTIONS;
    effect.count = count;

    return effect;
}

EventEffect EventEffect::RemoveCards(int count)
{
    EventEffect effect;
    effect.type = EventEffectType::REMOVE_CARDS;
    effect.count = count;

    return effect;
}

EventEffect EventEffect::UpgradeCards(int count)
{
    EventEffect effect;
    effect.type = EventEffectType::UPGRADE_CARDS;
    effect.count = count;

    return effect;
}

EventEffect EventEffect::UpgradeRandom(int count)
{
    EventEffect effect;
    effect.type = EventEffectType::UPGRADE_RANDOM_CARDS;
    effect.count = count;

    return effect;
}

EventEffect EventEffect::TransformCards(int count)
{
    EventEffect effect;
    effect.type = EventEffectType::TRANSFORM_CARDS;
    effect.count = count;

    return effect;
}

EventEffect EventEffect::DuplicateCard()
{
    EventEffect effect;
    effect.type = EventEffectType::DUPLICATE_CARD;

    return effect;
}

EventEffect EventEffect::CleanseCurses()
{
    EventEffect effect;
    effect.type = EventEffectType::CLEANSE_CURSES;

    return effect;
}

EventEffect EventEffect::LosePotion()
{
    EventEffect effect;
    effect.type = EventEffectType::LOSE_POTION;

    return effect;
}

EventEffect EventEffect::LoseCard()
{
    EventEffect effect;
    effect.type = EventEffectType::LOSE_CARD;

    return effect;
}

EventEffect EventEffect::BurnOffering()
{
    EventEffect effect;
    effect.type = EventEffectType::BURN_OFFERING;

    return effect;
}

EventEffect EventEffect::Fight(std::vector<MonsterId> monsters, RelicId prize)
{
    EventEffect effect;
    effect.type = EventEffectType::FIGHT;
    effect.monsters = std::move(monsters);
    effect.prize = prize;

    return effect;
}

EventEffect EventEffect::FightFor(std::vector<MonsterId> monsters,
                                 RelicTier tier)
{
    EventEffect effect;
    effect.type = EventEffectType::FIGHT;
    effect.monsters = std::move(monsters);
    effect.prizeTier = tier;

    return effect;
}

EventEffect EventEffect::RemoveRandomOfType(CardType type)
{
    EventEffect effect;
    effect.type = EventEffectType::REMOVE_RANDOM_OF_TYPE;
    effect.cardType = type;

    return effect;
}

EventEffect EventEffect::UpgradeAll()
{
    EventEffect effect;
    effect.type = EventEffectType::UPGRADE_ALL;

    return effect;
}

EventEffect EventEffect::UpgradeAllBasic()
{
    EventEffect effect;
    effect.type = EventEffectType::UPGRADE_ALL_BASIC;

    return effect;
}

EventEffect EventEffect::ReplaceEvery(CardId goes, CardId card, int count)
{
    EventEffect effect;
    effect.type = EventEffectType::REPLACE_EVERY;
    effect.goes = goes;
    effect.card = card;
    effect.count = count;

    return effect;
}

EventEffect EventEffect::LoseRelic(RelicId id)
{
    EventEffect effect;
    effect.type = EventEffectType::LOSE_RELIC;
    effect.relic = id;

    return effect;
}

EventEffect EventEffect::OneOfRelics(std::vector<RelicId> choices)
{
    EventEffect effect;
    effect.type = EventEffectType::GAIN_ONE_OF_RELICS;
    effect.relics = std::move(choices);

    return effect;
}

EventEffect EventEffect::SkullToll()
{
    EventEffect effect;
    effect.type = EventEffectType::SKULL_TOLL;

    return effect;
}

EventEffect EventEffect::Wager(int stake, int chance, int payout)
{
    EventEffect effect;
    effect.type = EventEffectType::WAGER;
    effect.amount = stake;
    effect.percent = chance;
    effect.high = payout;

    return effect;
}

EventEffect EventEffect::ToTheBoss()
{
    EventEffect effect;
    effect.type = EventEffectType::TO_THE_BOSS;

    return effect;
}

EventEffect EventEffect::FightOldBoss()
{
    EventEffect effect;
    effect.type = EventEffectType::FIGHT_OLD_BOSS;

    return effect;
}

EventEffect EventEffect::Spin()
{
    EventEffect effect;
    effect.type = EventEffectType::SPIN_WHEEL;

    return effect;
}

EventEffect EventEffect::Reach()
{
    EventEffect effect;
    effect.type = EventEffectType::REACH_INTO_OOZE;

    return effect;
}

EventEffect EventEffect::Search()
{
    EventEffect effect;
    effect.type = EventEffectType::SEARCH_BODY;

    return effect;
}

EventEffect EventEffect::TradeFace()
{
    EventEffect effect;
    effect.type = EventEffectType::TRADE_FACE;

    return effect;
}

EventOption::EventOption(std::string label, std::vector<EventEffect> effects,
                         int nextStage)
    : label(std::move(label)),
      effects(std::move(effects)),
      nextStage(nextStage)
{
    // Nothing else to set up.
}

EventOption& EventOption::Costs(int gold)
{
    requirement = EventRequirement::GOLD;
    requirementValue = gold;
    goldCost = gold;

    return *this;
}

EventOption& EventOption::Needs(EventRequirement wanted, int value)
{
    requirement = wanted;
    requirementValue = value;

    return *this;
}

EventOption& EventOption::NeedsRelic(RelicId id)
{
    requirement = EventRequirement::HAS_RELIC;
    requirementRelic = id;

    return *this;
}

Event::Event(EventId id, std::string name, EventKind kind)
    : m_id(id), m_name(std::move(name)), m_kind(kind)
{
    // Nothing else to set up.
}

void Event::AddStage(std::vector<EventOption> options)
{
    m_stages.emplace_back(std::move(options));
}

EventId Event::GetId() const
{
    return m_id;
}

const std::string& Event::GetName() const
{
    return m_name;
}

EventKind Event::GetKind() const
{
    return m_kind;
}

const std::vector<EventOption>& Event::GetOptions() const
{
    if (m_done || m_stage < 0 ||
        static_cast<std::size_t>(m_stage) >= m_stages.size())
    {
        return m_none;
    }

    return m_stages[static_cast<std::size_t>(m_stage)];
}

int Event::GetStage() const
{
    return m_stage;
}

bool Event::IsDone() const
{
    return m_done;
}

void Event::GoTo(int stage)
{
    if (stage < 0 || static_cast<std::size_t>(stage) >= m_stages.size())
    {
        m_done = true;

        return;
    }

    if (stage != m_stage)
    {
        // A new stage starts its own count.
        m_tries = 0;
    }

    m_stage = stage;
}

int Event::GetTries() const
{
    return m_tries;
}

void Event::CountTry()
{
    ++m_tries;
}

int Event::GetOptionTries(std::size_t index) const
{
    return index < m_optionTries.size() ? m_optionTries[index] : 0;
}

void Event::CountOption(std::size_t index)
{
    if (index >= m_optionTries.size())
    {
        m_optionTries.resize(index + 1, 0);
    }

    ++m_optionTries[index];
}

std::vector<int>& Event::GetBag()
{
    return m_bag;
}

const std::vector<int>& Event::GetBag() const
{
    return m_bag;
}
}  // namespace ConquerTheSpire
