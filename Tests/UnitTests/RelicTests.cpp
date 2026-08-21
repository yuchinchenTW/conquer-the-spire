#include "doctest.h"

#include <conquer-the-spire/Battle/Battle.hpp>
#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Enums/CardId.hpp>
#include <conquer-the-spire/Monsters/MonsterLibrary.hpp>
#include <conquer-the-spire/Relics/RelicRegistry.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

using namespace ConquerTheSpire;

namespace
{
//! Builds a started battle with \p relics in hand and a deck of exactly
//! \p ids.
Battle BattleWith(const std::vector<CardId>& ids,
                  const std::vector<RelicId>& relics,
                  std::vector<Monster> monsters, int playerHealth = 80)
{
    Player player("Ironclad", playerHealth);

    for (const CardId id : ids)
    {
        player.AddCardToDeck(CardRegistry::Get(id));
    }

    for (const RelicId id : relics)
    {
        player.AddRelic(RelicRegistry::Get(id));
    }

    Battle battle(std::move(player), std::move(monsters), 13);
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

//! A monster that lands a Weak on the player every turn.
Monster Weakener(int health)
{
    return Monster("Weakener", health,
                   { MonsterMove::Debuff("Lick", PowerType::WEAK, 1) });
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

TEST_CASE("Anchor hands over block that survives the first turn")
{
    Battle battle =
        BattleWith({ CardId::STRIKE_RED }, { RelicId::ANCHOR }, { Dummy(50) });

    CHECK(battle.GetPlayer().GetBlock() == 10);
}

TEST_CASE("Vajra and Oddly Smooth Stone open with Strength and Dexterity")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED, CardId::DEFEND_RED },
                               { RelicId::VAJRA,
                                 RelicId::ODDLY_SMOOTH_STONE },
                               { Dummy(50) });

    CHECK(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 1);
    CHECK(battle.GetPlayer().GetPower(PowerType::DEXTERITY) == 1);

    REQUIRE(battle.PlayCard(Idx(battle, "Strike")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 43);

    REQUIRE(battle.PlayCard(Idx(battle, "Defend")) == true);
    CHECK(battle.GetPlayer().GetBlock() == 6);
}

TEST_CASE("Bag of Marbles shakes everything as the battle opens")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { RelicId::BAG_OF_MARBLES },
                               { Dummy(50), Dummy(50) });

    for (const auto& monster : battle.GetMonsters())
    {
        CHECK(monster.GetPower(PowerType::VULNERABLE) == 1);
    }
}

TEST_CASE("Bag of Preparation draws on top of the opening hand")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED, CardId::STRIKE_RED,
                                 CardId::STRIKE_RED, CardId::STRIKE_RED,
                                 CardId::STRIKE_RED, CardId::STRIKE_RED,
                                 CardId::STRIKE_RED },
                               { RelicId::BAG_OF_PREPARATION },
                               { Dummy(50) });

    CHECK(battle.GetPlayer().GetHand().size() == 7u);
}

TEST_CASE("Bronze Scales hurts whatever attacks the player")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { RelicId::BRONZE_SCALES },
                               { Attacker(30, 5) });

    REQUIRE(battle.GetPlayer().GetPower(PowerType::THORNS) == 3);
    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 27);
}

TEST_CASE("Cracked Core and Nuclear Battery open with an orb")
{
    Battle battle = BattleWith({ CardId::STRIKE_BLUE },
                               { RelicId::CRACKED_CORE,
                                 RelicId::NUCLEAR_BATTERY },
                               { Dummy(50) });

    REQUIRE(battle.GetPlayer().GetOrbs().size() == 2u);
    CHECK(battle.GetPlayer().GetOrbs()[0].type == OrbType::LIGHTNING);
    CHECK(battle.GetPlayer().GetOrbs()[1].type == OrbType::PLASMA);
}

TEST_CASE("Pure Water hands over a Miracle")
{
    Battle battle = BattleWith({ CardId::STRIKE_GREEN },
                               { RelicId::PURE_WATER }, { Dummy(50) });

    CHECK(Idx(battle, "Miracle") < battle.GetPlayer().GetHand().size());
}

TEST_CASE("Ring of the Snake only draws on the first turn")
{
    Battle battle = BattleWith({ CardId::STRIKE_GREEN, CardId::STRIKE_GREEN,
                                 CardId::STRIKE_GREEN, CardId::STRIKE_GREEN,
                                 CardId::STRIKE_GREEN, CardId::STRIKE_GREEN,
                                 CardId::STRIKE_GREEN },
                               { RelicId::RING_OF_THE_SNAKE },
                               { Dummy(50) });

    CHECK(battle.GetPlayer().GetHand().size() == 7u);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetHand().size() == 5u);
}

TEST_CASE("Lantern pays out on the first turn and Happy Flower every third")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { RelicId::LANTERN, RelicId::HAPPY_FLOWER },
                               { Dummy(50) });

    CHECK(battle.GetPlayer().GetEnergy() == 4);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetEnergy() == 3);

    // The third turn brings the flower round.
    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetEnergy() == 4);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetEnergy() == 3);
}

TEST_CASE("Kunai, Shuriken and Ornamental Fan count the attacks")
{
    Battle battle = BattleWith({ CardId::SWIFT_STRIKE, CardId::SWIFT_STRIKE,
                                 CardId::SWIFT_STRIKE },
                               { RelicId::KUNAI, RelicId::SHURIKEN,
                                 RelicId::ORNAMENTAL_FAN },
                               { Dummy(50) });

    for (int i = 0; i < 2; ++i)
    {
        REQUIRE(battle.PlayCard(Idx(battle, "Swift Strike")) == true);
    }

    CHECK(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 0);

    // The third attack brings all three round.
    REQUIRE(battle.PlayCard(Idx(battle, "Swift Strike")) == true);

    CHECK(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 1);
    CHECK(battle.GetPlayer().GetPower(PowerType::DEXTERITY) == 1);

    // The Kunai went first, so the Dexterity is already in the block.
    CHECK(battle.GetPlayer().GetBlock() == 5);
}

TEST_CASE("Letter Opener counts the skills")
{
    Battle battle = BattleWith({ CardId::DEFEND_RED, CardId::DEFEND_RED,
                                 CardId::DEFEND_RED },
                               { RelicId::LETTER_OPENER },
                               { Dummy(50), Dummy(50) });

    for (int i = 0; i < 3; ++i)
    {
        REQUIRE(battle.PlayCard(Idx(battle, "Defend")) == true);
    }

    for (const auto& monster : battle.GetMonsters())
    {
        CHECK(monster.GetHealth() == 45);
    }
}

TEST_CASE("Nunchaku counts ten attacks")
{
    Battle battle = BattleWith({ CardId::SWIFT_STRIKE, CardId::SWIFT_STRIKE,
                                 CardId::SWIFT_STRIKE, CardId::SWIFT_STRIKE,
                                 CardId::SWIFT_STRIKE },
                               { RelicId::NUNCHAKU }, { Dummy(200) });

    for (int i = 0; i < 5; ++i)
    {
        REQUIRE(battle.PlayCard(Idx(battle, "Swift Strike")) == true);
    }

    REQUIRE(battle.EndTurn() == true);
    REQUIRE(battle.GetPlayer().GetEnergy() == 3);

    for (int i = 0; i < 4; ++i)
    {
        REQUIRE(battle.PlayCard(Idx(battle, "Swift Strike")) == true);
    }

    CHECK(battle.GetPlayer().GetEnergy() == 3);

    // The tenth attack pays out.
    REQUIRE(battle.PlayCard(Idx(battle, "Swift Strike")) == true);
    CHECK(battle.GetPlayer().GetEnergy() == 4);
}

TEST_CASE("Sundial and The Abacus watch the shuffles")
{
    Battle battle =
        BattleWith({ CardId::STRIKE_RED, CardId::STRIKE_RED },
                   { RelicId::SUNDIAL, RelicId::THE_ABACUS }, { Dummy(50) });

    // Three trips through the deck bring the Sundial round.
    for (int turn = 0; turn < 3; ++turn)
    {
        REQUIRE(battle.EndTurn() == true);
    }

    CHECK(battle.GetPlayer().GetEnergy() == 5);
    CHECK(battle.GetPlayer().GetBlock() == 6);
}

TEST_CASE("Mercury Hourglass burns everything every turn")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { RelicId::MERCURY_HOURGLASS },
                               { Dummy(50), Dummy(50) });

    for (const auto& monster : battle.GetMonsters())
    {
        CHECK(monster.GetHealth() == 47);
    }

    REQUIRE(battle.EndTurn() == true);

    for (const auto& monster : battle.GetMonsters())
    {
        CHECK(monster.GetHealth() == 44);
    }
}

TEST_CASE("Horn Cleat, Captain's Wheel and Stone Calendar wait for their turn")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { RelicId::HORN_CLEAT,
                                 RelicId::CAPTAINS_WHEEL,
                                 RelicId::STONE_CALENDAR },
                               { Dummy(200) });

    CHECK(battle.GetPlayer().GetBlock() == 0);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetBlock() == 14);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetBlock() == 18);

    // The calendar goes off at the end of the seventh turn.
    for (int turn = 3; turn <= 7; ++turn)
    {
        REQUIRE(battle.EndTurn() == true);
    }

    CHECK(battle.GetMonsters()[0].GetHealth() == 148);
}

TEST_CASE("Gremlin Horn pays out for a kill")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { RelicId::GREMLIN_HORN },
                               { Dummy(4), Dummy(50) });

    StackDrawPile(battle, CardId::DEFEND_RED, 3);

    const std::size_t handBefore = battle.GetPlayer().GetHand().size();

    REQUIRE(battle.PlayCard(Idx(battle, "Strike"), 0) == true);

    CHECK(battle.GetMonsters()[0].IsDead() == true);
    CHECK(battle.GetPlayer().GetEnergy() == 3);
    CHECK(battle.GetPlayer().GetHand().size() == handBefore);
}

TEST_CASE("Orichalcum blocks for a turn that ended with nothing")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { RelicId::ORICHALCUM },
                               { Attacker(30, 4) });

    REQUIRE(battle.EndTurn() == true);

    // The 6 block from the relic soaked the 4 damage.
    CHECK(battle.GetPlayer().GetHealth() == 80);
}

TEST_CASE("Akabeko puts everything behind the first attack")
{
    Battle battle =
        BattleWith({ CardId::STRIKE_RED, CardId::STRIKE_RED },
                   { RelicId::AKABEKO }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Strike")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 36);

    // Only the first one.
    REQUIRE(battle.PlayCard(Idx(battle, "Strike")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 30);
}

TEST_CASE("Strike Dummy only helps the cards named Strike")
{
    Battle battle =
        BattleWith({ CardId::STRIKE_RED, CardId::SWIFT_STRIKE, CardId::CLEAVE },
                   { RelicId::STRIKE_DUMMY }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Strike")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 41);

    REQUIRE(battle.PlayCard(Idx(battle, "Swift Strike")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 31);

    // Cleave is not a Strike.
    REQUIRE(battle.PlayCard(Idx(battle, "Cleave")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 23);
}

TEST_CASE("The Boot puts a floor under a small hit")
{
    Battle battle = BattleWith({ CardId::CLAW }, { RelicId::THE_BOOT },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 45);
}

TEST_CASE("Calipers only lets fifteen block go")
{
    Battle battle = BattleWith({ CardId::IMPERVIOUS }, { RelicId::CALIPERS },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    REQUIRE(battle.GetPlayer().GetBlock() == 30);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetBlock() == 15);
}

TEST_CASE("Ice Cream keeps the energy that was not spent")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED }, { RelicId::ICE_CREAM },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    REQUIRE(battle.GetPlayer().GetEnergy() == 2);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetEnergy() == 5);
}

TEST_CASE("Runic Pyramid never lets the hand go")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED, CardId::DEFEND_RED },
                               { RelicId::RUNIC_PYRAMID }, { Dummy(50) });

    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetPlayer().GetHand().size() == 2u);
    CHECK(battle.GetPlayer().GetDiscardPile().empty());
}

TEST_CASE("Velvet Choker stops after six cards and pays an energy for it")
{
    Battle battle = BattleWith({ CardId::SWIFT_STRIKE, CardId::SWIFT_STRIKE,
                                 CardId::SWIFT_STRIKE, CardId::SWIFT_STRIKE,
                                 CardId::SWIFT_STRIKE },
                               { RelicId::VELVET_CHOKER }, { Dummy(200) });

    CHECK(battle.GetPlayer().GetEnergy() == 4);

    for (int i = 0; i < 5; ++i)
    {
        REQUIRE(battle.PlayCard(Idx(battle, "Swift Strike")) == true);
    }

    battle.GetPlayer().GetHand().emplace_back(
        CardRegistry::Get(CardId::SWIFT_STRIKE));
    battle.GetPlayer().GetHand().emplace_back(
        CardRegistry::Get(CardId::SWIFT_STRIKE));

    REQUIRE(battle.PlayCard(0) == true);

    // That was the sixth.
    CHECK(battle.CanPlay(0) == false);
}

TEST_CASE("Ginger and Turnip turn the debuffs away")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { RelicId::GINGER, RelicId::TURNIP },
                               { Weakener(30) });

    battle.GetPlayer().AddPower(PowerType::FRAIL, 0);

    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetPlayer().GetPower(PowerType::WEAK) == 0);
}

TEST_CASE("Torii turns a small hit into almost nothing")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED }, { RelicId::TORII },
                               { Attacker(30, 5) });

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetHealth() == 79);
}

TEST_CASE("Tungsten Rod takes the edge off every health loss")
{
    Battle battle = BattleWith({ CardId::BLOODLETTING },
                               { RelicId::TUNGSTEN_ROD }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);

    // Three health became two.
    CHECK(battle.GetPlayer().GetHealth() == 78);
}

TEST_CASE("Blue Candle makes a curse playable at the price of health")
{
    Battle battle = BattleWith({ CardId::REGRET }, { RelicId::BLUE_CANDLE },
                               { Dummy(50) });

    REQUIRE(battle.CanPlay(0) == true);
    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetPlayer().GetHealth() == 79);
    CHECK(battle.GetPlayer().GetExhaustPile().size() == 1u);
    CHECK(battle.GetPlayer().GetHand().empty());
}

TEST_CASE("Medical Kit makes a status playable")
{
    Battle battle = BattleWith({ CardId::WOUND }, { RelicId::MEDICAL_KIT },
                               { Dummy(50) });

    REQUIRE(battle.CanPlay(0) == true);
    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetPlayer().GetExhaustPile().size() == 1u);
}

TEST_CASE("Chemical X counts two more into an X cost")
{
    Battle battle = BattleWith({ CardId::WHIRLWIND }, { RelicId::CHEMICAL_X },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);

    // Three energy and two for free, five hits of 5.
    CHECK(battle.GetMonsters()[0].GetHealth() == 25);
    CHECK(battle.GetPlayer().GetEnergy() == 0);
}

TEST_CASE("Unceasing Top hands over a card when the hand runs out")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { RelicId::UNCEASING_TOP }, { Dummy(50) });

    StackDrawPile(battle, CardId::DEFEND_RED, 2);

    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetPlayer().GetHand().size() == 1u);
    CHECK(battle.GetPlayer().GetHand()[0].GetName() == "Defend");
}

TEST_CASE("Lizard Tail picks the player back up once")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { RelicId::LIZARD_TAIL },
                               { Attacker(30, 20) }, 10);

    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetPhase() != BattlePhase::LOST);
    CHECK(battle.GetPlayer().GetHealth() == 5);

    // Only the once.
    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPhase() == BattlePhase::LOST);
}

TEST_CASE("Pocketwatch pays out for a quiet turn")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED, CardId::STRIKE_RED,
                                 CardId::STRIKE_RED, CardId::STRIKE_RED,
                                 CardId::STRIKE_RED, CardId::STRIKE_RED,
                                 CardId::STRIKE_RED, CardId::STRIKE_RED },
                               { RelicId::POCKETWATCH }, { Dummy(200) });

    REQUIRE(battle.PlayCard(0) == true);
    REQUIRE(battle.EndTurn() == true);

    // Five for the turn and three more from the relic.
    CHECK(battle.GetPlayer().GetHand().size() == 8u);
}

TEST_CASE("Hovering Kite pays for the first card thrown away each turn")
{
    Battle battle = BattleWith({ CardId::PREPARED, CardId::STRIKE_RED },
                               { RelicId::HOVERING_KITE }, { Dummy(50) });

    StackDrawPile(battle, CardId::DEFEND_RED, 1);

    REQUIRE(battle.PlayCard(Idx(battle, "Prepared"), 0, 0) == true);

    CHECK(battle.GetPlayer().GetEnergy() == 4);
}

TEST_CASE("Art of War pays out after a turn without an attack")
{
    Battle battle = BattleWith({ CardId::DEFEND_RED, CardId::STRIKE_RED },
                               { RelicId::ART_OF_WAR }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Defend")) == true);
    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetPlayer().GetEnergy() == 4);

    REQUIRE(battle.PlayCard(Idx(battle, "Strike")) == true);
    REQUIRE(battle.EndTurn() == true);

    // An attack was played, so nothing extra this time.
    CHECK(battle.GetPlayer().GetEnergy() == 3);
}

TEST_CASE("Centennial Puzzle and Runic Cube answer the first health lost")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED, CardId::STRIKE_RED,
                                 CardId::STRIKE_RED, CardId::STRIKE_RED,
                                 CardId::STRIKE_RED, CardId::STRIKE_RED,
                                 CardId::STRIKE_RED, CardId::STRIKE_RED,
                                 CardId::STRIKE_RED, CardId::STRIKE_RED },
                               { RelicId::CENTENNIAL_PUZZLE,
                                 RelicId::RUNIC_CUBE },
                               { Attacker(30, 5) });

    REQUIRE(battle.GetPlayer().GetHand().size() == 5u);
    REQUIRE(battle.EndTurn() == true);

    // Five for the turn, one from the Cube and three from the Puzzle.
    CHECK(battle.GetPlayer().GetHand().size() == 9u);
}

TEST_CASE("Orange Pellets clears the debuffs once all three kinds are played")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED, CardId::DEFEND_RED,
                                 CardId::INFLAME },
                               { RelicId::ORANGE_PELLETS }, { Dummy(50) });

    battle.GetPlayer().AddPower(PowerType::VULNERABLE, 2);
    battle.GetPlayer().AddPower(PowerType::WEAK, 2);

    REQUIRE(battle.PlayCard(Idx(battle, "Strike")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Defend")) == true);

    CHECK(battle.GetPlayer().GetPower(PowerType::WEAK) == 2);

    REQUIRE(battle.PlayCard(Idx(battle, "Inflame")) == true);

    CHECK(battle.GetPlayer().GetPower(PowerType::VULNERABLE) == 0);
    CHECK(battle.GetPlayer().GetPower(PowerType::WEAK) == 0);
}

TEST_CASE("The boss relics that cost something refill one more energy")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { RelicId::COFFEE_DRIPPER }, { Dummy(50) });

    CHECK(battle.GetPlayer().GetEnergy() == 4);

    Battle two = BattleWith({ CardId::STRIKE_RED },
                            { RelicId::SOZU, RelicId::RUNIC_DOME },
                            { Dummy(50) });

    CHECK(two.GetPlayer().GetEnergy() == 5);
}

TEST_CASE("Snecko Eye draws two more every turn")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED, CardId::STRIKE_RED,
                                 CardId::STRIKE_RED, CardId::STRIKE_RED,
                                 CardId::STRIKE_RED, CardId::STRIKE_RED,
                                 CardId::STRIKE_RED },
                               { RelicId::SNECKO_EYE }, { Dummy(50) });

    CHECK(battle.GetPlayer().GetHand().size() == 7u);
}

TEST_CASE("Charon's Ashes and Dead Branch answer an exhausted card")
{
    Battle battle = BattleWith({ CardId::SEEING_RED },
                               { RelicId::CHARONS_ASHES,
                                 RelicId::DEAD_BRANCH },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 47);
    CHECK(battle.GetPlayer().GetHand().size() == 1u);
}

TEST_CASE("Fossilized Helix and Clockwork Souvenir open with a guard up")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { RelicId::FOSSILIZED_HELIX,
                                 RelicId::CLOCKWORK_SOUVENIR },
                               { Attacker(30, 12) });

    CHECK(battle.GetPlayer().GetPower(PowerType::BUFFER) == 1);
    CHECK(battle.GetPlayer().GetPower(PowerType::ARTIFACT) == 1);

    REQUIRE(battle.EndTurn() == true);

    // The Buffer ate the hit.
    CHECK(battle.GetPlayer().GetHealth() == 80);
}

TEST_CASE("Incense Burner and Inserter come round every few turns")
{
    Battle battle = BattleWith({ CardId::STRIKE_BLUE },
                               { RelicId::INCENSE_BURNER,
                                 RelicId::INSERTER },
                               { Dummy(200) });

    CHECK(battle.GetPlayer().GetOrbSlots() == 3);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetOrbSlots() == 4);

    for (int turn = 2; turn < 6; ++turn)
    {
        REQUIRE(battle.EndTurn() == true);
    }

    CHECK(battle.GetPlayer().GetPower(PowerType::INTANGIBLE) == 1);

    // The Inserter came round on turns two, four and six.
    CHECK(battle.GetPlayer().GetOrbSlots() == 6);
}

TEST_CASE("Philosopher's Stone gives the enemies Strength as well")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { RelicId::PHILOSOPHERS_STONE },
                               { Attacker(30, 5) });

    CHECK(battle.GetPlayer().GetEnergy() == 4);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::STRENGTH) == 1);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetHealth() == 74);
}

TEST_CASE("Mummified Hand drops the cost of a card when a power lands")
{
    Battle battle = BattleWith({ CardId::INFLAME, CardId::BLUDGEON },
                               { RelicId::MUMMIFIED_HAND }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Inflame")) == true);

    const std::size_t index = Idx(battle, "Bludgeon");
    CHECK(battle.GetEffectiveCost(battle.GetPlayer().GetHand()[index]) == 0);
}

TEST_CASE("Bird-Faced Urn patches the player up when a power lands")
{
    Battle battle = BattleWith({ CardId::INFLAME },
                               { RelicId::BIRD_FACED_URN },
                               { Attacker(30, 10) });

    REQUIRE(battle.EndTurn() == true);
    REQUIRE(battle.GetPlayer().GetHealth() == 70);

    REQUIRE(battle.PlayCard(Idx(battle, "Inflame")) == true);
    CHECK(battle.GetPlayer().GetHealth() == 72);
}

TEST_CASE("Brimstone arms both sides")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED }, { RelicId::BRIMSTONE },
                               { Dummy(50) });

    CHECK(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 2);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::STRENGTH) == 1);

    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 4);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::STRENGTH) == 2);
}

TEST_CASE("Mutagenic Strength lends Strength for the first turn")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { RelicId::MUTAGENIC_STRENGTH },
                               { Dummy(50) });

    CHECK(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 3);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 0);
}

TEST_CASE("Gremlin Visage and Red Mask hand out a Weak")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { RelicId::GREMLIN_VISAGE, RelicId::RED_MASK },
                               { Dummy(50) });

    CHECK(battle.GetPlayer().GetPower(PowerType::WEAK) == 1);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::WEAK) == 1);
}

TEST_CASE("Toolbox hands over a colourless card")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED }, { RelicId::TOOLBOX },
                               { Dummy(50) });

    REQUIRE(battle.GetPlayer().GetHand().size() == 2u);

    bool foundColorless = false;

    for (const auto& card : battle.GetPlayer().GetHand())
    {
        if (card.GetColor() == CardColor::COLORLESS)
        {
            foundColorless = true;
        }
    }

    CHECK(foundColorless == true);
}

TEST_CASE("The registry builds every relic it lists")
{
    const std::vector<RelicId>& all = RelicRegistry::GetAll();

    CHECK(all.size() == 151u);

    for (const RelicId id : all)
    {
        const Relic relic = RelicRegistry::Get(id);

        CHECK(relic.GetId() == id);
        CHECK(relic.GetName().empty() == false);
        CHECK(relic.GetTier() != RelicTier::INVALID);
    }

    CHECK(RelicRegistry::GetPool(RelicTier::STARTER).size() == 4u);
    CHECK(RelicRegistry::GetPool(RelicTier::COMMON).size() == 32u);
    CHECK(RelicRegistry::GetPool(RelicTier::UNCOMMON).size() == 28u);
    CHECK(RelicRegistry::GetPool(RelicTier::RARE).size() == 27u);
    CHECK(RelicRegistry::GetPool(RelicTier::BOSS).size() == 25u);
    CHECK(RelicRegistry::GetPool(RelicTier::SHOP).size() == 17u);
    CHECK(RelicRegistry::GetPool(RelicTier::EVENT).size() == 18u);

    CHECK(RelicRegistry::GetStarterRelic(CardColor::RED) ==
          RelicId::BURNING_BLOOD);
    CHECK(RelicRegistry::GetStarterRelic(CardColor::GREEN) ==
          RelicId::RING_OF_THE_SNAKE);
    CHECK(RelicRegistry::GetStarterRelic(CardColor::BLUE) ==
          RelicId::CRACKED_CORE);

    CHECK(RelicRegistry::BonusMaxHealth(RelicId::STRAWBERRY) == 7);
    CHECK(RelicRegistry::BonusMaxHealth(RelicId::PEAR) == 10);
    CHECK(RelicRegistry::BonusMaxHealth(RelicId::MANGO) == 14);
}

