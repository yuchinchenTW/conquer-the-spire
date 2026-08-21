// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Models/Relic.hpp>

#include <utility>

namespace ConquerTheSpire
{
RelicTrigger RelicTrigger::On(RelicHook hook, std::vector<CardEffect> effects)
{
    RelicTrigger trigger;
    trigger.hook = hook;
    trigger.effects = std::move(effects);

    return trigger;
}

RelicTrigger RelicTrigger::Every(RelicHook hook, int every,
                                 std::vector<CardEffect> effects)
{
    RelicTrigger trigger = On(hook, std::move(effects));
    trigger.every = every;

    return trigger;
}

RelicTrigger RelicTrigger::EveryInTurn(RelicHook hook, int every,
                                      std::vector<CardEffect> effects)
{
    RelicTrigger trigger = Every(hook, every, std::move(effects));
    trigger.perTurn = true;

    return trigger;
}

RelicTrigger RelicTrigger::OnTurn(int turn, std::vector<CardEffect> effects)
{
    RelicTrigger trigger = On(RelicHook::TURN_START, std::move(effects));
    trigger.onTurn = turn;

    return trigger;
}

RelicTrigger RelicTrigger::OnTurnEnd(int turn,
                                     std::vector<CardEffect> effects)
{
    RelicTrigger trigger = On(RelicHook::TURN_END, std::move(effects));
    trigger.onTurn = turn;

    return trigger;
}

Relic::Relic(RelicId id, std::string name, RelicTier tier,
             std::vector<RelicTrigger> triggers)
    : m_id(id),
      m_name(std::move(name)),
      m_tier(tier),
      m_triggers(std::move(triggers))
{
    // Do nothing
}

RelicId Relic::GetId() const
{
    return m_id;
}

const std::string& Relic::GetName() const
{
    return m_name;
}

RelicTier Relic::GetTier() const
{
    return m_tier;
}

const std::vector<RelicTrigger>& Relic::GetTriggers() const
{
    return m_triggers;
}

int Relic::GetCounter() const
{
    return m_counter;
}

bool Relic::CountUp(int every)
{
    ++m_counter;

    if (every <= 1)
    {
        return true;
    }

    if (m_counter >= every)
    {
        m_counter = 0;
        return true;
    }

    return false;
}

void Relic::ResetCounter()
{
    m_counter = 0;
}

bool Relic::CountsPerTurn() const
{
    for (const auto& trigger : m_triggers)
    {
        if (trigger.perTurn)
        {
            return true;
        }
    }

    return false;
}

bool Relic::IsUsed() const
{
    return m_used;
}

void Relic::MarkUsed()
{
    m_used = true;
}
}  // namespace ConquerTheSpire
