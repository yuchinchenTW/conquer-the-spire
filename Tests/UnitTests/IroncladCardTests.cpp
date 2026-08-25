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
                  std::vector<Monster> monsters, int playerHealth = 80)
{
    Player player("Ironclad", playerHealth);

    for (const CardId id : ids)
    {
        player.AddCardToDeck(CardRegistry::Get(id));
    }

    Battle battle(std::move(player), std::move(monsters), 7);
    battle.Start();

    return battle;
}

//! A monster that never acts, so a test can watch damage and block alone.
Monster Dummy(int health)
{
    return Monsters::TrainingDummy(health);
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

//! A monster that hits for \p damage every turn.
Monster Attacker(int health, int damage)
{
    return Monster("Attacker", health,
                   { MonsterMove::Attack("Hit", damage) });
}
}  // namespace

TEST_CASE("An X cost spends all the energy and repeats that many times")
{
    Battle battle =
        BattleWith({ CardId::WHIRLWIND }, { Dummy(50) });

    // Three energy, so 5 damage lands three times.
    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 35);
    CHECK(battle.GetPlayer().GetEnergy() == 0);
}

TEST_CASE("Body Slam deals damage equal to the block held")
{
    Battle battle = BattleWith({ CardId::DEFEND_RED, CardId::BODY_SLAM },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Defend")) == true);
    REQUIRE(battle.GetPlayer().GetBlock() == 5);

    REQUIRE(battle.PlayCard(Idx(battle, "Body Slam")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 45);
}

TEST_CASE("Heavy Blade counts Strength three times")
{
    Battle battle = BattleWith({ CardId::INFLAME, CardId::HEAVY_BLADE },
                               { Dummy(100) });

    REQUIRE(battle.PlayCard(Idx(battle, "Inflame")) == true);
    REQUIRE(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 2);

    // 14 base plus three copies of 2 Strength.
    REQUIRE(battle.PlayCard(Idx(battle, "Heavy Blade")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 80);
}

TEST_CASE("Perfected Strike counts every card named Strike, itself included")
{
    Battle battle = BattleWith({ CardId::PERFECTED_STRIKE, CardId::STRIKE_RED,
                                 CardId::STRIKE_RED },
                               { Dummy(50) });

    // 6 base plus 2 for each of the three Strikes.
    REQUIRE(battle.PlayCard(Idx(battle, "Perfected Strike")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 38);
}

TEST_CASE("Searing Blow keeps growing with every upgrade")
{
    CHECK(CardRegistry::Get(CardId::SEARING_BLOW, 0).GetEffects()[0].value ==
          12);
    CHECK(CardRegistry::Get(CardId::SEARING_BLOW, 1).GetEffects()[0].value ==
          16);
    CHECK(CardRegistry::Get(CardId::SEARING_BLOW, 2).GetEffects()[0].value ==
          21);
    CHECK(CardRegistry::Get(CardId::SEARING_BLOW, 3).GetEffects()[0].value ==
          27);
}

TEST_CASE("Armaments upgrades the card it is pointed at")
{
    Battle battle = BattleWith({ CardId::ARMAMENTS, CardId::STRIKE_RED },
                               { Dummy(50) });

    // Armaments leaves the hand first, so the Strike is at index zero.
    REQUIRE(battle.PlayCard(Idx(battle, "Armaments"), 0, 0) == true);
    REQUIRE(battle.GetPlayer().GetHand().size() == 1u);
    CHECK(battle.GetPlayer().GetHand()[0].GetName() == "Strike+");

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 41);
}

TEST_CASE("Anger leaves a copy of itself in the discard pile")
{
    Battle battle =
        BattleWith({ CardId::ANGER }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 44);
    CHECK(battle.GetPlayer().GetDiscardPile().size() == 2u);
}

TEST_CASE("Wild Strike shuffles a Wound into the draw pile")
{
    Battle battle =
        BattleWith({ CardId::WILD_STRIKE }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 38);
    REQUIRE(battle.GetPlayer().GetDrawPile().size() == 1u);
    CHECK(battle.GetPlayer().GetDrawPile()[0].GetId() == CardId::WOUND);
}

TEST_CASE("Power Through puts two Wounds straight into the hand")
{
    Battle battle =
        BattleWith({ CardId::POWER_THROUGH }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetPlayer().GetBlock() == 15);
    REQUIRE(battle.GetPlayer().GetHand().size() == 2u);
    CHECK(battle.GetPlayer().GetHand()[0].GetId() == CardId::WOUND);
    CHECK(battle.GetPlayer().GetHand()[1].GetId() == CardId::WOUND);
}

TEST_CASE("A Wound cannot be played")
{
    Battle battle =
        BattleWith({ CardId::WOUND }, { Dummy(50) });

    CHECK(battle.CanPlay(0) == false);
    CHECK(battle.PlayCard(0) == false);
    CHECK(battle.GetPlayableCardIndices().empty());
}

TEST_CASE("Burn hurts while it is held at the end of the turn")
{
    Battle battle =
        BattleWith({ CardId::BURN }, { Dummy(50) });

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetHealth() == 78);
}

TEST_CASE("Void costs an energy the moment it is drawn")
{
    Battle battle =
        BattleWith({ CardId::VOID }, { Dummy(50) });

    CHECK(battle.GetPlayer().GetEnergy() == 2);
}

TEST_CASE("An ethereal card exhausts itself at the end of the turn")
{
    Battle battle =
        BattleWith({ CardId::DAZED }, { Dummy(50) });

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetExhaustPile().size() == 1u);
    CHECK(battle.GetPlayer().GetDiscardPile().empty());
}

TEST_CASE("Feel No Pain turns exhausted cards into block")
{
    Battle battle = BattleWith({ CardId::FEEL_NO_PAIN, CardId::SEEING_RED },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Feel No Pain")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Seeing Red")) == true);

    CHECK(battle.GetPlayer().GetBlock() == 3);
}

TEST_CASE("Dark Embrace draws a card whenever one is exhausted")
{
    Battle battle =
        BattleWith({ CardId::DARK_EMBRACE, CardId::SEEING_RED,
                     CardId::STRIKE_RED },
                   { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Dark Embrace")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Seeing Red")) == true);

    // The Strike that was left, plus the Dark Embrace drawn back out of the
    // reshuffled discard pile.
    CHECK(battle.GetPlayer().GetHand().size() == 2u);
    CHECK(battle.GetPlayer().GetDiscardPile().empty());
}

TEST_CASE("Demon Form gives Strength at the start of every turn")
{
    Battle battle =
        BattleWith({ CardId::DEMON_FORM }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 0);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 2);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 4);
}

TEST_CASE("Metallicize blocks at the end of the turn")
{
    Battle battle =
        BattleWith({ CardId::METALLICIZE }, { Attacker(30, 3) });

    REQUIRE(battle.PlayCard(0) == true);
    REQUIRE(battle.EndTurn() == true);

    // The 3 block from Metallicize soaked the 3 damage.
    CHECK(battle.GetPlayer().GetHealth() == 80);
}

TEST_CASE("Combust costs health and burns every enemy")
{
    Battle battle =
        BattleWith({ CardId::COMBUST }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetPlayer().GetHealth() == 79);
    CHECK(battle.GetMonsters()[0].GetHealth() == 45);
}

TEST_CASE("Barricade keeps the block across turns")
{
    Battle battle = BattleWith({ CardId::BARRICADE, CardId::DEFEND_RED },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Barricade")) == true);
    REQUIRE(battle.EndTurn() == true);

    REQUIRE(battle.PlayCard(Idx(battle, "Defend")) == true);
    REQUIRE(battle.GetPlayer().GetBlock() == 5);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetBlock() == 5);
}

TEST_CASE("Berserk trades Vulnerable for an extra energy each turn")
{
    Battle battle =
        BattleWith({ CardId::BERSERK }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetPlayer().GetPower(PowerType::VULNERABLE) == 2);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetEnergy() == 4);
}

TEST_CASE("Corruption makes skills free and exhausts them")
{
    Battle battle = BattleWith({ CardId::CORRUPTION, CardId::DEFEND_RED },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Corruption")) == true);
    REQUIRE(battle.GetPlayer().GetEnergy() == 0);

    const std::size_t defendIndex = Idx(battle, "Defend");
    CHECK(battle.GetEffectiveCost(battle.GetPlayer().GetHand()[defendIndex]) ==
          0);

    REQUIRE(battle.PlayCard(defendIndex) == true);
    CHECK(battle.GetPlayer().GetBlock() == 5);
    CHECK(battle.GetPlayer().GetExhaustPile().size() == 1u);
}

TEST_CASE("Double Tap plays the next attack twice")
{
    Battle battle = BattleWith({ CardId::DOUBLE_TAP, CardId::STRIKE_RED },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Double Tap")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Strike")) == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 38);
    CHECK(battle.GetPlayer().GetPower(PowerType::DOUBLE_TAP) == 0);
}

TEST_CASE("Rage blocks every time an attack is played")
{
    Battle battle = BattleWith({ CardId::RAGE, CardId::STRIKE_RED },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Rage")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Strike")) == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 44);
    CHECK(battle.GetPlayer().GetBlock() == 3);
}

TEST_CASE("Rupture turns health spent on a card into Strength")
{
    Battle battle = BattleWith({ CardId::RUPTURE, CardId::BLOODLETTING },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Rupture")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Bloodletting")) == true);

    CHECK(battle.GetPlayer().GetHealth() == 77);
    CHECK(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 1);
    CHECK(battle.GetPlayer().GetEnergy() == 4);
}

TEST_CASE("Juggernaut hits back whenever block is gained")
{
    Battle battle = BattleWith({ CardId::JUGGERNAUT, CardId::DEFEND_RED },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Juggernaut")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Defend")) == true);

    CHECK(battle.GetPlayer().GetBlock() == 5);
    CHECK(battle.GetMonsters()[0].GetHealth() == 45);
}

TEST_CASE("Limit Break doubles the Strength held")
{
    Battle battle = BattleWith({ CardId::INFLAME, CardId::LIMIT_BREAK },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Inflame")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Limit Break")) == true);

    CHECK(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 4);
    CHECK(battle.GetPlayer().GetExhaustPile().size() == 1u);
}

TEST_CASE("Entrench doubles the block held")
{
    Battle battle = BattleWith({ CardId::DEFEND_RED, CardId::ENTRENCH },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Defend")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Entrench")) == true);

    CHECK(battle.GetPlayer().GetBlock() == 10);
}

TEST_CASE("Flame Barrier burns whatever attacks the player")
{
    Battle battle = BattleWith({ CardId::FLAME_BARRIER }, { Attacker(30, 5) });

    REQUIRE(battle.PlayCard(0) == true);
    REQUIRE(battle.GetPlayer().GetBlock() == 12);

    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetPlayer().GetHealth() == 80);
    CHECK(battle.GetMonsters()[0].GetHealth() == 26);
}

TEST_CASE("Second Wind exhausts the cards that are not attacks")
{
    Battle battle =
        BattleWith({ CardId::SECOND_WIND, CardId::DEFEND_RED,
                     CardId::DEFEND_RED, CardId::STRIKE_RED },
                   { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Second Wind")) == true);

    // Two Defends exhausted, 5 block each, and the Strike stayed.
    CHECK(battle.GetPlayer().GetBlock() == 10);
    REQUIRE(battle.GetPlayer().GetHand().size() == 1u);
    CHECK(battle.GetPlayer().GetHand()[0].GetName() == "Strike");
}

TEST_CASE("Fiend Fire hits once for every card it exhausts")
{
    Battle battle = BattleWith({ CardId::FIEND_FIRE, CardId::STRIKE_RED,
                                 CardId::STRIKE_RED },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Fiend Fire")) == true);

    // Two cards exhausted, 7 damage each.
    CHECK(battle.GetMonsters()[0].GetHealth() == 36);
    CHECK(battle.GetPlayer().GetHand().empty());
}

TEST_CASE("Reaper heals for the damage that got through")
{
    Battle battle = BattleWith({ CardId::BLOODLETTING, CardId::REAPER },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Bloodletting")) == true);
    REQUIRE(battle.GetPlayer().GetHealth() == 77);

    REQUIRE(battle.PlayCard(Idx(battle, "Reaper")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 46);
    CHECK(battle.GetPlayer().GetHealth() == 80);
}

TEST_CASE("Feed raises the maximum health when it kills")
{
    Battle battle =
        BattleWith({ CardId::FEED }, { Dummy(5) });

    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetPhase() == BattlePhase::WON);
    CHECK(battle.GetPlayer().GetMaxHealth() == 83);
}

TEST_CASE("Dropkick pays back when the target is Vulnerable")
{
    Battle battle = BattleWith({ CardId::BASH, CardId::DROPKICK },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Bash")) == true);
    REQUIRE(battle.GetPlayer().GetEnergy() == 1);

    // 5 damage raised to 7 by Vulnerable, and the energy comes straight back.
    REQUIRE(battle.PlayCard(Idx(battle, "Dropkick")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 35);
    CHECK(battle.GetPlayer().GetEnergy() == 1);
}

TEST_CASE("Spot Weakness only pays off against a monster winding up to attack")
{
    Battle attacking =
        BattleWith({ CardId::SPOT_WEAKNESS }, { Attacker(30, 5) });

    REQUIRE(attacking.PlayCard(0) == true);
    CHECK(attacking.GetPlayer().GetPower(PowerType::STRENGTH) == 3);

    Battle idle =
        BattleWith({ CardId::SPOT_WEAKNESS }, { Dummy(30) });

    REQUIRE(idle.PlayCard(0) == true);
    CHECK(idle.GetPlayer().GetPower(PowerType::STRENGTH) == 0);
}

TEST_CASE("Clash can only be played from a hand of nothing but attacks")
{
    Battle blocked = BattleWith({ CardId::CLASH, CardId::DEFEND_RED },
                                { Dummy(50) });

    CHECK(blocked.CanPlay(Idx(blocked, "Clash")) == false);

    Battle allowed = BattleWith({ CardId::CLASH, CardId::STRIKE_RED },
                                { Dummy(50) });

    REQUIRE(allowed.PlayCard(Idx(allowed, "Clash")) == true);
    CHECK(allowed.GetMonsters()[0].GetHealth() == 36);
}

TEST_CASE("Blood for Blood gets cheaper every time the player bleeds")
{
    Battle battle =
        BattleWith({ CardId::BLOOD_FOR_BLOOD, CardId::BLOODLETTING },
                   { Dummy(50) });

    const std::size_t bloodIndex = Idx(battle, "Blood for Blood");
    CHECK(battle.GetEffectiveCost(battle.GetPlayer().GetHand()[bloodIndex]) ==
          4);
    CHECK(battle.CanPlay(bloodIndex) == false);

    REQUIRE(battle.PlayCard(Idx(battle, "Bloodletting")) == true);
    REQUIRE(battle.GetHealthLossCount() == 1);

    const std::size_t cheaper = Idx(battle, "Blood for Blood");
    CHECK(battle.GetEffectiveCost(battle.GetPlayer().GetHand()[cheaper]) == 3);

    REQUIRE(battle.PlayCard(cheaper) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 32);
}

TEST_CASE("Sentinel pays out when something else exhausts it")
{
    Battle battle = BattleWith({ CardId::FIEND_FIRE, CardId::SENTINEL },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Fiend Fire")) == true);

    // Two energy for Fiend Fire, two back from the exhausted Sentinel.
    CHECK(battle.GetPlayer().GetEnergy() == 3);
    CHECK(battle.GetMonsters()[0].GetHealth() == 43);
}

TEST_CASE("Havoc plays the top of the draw pile and throws it away")
{
    Battle battle =
        BattleWith({ CardId::HAVOC }, { Dummy(50) });

    battle.GetPlayer().GetDrawPile().emplace_back(
        CardRegistry::Get(CardId::STRIKE_RED));

    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 44);
    REQUIRE(battle.GetPlayer().GetExhaustPile().size() == 1u);
    CHECK(battle.GetPlayer().GetExhaustPile()[0].GetName() == "Strike");
}

TEST_CASE("Exhume brings a card back out of the exhaust pile")
{
    Battle battle =
        BattleWith({ CardId::EXHUME }, { Dummy(50) });

    battle.GetPlayer().GetExhaustPile().emplace_back(
        CardRegistry::Get(CardId::BLUDGEON));

    REQUIRE(battle.PlayCard(0, 0, 0) == true);

    REQUIRE(battle.GetPlayer().GetHand().size() == 1u);
    CHECK(battle.GetPlayer().GetHand()[0].GetName() == "Bludgeon");

    // Exhume exhausts itself, so the pile holds it instead.
    REQUIRE(battle.GetPlayer().GetExhaustPile().size() == 1u);
    CHECK(battle.GetPlayer().GetExhaustPile()[0].GetName() == "Exhume");
}

TEST_CASE("Headbutt puts a card from the discard pile back on top")
{
    Battle battle =
        BattleWith({ CardId::HEADBUTT }, { Dummy(50) });

    battle.GetPlayer().GetDiscardPile().emplace_back(
        CardRegistry::Get(CardId::BLUDGEON));

    REQUIRE(battle.PlayCard(0, 0, 0) == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 41);
    REQUIRE(battle.GetPlayer().GetDrawPile().size() == 1u);
    CHECK(battle.GetPlayer().GetDrawPile()[0].GetName() == "Bludgeon");
}

TEST_CASE("Dual Wield copies the card it is pointed at")
{
    Battle battle = BattleWith({ CardId::DUAL_WIELD, CardId::BLUDGEON },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Dual Wield"), 0, 0) == true);

    REQUIRE(battle.GetPlayer().GetHand().size() == 2u);
    CHECK(battle.GetPlayer().GetHand()[0].GetName() == "Bludgeon");
    CHECK(battle.GetPlayer().GetHand()[1].GetName() == "Bludgeon");
}

TEST_CASE("Infernal Blade hands over an attack that costs nothing this turn")
{
    Battle battle =
        BattleWith({ CardId::INFERNAL_BLADE }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);

    REQUIRE(battle.GetPlayer().GetHand().size() == 1u);

    const Card& made = battle.GetPlayer().GetHand()[0];
    CHECK(made.GetCardType() == CardType::ATTACK);
    CHECK(battle.GetEffectiveCost(made) == 0);
}

TEST_CASE("Evolve draws an extra card whenever a status turns up")
{
    Battle battle = BattleWith({ CardId::EVOLVE, CardId::POMMEL_STRIKE },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Evolve")) == true);

    battle.GetPlayer().GetDrawPile().emplace_back(
        CardRegistry::Get(CardId::WOUND));

    // Pommel Strike draws the Wound, and Evolve draws one more on top.
    REQUIRE(battle.PlayCard(Idx(battle, "Pommel Strike")) == true);
    CHECK(battle.GetPlayer().GetHand().size() == 2u);
}

TEST_CASE("Fire Breathing burns everything when a status turns up")
{
    Battle battle =
        BattleWith({ CardId::FIRE_BREATHING, CardId::POMMEL_STRIKE },
                   { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Fire Breathing")) == true);

    battle.GetPlayer().GetDrawPile().emplace_back(
        CardRegistry::Get(CardId::WOUND));

    // 9 from Pommel Strike, then 6 for drawing the Wound.
    REQUIRE(battle.PlayCard(Idx(battle, "Pommel Strike")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 35);
}

TEST_CASE("Thunderclap hits and shakes every enemy")
{
    Battle battle =
        BattleWith({ CardId::THUNDERCLAP },
                   { Dummy(20), Dummy(20) });

    REQUIRE(battle.PlayCard(0) == true);

    for (const auto& monster : battle.GetMonsters())
    {
        CHECK(monster.GetHealth() == 16);
        CHECK(monster.GetPower(PowerType::VULNERABLE) == 1);
    }
}

TEST_CASE("Flex gives Strength that goes away at the end of the turn")
{
    Battle battle = BattleWith({ CardId::FLEX, CardId::STRIKE_RED },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Flex")) == true);
    CHECK(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 2);

    REQUIRE(battle.PlayCard(Idx(battle, "Strike")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 42);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 0);
}

TEST_CASE("Battle Trance draws three and then stops the draws")
{
    Battle battle =
        BattleWith({ CardId::BATTLE_TRANCE }, { Dummy(50) });

    for (int i = 0; i < 5; ++i)
    {
        battle.GetPlayer().GetDrawPile().emplace_back(
            CardRegistry::Get(CardId::STRIKE_RED));
    }

    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetPlayer().GetHand().size() == 3u);
    CHECK(battle.GetPlayer().GetPower(PowerType::NO_DRAW) == 1);

    // The draw from Shrug It Off would be swallowed by No Draw.
    battle.GetPlayer().GetHand().emplace_back(
        CardRegistry::Get(CardId::SHRUG_IT_OFF));

    REQUIRE(battle.PlayCard(Idx(battle, "Shrug It Off")) == true);
    CHECK(battle.GetPlayer().GetHand().size() == 3u);
}

TEST_CASE("Immolate leaves a Burn behind in the discard pile")
{
    Battle battle =
        BattleWith({ CardId::IMMOLATE }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 29);

    const std::vector<Card>& discard = battle.GetPlayer().GetDiscardPile();
    REQUIRE(discard.size() == 2u);
    CHECK(discard[0].GetId() == CardId::BURN);
}

TEST_CASE("Sword Boomerang hits three times")
{
    Battle battle =
        BattleWith({ CardId::SWORD_BOOMERANG }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 41);
}

TEST_CASE("Offering trades health for energy and cards")
{
    Battle battle =
        BattleWith({ CardId::OFFERING }, { Dummy(50) });

    for (int i = 0; i < 5; ++i)
    {
        battle.GetPlayer().GetDrawPile().emplace_back(
            CardRegistry::Get(CardId::STRIKE_RED));
    }

    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetPlayer().GetHealth() == 74);
    CHECK(battle.GetPlayer().GetEnergy() == 5);
    CHECK(battle.GetPlayer().GetHand().size() == 3u);
    CHECK(battle.GetPlayer().GetExhaustPile().size() == 1u);
}

TEST_CASE("An upgraded card keeps its name with a plus on it")
{
    CHECK(CardRegistry::Get(CardId::STRIKE_RED).GetName() == "Strike");
    CHECK(CardRegistry::Get(CardId::STRIKE_RED, 1).GetName() == "Strike+");
    CHECK(CardRegistry::Get(CardId::STRIKE_RED, 1).IsUpgraded() == true);

    // Statuses and curses have no upgraded form.
    CHECK(CardRegistry::Get(CardId::WOUND, 1).GetName() == "Wound");
    CHECK(CardRegistry::Get(CardId::REGRET, 1).IsUpgraded() == false);
}

TEST_CASE("The registry builds every card it lists")
{
    const CardColor colours[] = { CardColor::RED, CardColor::STATUS,
                                  CardColor::CURSE };

    for (const CardColor colour : colours)
    {
        const std::vector<CardId>& pool = CardRegistry::GetPool(colour);
        CHECK(pool.empty() == false);

        for (const CardId id : pool)
        {
            const Card card = CardRegistry::Get(id);

            CHECK(card.GetId() == id);
            CHECK(card.GetColor() == colour);
            CHECK(card.GetName().empty() == false);
            CHECK(card.GetCardType() != CardType::INVALID);
            CHECK(card.GetTarget() != CardTarget::INVALID);
            CHECK(card.GetRarity() != CardRarity::INVALID);
        }
    }
}

TEST_CASE("The Ironclad pool holds every card of the character")
{
    CHECK(CardRegistry::GetPool(CardColor::RED, CardRarity::BASIC).size() ==
          3u);
    CHECK(CardRegistry::GetPool(CardColor::RED, CardRarity::COMMON).size() ==
          20u);
    CHECK(CardRegistry::GetPool(CardColor::RED, CardRarity::UNCOMMON).size() ==
          36u);
    CHECK(CardRegistry::GetPool(CardColor::RED, CardRarity::RARE).size() ==
          16u);
    CHECK(CardRegistry::GetPool(CardColor::RED).size() == 75u);

    CHECK(CardRegistry::GetPool(CardColor::STATUS).size() == 5u);
    CHECK(CardRegistry::GetPool(CardColor::CURSE).size() == 14u);

    // Infernal Blade only pulls attacks that a run can hand out.
    const std::vector<CardId> attacks =
        CardRegistry::GetAttackPool(CardColor::RED);

    CHECK(attacks.empty() == false);

    for (const CardId id : attacks)
    {
        const Card card = CardRegistry::Get(id);

        CHECK(card.GetCardType() == CardType::ATTACK);
        CHECK(card.GetRarity() != CardRarity::BASIC);
        CHECK(card.GetRarity() != CardRarity::SPECIAL);
    }
}

TEST_CASE("The Ironclad starts a run with the deck it should")
{
    const std::vector<Card> deck =
        CardRegistry::MakeStarterDeck(CardColor::RED);

    REQUIRE(deck.size() == 10u);

    int strikes = 0;
    int defends = 0;
    int bashes = 0;

    for (const auto& card : deck)
    {
        if (card.GetId() == CardId::STRIKE_RED)
        {
            ++strikes;
        }
        else if (card.GetId() == CardId::DEFEND_RED)
        {
            ++defends;
        }
        else if (card.GetId() == CardId::BASH)
        {
            ++bashes;
        }
    }

    CHECK(strikes == 5);
    CHECK(defends == 4);
    CHECK(bashes == 1);
}

TEST_CASE("What a card keeps giving is told apart from what it gives once")
{
    // Inflame hands two Strength over and is done; Demon Form hands two over
    // every turn for the rest of the fight. Both used to come out at power
    // two, which is the same three numbers for a common card and a rare one.
    const CardWorth& inflame = CardRegistry::Worth(CardId::INFLAME, 0);
    const CardWorth& demon = CardRegistry::Worth(CardId::DEMON_FORM, 0);

    CHECK(inflame.power == 2);
    CHECK(inflame.lasting == 0);
    CHECK(demon.power == 0);
    CHECK(demon.lasting == 2);

    // Metallicize blocks three every turn, Corruption changes what the cards
    // themselves cost - neither is a one-off.
    CHECK(CardRegistry::Worth(CardId::METALLICIZE, 0).lasting == 3);
    CHECK(CardRegistry::Worth(CardId::CORRUPTION, 0).lasting > 1);
    CHECK(CardRegistry::Worth(CardId::CORRUPTION, 0).power == 0);

    // Flex hands two Strength over and takes them back at the end of the
    // turn. Counting the taking back as more giving made it the strongest
    // card on the table for a card that does nothing.
    const CardWorth& flex = CardRegistry::Worth(CardId::FLEX, 0);

    CHECK(flex.power == 2);
    CHECK(flex.cost == 2);
    CHECK(flex.lasting == 0);

    // A debuff put on somebody else is still worth having.
    const CardWorth& clothesline = CardRegistry::Worth(CardId::CLOTHESLINE, 0);

    CHECK(clothesline.power > 0);
    CHECK(clothesline.cost == 2);

    // What a card charges in health is not what it charges in energy: both
    // of these are nought energy cards.
    const CardWorth& bleed = CardRegistry::Worth(CardId::BLOODLETTING, 0);
    const CardWorth& offering = CardRegistry::Worth(CardId::OFFERING, 0);

    CHECK(bleed.cost == 0);
    CHECK(bleed.health == 3);
    CHECK(bleed.energy == 2);

    CHECK(offering.cost == 0);
    CHECK(offering.health == 6);
    CHECK(offering.energy == 2);
    CHECK(offering.draw == 3);
}

TEST_CASE("A card that reads off the table says its rate, not nought")
{
    // Fiend Fire throws the hand away and hits for seven a card. Written as
    // nought damage from the cards exhausted, it came out as a two energy
    // card that does nothing at all - and it is one of the hardest hitting
    // cards the Ironclad has.
    const CardWorth& fiend = CardRegistry::Worth(CardId::FIEND_FIRE, 0);

    CHECK(fiend.damage == 7);
    CHECK(fiend.scales == 1);
    CHECK(CardRegistry::Worth(CardId::FIEND_FIRE, 1).damage == 10);

    // Second Wind blocks five a card exhausted.
    CHECK(CardRegistry::Worth(CardId::SECOND_WIND, 0).block == 5);

    // Heavy Blade has its damage written on it and the strength on top, so
    // the written figure stands and the rate only says it grows.
    const CardWorth& heavy = CardRegistry::Worth(CardId::HEAVY_BLADE, 0);

    CHECK(heavy.damage == 14);
    CHECK(heavy.scales == 1);

    // A plain card says nothing about rates.
    CHECK(CardRegistry::Worth(CardId::STRIKE_RED, 0).scales == 0);
}

TEST_CASE("What a card throws away is counted, not judged")
{
    // Thrown away is a price when the cards were worth playing and a payment
    // when they were not; a deck with Dark Embrace in it is paid for every
    // card that goes. So the count is stated and the sign is left to
    // whoever reads it beside the deck.
    const CardWorth& fiend = CardRegistry::Worth(CardId::FIEND_FIRE, 0);

    // The hand it eats, and itself.
    CHECK(fiend.exhausts == 5);
    CHECK(fiend.power == 0);

    // True Grit exhausts one card of the hand.
    CHECK(CardRegistry::Worth(CardId::TRUE_GRIT, 0).exhausts >= 1);

    // A card that exhausts itself and nothing else counts one.
    CHECK(CardRegistry::Worth(CardId::LIMIT_BREAK, 0).exhausts == 1);
    CHECK(CardRegistry::Worth(CardId::LIMIT_BREAK, 1).exhausts == 0);

    // And one that stays in the deck throws nothing away.
    CHECK(CardRegistry::Worth(CardId::STRIKE_RED, 0).exhausts == 0);
}

TEST_CASE("A bomb put on yourself is not a debuff")
{
    // The Bomb sits on the climber for three turns and then goes off in the
    // enemies faces. Counted among the things that are bad to hold, it read
    // as a two energy card that charges you and hands over nothing.
    const CardWorth& bomb = CardRegistry::Worth(CardId::THE_BOMB, 0);

    CHECK(bomb.cost == 2);
    CHECK(bomb.power > 0);
}

TEST_CASE("A curse says what holding it costs")
{
    // A curse deals no damage and blocks nothing, so every figure of its
    // worth was nought - the same nought a card that does nothing has - and
    // its cost was the sentinel an unplayable card carries, which read as a
    // card that hands energy back. A Regret came out cheaper than a Strike.
    const CardWorth& regret = CardRegistry::Worth(CardId::REGRET, 0);
    const CardWorth& strike = CardRegistry::Worth(CardId::STRIKE_RED, 0);

    CHECK(regret.cost == 0);
    CHECK(regret.unplayable == 1);
    CHECK(regret.harm > 0);

    CHECK(strike.cost == 1);
    CHECK(strike.unplayable == 0);
    CHECK(strike.harm == 0);

    // The ones that do something every turn say more than the ones that only
    // take up a draw.
    CHECK(CardRegistry::Worth(CardId::DECAY, 0).harm >
          CardRegistry::Worth(CardId::CLUMSY, 0).harm);
    CHECK(CardRegistry::Worth(CardId::BURN, 1).harm >
          CardRegistry::Worth(CardId::BURN, 0).harm);

    // And a status is a wasted draw like the rest.
    CHECK(CardRegistry::Worth(CardId::WOUND, 0).harm > 0);
    CHECK(CardRegistry::Worth(CardId::WOUND, 0).unplayable == 1);

    // What is worth tearing out sits at the bottom of the rarities.
    CHECK(strike.rarity == 0);
    CHECK(CardRegistry::Worth(CardId::DEMON_FORM, 0).rarity == 3);
    CHECK(CardRegistry::Worth(CardId::INFLAME, 0).rarity == 2);
}

TEST_CASE("A Pride leaves its copy behind at the end of the turn")
{
    // Playing it puts a copy on top of the draw pile - but not until the
    // turn is over, so drawing after playing one cannot turn it up again in
    // the same turn.
    Battle battle = BattleWith({ CardId::PRIDE, CardId::OFFERING },
                               { Dummy(60) });

    const std::size_t before = battle.GetPlayer().GetDrawPile().size();

    REQUIRE(battle.PlayCard(Idx(battle, "Pride")) == true);

    // Gone from the hand, and nothing added to the pile yet.
    CHECK(battle.GetPlayer().GetExhaustPile().size() == 1u);
    CHECK(battle.GetPlayer().GetDrawPile().size() == before);

    // Drawing three cards this turn cannot turn up the copy.
    REQUIRE(battle.PlayCard(Idx(battle, "Offering")) == true);

    for (const auto& card : battle.GetPlayer().GetHand())
    {
        CHECK(card.GetId() != CardId::PRIDE);
    }

    REQUIRE(battle.EndTurn() == true);

    // Now it is back: put on top as the turn ended, and drawn again with the
    // next hand, which is where a card on top of the pile goes.
    bool found = false;

    for (const auto& card : battle.GetPlayer().GetDrawPile())
    {
        found = found || card.GetId() == CardId::PRIDE;
    }

    for (const auto& card : battle.GetPlayer().GetHand())
    {
        found = found || card.GetId() == CardId::PRIDE;
    }

    CHECK(found == true);
}
