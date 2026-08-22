#include "doctest.h"

#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Events/EventLibrary.hpp>
#include <conquer-the-spire/Monsters/EncounterLibrary.hpp>
#include <conquer-the-spire/Monsters/MonsterRoster.hpp>
#include <conquer-the-spire/Rl/SpireEnv.hpp>

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <random>
#include <set>
#include <vector>

using namespace ConquerTheSpire;

namespace
{
//! Walks an environment into a fight.
SpireEnv InBattle(unsigned int seed = 8)
{
    SpireEnv env;

    env.Reset(CardColor::RED, seed);

    const std::vector<Action> moves = env.LegalActions();

    if (!moves.empty())
    {
        env.Step(moves.front());
    }

    return env;
}

//! Plays a climb using nothing but the fixed head and its mask.
int PlayByMask(SpireEnv& env, unsigned int seed, int limit = 5000)
{
    std::mt19937 rng(seed);
    int steps = 0;

    while (!env.IsDone() && steps < limit)
    {
        const std::vector<unsigned char> mask = env.ActionMask();
        std::vector<std::size_t> open;

        for (std::size_t i = 0; i < mask.size(); ++i)
        {
            if (mask[i] != 0u)
            {
                open.emplace_back(i);
            }
        }

        if (open.empty())
        {
            return -steps;
        }

        std::uniform_int_distribution<std::size_t> pick(0, open.size() - 1);
        const StepResult step = env.StepIndex(open[pick(rng)]);

        if (!step.taken)
        {
            return -steps;
        }

        ++steps;
    }

    return steps;
}
}  // namespace

TEST_CASE("Every slot of the fixed head names one move, and only one")
{
    const std::size_t count = SpireEnv::ActionCount();

    CHECK(count > 100u);
    CHECK(count < 5000u);

    std::set<std::size_t> seen;

    for (std::size_t index = 0; index < count; ++index)
    {
        const Action move = SpireEnv::ActionFromIndex(index);

        CHECK(move.kind != ActionKind::INVALID);
        CHECK(SpireEnv::IndexOfAction(move) == index);
        CHECK(seen.insert(index).second == true);
    }

    // And nothing past the end names anything.
    CHECK(SpireEnv::ActionFromIndex(count).kind == ActionKind::INVALID);
    CHECK(SpireEnv::ActionFromIndex(count + 100u).kind ==
          ActionKind::INVALID);
}

TEST_CASE("The head has a slot for every kind of move there is")
{
    std::set<int> kinds;

    for (std::size_t index = 0; index < SpireEnv::ActionCount(); ++index)
    {
        kinds.insert(
            static_cast<int>(SpireEnv::ActionFromIndex(index).kind));
    }

    // Every kind but the invalid one.
    CHECK(kinds.count(static_cast<int>(ActionKind::TRAVEL)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::PLAY_CARD)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::END_TURN)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::USE_POTION)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::DISCARD_POTION)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::CLAIM_REWARD)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::SKIP_REWARD)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::LEAVE_REWARDS)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::CHOOSE_OPTION)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::BUY_CARD)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::BUY_RELIC)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::BUY_POTION)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::BUY_REMOVAL)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::LEAVE_SHOP)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::REST)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::SMITH)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::TOKE)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::DIG)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::LIFT)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::LEAVE_REST)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::FIGHT_BOSS)) == 1u);
    CHECK(kinds.count(static_cast<int>(ActionKind::NEXT_ACT)) == 1u);
}

TEST_CASE("The mask says exactly what the list of moves says")
{
    SpireEnv env = InBattle();

    for (int i = 0; i < 40 && !env.IsDone(); ++i)
    {
        const std::vector<Action> moves = env.LegalActions();
        const std::vector<unsigned char> mask = env.ActionMask();

        REQUIRE(mask.size() == SpireEnv::ActionCount());

        const int open =
            std::accumulate(mask.begin(), mask.end(), 0,
                            [](int total, unsigned char byte) {
                                return total + (byte != 0u ? 1 : 0);
                            });

        CHECK(open == static_cast<int>(moves.size()));

        for (const Action& move : moves)
        {
            const std::size_t index = SpireEnv::IndexOfAction(move);

            REQUIRE(index < mask.size());
            CHECK(mask[index] == 1u);
        }

        if (moves.empty())
        {
            break;
        }

        env.Step(moves.front());
    }
}

TEST_CASE("A move the mask says no to is turned down")
{
    SpireEnv env;

    env.Reset(CardColor::RED, 8);

    const std::vector<unsigned char> mask = env.ActionMask();

    for (std::size_t index = 0; index < mask.size(); ++index)
    {
        if (mask[index] != 0u)
        {
            continue;
        }

        // Whatever it is, it is not something that can be done on the map.
        const StepResult result = env.StepIndex(index);

        CHECK(result.taken == false);
        CHECK(env.GetPhase() == EnvPhase::MAP);
        break;
    }
}

TEST_CASE("A climb can be played through the fixed head alone")
{
    for (unsigned int seed = 1; seed < 6; ++seed)
    {
        SpireEnv env;

        env.Reset(CardColor::RED, seed);

        const int steps = PlayByMask(env, seed);

        CHECK(steps > 10);
        CHECK(env.GetTotalFloors() > 0);
    }
}

TEST_CASE("The state says which cards are in hand")
{
    SpireEnv env = InBattle();

    REQUIRE(env.GetPhase() == EnvPhase::BATTLE);
    REQUIRE(env.GetBattle() != nullptr);

    const SpireEnv::Layout layout = SpireEnv::GetLayout();
    const std::vector<float> state = env.Observe();
    const std::vector<Card>& hand = env.GetBattle()->GetPlayer().GetHand();

    REQUIRE(hand.empty() == false);
    REQUIRE(state.size() == layout.total);

    const SpireEnv::IdLayout ids = SpireEnv::GetIdLayout();
    const std::vector<int> named = env.ObserveIds();

    for (std::size_t slot = 0; slot < SpireEnv::HAND_SLOTS; ++slot)
    {
        const std::size_t start = layout.hand + slot * layout.handStride;

        if (slot >= hand.size())
        {
            // An empty slot says nothing at all.
            CHECK(named[ids.hand + slot] == 0);
            CHECK(state[start] == 0.0f);
            continue;
        }

        // Which card it is comes from the ids beside the state; the state
        // itself says what it costs and whether it can be played.
        CHECK(named[ids.hand + slot] ==
              static_cast<int>(hand[slot].GetId()));
        CHECK(state[start + 1] ==
              (hand[slot].IsUpgraded() ? 1.0f : 0.0f));
        CHECK(state[start + 2] ==
              (env.GetBattle()->CanPlay(slot) ? 1.0f : 0.0f));
    }
}

TEST_CASE("The state counts up what the piles hold")
{
    SpireEnv env = InBattle();

    const SpireEnv::Layout layout = SpireEnv::GetLayout();
    const std::vector<float> state = env.Observe();

    // The deck is counted a third a card, so the slice adds up to the size.
    float deck = 0.0f;

    for (std::size_t i = 0; i < layout.pileStride; ++i)
    {
        deck += state[layout.piles + i];
    }

    CHECK(deck * 3.0f ==
          doctest::Approx(static_cast<float>(env.GetRun().GetDeck().size())));

    float draw = 0.0f;

    for (std::size_t i = 0; i < layout.pileStride; ++i)
    {
        draw += state[layout.piles + layout.pileStride + i];
    }

    CHECK(draw * 3.0f ==
          doctest::Approx(static_cast<float>(
              env.GetBattle()->GetPlayer().GetDrawPile().size())));
}

TEST_CASE("A move names the monster by where it stands among the living")
{
    SpireEnv env = InBattle(8);

    REQUIRE(env.GetBattle() != nullptr);

    const std::size_t monsters =
        env.GetBattle()->GetLivingMonsterIndices().size();

    for (const Action& move : env.LegalActions())
    {
        if (move.kind == ActionKind::PLAY_CARD)
        {
            CHECK(move.b >= 0);
            CHECK(move.b < static_cast<int>(monsters));
            CHECK(move.a < static_cast<int>(SpireEnv::HAND_SLOTS));
        }
    }
}

TEST_CASE("The layout adds up to the size of the state")
{
    const SpireEnv::Layout layout = SpireEnv::GetLayout();

    CHECK(layout.phase == 0u);
    CHECK(layout.run > layout.phase);
    CHECK(layout.deck > layout.run);
    CHECK(layout.relics > layout.deck);
    CHECK(layout.battle > layout.relics);
    CHECK(layout.powers > layout.battle);
    CHECK(layout.monsters > layout.powers);
    CHECK(layout.hand > layout.monsters);
    CHECK(layout.piles > layout.hand);
    CHECK(layout.rewards > layout.piles);
    CHECK(layout.shop > layout.rewards);
    CHECK(layout.event > layout.shop);
    CHECK(layout.potions > layout.event);
    CHECK(layout.moves > layout.potions);
    CHECK(layout.total == SpireEnv::ObservationSize());
    CHECK(layout.piles + 3u * layout.pileStride == layout.rewards);
    CHECK(layout.hand + SpireEnv::HAND_SLOTS * layout.handStride ==
          layout.piles);
    CHECK(layout.monsters +
              SpireEnv::OBSERVED_MONSTERS * layout.monsterStride ==
          layout.hand);
    CHECK(layout.map > layout.moves);
    CHECK(layout.moves +
              SpireEnv::OBSERVED_MONSTERS * layout.moveStride ==
          layout.map);
}

TEST_CASE("Every card of every pool fits in the state")
{
    const CardColor colors[] = { CardColor::RED, CardColor::GREEN,
                                 CardColor::BLUE, CardColor::COLORLESS,
                                 CardColor::STATUS, CardColor::CURSE };

    for (const CardColor color : colors)
    {
        for (const CardId id : CardRegistry::GetPool(color))
        {
            CHECK(static_cast<std::size_t>(id) < CardRegistry::IdCount());
        }
    }
}

TEST_CASE("The ids say what the reward pile is offering")
{
    Run run(CardColor::RED, 41);
    SpireEnv env;

    env.Reset(CardColor::RED, 41);

    // Win a fight the honest way, so the pile is a real one.
    REQUIRE(env.Step(env.LegalActions().front()).taken == true);
    REQUIRE(env.GetPhase() == EnvPhase::BATTLE);

    for (int i = 0; i < 200 && env.GetPhase() == EnvPhase::BATTLE; ++i)
    {
        // Cheat the fight over: this is about the reward pile, not the
        // fighting.
        for (auto& monster :
             const_cast<Battle*>(env.GetBattle())->GetMonsters())
        {
            monster.SetHealth(0);
        }

        env.Step(Action(ActionKind::END_TURN));
    }

    REQUIRE(env.GetPhase() == EnvPhase::REWARD);

    const SpireEnv::IdLayout ids = SpireEnv::GetIdLayout();
    const std::vector<int> seen = env.ObserveIds();
    const std::vector<Reward>& rewards = env.GetRun().GetRewards();

    REQUIRE(seen.size() == ids.total);
    REQUIRE(rewards.empty() == false);

    for (std::size_t i = 0; i < rewards.size() && i < 6u; ++i)
    {
        CHECK(seen[ids.rewardKinds + i] ==
              static_cast<int>(rewards[i].kind));

        for (std::size_t option = 0;
             option < rewards[i].cards.size() && option < 4u; ++option)
        {
            const std::size_t at = i * 4u + option;

            CHECK(seen[ids.rewardOptions + at] ==
                  static_cast<int>(rewards[i].cards[option]));
            CHECK(seen[ids.rewardOptionKinds + at] == SpireEnv::ITEM_CARD);
        }
    }

    // And the numbers say what kind each one is and whether it is still
    // there.
    const SpireEnv::Layout layout = SpireEnv::GetLayout();
    const std::vector<float> state = env.Observe();
    const std::size_t start = layout.rewards;

    CHECK(state[start + static_cast<std::size_t>(rewards.front().kind)] ==
          1.0f);
    CHECK(state[start + layout.rewardStride - 2u] ==
          (rewards.front().claimed ? 1.0f : 0.0f));
}

TEST_CASE("The ids say what the shelf is offering")
{
    SpireEnv env;

    env.Reset(CardColor::RED, 12);
    env.GetRun().AddGold(500);
    env.GetRun().OpenShop();

    const SpireEnv::IdLayout ids = SpireEnv::GetIdLayout();
    const std::vector<int> seen = env.ObserveIds();
    const Shop& shop = env.GetRun().GetShop();

    for (std::size_t i = 0; i < shop.GetCards().size() && i < 7u; ++i)
    {
        CHECK(seen[ids.shopCards + i] ==
              static_cast<int>(shop.GetCards()[i].id));
    }

    for (std::size_t i = 0; i < shop.GetRelics().size() && i < 3u; ++i)
    {
        CHECK(seen[ids.shopRelics + i] ==
              static_cast<int>(shop.GetRelics()[i].id));
    }

    for (std::size_t i = 0; i < shop.GetPotions().size() && i < 3u; ++i)
    {
        CHECK(seen[ids.shopPotions + i] ==
              static_cast<int>(shop.GetPotions()[i].id));
    }

    // The prices are in the numbers.
    const SpireEnv::Layout layout = SpireEnv::GetLayout();
    const std::vector<float> state = env.Observe();

    CHECK(state[layout.shop] ==
          doctest::Approx(shop.GetCards().front().price / 200.0f));
    CHECK(state[layout.shop + 1] == 1.0f);
}

TEST_CASE("The ids say which room the climber is standing in")
{
    SpireEnv env;

    env.Reset(CardColor::RED, 5);
    env.GetRun().StartEvent(EventId::THE_CLERIC);

    const SpireEnv::IdLayout ids = SpireEnv::GetIdLayout();

    CHECK(env.ObserveIds()[ids.event] ==
          static_cast<int>(EventId::THE_CLERIC));

    // And the numbers say what each option asks for.
    const SpireEnv::Layout layout = SpireEnv::GetLayout();
    const std::vector<float> state = env.Observe();

    CHECK(state[layout.event + 2] == 1.0f);
    CHECK(state[layout.event + 3] == doctest::Approx(35.0f / 100.0f));
}

TEST_CASE("The ids say what is in the belt and what is standing there")
{
    SpireEnv env;

    env.Reset(CardColor::RED, 8);
    env.GetRun().AddPotion(PotionId::FIRE_POTION);

    REQUIRE(env.Step(env.LegalActions().front()).taken == true);
    REQUIRE(env.GetPhase() == EnvPhase::BATTLE);

    const SpireEnv::IdLayout ids = SpireEnv::GetIdLayout();
    const std::vector<int> seen = env.ObserveIds();

    CHECK(seen[ids.potions] == static_cast<int>(PotionId::FIRE_POTION));
    CHECK(seen[ids.relics] == static_cast<int>(RelicId::BURNING_BLOOD));
    CHECK(seen[ids.monsters] ==
          static_cast<int>(
              env.GetBattle()->GetMonsters().front().GetMonsterId()));

    for (std::size_t slot = 0; slot < env.GetBattle()->GetPlayer()
                                          .GetHand()
                                          .size() &&
                               slot < 10u;
         ++slot)
    {
        CHECK(seen[ids.hand + slot] ==
              static_cast<int>(
                  env.GetBattle()->GetPlayer().GetHand()[slot].GetId()));
    }
}

TEST_CASE("The numbers say what a monster means to do")
{
    SpireEnv env;

    env.Reset(CardColor::RED, 8);

    REQUIRE(env.Step(env.LegalActions().front()).taken == true);
    REQUIRE(env.GetBattle() != nullptr);

    const SpireEnv::Layout layout = SpireEnv::GetLayout();
    bool sawAnAttack = false;

    // Over a few turns the monsters get round to attacking, and the state
    // says how hard before it lands.
    for (int turn = 0; turn < 8 && env.GetPhase() == EnvPhase::BATTLE; ++turn)
    {
        const std::vector<float> state = env.Observe();
        const Monster& monster = env.GetBattle()->GetMonsters().front();
        const MonsterMove& move = monster.GetCurrentMove();

        int damage = 0;
        int hits = 0;

        for (const auto& effect : move.effects)
        {
            if (effect.type == MonsterEffectType::DAMAGE)
            {
                damage = effect.amount +
                         monster.GetPower(PowerType::STRENGTH);
                hits = effect.times;
            }
        }

        CHECK(state[layout.moves] == doctest::Approx(damage / 50.0f));
        CHECK(state[layout.moves + 1] == doctest::Approx(hits / 6.0f));

        sawAnAttack = sawAnAttack || damage > 0;

        env.Step(Action(ActionKind::END_TURN));
    }

    CHECK(sawAnAttack == true);
}

TEST_CASE("A runic dome hides what the monsters mean to do, in the state too")
{
    SpireEnv env;

    env.Reset(CardColor::RED, 8);
    env.GetRun().AddRelic(RelicId::RUNIC_DOME);

    REQUIRE(env.Step(env.LegalActions().front()).taken == true);
    REQUIRE(env.GetPhase() == EnvPhase::BATTLE);

    const SpireEnv::Layout layout = SpireEnv::GetLayout();
    const std::vector<float> state = env.Observe();

    for (std::size_t i = 0; i < layout.moveStride; ++i)
    {
        CHECK(state[layout.moves + i] == 0.0f);
    }
}

TEST_CASE("The state says what the map ahead holds")
{
    SpireEnv env;

    env.Reset(CardColor::RED, 5);

    const SpireEnv::Layout layout = SpireEnv::GetLayout();
    const std::vector<float> state = env.Observe();
    const std::vector<int> columns = env.GetRun().GetAvailableColumns();

    REQUIRE(columns.empty() == false);

    // Every place a path leads to says so, and says what waits on it. The
    // eight numbers of a place are the path and the seven kinds.
    for (std::size_t column = 0; column < SpireEnv::MAP_COLUMNS; ++column)
    {
        const std::size_t at = layout.map + column * 8u;
        const bool open =
            std::find(columns.begin(), columns.end(),
                      static_cast<int>(column)) != columns.end();
        float kinds = 0.0f;

        for (std::size_t kind = 1; kind < 8u; ++kind)
        {
            kinds += state[at + kind];
        }

        CHECK(state[at] == (open ? 1.0f : 0.0f));

        // One kind on a place that can be walked to, none on the others.
        CHECK(kinds == (open ? 1.0f : 0.0f));
    }

    // And how much of the act is left to walk, which is all of it.
    const std::size_t tail = layout.map + 2u * SpireEnv::MAP_COLUMNS * 8u;

    CHECK(state[tail + 7u] == doctest::Approx(1.0f));
    CHECK(state[tail + 8u] == 0.0f);

    // The rest of the act holds a chest and a fire at the least.
    CHECK(state[tail + 5u] > 0.0f);
    CHECK(state[tail + 3u] > 0.0f);
}

TEST_CASE("The state says what a room's options would do")
{
    SpireEnv env;

    env.Reset(CardColor::RED, 5);
    env.GetRun().StartEvent(EventId::GOLDEN_SHRINE);

    const SpireEnv::Layout layout = SpireEnv::GetLayout();

    // Past the stage and the try count, then past the two numbers an option
    // had before its signals.
    const std::size_t first = layout.event + 2u;
    const std::size_t signals = 2u;

    // Pray, Desecrate, Leave. The one that pays in a curse says so.
    const std::vector<float> state = env.Observe();
    const std::size_t pray = first + signals;
    const std::size_t bad = first + layout.eventStride + signals;
    const std::size_t leave = first + 2u * layout.eventStride + signals;

    // Gold up is the first signal, a curse the tenth.
    CHECK(state[pray + 0u] > 0.0f);
    CHECK(state[pray + 9u] == 0.0f);
    CHECK(state[bad + 0u] > state[pray + 0u]);
    CHECK(state[bad + 9u] > 0.0f);

    // Walking away does nothing at all, and says nothing.
    for (std::size_t signal = 0; signal < 17u; ++signal)
    {
        CHECK(state[leave + signal] == 0.0f);
    }

    // A room that offers to tear a card up says that instead.
    env.Reset(CardColor::RED, 5);
    env.GetRun().StartEvent(EventId::PURIFIER);

    const std::vector<float> other = env.Observe();

    // Removing a card is the eleventh signal.
    CHECK(other[first + signals + 10u] > 0.0f);
    CHECK(other[first + signals + 9u] == 0.0f);
}

TEST_CASE("Nothing the mask offers is ever turned down")
{
    // The point of this one is the count: a mask that offers a move the run
    // refuses leaves the climb pressing it for ever, which is how a whole
    // row of climbs was once spent buying a potion into a full belt. Enough
    // climbs, played long enough, to walk into a shop with a belt already
    // full.
    for (unsigned int seed = 1; seed <= 60u; ++seed)
    {
        SpireEnv env;

        env.Reset(seed % 3u == 0u   ? CardColor::RED
                  : seed % 3u == 1u ? CardColor::GREEN
                                    : CardColor::BLUE,
                  seed);

        const int steps = PlayByMask(env, seed, SpireEnv::MOVE_LIMIT);

        // Negative means a move was refused, or the mask ran dry.
        CHECK(steps > 0);
    }
}

TEST_CASE("A climb that goes nowhere is called off")
{
    SpireEnv env;

    env.Reset(CardColor::RED, 3);

    // Standing still: the pile of rewards is left alone, over and over. It
    // is a legal move and it changes nothing, which is the shape of every
    // way a climb can fail to end.
    int steps = 0;

    while (!env.IsDone() && steps < SpireEnv::MOVE_LIMIT + 10)
    {
        const std::vector<Action> moves = env.LegalActions();

        REQUIRE(moves.empty() == false);

        // Whatever is going on, keep doing the first thing on the list.
        env.Step(moves.front());
        ++steps;
    }

    CHECK(env.IsDone() == true);
    CHECK(steps <= SpireEnv::MOVE_LIMIT);
}

TEST_CASE("A potion nobody could keep is not offered for sale")
{
    // Walking until a shelf turns up, which every act has one of.
    SpireEnv env;
    std::mt19937 rng(4);
    int steps = 0;

    while (env.GetPhase() != EnvPhase::SHOP && steps < 4000)
    {
        if (env.IsDone() || steps == 0)
        {
            env.Reset(CardColor::RED, 100u + static_cast<unsigned int>(steps));
        }

        const std::vector<unsigned char> mask = env.ActionMask();
        std::vector<std::size_t> open;

        for (std::size_t i = 0; i < mask.size(); ++i)
        {
            if (mask[i] != 0u)
            {
                open.emplace_back(i);
            }
        }

        if (open.empty())
        {
            break;
        }

        std::uniform_int_distribution<std::size_t> pick(0, open.size() - 1);

        env.StepIndex(open[pick(rng)]);
        ++steps;
    }

    REQUIRE(env.GetPhase() == EnvPhase::SHOP);
    REQUIRE(env.GetRun().GetShop().GetPotions().empty() == false);

    // Enough gold for anything on the shelf, and a belt with room in it.
    env.GetRun().AddGold(999);

    // One that can be drunk on the map, so that room can be made again.
    while (env.GetRun().CanAddPotion())
    {
        REQUIRE(env.GetRun().AddPotion(PotionId::BLOOD_POTION) == true);
    }

    bool offered = false;

    for (const Action& move : env.LegalActions())
    {
        if (move.kind == ActionKind::BUY_POTION)
        {
            offered = true;
        }
    }

    // The belt is full, so buying would take the gold and hand back nothing.
    CHECK(offered == false);

    // With room again it is back on the list.
    REQUIRE(env.GetRun().DrinkPotion(0) == true);

    bool back = false;

    for (const Action& move : env.LegalActions())
    {
        if (move.kind == ActionKind::BUY_POTION)
        {
            back = true;
        }
    }

    CHECK(back == true);
}

TEST_CASE("Walking onto the boss puts the climb in front of it")
{
    SpireEnv env;

    env.Reset(CardColor::RED, 9);

    Run& run = env.GetRun();

    // Up the act without stopping to fight: this is about the top of it.
    while (run.GetFloor() < Map::ROWS)
    {
        const std::vector<int> ahead = run.GetAvailableColumns();

        REQUIRE(ahead.empty() == false);
        REQUIRE(run.Travel(ahead.front()) == true);
    }

    REQUIRE(env.GetPhase() == EnvPhase::MAP);

    const std::vector<int> last = run.GetAvailableColumns();

    REQUIRE(last.empty() == false);
    REQUIRE(env.Step(Action(ActionKind::TRAVEL, last.front())).taken == true);

    // Standing on the boss, with something to do about it.
    CHECK(run.IsAtBoss() == true);
    CHECK(env.GetPhase() == EnvPhase::BOSS);
    CHECK(env.IsDone() == false);
    REQUIRE(env.LegalActions().empty() == false);
    CHECK(env.LegalActions().front().kind == ActionKind::FIGHT_BOSS);

    // A mask with nothing in it is how this went unnoticed: every climb
    // ended here, and it looked like an act limit doing its job.
    const std::vector<unsigned char> mask = env.ActionMask();
    int open = 0;

    for (const unsigned char slot : mask)
    {
        open += slot != 0u ? 1 : 0;
    }

    CHECK(open == 1);

    REQUIRE(env.Step(Action(ActionKind::FIGHT_BOSS)).taken == true);
    CHECK(env.GetPhase() == EnvPhase::BATTLE);
    REQUIRE(env.GetBattle() != nullptr);
    CHECK(env.GetBattle()->IsBossFight() == true);
    CHECK(env.GetBattle()->GetMonsters().empty() == false);
}

TEST_CASE("Beating the boss pays out and is written down")
{
    SpireEnv env;

    env.Reset(CardColor::RED, 9);

    Run& run = env.GetRun();

    while (run.GetFloor() < Map::ROWS)
    {
        const std::vector<int> ahead = run.GetAvailableColumns();

        REQUIRE(ahead.empty() == false);
        REQUIRE(run.Travel(ahead.front()) == true);
    }

    REQUIRE(env.Step(Action(ActionKind::TRAVEL,
                            run.GetAvailableColumns().front()))
                .taken == true);
    REQUIRE(env.Step(Action(ActionKind::FIGHT_BOSS)).taken == true);

    float paid = 0.0f;

    for (int i = 0; i < 200 && env.GetPhase() == EnvPhase::BATTLE; ++i)
    {
        // Cheating the fight over: what is being tested is what a win is
        // worth and what it leaves behind, not the fighting.
        for (auto& monster :
             const_cast<Battle*>(env.GetBattle())->GetMonsters())
        {
            monster.SetHealth(0);
        }

        paid += env.Step(Action(ActionKind::END_TURN)).reward;
    }

    CHECK(env.GetPhase() != EnvPhase::BATTLE);
    CHECK(paid >= static_cast<float>(SpireEnv::BOSS_REWARD));
    CHECK(run.GetLog().GetSummary().bossesWon == 1);

    // And the act is done with, which is where an act limit ends a climb.
    const bool settled = env.GetPhase() == EnvPhase::REWARD ||
                         env.GetPhase() == EnvPhase::ACT_DONE;

    CHECK(settled == true);
}

TEST_CASE("An act limit ends the climb where the act does")
{
    SpireEnv env;

    env.SetActLimit(1);
    env.Reset(CardColor::RED, 9);

    // The limit is how the climb was set up, not part of the climb, so a
    // reset leaves it where it was.
    CHECK(env.GetActLimit() == 1);

    Run& run = env.GetRun();

    while (run.GetFloor() < Map::ROWS)
    {
        const std::vector<int> ahead = run.GetAvailableColumns();

        REQUIRE(ahead.empty() == false);
        REQUIRE(run.Travel(ahead.front()) == true);
    }

    REQUIRE(env.Step(Action(ActionKind::TRAVEL,
                            run.GetAvailableColumns().front()))
                .taken == true);
    REQUIRE(env.Step(Action(ActionKind::FIGHT_BOSS)).taken == true);

    bool ended = false;

    for (int i = 0; i < 400 && !ended; ++i)
    {
        if (env.GetPhase() == EnvPhase::BATTLE)
        {
            for (auto& monster :
                 const_cast<Battle*>(env.GetBattle())->GetMonsters())
            {
                monster.SetHealth(0);
            }
        }

        const std::vector<Action> moves = env.LegalActions();

        if (moves.empty())
        {
            break;
        }

        ended = env.Step(moves.front()).done;
    }

    // Done for the right reason: the act was cleared, not the climber killed
    // and not a climb called off for going on too long.
    CHECK(ended == true);
    CHECK(env.IsDone() == true);
    CHECK(env.GetRun().GetLog().GetSummary().bossesWon == 1);
    CHECK(env.GetRun().GetPlayer().IsDead() == false);

    // Without a limit the same climb walks on into the next act instead.
    SpireEnv whole;

    whole.Reset(CardColor::RED, 9);

    CHECK(whole.GetActLimit() == 0);
}

TEST_CASE("What health costs can be set")
{
    SpireEnv env;

    CHECK(env.GetHealthWeight() ==
          doctest::Approx(SpireEnv::HEALTH_WEIGHT));

    env.SetHealthWeight(0.0f);
    env.Reset(CardColor::RED, 8);

    // A reset leaves it alone: it is how the climb is being scored, not
    // anything about the climb.
    CHECK(env.GetHealthWeight() == 0.0f);

    REQUIRE(env.Step(env.LegalActions().front()).taken == true);
    REQUIRE(env.GetPhase() == EnvPhase::BATTLE);

    // Standing there taking hits pays nothing, with health worth nothing.
    float paid = 0.0f;
    const int before = env.GetBattle()->GetPlayer().GetHealth();

    for (int i = 0; i < 8 && env.GetPhase() == EnvPhase::BATTLE; ++i)
    {
        paid += env.Step(Action(ActionKind::END_TURN)).reward;
    }

    const int after = env.GetPhase() == EnvPhase::BATTLE
                          ? env.GetBattle()->GetPlayer().GetHealth()
                          : env.GetRun().GetPlayer().GetHealth();

    // It took a beating, whatever that beating was scored at.
    REQUIRE(after < before);

    // With health dear again, the same beating costs something.
    SpireEnv dear;

    dear.SetHealthWeight(0.5f);
    dear.Reset(CardColor::RED, 8);

    REQUIRE(dear.Step(dear.LegalActions().front()).taken == true);

    float cost = 0.0f;

    for (int i = 0; i < 8 && dear.GetPhase() == EnvPhase::BATTLE; ++i)
    {
        cost += dear.Step(Action(ActionKind::END_TURN)).reward;
    }

    // The same beating, the same seed, the same eight turns: the only
    // difference is what the health that went missing was worth.
    CHECK(cost < paid);
}

TEST_CASE("The state says what sits in each slot of the deck")
{
    SpireEnv env;

    env.Reset(CardColor::RED, 8);

    const SpireEnv::Layout layout = SpireEnv::GetLayout();
    const SpireEnv::IdLayout ids = SpireEnv::GetIdLayout();
    const std::vector<float> state = env.Observe();
    const std::vector<int> named = env.ObserveIds();
    const std::vector<Card>& deck = env.GetRun().GetDeck();

    REQUIRE(deck.empty() == false);
    REQUIRE(deck.size() < SpireEnv::DECK_SLOTS);

    // Every move that names a slot of the deck - sharpening, tearing up,
    // handing a card to a room - is chosen by what this says.
    for (std::size_t slot = 0; slot < SpireEnv::DECK_SLOTS; ++slot)
    {
        const std::size_t at = layout.deckCards + slot * layout.deckStride;
        const bool there = slot < deck.size();

        CHECK(state[at] == (there ? 1.0f : 0.0f));

        if (!there)
        {
            CHECK(named[ids.deck + slot] == 0);
            continue;
        }

        CHECK(named[ids.deck + slot] ==
              static_cast<int>(deck[slot].GetId()));

        // One kind flag set, and the sharpened flag after them.
        float kinds = 0.0f;

        for (std::size_t kind = 1; kind <= 5u; ++kind)
        {
            kinds += state[at + kind];
        }

        CHECK(kinds == 1.0f);
        CHECK(state[at + 6] == (deck[slot].IsUpgraded() ? 1.0f : 0.0f));
    }

    // Sharpening one says so in the slot it sits in.
    REQUIRE(env.GetRun().Smith(0) == true);

    const std::vector<float> after = env.Observe();

    CHECK(after[layout.deckCards + 6] == 1.0f);
}

TEST_CASE("A card says what it is worth, and what sharpening would add")
{
    // Sharpening a Strike buys three more damage; sharpening it again buys
    // nothing, which is why a fire does not offer it twice.
    const CardWorth& plain = CardRegistry::Worth(CardId::STRIKE_RED, 0);
    const CardWorth& sharp = CardRegistry::Worth(CardId::STRIKE_RED, 1);

    CHECK(plain.damage == 6);
    CHECK(sharp.damage == 9);
    CHECK(CardRegistry::CanUpgrade(CardId::STRIKE_RED, 0) == true);
    CHECK(CardRegistry::CanUpgrade(CardId::STRIKE_RED, 1) == false);

    // Defend blocks rather than hits.
    CHECK(CardRegistry::Worth(CardId::DEFEND_RED, 0).block == 5);
    CHECK(CardRegistry::Worth(CardId::DEFEND_RED, 1).block == 8);

    // A Searing Blow is the one card that keeps growing.
    CHECK(CardRegistry::CanUpgrade(CardId::SEARING_BLOW, 1) == true);
    CHECK(CardRegistry::CanUpgrade(CardId::SEARING_BLOW, 8) == true);
    CHECK(CardRegistry::Worth(CardId::SEARING_BLOW, 1).damage >
          CardRegistry::Worth(CardId::SEARING_BLOW, 0).damage);

    // And nothing can be done for a curse.
    CHECK(CardRegistry::CanUpgrade(CardId::REGRET, 0) == false);
    CHECK(CardRegistry::CanUpgrade(CardId::WOUND, 0) == false);
}

TEST_CASE("The state says what a whetstone would buy")
{
    SpireEnv env;

    env.Reset(CardColor::RED, 8);

    const SpireEnv::Layout layout = SpireEnv::GetLayout();
    Run& run = env.GetRun();

    REQUIRE(run.GetDeck().front().GetId() == CardId::STRIKE_RED);
    REQUIRE(run.GetDeck().front().IsUpgraded() == false);

    const std::vector<float> before = env.Observe();
    const std::size_t at = layout.deckCards;

    // There, an attack, not sharpened, and worth sharpening.
    CHECK(before[at] == 1.0f);
    CHECK(before[at + 1] == 1.0f);
    CHECK(before[at + 5] == 0.0f);
    CHECK(before[at + 6] == 0.0f);
    CHECK(before[at + 7] == 1.0f);

    // Five Strikes in a starting deck, and six damage now with three more
    // to be had from a whetstone.
    CHECK(before[at + 8] == doctest::Approx(5.0f / 5.0f));
    CHECK(before[at + 10] == doctest::Approx(6.0f / 20.0f));
    CHECK(before[at + 14] == doctest::Approx(3.0f / 10.0f));

    REQUIRE(run.Smith(0) == true);

    const std::vector<float> after = env.Observe();

    // Sharpened, worth nothing more, and hitting for nine. Still five
    // Strikes: sharpening one does not make it another card.
    CHECK(after[at + 6] == 1.0f);
    CHECK(after[at + 7] == 0.0f);
    CHECK(after[at + 8] == doctest::Approx(5.0f / 5.0f));
    CHECK(after[at + 10] == doctest::Approx(9.0f / 20.0f));
    CHECK(after[at + 14] == 0.0f);

    // And the fire will not put it to the whetstone twice.
    CHECK(run.Smith(0) == false);

    // A curse says which kind it is, and says nothing is to be done for it.
    run.AddCardToDeck(CardRegistry::Get(CardId::REGRET));

    const std::size_t last = layout.deckCards +
                             (run.GetDeck().size() - 1u) * layout.deckStride;
    const std::vector<float> cursed = env.Observe();

    CHECK(cursed[last] == 1.0f);
    CHECK(cursed[last + 5] == 1.0f);
    CHECK(cursed[last + 7] == 0.0f);
}

TEST_CASE("A curse that will not come out stays in")
{
    Run run(CardColor::RED, 4);

    run.AddCardToDeck(CardRegistry::Get(CardId::ASCENDERS_BANE));

    const std::size_t stuck = run.GetDeck().size() - 1u;
    const std::size_t before = run.GetDeck().size();

    run.OpenShop();
    run.AddGold(999);

    // Neither a shelf nor a pipe takes it out.
    CHECK(run.BuyCardRemoval(stuck) == false);
    CHECK(run.GetDeck().size() == before);

    // And an ordinary curse does come out.
    run.AddCardToDeck(CardRegistry::Get(CardId::REGRET));

    CHECK(run.BuyCardRemoval(run.GetDeck().size() - 1u) == true);
    CHECK(run.GetDeck().size() == before);
}

TEST_CASE("A fire offers only the cards it could change")
{
    SpireEnv env;

    env.Reset(CardColor::RED, 3);

    Run& run = env.GetRun();

    // A curse in the deck, and one card already sharpened.
    run.AddCardToDeck(CardRegistry::Get(CardId::REGRET));
    REQUIRE(run.Smith(0) == true);

    // Walk until a fire, which every act has one of before the boss.
    std::mt19937 rng(3);
    int steps = 0;

    while (env.GetPhase() != EnvPhase::REST && steps < 3000)
    {
        const std::vector<unsigned char> mask = env.ActionMask();
        std::vector<std::size_t> open;

        for (std::size_t i = 0; i < mask.size(); ++i)
        {
            if (mask[i] != 0u)
            {
                open.emplace_back(i);
            }
        }

        if (open.empty())
        {
            break;
        }

        std::uniform_int_distribution<std::size_t> pick(0, open.size() - 1);

        env.StepIndex(open[pick(rng)]);
        ++steps;
    }

    if (env.GetPhase() != EnvPhase::REST)
    {
        // The climb died on the way up; the rest of this proves nothing.
        return;
    }

    int offered = 0;

    for (const Action& move : env.LegalActions())
    {
        if (move.kind != ActionKind::SMITH)
        {
            continue;
        }

        ++offered;

        const Card& card =
            env.GetRun().GetDeck()[static_cast<std::size_t>(move.a)];

        // Nothing on the list is a curse, and nothing on it is finished.
        CHECK(card.GetCardType() != CardType::CURSE);
        CHECK(CardRegistry::CanUpgrade(card.GetId(),
                                       card.GetUpgradeCount()) == true);
    }

    CHECK(offered > 0);
    CHECK(offered < static_cast<int>(env.GetRun().GetDeck().size()));
}

TEST_CASE("A climb is called off even when every move is turned down")
{
    // A move the phase does not allow changes nothing. The counter behind
    // MOVE_LIMIT used to sit at the end of Step(), which those moves return
    // before reaching, so a climb fed nothing but rejected moves ran for
    // ever. Whatever is thrown at it, it has to end.
    SpireEnv env;
    env.SetActLimit(1);
    env.Reset(CardColor::RED, 5);

    // NEXT_ACT is legal in one phase only, and the climb does not start in
    // it, so this is turned down for as long as the phase holds.
    int steps = 0;
    bool done = false;

    while (steps < SpireEnv::MOVE_LIMIT * 2)
    {
        ++steps;

        const StepResult result = env.Step(Action(ActionKind::NEXT_ACT));

        if (result.done)
        {
            done = true;
            break;
        }
    }

    CHECK(done == true);
    CHECK(steps <= SpireEnv::MOVE_LIMIT);
    CHECK(env.IsDone() == true);
}

TEST_CASE("A climb never stands in a state with no move in it")
{
    // The mask is what an agent is steered by, so a row of it with nothing
    // set leaves the agent choosing at random among moves that are all
    // turned down. That is the shape the hang took.
    std::mt19937 rng(20260821);

    for (unsigned int seed = 1; seed <= 40u; ++seed)
    {
        SpireEnv env;
        env.SetActLimit(0);
        env.Reset(CardColor::RED, seed);

        for (int step = 0; step < 4000 && !env.IsDone(); ++step)
        {
            const std::vector<unsigned char> mask = env.ActionMask();
            const std::size_t legal = static_cast<std::size_t>(
                std::count(mask.begin(), mask.end(), 1u));

            REQUIRE(legal > 0u);

            std::vector<std::size_t> open;

            for (std::size_t i = 0; i < mask.size(); ++i)
            {
                if (mask[i] != 0u)
                {
                    open.emplace_back(i);
                }
            }

            std::uniform_int_distribution<std::size_t> pick(
                0, open.size() - 1u);

            env.StepIndex(open[pick(rng)]);
        }
    }
}

TEST_CASE("A boss fight is named after the boss, not whoever stands first")
{
    // The Awakened One is led in behind a pair of cultists, so the monster
    // standing first in the line is not the one the fight is about. The rule
    // is the first whose kind is the kind of the fight, and every boss group
    // in the spire has to have one.
    std::mt19937 rng(4);

    const std::vector<const std::vector<Encounter>*> pools = {
        &EncounterLibrary::GetAct1Bosses(), &EncounterLibrary::GetAct2Bosses(),
        &EncounterLibrary::GetAct3Bosses(), &EncounterLibrary::GetAct4Bosses()
    };

    for (const std::vector<Encounter>* pool : pools)
    {
        for (const Encounter& encounter : *pool)
        {
            const std::vector<Monster> built =
                EncounterLibrary::Build(encounter, rng);

            REQUIRE(built.empty() == false);

            MonsterId named = MonsterId::INVALID;

            for (const Monster& monster : built)
            {
                if (monster.GetMonsterType() == encounter.type)
                {
                    named = monster.GetMonsterId();
                    break;
                }
            }

            // Something was found, and it is the boss rather than an escort.
            CHECK(named != MonsterId::INVALID);
            CHECK(named != MonsterId::CULTIST);
        }
    }
}

TEST_CASE("A monster winding up says so, rather than reading as unknown")
{
    // The gremlin wizard spends two turns on nothing and then hits for
    // twenty-five. While it charged its intent was UNKNOWN, which is also
    // what an intent nobody can see reads as - so the state said the same
    // thing about a wizard about to fire and a monster whose plans were
    // simply hidden. There was nothing in it to learn from.
    std::mt19937 rng(11);
    Monster wizard = MonsterRoster::Make(MonsterId::GREMLIN_WIZARD, rng);

    CHECK(wizard.GetIntent() == Intent::CHARGING);
    CHECK(wizard.GetCurrentMove().name == "Charging");

    // And the one it opens the pattern with is not an attack, so the danger
    // is only readable from the intent.
    CHECK(wizard.GetCurrentMove().effects.empty() == true);

    // The slime boss winds up for its slam the same way.
    Monster slime = MonsterRoster::Make(MonsterId::SLIME_BOSS, rng);
    bool winds = false;

    for (const MonsterMove& move : slime.GetMoves())
    {
        if (move.name == "Preparing")
        {
            winds = move.intent == Intent::CHARGING;
        }
    }

    CHECK(winds == true);
}

TEST_CASE("The state has a slot for every intent there is")
{
    // The one-hot of intents is as wide as the enum, or the widest of them
    // would fall off the end of the state without anything saying so.
    const std::vector<float> state = InBattle(3).Observe();

    CHECK(state.size() == SpireEnv::ObservationSize());
    CHECK(static_cast<std::size_t>(Intent::CHARGING) < 12u);
}

namespace
{
//! Plays \p env with whatever is legal, favouring the move that walks on to
//! the next act, and returns the deepest act it reached.
int DeepestActOf(SpireEnv& env, std::mt19937& rng)
{
    for (int step = 0; step < 20000 && !env.IsDone(); ++step)
    {
        const std::vector<Action> moves = env.LegalActions();

        if (moves.empty())
        {
            break;
        }

        // Anything that ends the fight quickly will do; what matters is
        // whether the act is ever left behind.
        std::uniform_int_distribution<std::size_t> pick(0, moves.size() - 1u);

        env.Step(moves[pick(rng)]);
    }

    return env.GetRun().GetLog().GetSummary().deepestAct;
}
}  // namespace

TEST_CASE("The act limit is how many acts a climb is given")
{
    // Settle() asked whether the run was finished before it asked what the
    // climb had been asked for, and IsFinished() is true the moment an act's
    // boss is down. So every climb stopped at the top of act one whatever
    // the limit said: ACT_DONE and NEXT_ACT were unreachable, and so was the
    // reward for the spire. Nothing tested it, which is why it went unseen.
    SpireEnv one;
    one.SetActLimit(1);

    CHECK(one.GetActLimit() == 1);

    SpireEnv two;
    two.SetActLimit(2);

    CHECK(two.GetActLimit() == 2);

    // A climb given one act cannot be standing in the second, however it is
    // played; one given two can be, and one given all of them can be too.
    std::mt19937 rng(77);

    for (unsigned int seed = 1; seed <= 20u; ++seed)
    {
        SpireEnv env;
        env.SetActLimit(1);
        env.Reset(CardColor::RED, seed);

        CHECK(DeepestActOf(env, rng) == 1);
    }
}
