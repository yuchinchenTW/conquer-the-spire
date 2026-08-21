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
                  std::vector<Monster> monsters, int playerHealth = 70)
{
    Player player("Silent", playerHealth);

    for (const CardId id : ids)
    {
        player.AddCardToDeck(CardRegistry::Get(id));
    }

    Battle battle(std::move(player), std::move(monsters), 11);
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

TEST_CASE("Blade Dance fills the hand with Shivs")
{
    Battle battle = BattleWith({ CardId::BLADE_DANCE }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(CountInHand(battle, "Shiv") == 3);

    // A Shiv is free, hits for 4 and then leaves the battle.
    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 46);
    CHECK(battle.GetPlayer().GetEnergy() == 2);
    CHECK(battle.GetPlayer().GetExhaustPile().size() == 1u);
}

TEST_CASE("Accuracy makes every Shiv hit harder")
{
    Battle battle =
        BattleWith({ CardId::ACCURACY, CardId::BLADE_DANCE }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Accuracy")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Blade Dance")) == true);

    // 4 base and 4 from Accuracy.
    REQUIRE(battle.PlayCard(Idx(battle, "Shiv")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 42);
}

TEST_CASE("Deadly Poison eats away at the start of the monster turn")
{
    Battle battle = BattleWith({ CardId::DEADLY_POISON }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::POISON) == 5);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 45);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::POISON) == 4);
}

TEST_CASE("Catalyst doubles the poison already on the target")
{
    Battle battle =
        BattleWith({ CardId::DEADLY_POISON, CardId::CATALYST }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Deadly Poison")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Catalyst")) == true);

    CHECK(battle.GetMonsters()[0].GetPower(PowerType::POISON) == 10);
}

TEST_CASE("Bane hits twice as hard into poison")
{
    Battle battle =
        BattleWith({ CardId::DEADLY_POISON, CardId::BANE }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Deadly Poison")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Bane")) == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 36);
}

TEST_CASE("Noxious Fumes poisons everything every turn")
{
    Battle battle = BattleWith({ CardId::NOXIOUS_FUMES },
                               { Dummy(50), Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::POISON) == 0);

    REQUIRE(battle.EndTurn() == true);

    for (const auto& monster : battle.GetMonsters())
    {
        CHECK(monster.GetPower(PowerType::POISON) == 2);
    }
}

TEST_CASE("Envenom poisons whatever an attack gets through to")
{
    Battle battle =
        BattleWith({ CardId::ENVENOM, CardId::STRIKE_GREEN }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Envenom")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Strike")) == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 44);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::POISON) == 1);
}

TEST_CASE("Corpse Explosion spreads a dying monster over the rest")
{
    Battle battle =
        BattleWith({ CardId::CORPSE_EXPLOSION }, { Dummy(6), Dummy(50) });

    REQUIRE(battle.PlayCard(0, 0) == true);
    REQUIRE(battle.EndTurn() == true);

    // The poison finished the first monster off, and its 6 maximum health
    // landed on the other one.
    CHECK(battle.GetMonsters()[0].IsDead() == true);
    CHECK(battle.GetMonsters()[1].GetHealth() == 44);
}

TEST_CASE("Tactician pays out when it is thrown away")
{
    Battle battle =
        BattleWith({ CardId::PREPARED, CardId::TACTICIAN }, { Dummy(50) });

    StackDrawPile(battle, CardId::STRIKE_GREEN, 2);

    // Prepared draws a card and then discards the Tactician.
    REQUIRE(battle.PlayCard(Idx(battle, "Prepared"), 0, 0) == true);

    CHECK(battle.GetPlayer().GetEnergy() == 4);
    CHECK(battle.GetCardsDiscardedThisTurn() == 1);
}

TEST_CASE("Reflex draws when it is thrown away")
{
    Battle battle =
        BattleWith({ CardId::PREPARED, CardId::REFLEX }, { Dummy(50) });

    StackDrawPile(battle, CardId::STRIKE_GREEN, 5);

    REQUIRE(battle.PlayCard(Idx(battle, "Prepared"), 0, 0) == true);

    // One card from Prepared, then two more because the Reflex was discarded.
    CHECK(battle.GetPlayer().GetHand().size() == 3u);
}

TEST_CASE("Sneaky Strike pays back once something has been discarded")
{
    Battle battle =
        BattleWith({ CardId::PREPARED, CardId::SNEAKY_STRIKE }, { Dummy(50) });

    StackDrawPile(battle, CardId::STRIKE_GREEN, 1);

    // Prepared draws the Strike and discards it again.
    REQUIRE(battle.PlayCard(Idx(battle, "Prepared"), 0, 1) == true);
    REQUIRE(battle.GetCardsDiscardedThisTurn() == 1);

    REQUIRE(battle.PlayCard(Idx(battle, "Sneaky Strike")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 38);
    CHECK(battle.GetPlayer().GetEnergy() == 3);
}

TEST_CASE("Eviscerate gets cheaper with every card discarded")
{
    Battle battle =
        BattleWith({ CardId::EVISCERATE, CardId::PREPARED }, { Dummy(50) });

    StackDrawPile(battle, CardId::STRIKE_GREEN, 1);

    const std::size_t before = Idx(battle, "Eviscerate");
    CHECK(battle.GetEffectiveCost(battle.GetPlayer().GetHand()[before]) == 3);

    REQUIRE(battle.PlayCard(Idx(battle, "Prepared"), 0, 1) == true);

    const std::size_t after = Idx(battle, "Eviscerate");
    CHECK(battle.GetEffectiveCost(battle.GetPlayer().GetHand()[after]) == 2);
}

TEST_CASE("Masterful Stab gets dearer every time the player bleeds")
{
    Battle battle = BattleWith({ CardId::MASTERFUL_STAB }, { Attacker(30, 5) });

    CHECK(battle.GetEffectiveCost(battle.GetPlayer().GetHand()[0]) == 0);

    REQUIRE(battle.EndTurn() == true);
    REQUIRE(battle.GetHealthLossCount() == 1);

    const std::size_t index = Idx(battle, "Masterful Stab");
    CHECK(battle.GetEffectiveCost(battle.GetPlayer().GetHand()[index]) == 1);
}

TEST_CASE("Calculated Gamble throws the hand away and draws it back")
{
    Battle battle = BattleWith({ CardId::CALCULATED_GAMBLE,
                                 CardId::STRIKE_GREEN, CardId::STRIKE_GREEN },
                               { Dummy(50) });

    StackDrawPile(battle, CardId::DEFEND_GREEN, 5);

    REQUIRE(battle.PlayCard(Idx(battle, "Calculated Gamble")) == true);

    CHECK(battle.GetPlayer().GetHand().size() == 2u);
    CHECK(CountInHand(battle, "Defend") == 2);
}

TEST_CASE("Storm of Steel trades the hand for Shivs")
{
    Battle battle = BattleWith({ CardId::STORM_OF_STEEL, CardId::STRIKE_GREEN,
                                 CardId::DEFEND_GREEN },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Storm of Steel")) == true);

    CHECK(battle.GetPlayer().GetHand().size() == 2u);
    CHECK(CountInHand(battle, "Shiv") == 2);
}

TEST_CASE("Unload throws away everything that is not an attack")
{
    Battle battle = BattleWith({ CardId::UNLOAD, CardId::DEFEND_GREEN,
                                 CardId::STRIKE_GREEN },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Unload")) == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 36);
    REQUIRE(battle.GetPlayer().GetHand().size() == 1u);
    CHECK(battle.GetPlayer().GetHand()[0].GetName() == "Strike");
}

TEST_CASE("Finisher hits once for every attack played this turn")
{
    Battle battle = BattleWith({ CardId::STRIKE_GREEN, CardId::STRIKE_GREEN,
                                 CardId::FINISHER },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Strike")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Strike")) == true);

    // Two Strikes and the Finisher itself, 6 damage each.
    REQUIRE(battle.PlayCard(Idx(battle, "Finisher")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 20);
}

TEST_CASE("Flechettes hits once for every skill still in hand")
{
    Battle battle = BattleWith({ CardId::FLECHETTES, CardId::DEFEND_GREEN,
                                 CardId::DEFEND_GREEN, CardId::BACKFLIP },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Flechettes")) == true);

    // Three skills left in hand, 4 damage each.
    CHECK(battle.GetMonsters()[0].GetHealth() == 38);
}

TEST_CASE("Skewer stabs once per energy spent")
{
    Battle battle = BattleWith({ CardId::SKEWER }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 29);
}

TEST_CASE("Malaise takes Strength and gives Weak by the energy spent")
{
    Battle battle = BattleWith({ CardId::MALAISE }, { Dummy(50) });

    battle.GetMonsters()[0].AddPower(PowerType::STRENGTH, 5);

    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetMonsters()[0].GetPower(PowerType::STRENGTH) == 2);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::WEAK) == 3);
}

TEST_CASE("Doppelganger pays out on the following turn")
{
    Battle battle = BattleWith({ CardId::DOPPELGANGER }, { Dummy(50) });

    StackDrawPile(battle, CardId::STRIKE_GREEN, 8);

    REQUIRE(battle.PlayCard(0) == true);
    REQUIRE(battle.EndTurn() == true);

    // Three energy went in, so three energy and three cards come back.
    CHECK(battle.GetPlayer().GetEnergy() == 6);
    CHECK(battle.GetPlayer().GetHand().size() == 8u);
}

TEST_CASE("Outmaneuver saves energy for the next turn")
{
    Battle battle = BattleWith({ CardId::OUTMANEUVER }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetPlayer().GetEnergy() == 5);
}

TEST_CASE("Dodge and Roll blocks this turn and the next")
{
    Battle battle = BattleWith({ CardId::DODGE_AND_ROLL }, { Attacker(30, 4) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetPlayer().GetBlock() == 4);

    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetPlayer().GetHealth() == 70);
    CHECK(battle.GetPlayer().GetBlock() == 4);
}

TEST_CASE("Blur keeps the block through the next turn")
{
    Battle battle = BattleWith({ CardId::BLUR }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    REQUIRE(battle.GetPlayer().GetBlock() == 5);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetBlock() == 5);

    // Only for the one turn, though.
    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetBlock() == 0);
}

TEST_CASE("Phantasmal Killer doubles the damage of the next turn")
{
    Battle battle = BattleWith({ CardId::PHANTASMAL_KILLER,
                                 CardId::STRIKE_GREEN },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Phantasmal Killer")) == true);
    REQUIRE(battle.EndTurn() == true);

    REQUIRE(battle.PlayCard(Idx(battle, "Strike")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 38);
}

TEST_CASE("Wraith Form trades Dexterity for Intangible")
{
    Battle battle = BattleWith({ CardId::WRAITH_FORM }, { Attacker(30, 20) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetPlayer().GetPower(PowerType::INTANGIBLE) == 2);

    REQUIRE(battle.EndTurn() == true);

    // Intangible turned a 20 damage hit into 1.
    CHECK(battle.GetPlayer().GetHealth() == 69);
    CHECK(battle.GetPlayer().GetPower(PowerType::DEXTERITY) == -1);
}

TEST_CASE("Well-Laid Plans keeps a card back over the turn")
{
    Battle battle = BattleWith({ CardId::WELL_LAID_PLANS, CardId::STRIKE_GREEN,
                                 CardId::DEFEND_GREEN },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Well-Laid Plans")) == true);

    const std::string kept = battle.GetPlayer().GetHand()[0].GetName();

    REQUIRE(battle.EndTurn() == true);

    // The first card in hand stayed, and it is not in the discard pile.
    CHECK(battle.GetPlayer().GetHand().size() == 3u);
    CHECK(battle.GetPlayer().GetHand()[0].GetName() == kept);

    for (const auto& card : battle.GetPlayer().GetDiscardPile())
    {
        CHECK(card.GetName() != kept);
    }
}

TEST_CASE("After Image blocks for every card played after it")
{
    Battle battle =
        BattleWith({ CardId::AFTER_IMAGE, CardId::STRIKE_GREEN },
                   { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "After Image")) == true);

    // The card that granted it does not set it off.
    CHECK(battle.GetPlayer().GetBlock() == 0);

    REQUIRE(battle.PlayCard(Idx(battle, "Strike")) == true);
    CHECK(battle.GetPlayer().GetBlock() == 1);
}

TEST_CASE("A Thousand Cuts nicks everything for every card played")
{
    Battle battle = BattleWith({ CardId::A_THOUSAND_CUTS,
                                 CardId::STRIKE_GREEN },
                               { Dummy(50), Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "A Thousand Cuts")) == true);
    CHECK(battle.GetMonsters()[1].GetHealth() == 50);

    REQUIRE(battle.PlayCard(Idx(battle, "Strike"), 0) == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 43);
    CHECK(battle.GetMonsters()[1].GetHealth() == 49);
}

TEST_CASE("Burst plays the next skill twice")
{
    Battle battle =
        BattleWith({ CardId::BURST, CardId::DEFEND_GREEN }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Burst")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Defend")) == true);

    CHECK(battle.GetPlayer().GetBlock() == 10);
    CHECK(battle.GetPlayer().GetPower(PowerType::BURST) == 0);
}

TEST_CASE("Bullet Time drops every cost for the turn")
{
    Battle battle = BattleWith({ CardId::BULLET_TIME, CardId::STRIKE_GREEN,
                                 CardId::DEFEND_GREEN },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Bullet Time")) == true);
    REQUIRE(battle.GetPlayer().GetEnergy() == 0);

    REQUIRE(battle.PlayCard(Idx(battle, "Strike")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Defend")) == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 44);
    CHECK(battle.GetPlayer().GetBlock() == 5);
}

TEST_CASE("Grand Finale waits until the draw pile is empty")
{
    Battle battle =
        BattleWith({ CardId::GRAND_FINALE, CardId::STRIKE_GREEN },
                   { Dummy(50) });

    const std::size_t finale = Idx(battle, "Grand Finale");
    CHECK(battle.CanPlay(finale) == true);

    StackDrawPile(battle, CardId::STRIKE_GREEN, 1);
    CHECK(battle.CanPlay(finale) == false);

    battle.GetPlayer().GetDrawPile().clear();
    REQUIRE(battle.PlayCard(finale) == true);
    CHECK(battle.GetMonsters()[0].IsDead() == true);
}

TEST_CASE("Terror leaves the target Vulnerable for the rest of the fight")
{
    Battle battle = BattleWith({ CardId::TERROR }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::VULNERABLE) == 99);
}

TEST_CASE("Piercing Wail borrows Strength for a turn")
{
    Battle battle = BattleWith({ CardId::PIERCING_WAIL }, { Dummy(50) });

    battle.GetMonsters()[0].AddPower(PowerType::STRENGTH, 5);

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::STRENGTH) == -1);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::STRENGTH) == 5);
}

TEST_CASE("Choke makes the target pay for every card played after it")
{
    Battle battle =
        BattleWith({ CardId::CHOKE, CardId::STRIKE_GREEN }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Choke")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 38);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::CHOKED) == 3);

    // 6 from the Strike and 3 more for having played it.
    REQUIRE(battle.PlayCard(Idx(battle, "Strike")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 29);
}

TEST_CASE("Endless Agony copies itself every time it is drawn")
{
    Battle battle = BattleWith({ CardId::ENDLESS_AGONY }, { Dummy(50) });

    // Drawing it for the opening hand already made a copy.
    CHECK(CountInHand(battle, "Endless Agony") == 2);
}

TEST_CASE("Escape Plan only blocks when it turns up a skill")
{
    Battle battle = BattleWith({ CardId::ESCAPE_PLAN }, { Dummy(50) });

    StackDrawPile(battle, CardId::DEFEND_GREEN, 1);

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetPlayer().GetBlock() == 3);

    Battle other = BattleWith({ CardId::ESCAPE_PLAN }, { Dummy(50) });

    StackDrawPile(other, CardId::STRIKE_GREEN, 1);

    REQUIRE(other.PlayCard(0) == true);
    CHECK(other.GetPlayer().GetBlock() == 0);
}

TEST_CASE("Expertise draws the hand up to six cards")
{
    Battle battle = BattleWith({ CardId::EXPERTISE }, { Dummy(50) });

    StackDrawPile(battle, CardId::STRIKE_GREEN, 8);

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetPlayer().GetHand().size() == 6u);
}

TEST_CASE("Setup puts a card on top of the draw pile for nothing")
{
    Battle battle =
        BattleWith({ CardId::SETUP, CardId::DIE_DIE_DIE }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Setup"), 0, 0) == true);

    REQUIRE(battle.GetPlayer().GetDrawPile().size() == 1u);

    const Card& stacked = battle.GetPlayer().GetDrawPile()[0];
    CHECK(stacked.GetName() == "Die Die Die");
    CHECK(battle.GetEffectiveCost(stacked) == 0);
}

TEST_CASE("Nightmare hands over three copies on the next turn")
{
    Battle battle =
        BattleWith({ CardId::NIGHTMARE, CardId::STRIKE_GREEN }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Nightmare"), 0, 0) == true);
    REQUIRE(battle.EndTurn() == true);

    CHECK(CountInHand(battle, "Strike") == 4);
}

TEST_CASE("Glass Knife blunts itself with every use")
{
    Battle battle = BattleWith({ CardId::GLASS_KNIFE }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 34);

    REQUIRE(battle.EndTurn() == true);

    // 8 became 6, twice over.
    REQUIRE(battle.PlayCard(Idx(battle, "Glass Knife")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 22);
}

TEST_CASE("Caltrops hurts whatever attacks the player")
{
    Battle battle = BattleWith({ CardId::CALTROPS }, { Attacker(30, 5) });

    REQUIRE(battle.PlayCard(0) == true);
    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 27);
}

TEST_CASE("Infinite Blades hands over a Shiv every turn")
{
    Battle battle = BattleWith({ CardId::INFINITE_BLADES }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    REQUIRE(battle.EndTurn() == true);

    CHECK(CountInHand(battle, "Shiv") == 1);
}

TEST_CASE("Tools of the Trade trades a card every turn")
{
    Battle battle = BattleWith({ CardId::TOOLS_OF_THE_TRADE }, { Dummy(50) });

    StackDrawPile(battle, CardId::STRIKE_GREEN, 8);

    REQUIRE(battle.PlayCard(0) == true);
    REQUIRE(battle.EndTurn() == true);

    // Five for the turn, one more from the power, and one thrown away.
    CHECK(battle.GetPlayer().GetHand().size() == 5u);
    CHECK(battle.GetCardsDiscardedThisTurn() == 1);
}

TEST_CASE("The Silent pool holds every card of the character")
{
    CHECK(CardRegistry::GetPool(CardColor::GREEN, CardRarity::BASIC).size() ==
          4u);
    CHECK(CardRegistry::GetPool(CardColor::GREEN, CardRarity::COMMON).size() ==
          19u);
    CHECK(CardRegistry::GetPool(CardColor::GREEN,
                                CardRarity::UNCOMMON).size() == 33u);
    CHECK(CardRegistry::GetPool(CardColor::GREEN, CardRarity::RARE).size() ==
          19u);
    CHECK(CardRegistry::GetPool(CardColor::GREEN).size() == 76u);

    for (const CardId id : CardRegistry::GetPool(CardColor::GREEN))
    {
        const Card card = CardRegistry::Get(id);

        CHECK(card.GetId() == id);
        CHECK(card.GetColor() == CardColor::GREEN);
        CHECK(card.GetName().empty() == false);
        CHECK(card.GetCardType() != CardType::INVALID);
        CHECK(card.GetRarity() != CardRarity::INVALID);
    }
}

TEST_CASE("The Silent starts a run with the deck she should")
{
    const std::vector<Card> deck =
        CardRegistry::MakeStarterDeck(CardColor::GREEN);

    REQUIRE(deck.size() == 12u);

    int strikes = 0;
    int defends = 0;

    for (const auto& card : deck)
    {
        if (card.GetId() == CardId::STRIKE_GREEN)
        {
            ++strikes;
        }
        else if (card.GetId() == CardId::DEFEND_GREEN)
        {
            ++defends;
        }
    }

    CHECK(strikes == 5);
    CHECK(defends == 5);
    CHECK(deck[10].GetId() == CardId::NEUTRALIZE);
    CHECK(deck[11].GetId() == CardId::SURVIVOR);
}
