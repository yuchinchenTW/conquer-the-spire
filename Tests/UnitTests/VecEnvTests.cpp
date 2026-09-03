#include "doctest.h"

#include <conquer-the-spire/Rl/VecSpireEnv.hpp>
#include <conquer-the-spire/Run/Run.hpp>

#include <algorithm>
#include <cstddef>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace ConquerTheSpire;

namespace
{
//! Picks the first legal move of a mask row, or nothing at all.
std::size_t FirstLegal(const std::vector<unsigned char>& mask,
                       std::size_t row)
{
    const std::size_t stride = SpireEnv::ActionCount();

    for (std::size_t i = 0; i < stride; ++i)
    {
        if (mask[row * stride + i] != 0u)
        {
            return i;
        }
    }

    return stride;
}
//! A save of a climb standing at the top of the second act.
//!
//! Made by walking a run rather than by playing one: random moves die in the
//! first act every time, so a row of them never reaches the second and there
//! would be nothing to pick up. Travel walks the map without fighting, which
//! is exactly enough to get through the door.
std::string SecondActSave(unsigned int seed)
{
    Run run(CardColor::RED, seed);

    while (!run.GetAvailableColumns().empty())
    {
        run.Travel(run.GetAvailableColumns().front());
    }

    if (!run.IsAtBoss())
    {
        return std::string();
    }

    run.FinishBoss();

    if (!run.AdvanceAct() || run.GetAct() != 2)
    {
        return std::string();
    }

    std::ostringstream out;

    out << "env " << static_cast<int>(EnvPhase::MAP) << " 0 0\n"
        << run.Serialize();

    return out.str();
}

//! Plays \p ticks of \p row at random and returns how many climbs that ended
//! in it had been picked up part-way up.
int PlayedDeep(VecSpireEnv& row, int ticks, unsigned int dieSeed)
{
    const std::size_t rows = row.GetCount();
    const std::size_t stride = SpireEnv::ActionCount();
    const std::size_t slots = RunLog::Summary::SLOTS;
    std::vector<unsigned char> mask(rows * stride, 0u);
    std::vector<std::size_t> actions(rows, 0u);
    std::vector<unsigned char> dones(rows, 0u);
    std::vector<int> last(rows * slots, 0);
    std::mt19937 rng(dieSeed);
    int deep = 0;

    for (int tick = 0; tick < ticks; ++tick)
    {
        row.ActionMask(mask.data());

        for (std::size_t i = 0; i < rows; ++i)
        {
            std::vector<std::size_t> open;

            for (std::size_t slot = 0; slot < stride; ++slot)
            {
                if (mask[i * stride + slot] != 0u)
                {
                    open.emplace_back(slot);
                }
            }

            if (open.empty())
            {
                actions[i] = 0u;
                continue;
            }

            std::uniform_int_distribution<std::size_t> pick(
                0, open.size() - 1);

            actions[i] = open[pick(rng)];
        }

        row.Step(actions.data(), nullptr, dones.data(), nullptr, nullptr,
                 nullptr);
        row.ReadLastSummaries(last.data());

        for (std::size_t i = 0; i < rows; ++i)
        {
            // The last slot of a summary, which is whether the climb it
            // describes was picked up part-way up.
            if (dones[i] != 0u && last[i * slots + (slots - 1u)] != 0)
            {
                ++deep;
            }
        }
    }

    return deep;
}
}  // namespace

TEST_CASE("Starting over is not paid for the ceiling coming back")
{
    // A climber that dies at a ceiling it sold down starts the next climb at
    // eighty again, and a price on the ceiling that is paid both ways would
    // hand over a reward for the difference: a reward for dying. The next
    // climb is started after the step is scored rather than during it, and
    // this is the test that says so.
    const std::size_t rows = 8u;
    const std::size_t stride = SpireEnv::ActionCount();
    float paid[2] = { 0.0f, 0.0f };
    int ended[2] = { 0, 0 };

    for (int which = 0; which < 2; ++which)
    {
        VecSpireEnv row(rows);

        row.SetAutoReset(true);
        row.SetMaxHealthWeight(which == 0 ? 0.0f : 0.05f);
        row.Reset(CardColor::RED, 41u);

        std::vector<unsigned char> mask(rows * stride, 0u);
        std::vector<std::size_t> actions(rows, 0u);
        std::vector<float> rewards(rows, 0.0f);
        std::vector<unsigned char> dones(rows, 0u);
        std::mt19937 rng(11u);

        for (int tick = 0; tick < 400; ++tick)
        {
            row.ActionMask(mask.data());

            for (std::size_t i = 0; i < rows; ++i)
            {
                std::vector<std::size_t> open;

                for (std::size_t slot = 0; slot < stride; ++slot)
                {
                    if (mask[i * stride + slot] != 0u)
                    {
                        open.emplace_back(slot);
                    }
                }

                if (open.empty())
                {
                    actions[i] = 0u;
                    continue;
                }

                std::uniform_int_distribution<std::size_t> pick(
                    0, open.size() - 1);

                actions[i] = open[pick(rng)];
            }

            row.Step(actions.data(), rewards.data(), dones.data(), nullptr,
                     nullptr, nullptr);

            for (std::size_t i = 0; i < rows; ++i)
            {
                // Only the steps that ended a climb, which are the only ones
                // a start-over could be hiding in.
                if (dones[i] != 0u)
                {
                    paid[which] += rewards[i];
                    ++ended[which];
                }
            }
        }
    }

    // The same rows, the same die, the same climbs ending in the same places.
    REQUIRE(ended[0] > 0);
    REQUIRE(ended[0] == ended[1]);
    CHECK(paid[1] == doctest::Approx(paid[0]));
}

TEST_CASE("A row of climbs is seeded one after another")
{
    VecSpireEnv row(4);

    row.Reset(CardColor::RED, 100);

    CHECK(row.GetCount() == 4u);

    for (std::size_t i = 0; i < row.GetCount(); ++i)
    {
        SpireEnv alone;

        alone.Reset(CardColor::RED, 100 + static_cast<unsigned int>(i));

        CHECK(row.At(i).Observe() == alone.Observe());
    }
}

TEST_CASE("A row steps the same way as the climbs would on their own")
{
    VecSpireEnv row(3);

    row.Reset(CardColor::GREEN, 21);

    std::vector<SpireEnv> alone(3);

    for (std::size_t i = 0; i < 3u; ++i)
    {
        alone[i].Reset(CardColor::GREEN, 21 + static_cast<unsigned int>(i));
    }

    std::vector<unsigned char> mask(3u * SpireEnv::ActionCount(), 0u);
    std::vector<std::size_t> actions(3u, 0u);
    std::vector<float> rewards(3u, 0.0f);
    std::vector<unsigned char> dones(3u, 0u);
    std::vector<unsigned char> taken(3u, 0u);

    row.SetAutoReset(false);

    for (int tick = 0; tick < 30; ++tick)
    {
        row.ActionMask(mask.data());

        for (std::size_t i = 0; i < 3u; ++i)
        {
            actions[i] = FirstLegal(mask, i);
        }

        row.Step(actions.data(), rewards.data(), dones.data(), taken.data(),
                 nullptr, nullptr);

        for (std::size_t i = 0; i < 3u; ++i)
        {
            const StepResult result = alone[i].StepIndex(actions[i]);

            CHECK(result.reward == rewards[i]);
            CHECK(result.taken == (taken[i] != 0u));
            CHECK(row.At(i).Observe() == alone[i].Observe());
        }
    }
}

TEST_CASE("A climb that ends starts another one on its own")
{
    VecSpireEnv row(2);

    row.Reset(CardColor::RED, 8);

    std::mt19937 rng(4);
    std::vector<unsigned char> mask(2u * SpireEnv::ActionCount(), 0u);
    std::vector<std::size_t> actions(2u, 0u);
    std::vector<unsigned char> dones(2u, 0u);
    std::vector<float> returns(2u, 0.0f);
    std::vector<int> lengths(2u, 0);

    int ended = 0;

    for (int tick = 0; tick < 3000 && ended < 3; ++tick)
    {
        row.ActionMask(mask.data());

        for (std::size_t i = 0; i < 2u; ++i)
        {
            std::vector<std::size_t> open;
            const std::size_t stride = SpireEnv::ActionCount();

            for (std::size_t slot = 0; slot < stride; ++slot)
            {
                if (mask[i * stride + slot] != 0u)
                {
                    open.emplace_back(slot);
                }
            }

            REQUIRE(open.empty() == false);

            std::uniform_int_distribution<std::size_t> pick(
                0, open.size() - 1);

            actions[i] = open[pick(rng)];
        }

        row.Step(actions.data(), nullptr, dones.data(), nullptr,
                 returns.data(), lengths.data());

        for (std::size_t i = 0; i < 2u; ++i)
        {
            if (dones[i] == 0u)
            {
                continue;
            }

            ++ended;

            // What it came to is reported on the tick it ended, and the next
            // climb is already standing at the bottom.
            CHECK(lengths[i] > 0);
            CHECK(row.At(i).GetPhase() == EnvPhase::MAP);
            CHECK(row.At(i).GetTotalFloors() == 0);
            CHECK(row.At(i).IsDone() == false);
        }
    }

    CHECK(ended >= 3);
}

TEST_CASE("A row told not to start over stays where it is")
{
    VecSpireEnv row(1);

    row.Reset(CardColor::RED, 8);
    row.SetAutoReset(false);

    std::mt19937 rng(2);
    std::vector<unsigned char> mask(SpireEnv::ActionCount(), 0u);
    std::vector<std::size_t> actions(1u, 0u);
    std::vector<unsigned char> dones(1u, 0u);

    for (int tick = 0; tick < 3000; ++tick)
    {
        row.ActionMask(mask.data());

        std::vector<std::size_t> open;

        for (std::size_t slot = 0; slot < mask.size(); ++slot)
        {
            if (mask[slot] != 0u)
            {
                open.emplace_back(slot);
            }
        }

        if (open.empty())
        {
            break;
        }

        std::uniform_int_distribution<std::size_t> pick(0, open.size() - 1);

        actions[0] = open[pick(rng)];
        row.Step(actions.data(), nullptr, dones.data(), nullptr, nullptr,
                 nullptr);

        if (dones[0] != 0u)
        {
            break;
        }
    }

    CHECK(row.At(0).IsDone() == true);
    CHECK(row.At(0).GetTotalFloors() > 0);
}

TEST_CASE("The rows of a batch are written one after another")
{
    VecSpireEnv row(3);

    row.Reset(CardColor::BLUE, 55);

    std::vector<float> states(3u * SpireEnv::ObservationSize(), 0.0f);
    std::vector<int> ids(3u * SpireEnv::IdCount(), 0);
    std::vector<unsigned char> mask(3u * SpireEnv::ActionCount(), 0u);

    row.Observe(states.data());
    row.ObserveIds(ids.data());
    row.ActionMask(mask.data());

    for (std::size_t i = 0; i < 3u; ++i)
    {
        const std::vector<float> one = row.At(i).Observe();
        const std::vector<int> mine = row.At(i).ObserveIds();
        const std::vector<unsigned char> allowed = row.At(i).ActionMask();

        for (std::size_t slot = 0; slot < one.size(); ++slot)
        {
            REQUIRE(states[i * one.size() + slot] == one[slot]);
        }

        for (std::size_t slot = 0; slot < mine.size(); ++slot)
        {
            REQUIRE(ids[i * mine.size() + slot] == mine[slot]);
        }

        for (std::size_t slot = 0; slot < allowed.size(); ++slot)
        {
            REQUIRE(mask[i * allowed.size() + slot] == allowed[slot]);
        }
    }
}

TEST_CASE("A die can play whole climbs on this side of the wall")
{
    const std::size_t runs = 20;
    std::vector<float> returns(runs, 0.0f);
    std::vector<int> floors(runs, 0);
    std::vector<int> steps(runs, 0);

    VecSpireEnv::RollRandom(CardColor::RED, 1, runs, returns.data(),
                            floors.data(), steps.data());

    int deepest = 0;

    for (std::size_t run = 0; run < runs; ++run)
    {
        CHECK(steps[run] > 0);
        CHECK(floors[run] > 0);
        CHECK(returns[run] > -1000.0f);
        CHECK(returns[run] < 1000.0f);

        deepest = std::max(deepest, floors[run]);
    }

    CHECK(deepest > 1);

    // And the same seed gives the same twenty climbs.
    std::vector<float> again(runs, 0.0f);

    VecSpireEnv::RollRandom(CardColor::RED, 1, runs, again.data(), nullptr,
                            nullptr);

    CHECK(again == returns);
}

TEST_CASE("A row of one is still a row")
{
    VecSpireEnv row(0);

    row.Reset(CardColor::RED, 3);

    CHECK(row.GetCount() == 1u);
    CHECK(row.At(0).GetPhase() == EnvPhase::MAP);
    CHECK(row.At(99).GetPhase() == EnvPhase::MAP);
}

TEST_CASE("A row asked for it starts climbs part-way up, and one not asked "
          "for it holds nothing")
{
    // Every climb starts on the first floor, so the acts the climber loses in
    // are the ones it practises least. A row that is asked to keeps a copy of
    // a climb whenever it comes up into a new act and starts some share of
    // its climbs from one of those copies instead of from the bottom.
    //
    // Both halves are here together because either on its own reads the wrong
    // way: a row that keeps nothing and a row that keeps copies and never
    // uses them both come to no climbs picked up part-way.
    const std::string save = SecondActSave(7u);

    REQUIRE(save.empty() == false);

    int deep[2] = { 0, 0 };
    std::size_t held[2] = { 0u, 0u };

    for (int which = 0; which < 2; ++which)
    {
        VecSpireEnv row(4u);

        row.SetAutoReset(true);
        row.SetDeepShare(which == 0 ? 0.0f : 1.0f);
        row.Reset(CardColor::RED, 41u);

        // Put every climb into the second act, which walking there is the
        // only way to do and which random moves never manage.
        for (std::size_t i = 0; i < row.GetCount(); ++i)
        {
            REQUIRE(row.At(i).Load(save) == true);
        }

        // Then a move nothing will take, so that the climbs are still
        // standing on the second act's map when the row comes to look at
        // them. A real climb does come up into an act on the map - the step
        // that took the boss down leaves it there - but a random move from
        // that map walks straight into a fight, and a climb in a fight cannot
        // be written out.
        std::vector<std::size_t> nowhere(row.GetCount(),
                                         SpireEnv::ActionCount());
        std::vector<unsigned char> taken(row.GetCount(), 1u);

        row.Step(nowhere.data(), nullptr, nullptr, taken.data(), nullptr,
                 nullptr);

        REQUIRE(taken[0] == 0u);

        held[which] = row.GetDeepHeld(2);

        // And now that there is something on the shelf, the climbs that end
        // are started again from it rather than from the bottom.
        deep[which] = PlayedDeep(row, 600, 11u);
    }

    CHECK(held[0] == 0u);
    CHECK(held[1] == 4u);
    CHECK(deep[0] == 0);
    CHECK(deep[1] > 0);

    // And no shelf for an act there is no picking up in.
    VecSpireEnv row(2u);

    row.SetDeepShare(1.0f);
    row.Reset(CardColor::RED, 3u);

    CHECK(row.GetDeepHeld(1) == 0u);
    CHECK(row.GetDeepHeld(4) == 0u);
}

TEST_CASE("A climb picked up part-way up does not carry the last one's count")
{
    // The log is not written into a save, so loading a run leaves whatever
    // was in it - and what was in it is the climb this env was playing
    // before. Without clearing it the floors and the fights of the one being
    // dropped are added to the one being picked up, and every table that
    // reads a summary reads two climbs at once.
    const std::string save = SecondActSave(7u);

    REQUIRE(save.empty() == false);

    SpireEnv env;
    std::mt19937 rng(5u);

    env.Reset(CardColor::RED, 12u);

    for (int tick = 0; tick < 4000 && !env.IsDone(); ++tick)
    {
        const std::vector<Action> moves = env.LegalActions();

        if (moves.empty())
        {
            break;
        }

        std::uniform_int_distribution<std::size_t> pick(0, moves.size() - 1);

        env.Step(moves[pick(rng)]);
    }

    REQUIRE(env.GetRun().GetLog().GetSummary().floors > 0);
    REQUIRE(env.GetRun().GetLog().GetSummary().died > 0);

    REQUIRE(env.Load(save) == true);

    CHECK(env.GetRun().GetLog().GetSummary().floors == 0);
    CHECK(env.GetRun().GetLog().GetSummary().died == 0);
    CHECK(env.GetRun().GetLog().GetSummary().fightsWon == 0);
}

TEST_CASE("A climb picked up part-way up is left out of the tables")
{
    // Its fights were reached with a deck the first act did not build and its
    // ending is one act's worth of danger rather than three, so letting it
    // into the tables would move every share in them without any of the
    // choices behind them having changed.
    const std::string save = SecondActSave(7u);

    REQUIRE(save.empty() == false);

    std::size_t rows[2] = { 0u, 0u };

    for (int which = 0; which < 2; ++which)
    {
        SpireEnv env;
        std::mt19937 rng(5u);

        env.ClearStats();

        REQUIRE(env.Load(save) == true);

        if (which == 1)
        {
            env.NoteStartedDeep();
        }

        CHECK(env.StartedDeep() == (which == 1));

        for (int tick = 0; tick < 4000 && !env.IsDone(); ++tick)
        {
            const std::vector<Action> moves = env.LegalActions();

            if (moves.empty())
            {
                break;
            }

            std::uniform_int_distribution<std::size_t> pick(
                0, moves.size() - 1);

            env.Step(moves[pick(rng)]);
        }

        REQUIRE(env.IsDone() == true);

        rows[which] = env.GetStats().GetRowCount();
    }

    CHECK(rows[0] > 0u);
    CHECK(rows[1] == 0u);
}

TEST_CASE("A shelf holds the middle of an act and not only its doorway")
{
    // An act is only ever entered whole: the climber rests before a boss and
    // walks through the door at four fifths of its health. So a shelf filled
    // at the doors is all of a climb that is doing well, and what the climber
    // actually loses is the middle - it walks into the second act at 83% of
    // its health and into the fight that kills it at 35%, and a state like
    // that was never on the shelf to be handed back.
    //
    // A copy every floor puts the whole act on the shelf. What says so is
    // that the shelf outgrows the row: with a copy only at the door, a row of
    // four climbs can never hold more than the four that walked through it
    // this pass, however long it plays.
    const std::string save = SecondActSave(7u);

    REQUIRE(save.empty() == false);

    VecSpireEnv row(4u);

    row.SetAutoReset(true);
    row.SetDeepShare(1.0f);
    row.Reset(CardColor::RED, 41u);

    for (std::size_t i = 0; i < row.GetCount(); ++i)
    {
        REQUIRE(row.At(i).Load(save) == true);
    }

    // A move nothing will take, so that the climbs are standing on the second
    // act's map when the row first looks at them.
    std::vector<std::size_t> nowhere(row.GetCount(),
                                     SpireEnv::ActionCount());

    row.Step(nowhere.data(), nullptr, nullptr, nullptr, nullptr, nullptr);

    const std::size_t atTheDoor = row.GetDeepHeld(2);

    CHECK(atTheDoor == row.GetCount());

    PlayedDeep(row, 1200, 11u);

    // Every climb that ends is started again from the shelf, so a row that
    // only kept doorways would be stuck at what it had: the climbs it hands
    // back are already inside the act and never walk through the door again.
    CHECK(row.GetDeepHeld(2) > atTheDoor * 4u);
}

TEST_CASE("Asking what a move comes to leaves the climb where it was")
{
    // A policy that names a move is guessing what it comes to. This says: the
    // climb is copied, the move walked on the copy, and the copy read. What
    // has to be true is that the asking is free - the climbs must stand
    // exactly where they were - and that the answers are actually different
    // from one another, or there is nothing in them to choose by.
    const std::size_t rows = 8u;
    const std::size_t stride = SpireEnv::ActionCount();
    const std::size_t floats = SpireEnv::ObservationSize();
    const std::size_t asked = 3u;

    VecSpireEnv row(rows);

    // Started over as they end, so that every row has something legal to be
    // asked about. Walked with the first legal move rather than at random,
    // because that one dawdles in a fight instead of walking out of it.
    row.SetAutoReset(true);
    row.Reset(CardColor::RED, 8u);

    std::vector<unsigned char> mask(rows * stride, 0u);
    std::vector<std::size_t> actions(rows, 0u);
    std::mt19937 rng(5u);

    // Walk them all into a fight, where the asking is worth anything.
    for (int tick = 0; tick < 40; ++tick)
    {
        row.ActionMask(mask.data());

        for (std::size_t i = 0; i < rows; ++i)
        {
            actions[i] = FirstLegal(mask, i);
        }

        row.Step(actions.data(), nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    std::vector<float> before(rows * floats, 0.0f);

    row.Observe(before.data());
    row.ActionMask(mask.data());

    // Three legal moves apiece, or the same one over again where there are
    // fewer - what matters is that they are legal.
    std::vector<std::size_t> moves(rows * asked, 0u);

    for (std::size_t i = 0; i < rows; ++i)
    {
        std::vector<std::size_t> open;

        for (std::size_t slot = 0; slot < stride; ++slot)
        {
            if (mask[i * stride + slot] != 0u)
            {
                open.emplace_back(slot);
            }
        }

        REQUIRE(open.empty() == false);

        for (std::size_t which = 0; which < asked; ++which)
        {
            moves[i * asked + which] = open[which % open.size()];
        }
    }

    static_cast<void>(rng);

    std::vector<float> peeked(rows * asked * floats, 0.0f);
    std::vector<int> named(rows * asked * SpireEnv::IdCount(), 0);
    std::vector<unsigned char> over(rows * asked, 0u);

    std::vector<float> paid(rows * asked, 0.0f);

    row.Peek(moves.data(), asked, peeked.data(), named.data(), paid.data(),
             over.data());

    // Nothing moved.
    std::vector<float> after(rows * floats, 0.0f);

    row.Observe(after.data());

    CHECK(after == before);

    // And what came back is what the move would have led to, not what is
    // standing there now. Told to walk the same move, the row lands on what
    // the asking said it would.
    std::size_t different = 0;

    for (std::size_t i = 0; i < rows; ++i)
    {
        const float* was = before.data() + i * floats;
        const float* got = peeked.data() + i * asked * floats;

        different += std::equal(was, was + floats, got) ? 0u : 1u;
    }

    CHECK(different > 0u);

    for (std::size_t i = 0; i < rows; ++i)
    {
        actions[i] = moves[i * asked];
    }

    row.Step(actions.data(), nullptr, nullptr, nullptr, nullptr, nullptr);
    row.Observe(after.data());

    for (std::size_t i = 0; i < rows; ++i)
    {
        const float* said = peeked.data() + i * asked * floats;
        const float* is = after.data() + i * floats;

        CHECK(std::equal(said, said + floats, is) == true);
    }
}
