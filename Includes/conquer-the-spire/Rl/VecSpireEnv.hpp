// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_VEC_SPIRE_ENV_HPP
#define CONQUER_THE_SPIRE_VEC_SPIRE_ENV_HPP

#include <conquer-the-spire/Rl/SpireEnv.hpp>

#include <cstddef>
#include <random>
#include <string>
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

    //! What a point of the health ceiling is worth, for every climb of the
    //! row.
    void SetMaxHealthWeight(float weight);

    //! What a curse in the deck costs every climb of the row, a floor at a
    //! time.
    void SetCursePenalty(float penalty);

    //! How often a climb is picked up part-way up rather than started at the
    //! bottom, as a share of the climbs started.
    //!
    //! Every climb starts on the first floor, so the first act is where
    //! nearly all of a run's moves are spent, and the acts the climber is
    //! actually losing in are the ones it practises least: a third of the
    //! climbs reach the second act's boss and a tenth reach the third's. This
    //! puts that right by keeping a copy of a climb whenever it comes up into
    //! a new act, and starting some share of the climbs from one of those
    //! copies instead of from the bottom.
    //!
    //! The copies are the climber's own, taken as it plays, so the states it
    //! is dropped into are the ones it really reaches. Nothing is held from
    //! before the row started, and a full shelf is written round rather than
    //! left alone, so what it practises keeps up with what it has become.
    //!
    //! A copy is taken every floor and not only on the way into an act. An
    //! act is only ever entered whole - the climber rests before a boss and
    //! walks through the door at four fifths of its health - so copies taken
    //! at the door are all of a climb that is doing well. What it actually
    //! loses is the middle: it walks into the second act at 83% and into the
    //! fights that kill it at 35%, and a state like that was never on the
    //! shelf to be handed back. Every floor puts the whole act on the shelf,
    //! wounded and whole alike, in the proportions the climber really meets
    //! them in.
    //!
    //! A climb picked up this way is left out of every table: it walked fewer
    //! floors and met one act's dangers rather than three, so its floors and
    //! its ending are not the same measurement as a whole climb's.
    void SetDeepShare(float share);
    float GetDeepShare() const;

    //! How many copies are being held for \p act, which is nothing until the
    //! row has got that far a few times.
    std::size_t GetDeepHeld(int act) const;

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
    //! Picks \p index up part-way up, if there is anything to pick up and
    //! the die says to. Returns whether it did, so that the caller starts a
    //! climb from the bottom when it did not.
    bool StartDeep(std::size_t index);

    //! Keeps a copy of \p index as it stands, under \p act. Returns whether
    //! there was a copy to be had: a climb in a fight cannot be written out,
    //! so the caller asks again next step rather than losing the floor.
    bool Keep(std::size_t index, int act);

    //! The first and last act a climb may be picked up in. Not the first:
    //! starting there is what every other climb already does.
    static constexpr int SHALLOWEST_START = 2;
    static constexpr int DEEPEST_START = 3;

    //! How many copies are held for each act. A save is a couple of thousand
    //! characters, so the whole shelf is a few megabytes.
    //!
    //! Larger than it was, because a shelf now holds a whole act rather than
    //! its doorway and has that much more to be a fair sample of. It still
    //! turns over in seconds at any real speed, which is what matters: a copy
    //! the climber left a billion moves ago is a state it no longer reaches.
    static constexpr std::size_t DEEP_HELD = 4096;

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

    //! The copies, one shelf an act, and where the next one goes on each
    //! shelf once it is full.
    std::vector<std::vector<std::string>> m_deep;
    std::vector<std::size_t> m_deepNext;

    //! What act and floor each climb was last seen on, so that moving is
    //! something that can be noticed. A copy that could not be taken - the
    //! climb was in a fight - leaves these where they were, so the next step
    //! asks again rather than the floor going by unkept.
    std::vector<int> m_lastAct;
    std::vector<int> m_lastFloor;

    float m_deepShare = 0.0f;
    std::mt19937 m_deepRng;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_VEC_SPIRE_ENV_HPP
