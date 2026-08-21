#include "doctest.h"

#include <conquer-the-spire/Battle/Battle.hpp>
#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Enums/CardId.hpp>
#include <conquer-the-spire/Monsters/MonsterLibrary.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

using namespace ConquerTheSpire;

namespace
{
//! Builds a started battle. The seed is fixed, so every run plays out the same.
Battle MakeBattle(std::vector<Card> deck, std::vector<Monster> monsters,
                  int playerHealth = 80)
{
    Player player("Ironclad", playerHealth);

    for (auto& card : deck)
    {
        player.AddCardToDeck(std::move(card));
    }

    Battle battle(std::move(player), std::move(monsters), 42);
    battle.Start();

    return battle;
}

//! A monster that never acts, so a test can watch damage and block alone.
Monster Dummy(int health)
{
    return Monsters::TrainingDummy(health);
}

//! Returns a deck made of \p count copies of \p id, so the hand is known
//! without depending on the shuffle.
std::vector<Card> Deck(CardId id, std::size_t count)
{
    return std::vector<Card>(count, CardRegistry::Get(id));
}

//! Returns the hand index of \p name, or the hand size when it is not there.
std::size_t FindCard(const Battle& battle, const std::string& name)
{
    const std::vector<Card>& hand = battle.GetPlayer().GetHand();

    for (std::size_t i = 0; i < hand.size(); ++i)
    {
        if (hand[i].GetName() == name)
        {
            return i;
        }
    }

    return hand.size();
}
}  // namespace

TEST_CASE("Battle opens with a full hand and full energy")
{
    const Battle battle =
        MakeBattle(Deck(CardId::STRIKE_RED, 10), { Dummy(50) });

    CHECK(battle.GetPhase() == BattlePhase::PLAYER_TURN);
    CHECK(battle.GetTurn() == 1);
    CHECK(battle.GetPlayer().GetEnergy() == 3);
    CHECK(battle.GetPlayer().GetHand().size() == 5u);
    CHECK(battle.GetPlayer().GetDrawPile().size() == 5u);
    CHECK(battle.GetPlayer().GetDiscardPile().empty());
}

TEST_CASE("Strike damages the chosen monster and goes to the discard pile")
{
    Battle battle =
        MakeBattle(Deck(CardId::STRIKE_RED, 10), { Dummy(50) });

    CHECK(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 44);
    CHECK(battle.GetPlayer().GetEnergy() == 2);
    CHECK(battle.GetPlayer().GetHand().size() == 4u);
    CHECK(battle.GetPlayer().GetDiscardPile().size() == 1u);
}

TEST_CASE("Defend gains block that soaks the monster attack")
{
    Battle battle = MakeBattle(
        Deck(CardId::DEFEND_RED, 10),
        { Monster("Attacker", 30, { MonsterMove::Attack("Hit", 10) }) });

    CHECK(battle.PlayCard(0) == true);
    CHECK(battle.GetPlayer().GetBlock() == 5);

    CHECK(battle.EndTurn() == true);

    // 10 damage, 5 of it soaked by the block.
    CHECK(battle.GetPlayer().GetHealth() == 75);

    // Block does not carry into the next turn.
    CHECK(battle.GetPlayer().GetBlock() == 0);
}

TEST_CASE("Bash makes the target Vulnerable so the next Strike hits harder")
{
    std::vector<Card> deck = Deck(CardId::STRIKE_RED, 4);
    deck.emplace_back(CardRegistry::Get(CardId::BASH));

    Battle battle = MakeBattle(std::move(deck), { Dummy(50) });

    const std::size_t bashIndex = FindCard(battle, "Bash");
    REQUIRE(bashIndex < battle.GetPlayer().GetHand().size());

    CHECK(battle.PlayCard(bashIndex) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 42);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::VULNERABLE) == 2);
    CHECK(battle.GetPlayer().GetEnergy() == 1);

    const std::size_t strikeIndex = FindCard(battle, "Strike");
    REQUIRE(strikeIndex < battle.GetPlayer().GetHand().size());

    // 6 damage, raised to 9 by Vulnerable.
    CHECK(battle.PlayCard(strikeIndex) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 33);
}

TEST_CASE("Vulnerable decays at the end of the monster turn")
{
    std::vector<Card> deck = Deck(CardId::STRIKE_RED, 4);
    deck.emplace_back(CardRegistry::Get(CardId::BASH));

    Battle battle = MakeBattle(std::move(deck), { Dummy(50) });

    const std::size_t bashIndex = FindCard(battle, "Bash");
    REQUIRE(bashIndex < battle.GetPlayer().GetHand().size());
    REQUIRE(battle.PlayCard(bashIndex) == true);

    CHECK(battle.GetMonsters()[0].GetPower(PowerType::VULNERABLE) == 2);

    CHECK(battle.EndTurn() == true);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::VULNERABLE) == 1);
}

TEST_CASE("Inflame adds Strength to every later attack")
{
    std::vector<Card> deck = Deck(CardId::STRIKE_RED, 4);
    deck.emplace_back(CardRegistry::Get(CardId::INFLAME));

    Battle battle = MakeBattle(std::move(deck), { Dummy(50) });

    const std::size_t inflameIndex = FindCard(battle, "Inflame");
    REQUIRE(inflameIndex < battle.GetPlayer().GetHand().size());

    CHECK(battle.PlayCard(inflameIndex) == true);
    CHECK(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 2);

    // 6 base damage plus 2 Strength, twice.
    CHECK(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 42);
    CHECK(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 34);
    CHECK(battle.GetPlayer().GetEnergy() == 0);
}

TEST_CASE("Weak reduces the damage an attack deals")
{
    Battle battle = MakeBattle(Deck(CardId::STRIKE_RED, 10),
                               { Dummy(50) });

    battle.GetPlayer().AddPower(PowerType::WEAK, 1);

    // 6 damage rounded down to 4.
    CHECK(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 46);
}

TEST_CASE("Energy limits how many cards can be played in a turn")
{
    Battle battle = MakeBattle(Deck(CardId::STRIKE_RED, 10),
                               { Dummy(100) });

    CHECK(battle.PlayCard(0) == true);
    CHECK(battle.PlayCard(0) == true);
    CHECK(battle.PlayCard(0) == true);

    // Out of energy: the fourth Strike is refused and nothing changes.
    CHECK(battle.PlayCard(0) == false);
    CHECK(battle.GetPlayer().GetEnergy() == 0);
    CHECK(battle.GetPlayer().GetHand().size() == 2u);
    CHECK(battle.GetMonsters()[0].GetHealth() == 82);
}

TEST_CASE("GetPlayableCardIndices only reports cards that can be paid for")
{
    std::vector<Card> deck = Deck(CardId::STRIKE_RED, 4);
    deck.emplace_back(CardRegistry::Get(CardId::BASH));

    Battle battle = MakeBattle(std::move(deck), { Dummy(50) });

    CHECK(battle.GetPlayableCardIndices().size() == 5u);

    const std::size_t bashIndex = FindCard(battle, "Bash");
    REQUIRE(bashIndex < battle.GetPlayer().GetHand().size());
    REQUIRE(battle.PlayCard(bashIndex) == true);

    // 1 energy left, and every card in hand is a 1 cost Strike.
    CHECK(battle.GetPlayableCardIndices().size() == 4u);

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetPlayableCardIndices().empty());
}

TEST_CASE("Ending the turn discards the hand and draws a new one")
{
    Battle battle =
        MakeBattle(Deck(CardId::STRIKE_RED, 10), { Dummy(50) });

    CHECK(battle.EndTurn() == true);
    CHECK(battle.GetTurn() == 2);
    CHECK(battle.GetPhase() == BattlePhase::PLAYER_TURN);
    CHECK(battle.GetPlayer().GetEnergy() == 3);
    CHECK(battle.GetPlayer().GetHand().size() == 5u);
    CHECK(battle.GetPlayer().GetDiscardPile().size() == 5u);
    CHECK(battle.GetPlayer().GetDrawPile().empty());
}

TEST_CASE("The discard pile is reshuffled when the draw pile runs out")
{
    Battle battle =
        MakeBattle(Deck(CardId::STRIKE_RED, 6), { Dummy(50) });

    REQUIRE(battle.GetPlayer().GetDrawPile().size() == 1u);
    REQUIRE(battle.EndTurn() == true);

    // One card was left to draw, the other four came from the reshuffled
    // discard pile.
    CHECK(battle.GetPlayer().GetHand().size() == 5u);
    CHECK(battle.GetPlayer().GetDrawPile().size() == 1u);
    CHECK(battle.GetPlayer().GetDiscardPile().empty());
}

TEST_CASE("Cleave hits every living monster")
{
    Battle battle =
        MakeBattle(Deck(CardId::CLEAVE, 10),
                   { Dummy(20), Dummy(20) });

    CHECK(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 12);
    CHECK(battle.GetMonsters()[1].GetHealth() == 12);
}

TEST_CASE("Poison ticks at the start of the monster turn")
{
    Battle battle =
        MakeBattle(Deck(CardId::STRIKE_RED, 10), { Dummy(50) });

    battle.GetMonsters()[0].AddPower(PowerType::POISON, 3);

    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 47);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::POISON) == 2);
}

TEST_CASE("A monster telegraphs its move before it acts")
{
    Battle battle =
        MakeBattle(Deck(CardId::DEFEND_RED, 10), { Monsters::JawWorm() });

    CHECK(battle.GetMonsters()[0].GetIntent() == Intent::ATTACK);
    CHECK(battle.GetMonsters()[0].GetCurrentMove().name == "Chomp");

    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetPlayer().GetHealth() == 69);
    CHECK(battle.GetMonsters()[0].GetCurrentMove().name == "Thrash");
}

TEST_CASE("A monster script that does not loop stays on its last move")
{
    Battle battle =
        MakeBattle(Deck(CardId::STRIKE_RED, 10), { Monsters::Cultist() });

    // Incantation buffs the Cultist without attacking.
    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetHealth() == 80);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::STRENGTH) == 3);

    // From here on it only attacks: 6 base damage plus 3 Strength.
    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetHealth() == 71);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetHealth() == 62);
}

TEST_CASE("The battle is won when the last monster dies")
{
    Battle battle =
        MakeBattle(Deck(CardId::STRIKE_RED, 10), { Dummy(5) });

    CHECK(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].IsDead() == true);
    CHECK(battle.GetPhase() == BattlePhase::WON);
    CHECK(battle.IsDone() == true);

    // Nothing more can be played once the battle is over.
    CHECK(battle.PlayCard(0) == false);
    CHECK(battle.EndTurn() == false);
}

TEST_CASE("The battle is lost when the player runs out of health")
{
    Battle battle = MakeBattle(
        Deck(CardId::STRIKE_RED, 10),
        { Monster("Bruiser", 30, { MonsterMove::Attack("Smash", 10) }) }, 8);

    CHECK(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().IsDead() == true);
    CHECK(battle.GetPhase() == BattlePhase::LOST);
    CHECK(battle.IsDone() == true);
}

TEST_CASE("A card aimed at a dead monster is refused")
{
    Battle battle =
        MakeBattle(Deck(CardId::STRIKE_RED, 10),
                   { Dummy(5), Dummy(50) });

    REQUIRE(battle.PlayCard(0, 0) == true);
    REQUIRE(battle.GetMonsters()[0].IsDead() == true);

    CHECK(battle.PlayCard(0, 0) == false);
    CHECK(battle.GetPlayableCardIndices().size() == 4u);
    CHECK(battle.GetLivingMonsterIndices().size() == 1u);

    // The living monster is still a legal target.
    CHECK(battle.PlayCard(0, 1) == true);
    CHECK(battle.GetMonsters()[1].GetHealth() == 44);
}
