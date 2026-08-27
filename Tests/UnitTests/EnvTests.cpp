#include "doctest.h"

#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Relics/RelicRegistry.hpp>
#include <conquer-the-spire/Rl/SpireEnv.hpp>
#include <conquer-the-spire/Run/RunStats.hpp>

#include <algorithm>
#include <cstddef>
#include <random>
#include <set>
#include <string>
#include <vector>

using namespace ConquerTheSpire;

namespace
{
//! Plays a whole climb with a die, and reports what happened.
struct Rollout
{
    int steps = 0;
    int floors = 0;
    float reward = 0.0f;
    bool done = false;
    bool stuck = false;
    EnvPhase last = EnvPhase::INVALID;
};

Rollout Play(SpireEnv& env, unsigned int seed, int limit = 4000)
{
    std::mt19937 rng(seed);
    Rollout out;

    while (!env.IsDone() && out.steps < limit)
    {
        const std::vector<Action> moves = env.LegalActions();

        if (moves.empty())
        {
            out.stuck = true;
            out.last = env.GetPhase();
            break;
        }

        std::uniform_int_distribution<std::size_t> pick(0,
                                                        moves.size() - 1);
        const StepResult step = env.Step(moves[pick(rng)]);

        if (!step.taken)
        {
            out.stuck = true;
            out.last = env.GetPhase();
            break;
        }

        out.reward += step.reward;
        ++out.steps;
    }

    out.done = env.IsDone();
    out.floors = env.GetTotalFloors();
    out.last = env.GetPhase();

    return out;
}
}  // namespace

TEST_CASE("A run written out and read back is the same run")
{
    Run first(CardColor::RED, 77);

    // Get some state on it worth keeping: a few floors, some gold, a relic,
    // a card and a potion.
    REQUIRE(first.Travel(first.GetAvailableColumns().front()) == true);
    first.AddGold(250);
    first.AddRelic(RelicId::AKABEKO);
    first.AddCardToDeck(CardRegistry::Get(CardId::CLEAVE, 1));
    first.AddPotion(PotionId::FIRE_POTION);
    first.TakeKey(KeyType::RUBY);

    const std::string save = first.Serialize();

    Run second(CardColor::BLUE, 1);

    REQUIRE(second.Load(save) == true);

    CHECK(second.GetCharacter() == CardColor::RED);
    CHECK(second.GetFloor() == first.GetFloor());
    CHECK(second.GetColumn() == first.GetColumn());
    CHECK(second.GetGold() == first.GetGold());
    CHECK(second.GetAct() == first.GetAct());
    CHECK(second.HasKey(KeyType::RUBY) == true);
    CHECK(second.GetPlayer().GetHealth() == first.GetPlayer().GetHealth());
    CHECK(second.GetPlayer().GetMaxHealth() ==
          first.GetPlayer().GetMaxHealth());
    CHECK(second.GetPlayer().HasRelic(RelicId::AKABEKO) == true);
    CHECK(second.GetPlayer().GetPotions().size() == 1u);
    CHECK(second.GetDeck().size() == first.GetDeck().size());

    // The deck comes back card for card, upgrades and all.
    for (std::size_t i = 0; i < first.GetDeck().size(); ++i)
    {
        CHECK(second.GetDeck()[i].GetId() == first.GetDeck()[i].GetId());
        CHECK(second.GetDeck()[i].GetUpgradeCount() ==
              first.GetDeck()[i].GetUpgradeCount());
    }

    // And the map comes back node for node.
    for (int row = 0; row < Map::ROWS; ++row)
    {
        for (int column = 0; column < Map::COLUMNS; ++column)
        {
            const MapNode& one = first.GetMap().GetNode(row, column);
            const MapNode& two = second.GetMap().GetNode(row, column);

            CHECK(two.type == one.type);
            CHECK(two.exists == one.exists);
            CHECK(two.nextColumns == one.nextColumns);
        }
    }

    // Writing the second one out again gives the same text.
    CHECK(second.Serialize() == save);
}

TEST_CASE("A run read back rolls the same dice")
{
    Run first(CardColor::GREEN, 31);

    for (int i = 0; i < 3; ++i)
    {
        first.Travel(first.GetAvailableColumns().front());
    }

    Run second(CardColor::RED, 9);

    REQUIRE(second.Load(first.Serialize()) == true);

    // The same chest, out of the same books, with the same dice behind it.
    const ChestSize one = first.OpenChest();
    const ChestSize two = second.OpenChest();

    CHECK(one == two);
    REQUIRE(first.GetRewards().size() == second.GetRewards().size());

    for (std::size_t i = 0; i < first.GetRewards().size(); ++i)
    {
        CHECK(second.GetRewards()[i].kind == first.GetRewards()[i].kind);
        CHECK(second.GetRewards()[i].amount == first.GetRewards()[i].amount);
        CHECK(second.GetRewards()[i].relics == first.GetRewards()[i].relics);
    }

    CHECK(first.Serialize() == second.Serialize());
}

TEST_CASE("A save from another shape of the engine is turned away")
{
    Run run(CardColor::RED, 5);

    CHECK(run.Load("") == false);
    CHECK(run.Load("cts 99\n") == false);
    CHECK(run.Load("nonsense 1\n") == false);
}

TEST_CASE("The books a run rolls from survive being written out")
{
    Run first(CardColor::RED, 44);

    // Draw a few relics out of the pool, so the pools are no longer whole.
    first.AddGold(2000);
    first.OpenShop();

    const std::size_t common =
        first.GetRewardGenerator().CountRemaining(RelicTier::COMMON);
    const int chance = first.GetRewardGenerator().GetPotionChance();

    Run second(CardColor::RED, 1);

    REQUIRE(second.Load(first.Serialize()) == true);

    CHECK(second.GetRewardGenerator().CountRemaining(RelicTier::COMMON) ==
          common);
    CHECK(second.GetRewardGenerator().GetPotionChance() == chance);
    CHECK(second.GetShop().GetCards().size() ==
          first.GetShop().GetCards().size());
    CHECK(second.GetShop().GetRemovalPrice() ==
          first.GetShop().GetRemovalPrice());

    for (std::size_t i = 0; i < first.GetShop().GetCards().size(); ++i)
    {
        CHECK(second.GetShop().GetCards()[i].id ==
              first.GetShop().GetCards()[i].id);
        CHECK(second.GetShop().GetCards()[i].price ==
              first.GetShop().GetCards()[i].price);
    }
}

TEST_CASE("An environment opens on the map with somewhere to walk")
{
    SpireEnv env;

    env.Reset(CardColor::RED, 12);

    CHECK(env.GetPhase() == EnvPhase::MAP);
    CHECK(env.IsDone() == false);
    CHECK(env.GetBattle() == nullptr);
    CHECK(env.LegalActions().empty() == false);

    for (const Action& move : env.LegalActions())
    {
        CHECK(move.kind == ActionKind::TRAVEL);
    }
}

TEST_CASE("The state is a vector of the size it says it is")
{
    SpireEnv env;

    env.Reset(CardColor::BLUE, 3);

    const std::vector<float> state = env.Observe();

    CHECK(state.size() == SpireEnv::ObservationSize());
    CHECK(state.size() > 100u);

    // Nothing in it is wild.
    for (const float value : state)
    {
        CHECK(value >= -2.0f);
        CHECK(value <= 3.0f);
    }
}

TEST_CASE("An illegal move changes nothing")
{
    SpireEnv env;

    env.Reset(CardColor::RED, 8);

    const std::vector<float> before = env.Observe();
    const StepResult result = env.Step(Action(ActionKind::END_TURN));

    CHECK(result.taken == false);
    CHECK(result.reward == 0.0f);
    CHECK(env.Observe() == before);
}

TEST_CASE("Walking into a room opens whatever is in it")
{
    SpireEnv env;

    env.Reset(CardColor::RED, 8);

    const std::vector<Action> moves = env.LegalActions();

    REQUIRE(moves.empty() == false);
    REQUIRE(env.Step(moves.front()).taken == true);

    // The first row is always a fight, so the environment is in one.
    CHECK(env.GetPhase() == EnvPhase::BATTLE);
    CHECK(env.GetBattle() != nullptr);
    CHECK(env.GetTotalFloors() == 1);

    bool endTurn = false;
    bool playCard = false;

    for (const Action& move : env.LegalActions())
    {
        endTurn = endTurn || move.kind == ActionKind::END_TURN;
        playCard = playCard || move.kind == ActionKind::PLAY_CARD;
    }

    CHECK(endTurn == true);
    CHECK(playCard == true);
}

TEST_CASE("A die can play a whole climb without the engine getting stuck")
{
    for (unsigned int seed = 1; seed < 8; ++seed)
    {
        SpireEnv env;

        env.Reset(CardColor::RED, seed);

        const Rollout out = Play(env, seed);

        CHECK(out.stuck == false);
        CHECK(out.steps > 10);
        CHECK(out.floors > 0);

        // A die usually dies, and either way the run ends somewhere real.
        CHECK((out.done || out.steps >= 4000) == true);
    }
}

TEST_CASE("Every character can be played by a die")
{
    const CardColor characters[] = { CardColor::RED, CardColor::GREEN,
                                     CardColor::BLUE };

    for (const CardColor character : characters)
    {
        SpireEnv env;

        env.Reset(character, 21);

        const Rollout out = Play(env, 21);

        CHECK(out.stuck == false);
        CHECK(out.floors > 0);
    }
}

TEST_CASE("The same seed gives the same climb")
{
    SpireEnv one;
    SpireEnv two;

    one.Reset(CardColor::RED, 55);
    two.Reset(CardColor::RED, 55);

    const Rollout first = Play(one, 4);
    const Rollout second = Play(two, 4);

    CHECK(first.steps == second.steps);
    CHECK(first.floors == second.floors);
    CHECK(first.reward == second.reward);
    CHECK(one.Observe() == two.Observe());
}

TEST_CASE("A climb picked up from a save carries on the same way")
{
    SpireEnv first;
    std::string save;

    // Walk until the environment is somewhere a save can be taken. Over
    // several climbs rather than one: a climb that dies in its first fight
    // never reaches a map to be saved from, and which climbs do that moves
    // whenever anything about a monster changes. Pinning one seed made this
    // fail for reasons that had nothing to do with saving.
    for (unsigned int seed = 65; seed < 85u && save.empty(); ++seed)
    {
        first.Reset(CardColor::RED, seed);

        std::mt19937 rng(2);

        for (int i = 0; i < 400 && save.empty(); ++i)
        {
            const std::vector<Action> moves = first.LegalActions();

            if (moves.empty())
            {
                break;
            }

            std::uniform_int_distribution<std::size_t> pick(
                0, moves.size() - 1);

            first.Step(moves[pick(rng)]);

            if (first.GetPhase() == EnvPhase::MAP &&
                first.GetRun().GetFloor() > 1)
            {
                save = first.Save();
            }
        }
    }

    REQUIRE(save.empty() == false);

    SpireEnv second;

    REQUIRE(second.Load(save) == true);

    CHECK(second.GetPhase() == first.GetPhase());
    CHECK(second.GetTotalFloors() == first.GetTotalFloors());
    CHECK(second.Observe() == first.Observe());

    // And the two carry on together.
    const Rollout one = Play(first, 9);
    const Rollout two = Play(second, 9);

    CHECK(one.steps == two.steps);
    CHECK(one.floors == two.floors);
    CHECK(one.reward == two.reward);
}

TEST_CASE("A save taken in a fight says so rather than lying")
{
    SpireEnv env;

    env.Reset(CardColor::RED, 8);

    REQUIRE(env.Step(env.LegalActions().front()).taken == true);
    REQUIRE(env.GetPhase() == EnvPhase::BATTLE);

    CHECK(env.Save().empty() == true);
}

TEST_CASE("Climbing pays and dying costs")
{
    SpireEnv env;

    env.Reset(CardColor::RED, 8);

    const StepResult walked = env.Step(env.LegalActions().front());

    CHECK(walked.reward >= SpireEnv::FLOOR_REWARD - 0.001f);

    // Play it out and see that the ledger adds up to something.
    const Rollout out = Play(env, 8);

    CHECK(out.reward > -1000.0f);
    CHECK(out.reward < 1000.0f);
}

TEST_CASE("Nothing that cannot be aimed at is offered as a target")
{
    // The two lists have to agree at the edges. What is in the room is what
    // the state is built from, and the ordinal a move names is an offset into
    // that - so a monster that may not be aimed at keeps its place there and
    // has to be left out of what is offered instead. Offering it and then
    // turning the move away is what sends a policy round in circles.
    //
    // Done to whatever the first fight holds rather than to a darkling in
    // particular: the rule is about the two lists, not about one monster.
    SpireEnv env;
    bool fought = false;

    for (unsigned int seed = 1; seed <= 30u && !fought; ++seed)
    {
        env.Reset(CardColor::RED, seed);

        for (int step = 0; step < 200 && !env.IsDone(); ++step)
        {
            if (env.GetPhase() == EnvPhase::BATTLE &&
                env.GetBattle() != nullptr &&
                env.GetBattle()->GetMonsters().size() >= 2u)
            {
                fought = true;
                break;
            }

            const std::vector<Action> moves = env.LegalActions();

            if (moves.empty() || !env.Step(moves.front()).taken)
            {
                break;
            }
        }
    }

    REQUIRE(fought == true);

    Battle& battle = *env.GetBattle();

    // Knocked down and out of reach, whoever it happens to be.
    battle.GetMonsters()[0].SetHiddenWhenDown(true);
    battle.GetMonsters()[0].SetRegrowing(true);
    battle.GetMonsters()[0].SetHealth(0);

    const std::vector<std::size_t> living = battle.GetLivingMonsterIndices();

    // Still in the room, so the state still shows it and nobody behind it
    // has shifted a place along.
    REQUIRE(living.empty() == false);
    CHECK(living.front() == 0u);
    CHECK(battle.GetTargetableMonsterIndices().empty() == false);
    CHECK(battle.GetTargetableMonsterIndices().front() != 0u);

    int offered = 0;

    for (const Action& move : env.LegalActions())
    {
        if (move.kind != ActionKind::PLAY_CARD &&
            move.kind != ActionKind::USE_POTION)
        {
            continue;
        }

        ++offered;

        // Never the one lying down.
        CHECK(move.b != 0);

        const std::size_t which =
            static_cast<std::size_t>(move.b) < living.size()
                ? living[static_cast<std::size_t>(move.b)]
                : battle.GetMonsters().size();

        if (move.kind == ActionKind::PLAY_CARD)
        {
            CHECK(battle.CanPlay(static_cast<std::size_t>(move.a), which) ==
                  true);
        }
        else
        {
            CHECK(battle.CanUsePotion(static_cast<std::size_t>(move.a),
                                      which) == true);
        }
    }

    CHECK(offered > 0);
}

TEST_CASE("A climb asked for one act is finished when it clears that act")
{
    // A climb trained on a single act that puts that act's boss down was
    // going into the table as neither a win nor a death, so the one thing
    // such a run is trying to learn to do never showed in the win column at
    // all. The Run cannot say this - it only knows about the spire's own top
    // - so the env says it where the limit stops the climb.
    //
    // Driven rather than played: every monster met is put on one health, so
    // the climb reaches the top of the act instead of dying somewhere in the
    // middle the way random play does. What is under test is the accounting,
    // not the fighting.
    std::mt19937 rng(3);
    SpireEnv env;

    env.SetActLimit(1);
    env.Reset(CardColor::RED, 11);

    int walked = 0;

    while (!env.IsDone() && walked < 4000)
    {
        env.GetRun().GetPlayer().SetHealth(
            env.GetRun().GetPlayer().GetMaxHealth());

        if (Battle* fight = env.GetBattle(); fight != nullptr)
        {
            fight->GetPlayer().SetHealth(fight->GetPlayer().GetMaxHealth());

            for (Monster& one : fight->GetMonsters())
            {
                if (!one.IsGone())
                {
                    one.SetHealth(1);
                }
            }
        }

        const std::vector<Action> moves = env.LegalActions();

        if (moves.empty())
        {
            break;
        }

        // An attack if there is one, so the fight ends; otherwise anything.
        std::size_t chose = moves.size();

        for (std::size_t at = 0; at < moves.size(); ++at)
        {
            if (moves[at].kind == ActionKind::PLAY_CARD)
            {
                chose = at;
                break;
            }
        }

        if (chose == moves.size())
        {
            std::uniform_int_distribution<std::size_t> pick(
                0, moves.size() - 1);

            chose = pick(rng);
        }

        if (!env.Step(moves[chose]).taken)
        {
            std::uniform_int_distribution<std::size_t> pick(
                0, moves.size() - 1);

            env.Step(moves[pick(rng)]);
        }

        ++walked;
    }

    REQUIRE(env.IsDone() == true);
    REQUIRE(env.GetRun().GetPlayer().GetHealth() > 0);
    REQUIRE(env.GetRun().GetAct() == 1);

    RunStats stats;

    stats.Ingest(env.GetRun().GetLog());

    // It got as far as it was asked to get, so it is finished and not merely
    // stopped: one climb, one win, no death.
    CHECK(stats.GetRuns() == 1);
    CHECK(stats.GetWins() == 1);
    CHECK(stats.GetDeaths() == 0);
}
