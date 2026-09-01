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
      m_lastSummaries((count == 0u ? 1u : count) * RunLog::Summary::SLOTS, 0),
      m_deep(static_cast<std::size_t>(DEEPEST_START - SHALLOWEST_START + 1)),
      m_deepNext(
          static_cast<std::size_t>(DEEPEST_START - SHALLOWEST_START + 1), 0u),
      m_lastAct(count == 0u ? 1u : count, 1)
{
    // Nothing else to set up: a row of climbs is started by Reset().
}

void VecSpireEnv::Reset(CardColor character, unsigned int seed)
{
    m_character = character;
    m_seed = seed;
    m_nextSeed = seed + static_cast<unsigned int>(m_envs.size());

    // Off the seed the row was started with, so that a run played twice over
    // is played the same way twice over.
    m_deepRng.seed(seed + 0x9E3779B9u);

    for (std::size_t i = 0; i < m_envs.size(); ++i)
    {
        m_envs[i].Reset(character, seed + static_cast<unsigned int>(i));
        m_returns[i] = 0.0f;
        m_lengths[i] = 0;
        m_lastAct[i] = m_envs[i].GetRun().GetAct();
    }
}

void VecSpireEnv::ResetOne(std::size_t index, CardColor character,
                           unsigned int seed)
{
    if (index >= m_envs.size())
    {
        return;
    }

    // Part-way up if there is anywhere to be picked up and the die says so,
    // and from the bottom otherwise - which is every climb until the row has
    // reached the second act a few times, and every climb for good if nothing
    // asked for this.
    if (!StartDeep(index))
    {
        m_envs[index].Reset(character, seed);
    }

    m_returns[index] = 0.0f;
    m_lengths[index] = 0;
    m_lastAct[index] = m_envs[index].GetRun().GetAct();
}

void VecSpireEnv::SetDeepShare(float share)
{
    m_deepShare = share < 0.0f ? 0.0f : (share > 1.0f ? 1.0f : share);
}

float VecSpireEnv::GetDeepShare() const
{
    return m_deepShare;
}

std::size_t VecSpireEnv::GetDeepHeld(int act) const
{
    if (act < SHALLOWEST_START || act > DEEPEST_START)
    {
        return 0u;
    }

    return m_deep[static_cast<std::size_t>(act - SHALLOWEST_START)].size();
}

bool VecSpireEnv::StartDeep(std::size_t index)
{
    if (m_deepShare <= 0.0f)
    {
        return false;
    }

    // Which acts there is anything to be picked up in.
    std::vector<std::size_t> filled;

    for (std::size_t at = 0; at < m_deep.size(); ++at)
    {
        if (!m_deep[at].empty())
        {
            filled.emplace_back(at);
        }
    }

    if (filled.empty())
    {
        return false;
    }

    std::uniform_real_distribution<float> coin(0.0f, 1.0f);

    if (coin(m_deepRng) >= m_deepShare)
    {
        return false;
    }

    // Evenly between the acts rather than evenly among the copies. The whole
    // point is to be where the climber is losing, and the deeper act is
    // reached a third as often, so it has a third as many copies to offer -
    // drawing evenly among them would practise the shallower act three times
    // as much, which is the thing being fixed.
    std::uniform_int_distribution<std::size_t> which(0u, filled.size() - 1u);
    const std::vector<std::string>& held = m_deep[filled[which(m_deepRng)]];
    std::uniform_int_distribution<std::size_t> pick(0u, held.size() - 1u);

    if (!m_envs[index].Load(held[pick(m_deepRng)]))
    {
        return false;
    }

    m_envs[index].NoteStartedDeep();

    return true;
}

bool VecSpireEnv::Keep(std::size_t index, int act)
{
    // A row that was not asked for this pays nothing for it: no saves taken
    // and no shelf standing there holding a few megabytes of them.
    if (m_deepShare <= 0.0f || act < SHALLOWEST_START || act > DEEPEST_START)
    {
        return false;
    }

    const std::string save = m_envs[index].Save();

    // A climb in a fight cannot be written out. The act is noticed after the
    // step that changed it, and that step can have walked straight into a
    // room, so this is the ordinary case rather than a strange one - saying
    // no and being asked again next step is how the act gets kept at all.
    if (save.empty())
    {
        return false;
    }

    const std::size_t shelf = static_cast<std::size_t>(act - SHALLOWEST_START);

    if (m_deep[shelf].size() < DEEP_HELD)
    {
        m_deep[shelf].emplace_back(save);

        return true;
    }

    // Round and round rather than never again: the copies have to keep up
    // with the climber, or an act goes on being picked up in a state the
    // climber stopped reaching a billion moves ago.
    m_deep[shelf][m_deepNext[shelf]] = save;
    m_deepNext[shelf] = (m_deepNext[shelf] + 1u) % DEEP_HELD;

    return true;
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

void VecSpireEnv::SetMaxHealthWeight(float weight)
{
    for (auto& env : m_envs)
    {
        env.SetMaxHealthWeight(weight);
    }
}

void VecSpireEnv::SetCursePenalty(float penalty)
{
    for (auto& env : m_envs)
    {
        env.SetCursePenalty(penalty);
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
            // A climb that has just come up into a new act leaves a copy of
            // itself behind for another climb to be started from.
            // The act is only marked as kept once a copy was actually
            // taken, so a climb that came up into it and walked straight into
            // a fight is asked again on the step after.
            const int act = m_envs[i].GetRun().GetAct();

            if (act > m_lastAct[i] && Keep(i, act))
            {
                m_lastAct[i] = act;
            }

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

        // A climb picked up part-way up is not one of the climbs the tables
        // are about: its fights were reached with a deck the first act did
        // not build, and letting it in would move every share in them without
        // any of the choices behind them having changed. The summary is still
        // written out, so that whoever is reading can see it happened.
        if (!m_envs[i].StartedDeep())
        {
            m_stats.Ingest(m_envs[i].GetRun().GetLog());
        }

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
