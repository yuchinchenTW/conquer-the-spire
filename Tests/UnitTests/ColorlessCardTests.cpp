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

    Battle battle(std::move(player), std::move(monsters), 5);
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

TEST_CASE("Swift Strike costs nothing and still hits")
{
    Battle battle = BattleWith({ CardId::SWIFT_STRIKE }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 43);
    CHECK(battle.GetPlayer().GetEnergy() == 3);
}

TEST_CASE("Mind Blast hits for the size of the draw pile")
{
    Battle battle = BattleWith({ CardId::MIND_BLAST }, { Dummy(50) });

    StackDrawPile(battle, CardId::STRIKE_RED, 7);

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 43);
}

TEST_CASE("Enlightenment brings every cost in hand down to one")
{
    Battle battle =
        BattleWith({ CardId::ENLIGHTENMENT, CardId::BLUDGEON }, { Dummy(50) });

    const std::size_t before = Idx(battle, "Bludgeon");
    REQUIRE(battle.GetEffectiveCost(battle.GetPlayer().GetHand()[before]) == 3);

    REQUIRE(battle.PlayCard(Idx(battle, "Enlightenment")) == true);

    const std::size_t after = Idx(battle, "Bludgeon");
    CHECK(battle.GetEffectiveCost(battle.GetPlayer().GetHand()[after]) == 1);
}

TEST_CASE("Madness drops the cost of one card in hand")
{
    Battle battle =
        BattleWith({ CardId::MADNESS, CardId::BLUDGEON }, { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Madness")) == true);

    const std::size_t index = Idx(battle, "Bludgeon");
    CHECK(battle.GetEffectiveCost(battle.GetPlayer().GetHand()[index]) == 0);
}

TEST_CASE("Impatience only draws when no attack is held")
{
    Battle battle = BattleWith({ CardId::IMPATIENCE }, { Dummy(50) });

    StackDrawPile(battle, CardId::DEFEND_RED, 5);

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetPlayer().GetHand().size() == 2u);

    Battle other =
        BattleWith({ CardId::IMPATIENCE, CardId::STRIKE_RED }, { Dummy(50) });

    StackDrawPile(other, CardId::DEFEND_RED, 5);

    REQUIRE(other.PlayCard(Idx(other, "Impatience")) == true);

    // The Strike in hand kept it quiet.
    CHECK(other.GetPlayer().GetHand().size() == 1u);
}

TEST_CASE("Dark Shackles borrows the target's Strength for a turn")
{
    Battle battle = BattleWith({ CardId::DARK_SHACKLES }, { Dummy(50) });

    battle.GetMonsters()[0].AddPower(PowerType::STRENGTH, 5);

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::STRENGTH) == -4);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::STRENGTH) == 5);
}

TEST_CASE("An upgraded Blind shakes every enemy")
{
    Player player("Ironclad", 80);
    player.AddCardToDeck(CardRegistry::Get(CardId::BLIND, 1));

    Battle battle(std::move(player), { Dummy(50), Dummy(50) }, 5);
    battle.Start();

    REQUIRE(battle.PlayCard(0) == true);

    for (const auto& monster : battle.GetMonsters())
    {
        CHECK(monster.GetPower(PowerType::WEAK) == 2);
    }
}

TEST_CASE("Panic Button trades two turns of blocking for a wall")
{
    Battle battle = BattleWith({ CardId::PANIC_BUTTON, CardId::DEFEND_RED },
                               { Attacker(30, 5) });

    REQUIRE(battle.PlayCard(Idx(battle, "Panic Button")) == true);
    CHECK(battle.GetPlayer().GetBlock() == 30);

    REQUIRE(battle.EndTurn() == true);

    // Nothing can be blocked for the next two turns.
    REQUIRE(battle.PlayCard(Idx(battle, "Defend")) == true);
    CHECK(battle.GetPlayer().GetBlock() == 0);
}

TEST_CASE("Purity throws up to three cards away")
{
    Battle battle = BattleWith({ CardId::PURITY, CardId::STRIKE_RED,
                                 CardId::STRIKE_RED, CardId::DEFEND_RED },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Purity")) == true);

    CHECK(battle.GetPlayer().GetHand().empty());

    // The three cards from hand plus Purity itself.
    CHECK(battle.GetPlayer().GetExhaustPile().size() == 4u);
}

TEST_CASE("Panacea shrugs off the next debuff")
{
    Battle battle = BattleWith({ CardId::PANACEA }, { Weakener(30) });

    REQUIRE(battle.PlayCard(0) == true);
    REQUIRE(battle.GetPlayer().GetPower(PowerType::ARTIFACT) == 1);

    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetPlayer().GetPower(PowerType::WEAK) == 0);
    CHECK(battle.GetPlayer().GetPower(PowerType::ARTIFACT) == 0);
}

TEST_CASE("Apotheosis upgrades the whole deck")
{
    Battle battle =
        BattleWith({ CardId::APOTHEOSIS, CardId::STRIKE_RED }, { Dummy(50) });

    StackDrawPile(battle, CardId::STRIKE_RED, 2);
    battle.GetPlayer().GetDiscardPile().emplace_back(
        CardRegistry::Get(CardId::STRIKE_RED));

    REQUIRE(battle.PlayCard(Idx(battle, "Apotheosis")) == true);

    CHECK(battle.GetPlayer().GetHand()[0].GetName() == "Strike+");
    CHECK(battle.GetPlayer().GetDrawPile()[0].GetName() == "Strike+");
    CHECK(battle.GetPlayer().GetDiscardPile()[0].GetName() == "Strike+");

    // 6 damage became 9.
    REQUIRE(battle.PlayCard(Idx(battle, "Strike+")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 41);
}

TEST_CASE("Panache goes off on every fifth card")
{
    Battle battle = BattleWith({ CardId::PANACHE, CardId::SWIFT_STRIKE,
                                 CardId::SWIFT_STRIKE, CardId::SWIFT_STRIKE,
                                 CardId::SWIFT_STRIKE },
                               { Dummy(80) });

    REQUIRE(battle.PlayCard(Idx(battle, "Panache")) == true);

    // Three more cards keep the count below five.
    for (int i = 0; i < 3; ++i)
    {
        REQUIRE(battle.PlayCard(Idx(battle, "Swift Strike")) == true);
    }

    CHECK(battle.GetMonsters()[0].GetHealth() == 59);

    // The fifth card brings the blast: 7 from the card and 10 from Panache.
    REQUIRE(battle.PlayCard(Idx(battle, "Swift Strike")) == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 42);
}

TEST_CASE("Sadistic Nature answers every debuff landed")
{
    Battle battle = BattleWith({ CardId::SADISTIC_NATURE, CardId::TRIP },
                               { Dummy(50) });

    REQUIRE(battle.PlayCard(Idx(battle, "Sadistic Nature")) == true);
    REQUIRE(battle.PlayCard(Idx(battle, "Trip")) == true);

    CHECK(battle.GetMonsters()[0].GetPower(PowerType::VULNERABLE) == 2);
    CHECK(battle.GetMonsters()[0].GetHealth() == 45);
}

TEST_CASE("The Bomb goes off three turns later")
{
    Battle battle = BattleWith({ CardId::THE_BOMB }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 50);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 50);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetMonsters()[0].GetHealth() == 10);
}

TEST_CASE("Mayhem plays the top of the draw pile every turn")
{
    Battle battle = BattleWith({ CardId::MAYHEM }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);

    battle.GetPlayer().GetDrawPile().clear();

    // Five of these are drawn for the turn; Mayhem reaches for the next one.
    StackDrawPile(battle, CardId::BLUDGEON, 8);

    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetMonsters()[0].GetHealth() == 18);
    CHECK(battle.GetPlayer().GetExhaustPile().size() == 1u);
}

TEST_CASE("Magnetism hands over a colourless card every turn")
{
    Battle battle = BattleWith({ CardId::MAGNETISM }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);
    REQUIRE(battle.EndTurn() == true);

    // The Magnetism drawn back out of the discard pile, and the card the
    // power handed over on top of it.
    REQUIRE(battle.GetPlayer().GetHand().size() == 2u);
    CHECK(battle.GetPlayer().GetHand()[1].GetColor() == CardColor::COLORLESS);
}

TEST_CASE("Secret Weapon digs an attack out of the draw pile")
{
    Battle battle = BattleWith({ CardId::SECRET_WEAPON }, { Dummy(50) });

    StackDrawPile(battle, CardId::DEFEND_RED, 1);
    StackDrawPile(battle, CardId::BLUDGEON, 1);
    StackDrawPile(battle, CardId::DEFEND_RED, 1);

    REQUIRE(battle.PlayCard(0) == true);

    REQUIRE(battle.GetPlayer().GetHand().size() == 1u);
    CHECK(battle.GetPlayer().GetHand()[0].GetName() == "Bludgeon");
    CHECK(battle.GetPlayer().GetDrawPile().size() == 2u);
}

TEST_CASE("Violence drags three attacks out of the draw pile")
{
    Battle battle = BattleWith({ CardId::VIOLENCE }, { Dummy(50) });

    StackDrawPile(battle, CardId::STRIKE_RED, 4);
    StackDrawPile(battle, CardId::DEFEND_RED, 2);

    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetPlayer().GetHand().size() == 3u);

    for (const auto& card : battle.GetPlayer().GetHand())
    {
        CHECK(card.GetCardType() == CardType::ATTACK);
    }
}

TEST_CASE("Deep Breath shuffles the discard pile back in")
{
    Battle battle = BattleWith({ CardId::DEEP_BREATH, CardId::STRIKE_RED },
                               { Dummy(50) });

    for (int i = 0; i < 3; ++i)
    {
        battle.GetPlayer().GetDiscardPile().emplace_back(
            CardRegistry::Get(CardId::DEFEND_RED));
    }

    REQUIRE(battle.PlayCard(Idx(battle, "Deep Breath")) == true);

    // Only the Deep Breath itself is left in the discard pile.
    REQUIRE(battle.GetPlayer().GetDiscardPile().size() == 1u);
    CHECK(battle.GetPlayer().GetDiscardPile()[0].GetName() == "Deep Breath");
    CHECK(battle.GetPlayer().GetDrawPile().size() == 2u);
    CHECK(battle.GetPlayer().GetHand().size() == 2u);
}

TEST_CASE("Forethought buries a card at the bottom for nothing")
{
    Battle battle =
        BattleWith({ CardId::FORETHOUGHT, CardId::BLUDGEON }, { Dummy(50) });

    StackDrawPile(battle, CardId::STRIKE_RED, 1);

    REQUIRE(battle.PlayCard(Idx(battle, "Forethought"), 0, 0) == true);

    REQUIRE(battle.GetPlayer().GetDrawPile().size() == 2u);
    CHECK(battle.GetPlayer().GetDrawPile()[0].GetName() == "Bludgeon");
    CHECK(battle.GetEffectiveCost(battle.GetPlayer().GetDrawPile()[0]) == 0);
}

TEST_CASE("Jack of All Trades hands over a colourless card")
{
    Battle battle = BattleWith({ CardId::JACK_OF_ALL_TRADES }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);

    REQUIRE(battle.GetPlayer().GetHand().size() == 1u);
    CHECK(battle.GetPlayer().GetHand()[0].GetColor() == CardColor::COLORLESS);
    CHECK(battle.GetEffectiveCost(battle.GetPlayer().GetHand()[0]) == 0);
}

TEST_CASE("Transmutation hands over a card for every energy spent")
{
    Battle battle = BattleWith({ CardId::TRANSMUTATION }, { Dummy(50) });

    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetPlayer().GetHand().size() == 3u);
    CHECK(battle.GetPlayer().GetEnergy() == 0);
}

TEST_CASE("Ritual Dagger grows on every kill")
{
    Battle battle =
        BattleWith({ CardId::RITUAL_DAGGER }, { Dummy(5), Dummy(50) });

    REQUIRE(battle.PlayCard(0, 0) == true);

    REQUIRE(battle.GetPlayer().GetExhaustPile().size() == 1u);
    CHECK(battle.GetPlayer().GetExhaustPile()[0].GetBonusDamage() == 3);
}

TEST_CASE("Miracle and Insight stay in hand between turns")
{
    Battle battle =
        BattleWith({ CardId::MIRACLE, CardId::INSIGHT }, { Dummy(50) });

    REQUIRE(battle.EndTurn() == true);

    // Both are retained, so the hand still holds them next turn.
    CHECK(battle.GetPlayer().GetHand().size() == 2u);
    CHECK(battle.GetPlayer().GetDiscardPile().empty());

    REQUIRE(battle.PlayCard(Idx(battle, "Miracle")) == true);
    CHECK(battle.GetPlayer().GetEnergy() == 4);
}

TEST_CASE("Bandage Up patches the player up")
{
    Battle battle = BattleWith({ CardId::BANDAGE_UP }, { Attacker(30, 10) });

    REQUIRE(battle.EndTurn() == true);
    REQUIRE(battle.GetPlayer().GetHealth() == 70);

    REQUIRE(battle.PlayCard(Idx(battle, "Bandage Up")) == true);
    CHECK(battle.GetPlayer().GetHealth() == 74);
}

TEST_CASE("The colourless pool holds every card that is not a character's")
{
    CHECK(CardRegistry::GetPool(CardColor::COLORLESS,
                                CardRarity::UNCOMMON).size() == 19u);
    CHECK(CardRegistry::GetPool(CardColor::COLORLESS,
                                CardRarity::RARE).size() == 15u);
    CHECK(CardRegistry::GetPool(CardColor::COLORLESS,
                                CardRarity::SPECIAL).size() == 7u);
    CHECK(CardRegistry::GetPool(CardColor::COLORLESS).size() == 41u);

    for (const CardId id : CardRegistry::GetPool(CardColor::COLORLESS))
    {
        const Card card = CardRegistry::Get(id);

        CHECK(card.GetId() == id);
        CHECK(card.GetColor() == CardColor::COLORLESS);
        CHECK(card.GetName().empty() == false);
        CHECK(card.GetCardType() != CardType::INVALID);
        CHECK(card.GetRarity() != CardRarity::INVALID);
    }
}

TEST_CASE("Every card in the registry builds and reports itself")
{
    const CardColor colours[] = { CardColor::RED,   CardColor::GREEN,
                                  CardColor::BLUE,  CardColor::COLORLESS,
                                  CardColor::STATUS, CardColor::CURSE };

    int total = 0;

    for (const CardColor colour : colours)
    {
        const std::vector<CardId>& pool = CardRegistry::GetPool(colour);

        CHECK(pool.empty() == false);
        total += static_cast<int>(pool.size());

        for (const CardId id : pool)
        {
            const Card card = CardRegistry::Get(id);
            const Card upgraded = CardRegistry::Get(id, 1);

            CHECK(card.GetId() == id);
            CHECK(upgraded.GetId() == id);
            CHECK(card.GetTarget() != CardTarget::INVALID);
        }
    }

    // 75 Ironclad, 76 Silent, 74 Defect, 40 colourless, 5 statuses and
    // 14 curses.
    CHECK(total == 285);
}
