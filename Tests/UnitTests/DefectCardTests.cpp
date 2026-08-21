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
//! Builds a started battle whose deck is exactly \p ids. Five cards or fewer
//! all land in the opening hand, so a test never depends on the shuffle.
Battle BattleWith(const std::vector<CardId>& ids,
                  std::vector<Monster> monsters, int playerHealth = 75)
{
    Player player("Defect", playerHealth);

    for (const CardId id : ids)
    {
        player.AddCardToDeck(CardRegistry::Get(id));
    }

    Battle battle(std::move(player), std::move(monsters), 23);
    battle.Start();

    return battle;
}

//! A monster that never acts, so a test can watch damage and block alone.
Monster Dummy(int health)
{
    return Monsters::TrainingDummy(health);
}

//! A monster that hits for \p damage every turn.
Monster Attacker(int health, int damage)
{
    return Monster("Attacker", health,
                   { MonsterMove::Attack("Hit", damage) });
}

//! Returns the hand index of \p name, or the hand size when it is not there.
std::size_t Idx(const Battle& battle, const std::string& name)
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

//! Stacks \p count copies of \p id on top of the draw pile.
void StackDrawPile(Battle& battle, CardId id, int count)
{
    for (int i = 0; i < count; ++i)
    {
        battle.GetPlayer().GetDrawPile().emplace_back(CardRegistry::Get(id));
    }
}
}  // namespace

TEST_CASE("Zap puts a Lightning orb into orbit")
{
    Battle battle = BattleWith({ CardId::ZAP }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);

    REQUIRE(battle.GetPlayer().GetOrbs().size() == 1u);
    CHECK(battle.GetPlayer().GetOrbs()[0].type == OrbType::LIGHTNING);
}

TEST_CASE("A Lightning orb goes off at the end of every turn")
{
    Battle battle = BattleWith({ CardId::ZAP }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 47);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 44);
}

TEST_CASE("Dualcast evokes the front orb twice and lets it go")
{
    Battle battle =
        BattleWith({ CardId::ZAP, CardId::DUALCAST }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Zap")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Dualcast")) == true);

    // Two evokes of 8 damage, and the orb is gone.
    CHECK(battle.GetMonsters()[0].GetHealth() == 34);
    CHECK(battle.GetPlayer().GetOrbs().empty());
}

TEST_CASE("Focus raises what every orb does")
{
    Battle battle =
        BattleWith({ CardId::DEFRAGMENT, CardId::ZAP }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Defragment")) == true);
    REQUIRE(battle.GetPlayer().GetPower(PowerType::FOCUS) == 1);

    CHECK(battle.OrbPower(OrbType::LIGHTNING, false) == 4);
    CHECK(battle.OrbPower(OrbType::LIGHTNING, true) == 9);

    // Focus leaves Plasma alone.
    CHECK(battle.OrbPower(OrbType::PLASMA, true) == 2);

    REQUIRE(battle.PlayCard(Idx(battle, "Zap")) == true);
    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 46);
}

TEST_CASE("A Frost orb blocks at the end of the turn")
{
    Battle battle = BattleWith({ CardId::COLD_SNAP }, { Attacker(30, 2) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 24);

    REQUIRE(battle.EndTurn() == true);

    // The 2 block from the orb soaked the hit.
    CHECK(battle.GetPlayer().GetHealth() == 75);
}

TEST_CASE("A Dark orb builds up and unloads when it is evoked")
{
    Battle battle =
        BattleWith({ CardId::DARKNESS, CardId::RECURSION }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Darkness")) == true);
    REQUIRE(battle.GetPlayer().GetOrbs()[0].amount == 0);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetOrbs()[0].amount == 6);

    REQUIRE(battle.PlayCard(Idx(battle, "Recursion")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 44);
}

TEST_CASE("A Plasma orb hands over energy")
{
    Battle battle =
        BattleWith({ CardId::FUSION, CardId::RECURSION }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Fusion")) == true);
    REQUIRE(battle.EndTurn() == true);

    // Three for the turn and one from the orb sitting there.
    CHECK(battle.GetPlayer().GetEnergy() == 4);

    REQUIRE(battle.PlayCard(Idx(battle, "Recursion")) == true);
    CHECK(battle.GetPlayer().GetEnergy() == 5);
}

TEST_CASE("A full orbit evokes the oldest orb to make room")
{
    Battle battle = BattleWith({ CardId::ZAP }, { Dummy(50) });

    CHECK(battle.GetPlayer().GetOrbSlots() == 3);

    for (int i = 0; i < 3; ++i)
    {
        battle.ChannelOrb(OrbType::FROST);
    }

    REQUIRE(battle.GetPlayer().GetOrbs().size() == 3u);

    // The fourth orb pushes the first one out, and a Frost gives block on the
    // way.
    battle.ChannelOrb(OrbType::LIGHTNING);

    CHECK(battle.GetPlayer().GetOrbs().size() == 3u);
    CHECK(battle.GetPlayer().GetBlock() == 5);
    CHECK(battle.GetPlayer().GetOrbs()[2].type == OrbType::LIGHTNING);
}

TEST_CASE("Capacitor makes room and Consume trades a slot for Focus")
{
    Battle battle =
        BattleWith({ CardId::CAPACITOR, CardId::CONSUME }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Capacitor")) == true);
    CHECK(battle.GetPlayer().GetOrbSlots() == 5);

    REQUIRE(battle.PlayCard(Idx(battle, "Consume")) == true);
    CHECK(battle.GetPlayer().GetOrbSlots() == 4);
    CHECK(battle.GetPlayer().GetPower(PowerType::FOCUS) == 2);
}

TEST_CASE("Barrage hits once for every orb in orbit")
{
    Battle battle = BattleWith({ CardId::BARRAGE }, { Dummy(50) });

    battle.ChannelOrb(OrbType::LIGHTNING);
    battle.ChannelOrb(OrbType::DARK);

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 42);
}

TEST_CASE("Compile Driver draws for every kind of orb in orbit")
{
    Battle battle = BattleWith({ CardId::COMPILE_DRIVER }, { Dummy(50) });

    StackDrawPile(battle, CardId::STRIKE_BLUE, 5);

    battle.ChannelOrb(OrbType::LIGHTNING);
    battle.ChannelOrb(OrbType::LIGHTNING);
    battle.ChannelOrb(OrbType::FROST);

    REQUIRE(battle.PlayCard(0) == true);

    // Two kinds of orb, so two cards.
    CHECK(battle.GetPlayer().GetHand().size() == 2u);
}

TEST_CASE("A Claw sharpens every other Claw")
{
    Battle battle =
        BattleWith({ CardId::CLAW, CardId::CLAW }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 47);

    // The second Claw learned from the first.
    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 42);
}

TEST_CASE("Streamline gets cheaper every time it is played")
{
    Battle battle = BattleWith({ CardId::STREAMLINE }, { Dummy(50) });

    CHECK(battle.GetEffectiveCost(battle.GetPlayer().GetHand()[0]) == 2);

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 35);

    REQUIRE(battle.GetPlayer().GetDiscardPile().size() == 1u);
    CHECK(battle.GetEffectiveCost(battle.GetPlayer().GetDiscardPile()[0]) == 1);
}

TEST_CASE("Steam Barrier gives a little less every time")
{
    Battle battle = BattleWith({ CardId::STEAM_BARRIER }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetPlayer().GetBlock() == 6);

    REQUIRE(battle.EndTurn() == true);

    REQUIRE(battle.PlayCard(Idx(battle, "Steam Barrier")) == true);
    CHECK(battle.GetPlayer().GetBlock() == 5);
}

TEST_CASE("Stack blocks for the size of the discard pile")
{
    Battle battle = BattleWith({ CardId::STACK }, { Dummy(50) });

    for (int i = 0; i < 4; ++i)
    {
        battle.GetPlayer().GetDiscardPile().emplace_back(
            CardRegistry::Get(CardId::STRIKE_BLUE));
    }

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetPlayer().GetBlock() == 4);
}

TEST_CASE("Aggregate hands over energy for the draw pile")
{
    Battle battle = BattleWith({ CardId::AGGREGATE }, { Dummy(50) });

    StackDrawPile(battle, CardId::STRIKE_BLUE, 8);

    REQUIRE(battle.PlayCard(0) == true);

    // Eight cards, one energy for every four, minus the one it cost.
    CHECK(battle.GetPlayer().GetEnergy() == 4);
}

TEST_CASE("Auto Shields only helps when there is no block yet")
{
    Battle battle =
        BattleWith({ CardId::AUTO_SHIELDS, CardId::DEFEND_BLUE },
                   { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Auto-Shields")) == true);
    CHECK(battle.GetPlayer().GetBlock() == 11);

    Battle other =
        BattleWith({ CardId::AUTO_SHIELDS, CardId::DEFEND_BLUE },
                   { Dummy(50) });

    REQUIRE(other.PlayCard(Idx(other, "Defend")) == true);
    REQUIRE(other.PlayCard(Idx(other, "Auto-Shields")) == true);

    CHECK(other.GetPlayer().GetBlock() == 5);
}

TEST_CASE("Blizzard hits for every Frost channelled this battle")
{
    Battle battle = BattleWith({ CardId::BLIZZARD }, { Dummy(50) });

    battle.ChannelOrb(OrbType::FROST);
    battle.ChannelOrb(OrbType::FROST);

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 46);
}

TEST_CASE("Lock-On makes the orbs hit harder")
{
    Battle battle =
        BattleWith({ CardId::BULLSEYE, CardId::ZAP }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Bullseye")) == true);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::LOCK_ON) == 2);
    CHECK(battle.GetMonsters()[0].GetHealth() == 42);

    REQUIRE(battle.PlayCard(Idx(battle, "Zap")) == true);
    REQUIRE(battle.EndTurn() == true);

    // 3 from the orb, raised to 4 by Lock-On.
    CHECK(battle.GetMonsters()[0].GetHealth() == 38);
}

TEST_CASE("Chaos throws whatever orb comes to hand")
{
    Battle battle = BattleWith({ CardId::CHAOS }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);

    REQUIRE(battle.GetPlayer().GetOrbs().size() == 1u);
    CHECK(battle.GetPlayer().GetOrbs()[0].type != OrbType::INVALID);
}

TEST_CASE("Chill channels a Frost for every enemy")
{
    Battle battle = BattleWith({ CardId::CHILL }, { Dummy(50), Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);

    REQUIRE(battle.GetPlayer().GetOrbs().size() == 2u);
    CHECK(battle.GetPlayer().GetOrbs()[0].type == OrbType::FROST);
    CHECK(battle.GetPlayer().GetOrbs()[1].type == OrbType::FROST);
}

TEST_CASE("Electrodynamics spreads the Lightning over everything")
{
    Battle battle =
        BattleWith({ CardId::ELECTRODYNAMICS }, { Dummy(50), Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    REQUIRE(battle.GetPlayer().GetOrbs().size() == 2u);

    REQUIRE(battle.EndTurn() == true);

    // Two orbs, 3 damage each, and both monsters felt both.
    for (const auto& monster : battle.GetMonsters())
    {
        CHECK(monster.GetHealth() == 44);
    }
}

TEST_CASE("Fission trades the orbs for energy and cards")
{
    Battle battle = BattleWith({ CardId::FISSION }, { Dummy(50) });

    StackDrawPile(battle, CardId::STRIKE_BLUE, 4);

    battle.ChannelOrb(OrbType::LIGHTNING);
    battle.ChannelOrb(OrbType::LIGHTNING);

    REQUIRE(battle.PlayCard(0) == true);

    // Two orbs went, so two energy and two cards came back. The base card
    // does not set them off on the way out.
    CHECK(battle.GetPlayer().GetOrbs().empty());
    CHECK(battle.GetPlayer().GetEnergy() == 5);
    CHECK(battle.GetPlayer().GetHand().size() == 2u);
    CHECK(battle.GetMonsters()[0].GetHealth() == 50);
}

TEST_CASE("An upgraded Fission evokes the orbs as they go")
{
    Player player("Defect", 75);
    player.AddCardToDeck(CardRegistry::Get(CardId::FISSION, 1));

    Battle battle(std::move(player), { Dummy(50) }, 23);
    battle.Start();

    battle.ChannelOrb(OrbType::LIGHTNING);
    battle.ChannelOrb(OrbType::LIGHTNING);

    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetPlayer().GetOrbs().empty());
    CHECK(battle.GetMonsters()[0].GetHealth() == 34);
}

TEST_CASE("Loop sets the front orb off again at the start of the turn")
{
    Battle battle = BattleWith({ CardId::ZAP, CardId::LOOP }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Zap")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Loop")) == true);

    // 3 at the end of this turn, then 3 more at the start of the next.
    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 44);
}

TEST_CASE("Machine Learning draws one more card every turn")
{
    Battle battle = BattleWith({ CardId::MACHINE_LEARNING }, { Dummy(50) });

    StackDrawPile(battle, CardId::STRIKE_BLUE, 8);

    REQUIRE(battle.PlayCard(0) == true);
    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetPlayer().GetHand().size() == 6u);
}

TEST_CASE("Echo Form plays the first card of the turn twice")
{
    Battle battle =
        BattleWith({ CardId::ECHO_FORM, CardId::STRIKE_BLUE }, { Dummy(50) });

    // The card that grants it does not echo itself.
    REQUIRE(battle.PlayCard(Idx(battle, "Echo Form")) == true);
    CHECK(battle.GetPlayer().GetPower(PowerType::ECHO_FORM) == 1);

    REQUIRE(battle.EndTurn() == true);

    REQUIRE(battle.PlayCard(Idx(battle, "Strike")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 38);
}

TEST_CASE("Buffer shrugs off a hit entirely")
{
    Battle battle = BattleWith({ CardId::BUFFER }, { Attacker(30, 12) });

    REQUIRE(battle.PlayCard(0) == true);
    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetPlayer().GetHealth() == 75);
    CHECK(battle.GetPlayer().GetPower(PowerType::BUFFER) == 0);

    // Only the one hit.
    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetHealth() == 63);
}

TEST_CASE("Static Discharge answers a hit with a Lightning orb")
{
    Battle battle =
        BattleWith({ CardId::STATIC_DISCHARGE }, { Attacker(30, 5) });

    REQUIRE(battle.PlayCard(0) == true);
    REQUIRE(battle.EndTurn() == true);

    REQUIRE(battle.GetPlayer().GetOrbs().size() == 1u);
    CHECK(battle.GetPlayer().GetOrbs()[0].type == OrbType::LIGHTNING);
}

TEST_CASE("Storm and Heatsinks answer every power played")
{
    Battle battle =
        BattleWith({ CardId::STORM, CardId::HEATSINKS, CardId::DEFRAGMENT },
                   { Dummy(50) });

    StackDrawPile(battle, CardId::STRIKE_BLUE, 5);

    REQUIRE(battle.PlayCard(Idx(battle, "Storm")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Heatsinks")) == true);

    // Storm answered the Heatsinks that came after it.
    CHECK(battle.GetPlayer().GetOrbs().size() == 1u);

    const std::size_t handBefore = battle.GetPlayer().GetHand().size();

    REQUIRE(battle.PlayCard(Idx(battle, "Defragment")) == true);

    // Another orb from Storm, and a card from Heatsinks.
    CHECK(battle.GetPlayer().GetOrbs().size() == 2u);
    CHECK(battle.GetPlayer().GetHand().size() == handBefore);
}

TEST_CASE("Biased Cognition hands Focus over and takes it back")
{
    Battle battle = BattleWith({ CardId::BIASED_COGNITION }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetPlayer().GetPower(PowerType::FOCUS) == 4);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetPower(PowerType::FOCUS) == 3);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetPower(PowerType::FOCUS) == 2);
}

TEST_CASE("Hyperbeam pays for its damage with Focus")
{
    Battle battle =
        BattleWith({ CardId::DEFRAGMENT, CardId::HYPERBEAM },
                   { Dummy(50), Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Defragment")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Hyperbeam")) == true);

    for (const auto& monster : battle.GetMonsters())
    {
        CHECK(monster.GetHealth() == 24);
    }

    CHECK(battle.GetPlayer().GetPower(PowerType::FOCUS) == -2);
}

TEST_CASE("Force Field gets cheaper with every power played")
{
    Battle battle =
        BattleWith({ CardId::FORCE_FIELD, CardId::DEFRAGMENT },
                   { Dummy(50) });

    const std::size_t before = Idx(battle, "Force Field");
    CHECK(battle.GetEffectiveCost(battle.GetPlayer().GetHand()[before]) == 4);

    REQUIRE(battle.PlayCard(Idx(battle, "Defragment")) == true);

    const std::size_t after = Idx(battle, "Force Field");
    CHECK(battle.GetEffectiveCost(battle.GetPlayer().GetHand()[after]) == 3);
}

TEST_CASE("Melter strips the block off first")
{
    Battle battle = BattleWith({ CardId::MELTER }, { Dummy(50) });

    battle.GetMonsters()[0].AddBlock(20);

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetBlock() == 0);
    CHECK(battle.GetMonsters()[0].GetHealth() == 40);
}

TEST_CASE("Recycle trades a card for its cost in energy")
{
    Battle battle =
        BattleWith({ CardId::RECYCLE, CardId::METEOR_STRIKE }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Recycle"), 0, 0) == true);

    // One energy for the Recycle, five back from the Meteor Strike.
    CHECK(battle.GetPlayer().GetEnergy() == 7);
    CHECK(battle.GetPlayer().GetExhaustPile().size() == 1u);
}

TEST_CASE("Double Energy doubles what is left")
{
    Battle battle = BattleWith({ CardId::DOUBLE_ENERGY }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetPlayer().GetEnergy() == 4);
}

TEST_CASE("Reboot shuffles everything back and draws again")
{
    Battle battle =
        BattleWith({ CardId::REBOOT, CardId::STRIKE_BLUE, CardId::DEFEND_BLUE },
                   { Dummy(50) });

    StackDrawPile(battle, CardId::STRIKE_BLUE, 3);

    REQUIRE(battle.PlayCard(Idx(battle, "Reboot")) == true);

    CHECK(battle.GetPlayer().GetHand().size() == 4u);
    CHECK(battle.GetPlayer().GetDiscardPile().empty());
}

TEST_CASE("Seek takes a card straight out of the draw pile")
{
    Battle battle = BattleWith({ CardId::SEEK }, { Dummy(50) });

    StackDrawPile(battle, CardId::METEOR_STRIKE, 1);

    REQUIRE(battle.PlayCard(0) == true);

    REQUIRE(battle.GetPlayer().GetHand().size() == 1u);
    CHECK(battle.GetPlayer().GetHand()[0].GetName() == "Meteor Strike");
    CHECK(battle.GetPlayer().GetDrawPile().empty());
}

TEST_CASE("All for One takes back everything that costs nothing")
{
    Battle battle = BattleWith({ CardId::ALL_FOR_ONE }, { Dummy(50) });

    battle.GetPlayer().GetDiscardPile().emplace_back(
        CardRegistry::Get(CardId::CLAW));
    battle.GetPlayer().GetDiscardPile().emplace_back(
        CardRegistry::Get(CardId::METEOR_STRIKE));

    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 40);
    REQUIRE(battle.GetPlayer().GetHand().size() == 1u);
    CHECK(battle.GetPlayer().GetHand()[0].GetName() == "Claw");
    CHECK(battle.GetPlayer().GetDiscardPile().size() == 2u);
}

TEST_CASE("Genetic Algorithm grows every time it is used")
{
    Battle battle = BattleWith({ CardId::GENETIC_ALGORITHM }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetPlayer().GetBlock() == 1);

    // It exhausts, so the copy that grew is the one in the exhaust pile.
    REQUIRE(battle.GetPlayer().GetExhaustPile().size() == 1u);
    CHECK(battle.GetPlayer().GetExhaustPile()[0].GetBonusBlock() == 2);
}

TEST_CASE("Multi-Cast evokes once for every energy spent")
{
    Battle battle = BattleWith({ CardId::MULTI_CAST }, { Dummy(50) });

    battle.ChannelOrb(OrbType::LIGHTNING);

    REQUIRE(battle.PlayCard(0) == true);

    // Three energy, so the orb fires three times.
    CHECK(battle.GetMonsters()[0].GetHealth() == 26);
}

TEST_CASE("Tempest channels a Lightning for every energy spent")
{
    Battle battle = BattleWith({ CardId::TEMPEST }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetPlayer().GetOrbs().size() == 3u);
    CHECK(battle.GetPlayer().GetExhaustPile().size() == 1u);
}

TEST_CASE("The Defect pool holds every card of the character")
{
    CHECK(CardRegistry::GetPool(CardColor::BLUE, CardRarity::BASIC).size() ==
          4u);
    CHECK(CardRegistry::GetPool(CardColor::BLUE, CardRarity::COMMON).size() ==
          18u);
    CHECK(CardRegistry::GetPool(CardColor::BLUE,
                                CardRarity::UNCOMMON).size() == 36u);
    CHECK(CardRegistry::GetPool(CardColor::BLUE, CardRarity::RARE).size() ==
          16u);
    CHECK(CardRegistry::GetPool(CardColor::BLUE).size() == 74u);

    for (const CardId id : CardRegistry::GetPool(CardColor::BLUE))
    {
        const Card card = CardRegistry::Get(id);

        CHECK(card.GetId() == id);
        CHECK(card.GetColor() == CardColor::BLUE);
        CHECK(card.GetName().empty() == false);
        CHECK(card.GetCardType() != CardType::INVALID);
        CHECK(card.GetRarity() != CardRarity::INVALID);
    }
}

TEST_CASE("The Defect starts a run with the deck it should")
{
    const std::vector<Card> deck =
        CardRegistry::MakeStarterDeck(CardColor::BLUE);

    REQUIRE(deck.size() == 10u);

    int strikes = 0;
    int defends = 0;

    for (const auto& card : deck)
    {
        if (card.GetId() == CardId::STRIKE_BLUE)
        {
            ++strikes;
        }
        else if (card.GetId() == CardId::DEFEND_BLUE)
        {
            ++defends;
        }
    }

    CHECK(strikes == 4);
    CHECK(defends == 4);
    CHECK(deck[8].GetId() == CardId::ZAP);
    CHECK(deck[9].GetId() == CardId::DUALCAST);
}
