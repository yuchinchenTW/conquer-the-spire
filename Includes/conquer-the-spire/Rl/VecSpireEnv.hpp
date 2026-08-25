// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_VEC_SPIRE_ENV_HPP
#define CONQUER_THE_SPIRE_VEC_SPIRE_ENV_HPP

#include <conquer-the-spire/Rl/SpireEnv.hpp>

#include <cstddef>
#include <random>
#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief VecSpireEnv class.
//!
//! A row of climbs stepped together. Whatever is on the other side of the
//! wall pays for one call a tick rather than one a climb, which is the whole
//! point: the engine itself takes a few microseconds a step, so a call for
//! every one of them is most of the bill.
//!
//! A climb that ends starts another one on its own, with the next seed, and
//! the tick it ended on says so. What it was worth altogether and how long it
//! lasted are reported on that same tick, which is what a learner writes down.
//!
class VecSpireEnv
{
 public:
    VecSpireEnv() = default;

    //! Builds \p count climbs side by side.
    explicit VecSpireEnv(std::size_t count);

    //! Starts every climb, the one at \p i seeded with \p seed plus \p i.
    void Reset(CardColor character, unsigned int seed);

    //! Starts the climb at \p index over.
    void ResetOne(std::size_t index, CardColor character, unsigned int seed);

    std::size_t GetCount() const;

    //! Ends every climb of the row once \p acts of the spire are cleared,
    //! 0 being the whole of it.
    void SetActLimit(int acts);

    //! What a point of health taken off costs every climb of the row.
    void SetHealthWeight(float weight);

    //! What a curse in the deck costs every climb of the row, a floor at a
    //! time.
    void SetCursePenalty(float penalty);

    //! Whether a climb that ends starts another one on its own. It does by
    //! default.
    bool GetAutoReset() const;
    void SetAutoReset(bool on);

    //! The climb at \p index, for a look at what is going on in it.
    const SpireEnv& At(std::size_t index) const;
    SpireEnv& At(std::size_t index);

    //! Writes every state one after another: \p out takes count times
    //! ObservationSize() floats.
    void Observe(float* out) const;

    //! The same for the ids and for the masks.
    void ObserveIds(int* out) const;
    void ActionMask(unsigned char* out) const;

    //! Takes one move in every climb. \p actions holds one index a climb;
    //! the rest are written to, one a climb, and any of them may be null:
    //!
    //! \p rewards  what the move was worth
    //! \p dones    whether the climb ended on this tick
    //! \p taken    whether the move was a legal one at all
    //! \p returns  what the climb that just ended was worth altogether
    //! \p lengths  how many moves it lasted
    void Step(const std::size_t* actions, float* rewards,
              unsigned char* dones, unsigned char* taken, float* returns,
              int* lengths);

    //! What came of the choices, over every climb this row has played.
    const RunStats& GetStats() const;
    void ClearStats();

    //! Reads the summary of every climb going on, one row of
    //! RunLog::Summary::SLOTS numbers each.
    void ReadSummaries(int* out) const;

    //! Reads the summary of the climb that ended last in each row. A row
    //! whose climb has not ended yet reads as nothing but zeroes.
    void ReadLastSummaries(int* out) const;

    //! Plays \p runs climbs right through with a die, all on this side of the
    //! wall, and writes down what each of them came to. This is the baseline
    //! a learner is measured against, and the quickest way to see how fast
    //! the engine is.
    static void RollRandom(CardColor character, unsigned int seed,
                           std::size_t runs, float* returns, int* floors,
                           int* steps, RunStats* stats = nullptr);

    //! The same, counted into this row's table.
    void RollRandomHere(CardColor character, unsigned int seed,
                        std::size_t runs, float* returns, int* floors,
                        int* steps);

 private:
    std::vector<SpireEnv> m_envs;
    std::vector<float> m_returns;
    std::vector<int> m_lengths;

    //! What the climb that ended last in each row came to, kept because the
    //! next one is started over the top of it.
    std::vector<int> m_lastSummaries;
    RunStats m_stats;
    CardColor m_character = CardColor::RED;
    unsigned int m_seed = 0;
    unsigned int m_nextSeed = 0;
    bool m_autoReset = true;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_VEC_SPIRE_ENV_HPP
