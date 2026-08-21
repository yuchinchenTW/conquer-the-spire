// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Rl/VecSpireEnv.hpp>

#include <algorithm>
#include <cstring>

namespace ConquerTheSpire
{
VecSpireEnv::VecSpireEnv(std::size_t count)
    : m_envs(count == 0u ? 1u : count),
      m_returns(count == 0u ? 1u : count, 0.0f),
      m_lengths(count == 0u ? 1u : count, 0),
      m_lastSummaries((count == 0u ? 1u : count) * RunLog::Summary::SLOTS, 0)
{
    // Nothing else to set up: a row of climbs is started by Reset().
}

void VecSpireEnv::Reset(CardColor character, unsigned int seed)
{
    m_character = character;
    m_seed = seed;
    m_nextSeed = seed + static_cast<unsigned int>(m_envs.size());

    for (std::size_t i = 0; i < m_envs.size(); ++i)
    {
        m_envs[i].Reset(character, seed + static_cast<unsigned int>(i));
        m_returns[i] = 0.0f;
        m_lengths[i] = 0;
    }
}

void VecSpireEnv::ResetOne(std::size_t index, CardColor character,
                           unsigned int seed)
{
    if (index >= m_envs.size())
    {
        return;
    }

    m_envs[index].Reset(character, seed);
    m_returns[index] = 0.0f;
    m_lengths[index] = 0;
}

std::size_t VecSpireEnv::GetCount() const
{
    return m_envs.size();
}

bool VecSpireEnv::GetAutoReset() const
{
    return m_autoReset;
}

void VecSpireEnv::SetHealthWeight(float weight)
{
    for (auto& env : m_envs)
    {
        env.SetHealthWeight(weight);
    }
}

void VecSpireEnv::SetActLimit(int acts)
{
    for (auto& env : m_envs)
    {
        env.SetActLimit(acts);
    }
}

void VecSpireEnv::SetAutoReset(bool on)
{
    m_autoReset = on;
}

const SpireEnv& VecSpireEnv::At(std::size_t index) const
{
    return m_envs[std::min(index, m_envs.size() - 1u)];
}

SpireEnv& VecSpireEnv::At(std::size_t index)
{
    return m_envs[std::min(index, m_envs.size() - 1u)];
}

void VecSpireEnv::Observe(float* out) const
{
    if (out == nullptr)
    {
        return;
    }

    const std::size_t stride = SpireEnv::ObservationSize();

    for (std::size_t i = 0; i < m_envs.size(); ++i)
    {
        const std::vector<float> state = m_envs[i].Observe();

        std::memcpy(out + i * stride, state.data(),
                    state.size() * sizeof(float));
    }
}

void VecSpireEnv::ObserveIds(int* out) const
{
    if (out == nullptr)
    {
        return;
    }

    const std::size_t stride = SpireEnv::IdCount();

    for (std::size_t i = 0; i < m_envs.size(); ++i)
    {
        const std::vector<int> ids = m_envs[i].ObserveIds();

        std::memcpy(out + i * stride, ids.data(), ids.size() * sizeof(int));
    }
}

void VecSpireEnv::ActionMask(unsigned char* out) const
{
    if (out == nullptr)
    {
        return;
    }

    const std::size_t stride = SpireEnv::ActionCount();

    for (std::size_t i = 0; i < m_envs.size(); ++i)
    {
        const std::vector<unsigned char> mask = m_envs[i].ActionMask();

        std::memcpy(out + i * stride, mask.data(), mask.size());
    }
}

void VecSpireEnv::Step(const std::size_t* actions, float* rewards,
                       unsigned char* dones, unsigned char* taken,
                       float* returns, int* lengths)
{
    for (std::size_t i = 0; i < m_envs.size(); ++i)
    {
        const std::size_t action = actions == nullptr ? 0u : actions[i];
        const StepResult result = m_envs[i].StepIndex(action);

        m_returns[i] += result.reward;
        ++m_lengths[i];

        if (rewards != nullptr)
        {
            rewards[i] = result.reward;
        }

        if (taken != nullptr)
        {
            taken[i] = result.taken ? 1u : 0u;
        }

        // A climb with nothing left to do is over as far as a learner is
        // concerned, whether it died or ran out of moves.
        const bool stuck = !result.done && m_envs[i].LegalActions().empty();
        const bool over = result.done || stuck;

        if (dones != nullptr)
        {
            dones[i] = over ? 1u : 0u;
        }

        if (!over)
        {
            continue;
        }

        if (returns != nullptr)
        {
            returns[i] = m_returns[i];
        }

        if (lengths != nullptr)
        {
            lengths[i] = m_lengths[i];
        }

        // What the climb came to is kept, because the next one is started
        // over the top of it.
        m_envs[i].GetRun().GetLog().ReadSummary(
            m_lastSummaries.data() + i * RunLog::Summary::SLOTS);
        m_stats.Ingest(m_envs[i].GetRun().GetLog());

        if (m_autoReset)
        {
            ResetOne(i, m_character, m_nextSeed);
            ++m_nextSeed;
        }
    }
}

const RunStats& VecSpireEnv::GetStats() const
{
    return m_stats;
}

void VecSpireEnv::ClearStats()
{
    m_stats.Clear();
}

void VecSpireEnv::ReadSummaries(int* out) const
{
    if (out == nullptr)
    {
        return;
    }

    for (std::size_t i = 0; i < m_envs.size(); ++i)
    {
        m_envs[i].GetRun().GetLog().ReadSummary(
            out + i * RunLog::Summary::SLOTS);
    }
}

void VecSpireEnv::ReadLastSummaries(int* out) const
{
    if (out != nullptr)
    {
        std::copy(m_lastSummaries.begin(), m_lastSummaries.end(), out);
    }
}

void VecSpireEnv::RollRandomHere(CardColor character, unsigned int seed,
                                std::size_t runs, float* returns,
                                int* floors, int* steps)
{
    RollRandom(character, seed, runs, returns, floors, steps, &m_stats);
}

void VecSpireEnv::RollRandom(CardColor character, unsigned int seed,
                             std::size_t runs, float* returns, int* floors,
                             int* steps, RunStats* stats)
{
    std::mt19937 rng(seed);

    for (std::size_t run = 0; run < runs; ++run)
    {
        SpireEnv env;

        env.Reset(character, seed + static_cast<unsigned int>(run));

        float total = 0.0f;
        int walked = 0;

        while (!env.IsDone() && walked < 5000)
        {
            const std::vector<Action> moves = env.LegalActions();

            if (moves.empty())
            {
                break;
            }

            std::uniform_int_distribution<std::size_t> pick(
                0, moves.size() - 1);

            total += env.Step(moves[pick(rng)]).reward;
            ++walked;
        }

        if (returns != nullptr)
        {
            returns[run] = total;
        }

        if (floors != nullptr)
        {
            floors[run] = env.GetTotalFloors();
        }

        if (steps != nullptr)
        {
            steps[run] = walked;
        }

        if (stats != nullptr)
        {
            stats->Ingest(env.GetRun().GetLog());
        }
    }
}
}  // namespace ConquerTheSpire
