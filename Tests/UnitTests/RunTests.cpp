#include "doctest.h"

#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Enums/CardId.hpp>
#include <conquer-the-spire/Monsters/MonsterLibrary.hpp>
#include <conquer-the-spire/Run/Run.hpp>

#include <algorithm>
#include <cstddef>
#include <vector>

using namespace ConquerTheSpire;

namespace
{
//! Walks the run up the act, taking the first way on offer every time.
void WalkToBoss(Run& run)
{
    while (!run.IsAtBoss())
    {
        const std::vector<int> ahead = run.GetAvailableColumns();

        REQUIRE(ahead.empty() == false);
        REQUIRE(run.Travel(ahead.front()) == true);
    }
}
}  // namespace

TEST_CASE("A run opens with the character's deck, relic, health and gold")
{
    const Run ironclad(CardColor::RED, 1);

    CHECK(ironclad.GetPlayer().GetMaxHealth() == 80);
    CHECK(ironclad.GetDeck().size() == 10u);
    CHECK(ironclad.GetPlayer().HasRelic(RelicId::BURNING_BLOOD) == true);
    CHECK(ironclad.GetGold() == 99);
    CHECK(ironclad.GetPlayer().GetColor() == CardColor::RED);

    const Run silent(CardColor::GREEN, 1);

    CHECK(silent.GetPlayer().GetMaxHealth() == 70);
    CHECK(silent.GetDeck().size() == 12u);
    CHECK(silent.GetPlayer().HasRelic(RelicId::RING_OF_THE_SNAKE) == true);

    const Run defect(CardColor::BLUE, 1);

    CHECK(defect.GetPlayer().GetMaxHealth() == 75);
    CHECK(defect.GetDeck().size() == 10u);
    CHECK(defect.GetPlayer().HasRelic(RelicId::CRACKED_CORE) == true);
}

TEST_CASE("A run starts off the map and steps onto the bottom row")
{
    Run run(CardColor::RED, 3);

    CHECK(run.GetFloor() == 0);
    CHECK(run.GetColumn() == -1);
    CHECK(run.GetCurrentNodeType() == MapNodeType::EMPTY);

    const std::vector<int> openings = run.GetAvailableColumns();

    REQUIRE(openings.empty() == false);
    CHECK(openings == run.GetMap().GetStartingColumns());

    // Nowhere else will do.
    CHECK(run.Travel(-1) == false);
    REQUIRE(run.Travel(openings.front()) == true);

    CHECK(run.GetFloor() == 1);
    CHECK(run.GetColumn() == openings.front());
    CHECK(run.GetCurrentNodeType() == MapNodeType::MONSTER);
}

TEST_CASE("A run can only walk where a path goes")
{
    Run run(CardColor::RED, 5);

    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);

    const std::vector<int> ahead = run.GetAvailableColumns();

    REQUIRE(ahead.empty() == false);

    for (int column = 0; column < Map::COLUMNS; ++column)
    {
        const bool onPath =
            std::find(ahead.begin(), ahead.end(), column) != ahead.end();

        if (!onPath)
        {
            CHECK(run.Travel(column) == false);
        }
    }

    CHECK(run.Travel(ahead.front()) == true);
    CHECK(run.GetFloor() == 2);
}

TEST_CASE("A run reaches the boss after the fifteen rows")
{
    Run run(CardColor::RED, 9);

    WalkToBoss(run);

    CHECK(run.GetFloor() == Map::ROWS + 1);
    CHECK(run.GetCurrentNodeType() == MapNodeType::BOSS);
    CHECK(run.IsAtBoss() == true);
    CHECK(run.IsFinished() == false);
    CHECK(run.GetAvailableColumns().empty());

    run.FinishBoss();

    CHECK(run.IsFinished() == true);
    CHECK(run.IsAtBoss() == false);
}

TEST_CASE("The floor walked to is always a place on the map")
{
    for (unsigned int seed = 1; seed <= 20; ++seed)
    {
        Run run(CardColor::GREEN, seed);

        while (!run.IsAtBoss())
        {
            const std::vector<int> ahead = run.GetAvailableColumns();

            REQUIRE(ahead.empty() == false);
            REQUIRE(run.Travel(ahead.back()) == true);

            CHECK(run.GetCurrentNodeType() != MapNodeType::EMPTY);
        }

        CHECK(run.GetFloor() == Map::ROWS + 1);
    }
}

TEST_CASE("A battle takes a copy of the player and hands the health back")
{
    Run run(CardColor::RED, 11);

    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);

    Battle battle = run.StartBattle({ Monsters::JawWorm() });

    // The battle works on a copy: the deck is untouched and the cards are in
    // the piles instead.
    CHECK(run.GetDeck().size() == 10u);
    CHECK(battle.GetPlayer().GetHand().size() == 5u);
    CHECK(battle.GetPlayer().GetHealth() == 80);

    REQUIRE(battle.EndTurn() == true);
    REQUIRE(battle.GetPlayer().GetHealth() == 69);

    // Nothing has come back yet.
    CHECK(run.GetPlayer().GetHealth() == 80);

    run.FinishBattle(battle);

    // 69 after the fight, and Burning Blood patched up 6 of it.
    CHECK(run.GetPlayer().GetHealth() == 75);
    CHECK(run.GetDeck().size() == 10u);
}

TEST_CASE("Black Blood and Meat on the Bone pay out after a fight")
{
    Run run(CardColor::RED, 12);
    run.AddRelic(RelicId::MEAT_ON_THE_BONE);

    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);

    Battle battle = run.StartBattle({ Monster("Bruiser", 40,
                                              { MonsterMove::Attack("Smash",
                                                                    45) }) });

    REQUIRE(battle.EndTurn() == true);
    REQUIRE(battle.GetPlayer().GetHealth() == 35);

    run.FinishBattle(battle);

    // 6 from Burning Blood and 12 more for being under half.
    CHECK(run.GetPlayer().GetHealth() == 53);
}

TEST_CASE("A rest site patches the player up")
{
    Run run(CardColor::RED, 13);

    run.GetPlayer().SetHealth(40);
    run.Rest();

    // Thirty per cent of eighty.
    CHECK(run.GetPlayer().GetHealth() == 64);

    run.Rest();
    CHECK(run.GetPlayer().GetHealth() == 80);
}

TEST_CASE("A rest site can upgrade a card instead")
{
    Run run(CardColor::RED, 14);

    REQUIRE(run.GetDeck()[0].GetName() == "Strike");
    REQUIRE(run.Smith(0) == true);

    CHECK(run.GetDeck()[0].GetName() == "Strike+");
    CHECK(run.GetDeck()[0].IsUpgraded() == true);

    // Nothing to upgrade there.
    CHECK(run.Smith(99) == false);

    run.AddCardToDeck(CardRegistry::Get(CardId::REGRET));
    CHECK(run.Smith(run.GetDeck().size() - 1) == false);
}

TEST_CASE("The deck can be added to and taken from between fights")
{
    Run run(CardColor::RED, 15);

    run.AddCardToDeck(CardRegistry::Get(CardId::BLUDGEON));

    REQUIRE(run.GetDeck().size() == 11u);
    CHECK(run.GetDeck().back().GetName() == "Bludgeon");

    REQUIRE(run.RemoveCardFromDeck(0) == true);
    CHECK(run.GetDeck().size() == 10u);
    CHECK(run.RemoveCardFromDeck(99) == false);
}

TEST_CASE("A relic that carries health raises the maximum when it is taken")
{
    Run run(CardColor::RED, 16);

    REQUIRE(run.GetPlayer().GetMaxHealth() == 80);

    run.AddRelic(RelicId::STRAWBERRY);
    CHECK(run.GetPlayer().GetMaxHealth() == 87);

    run.AddRelic(RelicId::PEAR);
    CHECK(run.GetPlayer().GetMaxHealth() == 97);

    run.AddRelic(RelicId::MANGO);
    CHECK(run.GetPlayer().GetMaxHealth() == 111);
}

TEST_CASE("Gold is spent and gathered")
{
    Run run(CardColor::RED, 17);

    CHECK(run.SpendGold(50) == true);
    CHECK(run.GetGold() == 49);

    CHECK(run.SpendGold(100) == false);
    CHECK(run.GetGold() == 49);

    run.AddGold(200);
    CHECK(run.GetGold() == 249);
}

TEST_CASE("Potions carry over from one fight to the next")
{
    Run run(CardColor::RED, 18);

    REQUIRE(run.AddPotion(PotionId::FIRE_POTION) == true);
    REQUIRE(run.AddPotion(PotionId::BLOCK_POTION) == true);
    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);

    Battle battle = run.StartBattle({ Monsters::TrainingDummy(50) });

    REQUIRE(battle.GetPlayer().GetPotions().size() == 2u);
    REQUIRE(battle.UsePotion(0, 0) == true);

    run.FinishBattle(battle);

    // The one that was drunk is gone for good.
    REQUIRE(run.GetPlayer().GetPotions().size() == 1u);
    CHECK(run.GetPlayer().GetPotions()[0].GetId() == PotionId::BLOCK_POTION);
}

TEST_CASE("A whole act can be walked and fought through")
{
    Run run(CardColor::RED, 21);

    int fights = 0;

    while (!run.IsAtBoss())
    {
        REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);

        switch (run.GetCurrentNodeType())
        {
            case MapNodeType::MONSTER:
            case MapNodeType::ELITE:
            {
                Battle battle = run.StartBattle({ Monsters::RedLouse() });

                // Swing away until somebody falls over.
                while (!battle.IsDone())
                {
                    const std::vector<std::size_t> playable =
                        battle.GetPlayableCardIndices();

                    if (playable.empty())
                    {
                        battle.EndTurn();
                        continue;
                    }

                    battle.PlayCard(playable.front());
                }

                run.FinishBattle(battle);
                ++fights;
                break;
            }

            case MapNodeType::REST:
                run.Rest();
                break;

            case MapNodeType::TREASURE:
                run.AddGold(100);
                break;

            default:
                break;
        }

        REQUIRE(run.GetPlayer().GetHealth() > 0);
    }

    CHECK(fights > 0);
    CHECK(run.GetGold() >= 199);

    run.FinishBoss();
    CHECK(run.IsFinished() == true);
}
