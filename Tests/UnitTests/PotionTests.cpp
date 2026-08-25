#include "doctest.h"

#include <conquer-the-spire/Battle/Battle.hpp>
#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Enums/CardId.hpp>
#include <conquer-the-spire/Monsters/MonsterLibrary.hpp>
#include <conquer-the-spire/Potions/PotionRegistry.hpp>
#include <conquer-the-spire/Run/Run.hpp>
#include <conquer-the-spire/Relics/RelicRegistry.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

using namespace ConquerTheSpire;

namespace
{
//! Builds a started battle carrying \p potions, with a deck of exactly
//! \p ids.
Battle BattleWith(const std::vector<CardId>& ids,
                  const std::vector<PotionId>& potions,
                  std::vector<Monster> monsters,
                  const std::vector<RelicId>& relics = {},
                  int playerHealth = 80)
{
    Player player("Ironclad", playerHealth);
    player.SetColor(CardColor::RED);

    for (const CardId id : ids)
    {
        player.AddCardToDeck(CardRegistry::Get(id));
    }

    for (const RelicId id : relics)
    {
        player.AddRelic(RelicRegistry::Get(id));
    }

    for (const PotionId id : potions)
    {
        player.AddPotion(PotionRegistry::Get(id));
    }

    Battle battle(std::move(player), std::move(monsters), 17);
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

//! Returns how many cards in hand are named \p name.
int CountInHand(const Battle& battle, const std::string& name)
{
    int found = 0;

    for (const auto& card : battle.GetPlayer().GetHand())
    {
        if (card.GetName() == name)
        {
            ++found;
        }
    }

    return found;
}
}  // namespace

TEST_CASE("Fire Potion burns the monster it is thrown at")
{
    Battle battle =
        BattleWith({ CardId::STRIKE_RED }, { PotionId::FIRE_POTION },
                   { Dummy(50), Dummy(50) });

    REQUIRE(battle.GetPlayer().GetPotions().size() == 1u);

    REQUIRE(battle.UsePotion(0, 1) == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 50);
    CHECK(battle.GetMonsters()[1].GetHealth() == 30);
    CHECK(battle.GetPlayer().GetPotions().empty());
}

TEST_CASE("Explosive Potion catches everything")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { PotionId::EXPLOSIVE_POTION },
                               { Dummy(50), Dummy(50) });

    REQUIRE(battle.UsePotion(0) == true);

    for (const auto& monster : battle.GetMonsters())
    {
        CHECK(monster.GetHealth() == 40);
    }
}

TEST_CASE("Block, Energy and Swift Potions hand over the plain things")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED, CardId::STRIKE_RED,
                                 CardId::STRIKE_RED, CardId::STRIKE_RED,
                                 CardId::STRIKE_RED, CardId::STRIKE_RED,
                                 CardId::STRIKE_RED, CardId::STRIKE_RED },
                               { PotionId::BLOCK_POTION,
                                 PotionId::ENERGY_POTION,
                                 PotionId::SWIFT_POTION },
                               { Dummy(50) });

    REQUIRE(battle.UsePotion(0) == true);
    CHECK(battle.GetPlayer().GetBlock() == 12);

    REQUIRE(battle.UsePotion(0) == true);
    CHECK(battle.GetPlayer().GetEnergy() == 5);

    REQUIRE(battle.UsePotion(0) == true);
    CHECK(battle.GetPlayer().GetHand().size() == 8u);
}

TEST_CASE("Fear and Weak Potions land the debuffs")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { PotionId::FEAR_POTION,
                                 PotionId::WEAK_POTION },
                               { Dummy(50) });

    REQUIRE(battle.UsePotion(0, 0) == true);
    REQUIRE(battle.UsePotion(0, 0) == true);

    CHECK(battle.GetMonsters()[0].GetPower(PowerType::VULNERABLE) == 3);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::WEAK) == 3);
}

TEST_CASE("Flex Potion lends Strength for the turn only")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { PotionId::FLEX_POTION }, { Dummy(50) });

    REQUIRE(battle.UsePotion(0) == true);
    CHECK(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 5);

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 39);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 0);
}

TEST_CASE("Blood Potion patches up a fifth of the maximum health")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { PotionId::BLOOD_POTION },
                               { Attacker(30, 20) });

    REQUIRE(battle.EndTurn() == true);
    REQUIRE(battle.GetPlayer().GetHealth() == 60);

    REQUIRE(battle.UsePotion(0) == true);
    CHECK(battle.GetPlayer().GetHealth() == 76);
}

TEST_CASE("Bottled Miracle hands over two Miracles")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { PotionId::BOTTLED_MIRACLE }, { Dummy(50) });

    REQUIRE(battle.UsePotion(0) == true);
    CHECK(CountInHand(battle, "Miracle") == 2);
}

TEST_CASE("Poison Potion poisons the target")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { PotionId::POISON_POTION }, { Dummy(50) });

    REQUIRE(battle.UsePotion(0, 0) == true);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::POISON) == 6);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 44);
}

TEST_CASE("Regen Potion heals a little less every turn")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { PotionId::REGEN_POTION },
                               { Attacker(30, 20) });

    REQUIRE(battle.EndTurn() == true);
    REQUIRE(battle.GetPlayer().GetHealth() == 60);

    REQUIRE(battle.UsePotion(0) == true);
    REQUIRE(battle.GetPlayer().GetPower(PowerType::REGENERATION) == 5);

    REQUIRE(battle.EndTurn() == true);

    // Healed 5 at the end of the turn, then took another 20.
    CHECK(battle.GetPlayer().GetHealth() == 45);
    CHECK(battle.GetPlayer().GetPower(PowerType::REGENERATION) == 4);
}

TEST_CASE("Duplication Potion plays the next card twice")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED, CardId::STRIKE_RED },
                               { PotionId::DUPLICATION_POTION },
                               { Dummy(50) });

    REQUIRE(battle.UsePotion(0) == true);

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 38);

    // Only the one card.
    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 32);
}

TEST_CASE("Cunning Potion hands over three sharpened Shivs")
{
    Battle battle = BattleWith({ CardId::STRIKE_GREEN },
                               { PotionId::CUNNING_POTION }, { Dummy(50) });

    REQUIRE(battle.UsePotion(0) == true);
    REQUIRE(CountInHand(battle, "Shiv+") == 3);

    REQUIRE(battle.PlayCard(Idx(battle, "Shiv+")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 44);
}

TEST_CASE("Ghost in a Jar and Heart of Iron hand over the powers")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { PotionId::GHOST_IN_A_JAR,
                                 PotionId::HEART_OF_IRON },
                               { Attacker(30, 20) });

    REQUIRE(battle.UsePotion(0) == true);
    REQUIRE(battle.UsePotion(0) == true);

    CHECK(battle.GetPlayer().GetPower(PowerType::INTANGIBLE) == 1);
    CHECK(battle.GetPlayer().GetPower(PowerType::METALLICIZE) == 6);

    REQUIRE(battle.EndTurn() == true);

    // Intangible turned the hit into 1, and the 6 block soaked it.
    CHECK(battle.GetPlayer().GetHealth() == 80);
}

TEST_CASE("Fruit Juice raises the maximum health")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { PotionId::FRUIT_JUICE }, { Dummy(50) });

    REQUIRE(battle.UsePotion(0) == true);

    CHECK(battle.GetPlayer().GetMaxHealth() == 85);
    CHECK(battle.GetPlayer().GetHealth() == 85);
}

TEST_CASE("Potion of Capacity and Essence of Darkness work on the orbs")
{
    Battle battle = BattleWith({ CardId::STRIKE_BLUE },
                               { PotionId::POTION_OF_CAPACITY,
                                 PotionId::ESSENCE_OF_DARKNESS },
                               { Dummy(50) });

    REQUIRE(battle.UsePotion(0) == true);
    CHECK(battle.GetPlayer().GetOrbSlots() == 5);

    REQUIRE(battle.UsePotion(0) == true);
    REQUIRE(battle.GetPlayer().GetOrbs().size() == 3u);

    for (const auto& orb : battle.GetPlayer().GetOrbs())
    {
        CHECK(orb.type == OrbType::DARK);
    }
}

TEST_CASE("Gambler's Brew trades the hand in")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED, CardId::STRIKE_RED,
                                 CardId::DEFEND_RED, CardId::DEFEND_RED,
                                 CardId::DEFEND_RED, CardId::DEFEND_RED,
                                 CardId::DEFEND_RED },
                               { PotionId::GAMBLERS_BREW }, { Dummy(50) });

    const std::size_t before = battle.GetPlayer().GetHand().size();

    REQUIRE(battle.UsePotion(0) == true);

    CHECK(battle.GetPlayer().GetHand().size() == before);
}

TEST_CASE("Liquid Memories takes a card back out of the discard pile")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { PotionId::LIQUID_MEMORIES }, { Dummy(50) });

    battle.GetPlayer().GetDiscardPile().emplace_back(
        CardRegistry::Get(CardId::BLUDGEON));

    REQUIRE(battle.UsePotion(0) == true);

    CHECK(Idx(battle, "Bludgeon") < battle.GetPlayer().GetHand().size());
    CHECK(battle.GetPlayer().GetDiscardPile().empty());
}

TEST_CASE("Attack Potion holds out three attacks and takes the one picked")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { PotionId::ATTACK_POTION }, { Dummy(50) });

    battle.RollOffer(battle.GetPlayer().GetPotions().front());

    const std::vector<CardId> shown = battle.GetOffered();

    // Three of them, all attacks, all different.
    REQUIRE(shown.size() == 3u);

    for (std::size_t i = 0; i < shown.size(); ++i)
    {
        CHECK(CardRegistry::Get(shown[i]).GetCardType() == CardType::ATTACK);

        for (std::size_t j = i + 1u; j < shown.size(); ++j)
        {
            CHECK(shown[i] != shown[j]);
        }
    }

    // And the one picked is the one that arrives.
    REQUIRE(battle.UsePotion(0, 0, 2u) == true);
    REQUIRE(battle.GetPlayer().GetHand().size() == 2u);
    CHECK(battle.GetPlayer().GetHand().back().GetId() == shown[2]);

    bool foundRedAttack = false;

    for (const auto& card : battle.GetPlayer().GetHand())
    {
        if (card.GetColor() == CardColor::RED &&
            card.GetCardType() == CardType::ATTACK &&
            card.GetName() != "Strike")
        {
            foundRedAttack = true;
        }
    }

    CHECK(foundRedAttack == true);
}

TEST_CASE("Sacred Bark pours a double")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { PotionId::BLOCK_POTION }, { Dummy(50) },
                               { RelicId::SACRED_BARK });

    REQUIRE(battle.UsePotion(0) == true);
    CHECK(battle.GetPlayer().GetBlock() == 24);
}

TEST_CASE("An elixir burns as many as are named, and no more")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED, CardId::DEFEND_RED,
                                CardId::BASH, CardId::ANGER,
                                CardId::SHRUG_IT_OFF },
                               { PotionId::ELIXIR }, { Dummy(50) });

    REQUIRE(battle.GetPlayer().GetHand().size() == 5u);

    // The first and the last of them, named in that order, and taking one out
    // must not move the other along.
    const CardId first = battle.GetPlayer().GetHand().front().GetId();
    const CardId last = battle.GetPlayer().GetHand().back().GetId();

    REQUIRE(battle.UsePotion(0, 0, std::vector<std::size_t>{ 0u, 4u }) ==
            true);

    CHECK(battle.GetPlayer().GetHand().size() == 3u);
    REQUIRE(battle.GetPlayer().GetExhaustPile().size() == 2u);
    CHECK(battle.GetPlayer().GetExhaustPile()[0].GetId() == first);
    CHECK(battle.GetPlayer().GetExhaustPile()[1].GetId() == last);

    // And naming none of them burns none of them.
    Battle spared = BattleWith({ CardId::STRIKE_RED, CardId::DEFEND_RED },
                               { PotionId::ELIXIR }, { Dummy(50) });

    REQUIRE(spared.UsePotion(0, 0, std::vector<std::size_t>{}) == true);

    CHECK(spared.GetPlayer().GetExhaustPile().empty() == true);
    CHECK(spared.GetPlayer().GetHand().size() == 2u);
}

TEST_CASE("Sacred Bark leaves the listed exceptions alone")
{
    // The wiki names five it cannot double, because there is no second half
    // to pour. An elixir run twice would burn twice as much, which is the
    // opposite of what it is for.
    Battle doubled = BattleWith({ CardId::STRIKE_RED },
                                { PotionId::BLOCK_POTION }, { Dummy(50) },
                                { RelicId::SACRED_BARK });

    REQUIRE(doubled.UsePotion(0) == true);
    CHECK(doubled.GetPlayer().GetBlock() == 24);

    // And the same relic pours one elixir, not two.
    Battle spared = BattleWith({ CardId::STRIKE_RED, CardId::DEFEND_RED,
                                CardId::BASH },
                               { PotionId::ELIXIR }, { Dummy(50) },
                               { RelicId::SACRED_BARK });

    REQUIRE(spared.UsePotion(0, 0, std::vector<std::size_t>{ 0u }) == true);

    CHECK(spared.GetPlayer().GetExhaustPile().size() == 1u);
}

TEST_CASE("Snecko Oil rolls what the hand costs")
{
    // Five drawn and then every price in hand rolled, nought to three. The
    // draw comes first, so the cards it just drew are among the rolled.
    Battle battle = BattleWith({ CardId::BLUDGEON, CardId::IMMOLATE,
                                CardId::DEMON_FORM, CardId::CLASH,
                                CardId::WHIRLWIND, CardId::CLEAVE,
                                CardId::ANGER, CardId::CARNAGE },
                               { PotionId::SNECKO_OIL }, { Dummy(200) });

    const std::size_t before = battle.GetPlayer().GetHand().size();

    REQUIRE(battle.UsePotion(0) == true);

    const std::vector<Card>& hand = battle.GetPlayer().GetHand();

    // It drew, and every card of the hand now carries a rolled price.
    CHECK(hand.size() > before);

    int rolled = 0;
    int moved = 0;
    int spared = 0;

    for (const Card& held : hand)
    {
        // The wiki: an X-cost card and an unplayable one are unaffected. An X
        // spends whatever is left whatever it says, so there is no price to
        // roll for it.
        if (held.GetCost() < 0)
        {
            ++spared;
            CHECK(held.HasCostThisTurn() == false);
            continue;
        }

        REQUIRE(held.HasCostThisTurn() == true);
        ++rolled;

        const int price = battle.GetEffectiveCost(held);

        CHECK(price >= 0);
        CHECK(price <= 3);

        moved += price != held.GetCost() ? 1 : 0;
    }

    // A whirlwind was in that deck, so the sparing is being tested and not
    // merely allowed for.
    CHECK(spared > 0);
    CHECK(rolled == static_cast<int>(hand.size()) - spared);

    // With that many cards of that many prices, some of them have to have
    // moved: a roll that never changes anything is not a roll.
    CHECK(moved > 0);
}

TEST_CASE("A Snecko Eye confuses what it draws")
{
    // The relic draws two more every turn, and the price of the two is that
    // every card drawn costs whatever it happens to cost. Only the drawing
    // was here, which made a boss relic into a gift.
    Battle plain = BattleWith({ CardId::BLUDGEON }, {}, { Dummy(200) });

    CHECK(plain.GetPlayer().GetPower(PowerType::CONFUSED) == 0);

    Battle eyed = BattleWith({ CardId::BLUDGEON, CardId::IMMOLATE,
                              CardId::DEMON_FORM, CardId::CLASH,
                              CardId::WHIRLWIND, CardId::CLEAVE,
                              CardId::ANGER },
                             {}, { Dummy(200) }, { RelicId::SNECKO_EYE });

    CHECK(eyed.GetPlayer().GetPower(PowerType::CONFUSED) > 0);

    // Two more in the opening hand than five, and every one of them priced by
    // the roll rather than by the card.
    CHECK(eyed.GetPlayer().GetHand().size() == 7u);

    int spared = 0;

    for (const Card& held : eyed.GetPlayer().GetHand())
    {
        if (held.GetCost() < 0)
        {
            ++spared;
            CHECK(held.HasCostThisTurn() == false);
            continue;
        }

        REQUIRE(held.HasCostThisTurn() == true);
        CHECK(eyed.GetEffectiveCost(held) >= 0);
        CHECK(eyed.GetEffectiveCost(held) <= 3);
    }

    // The whirlwind of that deck, left at X.
    CHECK(spared == 1);
}

TEST_CASE("The belt holds three, and five with the Potion Belt")
{
    Player player("Ironclad", 80);

    CHECK(player.GetPotionSlots() == 3);

    for (int i = 0; i < 4; ++i)
    {
        const bool room =
            player.AddPotion(PotionRegistry::Get(PotionId::FIRE_POTION));

        CHECK(room == (i < 3));
    }

    CHECK(player.GetPotions().size() == 3u);

    Player packed("Ironclad", 80);
    packed.AddRelic(RelicRegistry::Get(RelicId::POTION_BELT));

    CHECK(packed.GetPotionSlots() == 5);

    // Sozu turns every potion away.
    Player refusing("Ironclad", 80);
    refusing.AddRelic(RelicRegistry::Get(RelicId::SOZU));

    CHECK(refusing.AddPotion(PotionRegistry::Get(PotionId::FIRE_POTION)) ==
          false);
    CHECK(refusing.GetPotions().empty());
}

TEST_CASE("A fairy is never drunk on purpose")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { PotionId::FAIRY_IN_A_BOTTLE },
                               { Dummy(50) });

    CHECK(battle.CanUsePotion(0) == false);
    CHECK(battle.UsePotion(0) == false);
    CHECK(battle.GetPlayer().GetPotions().size() == 1u);
}

TEST_CASE("A smoke bomb is a way out of a fight, and leaves nothing behind")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { PotionId::SMOKE_BOMB }, { Dummy(50) });

    REQUIRE(battle.CanUsePotion(0) == true);
    REQUIRE(battle.UsePotion(0) == true);

    CHECK(battle.IsDone() == true);
    CHECK(battle.WasEscaped() == true);
    CHECK(battle.GetMonsters().front().HasEscaped() == true);
    CHECK(battle.GetMonsters().front().IsDead() == false);
}

TEST_CASE("A brew fills the belt, and squeezes no fruit in a fight")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { PotionId::ENTROPIC_BREW }, { Dummy(50) });

    REQUIRE(battle.UsePotion(0) == true);

    const std::vector<Potion>& held = battle.GetPlayer().GetPotions();

    CHECK(static_cast<int>(held.size()) ==
          battle.GetPlayer().GetPotionSlots());

    for (const auto& potion : held)
    {
        CHECK(potion.GetId() != PotionId::FRUIT_JUICE);
    }
}

TEST_CASE("A potion cannot be drunk at a monster that is not there")
{
    Battle battle =
        BattleWith({ CardId::STRIKE_RED }, { PotionId::FIRE_POTION },
                   { Dummy(50) });

    CHECK(battle.UsePotion(0, 5) == false);
    CHECK(battle.UsePotion(9) == false);
    CHECK(battle.GetPlayer().GetPotions().size() == 1u);
}

TEST_CASE("The registry builds every potion it lists")
{
    const std::vector<PotionId>& all = PotionRegistry::GetAll();

    CHECK(all.size() == 40u);

    for (const PotionId id : all)
    {
        const Potion potion = PotionRegistry::Get(id);

        CHECK(potion.GetId() == id);
        CHECK(potion.GetName().empty() == false);
        CHECK(potion.GetRarity() != PotionRarity::INVALID);
        CHECK(potion.GetTarget() != CardTarget::INVALID);
    }

    CHECK(PotionRegistry::GetPool(PotionRarity::COMMON).size() == 20u);
    CHECK(PotionRegistry::GetPool(PotionRarity::UNCOMMON).size() == 11u);
    CHECK(PotionRegistry::GetPool(PotionRarity::RARE).size() == 9u);
}

TEST_CASE("Potion pools only include the character's own potions")
{
    struct Case
    {
        CardColor character;
        PotionId kept;
        PotionId foreign;
    };

    const Case cases[] = {
        { CardColor::RED, PotionId::BLOOD_POTION, PotionId::POISON_POTION },
        { CardColor::GREEN, PotionId::POISON_POTION, PotionId::FOCUS_POTION },
        { CardColor::BLUE, PotionId::FOCUS_POTION, PotionId::BLOOD_POTION },
    };

    for (const Case& one : cases)
    {
        bool sawOwn = false;
        bool sawForeign = false;
        bool sawMiracle = false;

        for (const PotionId id : PotionRegistry::GetAll(one.character))
        {
            sawOwn = sawOwn || id == one.kept;
            sawForeign = sawForeign || id == one.foreign;
            sawMiracle = sawMiracle || id == PotionId::BOTTLED_MIRACLE;
        }

        CHECK(sawOwn == true);
        CHECK(sawForeign == false);
        CHECK(sawMiracle == false);
    }
}

TEST_CASE("Three potions are as good on the map as in a fight")
{
    Run run(CardColor::RED, 5);

    run.GetPlayer().LoseHealth(40);

    REQUIRE(run.AddPotion(PotionId::BLOOD_POTION) == true);

    const int hurt = run.GetPlayer().GetHealth();

    CHECK(run.CanDrinkPotion(0) == true);
    REQUIRE(run.DrinkPotion(0) == true);

    // A fifth of the whole.
    CHECK(run.GetPlayer().GetHealth() == hurt + 80 / 5);
    CHECK(run.GetPlayer().GetPotions().empty() == true);

    REQUIRE(run.AddPotion(PotionId::FRUIT_JUICE) == true);
    REQUIRE(run.DrinkPotion(0) == true);

    CHECK(run.GetPlayer().GetMaxHealth() == 85);
}

TEST_CASE("The rest of the belt stays corked outside a fight")
{
    Run run(CardColor::RED, 5);

    REQUIRE(run.AddPotion(PotionId::FIRE_POTION) == true);
    REQUIRE(run.AddPotion(PotionId::FAIRY_IN_A_BOTTLE) == true);

    CHECK(run.CanDrinkPotion(0) == false);
    CHECK(run.DrinkPotion(0) == false);
    CHECK(run.CanDrinkPotion(1) == false);
    CHECK(run.DrinkPotion(1) == false);

    // Nothing was spent, and either can still be thrown away.
    CHECK(run.GetPlayer().GetPotions().size() == 2u);
    CHECK(run.DiscardPotion(0) == true);
    CHECK(run.GetPlayer().GetPotions().size() == 1u);
}

TEST_CASE("A brew on the map fills the belt with anything at all")
{
    Run run(CardColor::RED, 12);

    REQUIRE(run.AddPotion(PotionId::ENTROPIC_BREW) == true);
    REQUIRE(run.DrinkPotion(0) == true);

    CHECK(static_cast<int>(run.GetPlayer().GetPotions().size()) ==
          run.GetPlayer().GetPotionSlots());
}

TEST_CASE("A fairy in a bottle drinks itself when a climber would die")
{
    Battle battle = BattleWith({ CardId::STRIKE_RED },
                               { PotionId::FAIRY_IN_A_BOTTLE },
                               { Dummy(50) });

    // Whatever brought the climber down, the fairy answers it.
    battle.GetPlayer().SetHealth(0);
    battle.EndTurn();

    CHECK(battle.GetPlayer().IsDead() == false);
    CHECK(battle.GetPlayer().GetHealth() ==
          battle.GetPlayer().GetMaxHealth() * 30 / 100);
    CHECK(battle.GetPlayer().GetPotions().empty() == true);
    CHECK(battle.GetPhase() != BattlePhase::LOST);
}

TEST_CASE("A fight walked out of leaves nothing on the floor")
{
    Run run(CardColor::RED, 8);

    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);
    REQUIRE(run.AddPotion(PotionId::SMOKE_BOMB) == true);

    Battle battle = run.StartBattleHere();

    REQUIRE(battle.UsePotion(0) == true);
    REQUIRE(battle.WasEscaped() == true);

    run.FinishBattle(battle);

    CHECK(run.GetRewards().empty() == true);
    CHECK(run.HasUnclaimedRewards() == false);
}

TEST_CASE("A brew drunk behind a sozu gives up rather than going round again")
{
    // Entropic Brew fills the belt, and the loop that filled it waited for
    // the belt to be full. A sozu turns every potion away, so the belt never
    // filled and the loop never came back: the engine sat inside
    // PotionRegistry::Get building a potion and throwing it away, one core
    // at a hundred percent, for as long as it was left alone. Training died
    // this way twice, hours in.
    Run run(CardColor::RED, 5);

    // The brew goes in first, because a sozu would refuse that too.
    REQUIRE(run.AddPotion(PotionId::ENTROPIC_BREW) == true);

    run.AddRelic(RelicId::SOZU);

    REQUIRE(run.GetPlayer().HasRelic(RelicId::SOZU) == true);
    REQUIRE(run.GetPlayer().GetPotions().size() == 1u);

    // This one catches the fault by never coming back, rather than by
    // failing: nothing a single-threaded test can do will bound a call into
    // the engine that does not return. A suite that stops here, with a core
    // at a hundred percent, is this test finding it.
    CHECK(run.DrinkPotion(0) == true);

    // The brew was drunk, and nothing came of it, which is what a sozu means.
    CHECK(run.GetPlayer().GetPotions().empty() == true);
}

TEST_CASE("A brew without a sozu still fills the belt")
{
    // The guard above must not be reached in the ordinary case.
    Run run(CardColor::RED, 9);

    REQUIRE(run.AddPotion(PotionId::ENTROPIC_BREW) == true);
    CHECK(run.DrinkPotion(0) == true);
    CHECK(static_cast<int>(run.GetPlayer().GetPotions().size()) ==
          run.GetPlayer().GetPotionSlots());
}
