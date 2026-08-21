#include "doctest.h"

#include <conquer-the-spire/Rl/VecSpireEnv.hpp>

#include <algorithm>
#include <cstddef>
#include <random>
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
}  // namespace

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
