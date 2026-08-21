#include "doctest.h"

#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Relics/RelicRegistry.hpp>
#include <conquer-the-spire/Rewards/RewardGenerator.hpp>
#include <conquer-the-spire/Run/Run.hpp>

#include <cstddef>
#include <map>
#include <random>
#include <utility>
#include <vector>

using namespace ConquerTheSpire;

namespace
{
//! Builds a player of \p color carrying \p relics.
Player MakePlayer(CardColor color, const std::vector<RelicId>& relics = {})
{
    Player player("Ironclad", 80);
    player.SetColor(color);

    for (const RelicId id : relics)
    {
        player.AddRelic(RelicRegistry::Get(id));
    }

    return player;
}

//! Returns a copy of the first reward of \p kind, or one whose kind is
//! invalid. A copy rather than a pointer, so that reaching into a reward list
//! that has already gone is not possible.
Reward Find(const std::vector<Reward>& rewards, RewardKind kind)
{
    for (const auto& reward : rewards)
    {
        if (reward.kind == kind)
        {
            return reward;
        }
    }

    return Reward();
}

//! Returns true when \p rewards holds one of \p kind.
bool Has(const std::vector<Reward>& rewards, RewardKind kind)
{
    return Find(rewards, kind).kind != RewardKind::INVALID;
}

//! Swings until the fight is over, always aiming at a monster that is still
//! standing.
void FightToTheEnd(Battle& battle)
{
    while (!battle.IsDone())
    {
        const std::vector<std::size_t> living =
            battle.GetLivingMonsterIndices();
        const std::vector<std::size_t> playable =
            battle.GetPlayableCardIndices();

        if (living.empty() || playable.empty())
        {
            battle.EndTurn();
            continue;
        }

        battle.PlayCard(playable.front(), living.front());
    }
}

//! Counts the rewards of \p kind.
int Count(const std::vector<Reward>& rewards, RewardKind kind)
{
    int found = 0;

    for (const auto& reward : rewards)
    {
        if (reward.kind == kind)
        {
            ++found;
        }
    }

    return found;
}
}  // namespace

TEST_CASE("A plain fight hands over gold and a choice of three cards")
{
    std::mt19937 rng(3);
    RewardGenerator generator(CardColor::RED, rng);
    const Player player = MakePlayer(CardColor::RED);

    const std::vector<Reward> rewards =
        generator.ForCombat(MonsterType::NORMAL, player, rng);

    const Reward gold = Find(rewards, RewardKind::GOLD);
    const Reward cards = Find(rewards, RewardKind::CARD_CHOICE);

    REQUIRE(gold.kind != RewardKind::INVALID);
    CHECK(gold.amount >= 10);
    CHECK(gold.amount <= 20);

    REQUIRE(cards.kind != RewardKind::INVALID);
    CHECK(cards.cards.size() == 3u);

    // Never the same card twice in one pick.
    CHECK(cards.cards[0] != cards.cards[1]);
    CHECK(cards.cards[1] != cards.cards[2]);
    CHECK(cards.cards[0] != cards.cards[2]);

    // A plain fight never hands over a relic.
    CHECK(Has(rewards, RewardKind::RELIC_CHOICE) == false);
}

TEST_CASE("The gold a fight pays out matches what it is worth")
{
    std::mt19937 rng(7);
    RewardGenerator generator(CardColor::RED, rng);
    const Player player = MakePlayer(CardColor::RED);

    for (int i = 0; i < 20; ++i)
    {
        const Reward elite =
            Find(generator.ForCombat(MonsterType::ELITE, player, rng),
                 RewardKind::GOLD);

        REQUIRE(elite.kind != RewardKind::INVALID);
        CHECK(elite.amount >= 25);
        CHECK(elite.amount <= 35);
    }

    const Reward boss = Find(generator.ForCombat(MonsterType::BOSS, player,
                                                  rng),
                              RewardKind::GOLD);

    REQUIRE(boss.kind != RewardKind::INVALID);
    CHECK(boss.amount >= 95);
    CHECK(boss.amount <= 105);
}

TEST_CASE("An elite leaves a relic behind")
{
    std::mt19937 rng(11);
    RewardGenerator generator(CardColor::RED, rng);
    const Player player = MakePlayer(CardColor::RED);

    const std::vector<Reward> rewards =
        generator.ForCombat(MonsterType::ELITE, player, rng);

    const Reward relic = Find(rewards, RewardKind::RELIC_CHOICE);

    REQUIRE(relic.kind != RewardKind::INVALID);
    REQUIRE(relic.relics.size() == 1u);
    CHECK(RelicRegistry::Get(relic.relics[0]).GetId() != RelicId::INVALID);
}

TEST_CASE("A boss offers three of its own relics and three rare cards")
{
    std::mt19937 rng(13);
    RewardGenerator generator(CardColor::RED, rng);
    const Player player = MakePlayer(CardColor::RED);

    const std::vector<Reward> rewards =
        generator.ForCombat(MonsterType::BOSS, player, rng);

    const Reward relic = Find(rewards, RewardKind::RELIC_CHOICE);

    REQUIRE(relic.kind != RewardKind::INVALID);
    REQUIRE(relic.relics.size() == 3u);

    for (const RelicId id : relic.relics)
    {
        CHECK(RelicRegistry::Get(id).GetTier() == RelicTier::BOSS);
    }

    const Reward cards = Find(rewards, RewardKind::CARD_CHOICE);

    REQUIRE(cards.kind != RewardKind::INVALID);
    REQUIRE(cards.cards.size() == 3u);

    for (const CardId id : cards.cards)
    {
        const Card card = CardRegistry::Get(id);

        CHECK(card.GetRarity() == CardRarity::RARE);
        CHECK(card.GetColor() == CardColor::RED);
    }
}

TEST_CASE("A boss of the first two acts can leave a potion as well")
{
    std::mt19937 rng(17);
    RewardGenerator generator(CardColor::RED, rng);
    const Player player =
        MakePlayer(CardColor::RED, { RelicId::WHITE_BEAST_STATUE });

    generator.BeginAct(2);

    CHECK(Has(generator.ForCombat(MonsterType::BOSS, player, rng),
              RewardKind::POTION) == true);
}

TEST_CASE("A boss of the last acts hands over neither cards nor potions")
{
    std::mt19937 rng(19);
    RewardGenerator generator(CardColor::RED, rng);
    const Player player =
        MakePlayer(CardColor::RED, { RelicId::WHITE_BEAST_STATUE });

    generator.BeginAct(3);

    const std::vector<Reward> rewards =
        generator.ForCombat(MonsterType::BOSS, player, rng);

    CHECK(Has(rewards, RewardKind::CARD_CHOICE) == false);
    CHECK(Has(rewards, RewardKind::POTION) == false);
    CHECK(Has(rewards, RewardKind::GOLD) == true);
    CHECK(Has(rewards, RewardKind::RELIC_CHOICE) == true);
}

TEST_CASE("A new act puts the potion chance back where it starts")
{
    std::mt19937 rng(23);
    RewardGenerator generator(CardColor::RED, rng);
    const Player player =
        MakePlayer(CardColor::RED, { RelicId::WHITE_BEAST_STATUE });

    for (int i = 0; i < 3; ++i)
    {
        generator.ForCombat(MonsterType::NORMAL, player, rng);
    }

    REQUIRE(generator.GetPotionChance() < 40);

    generator.BeginAct(2);

    CHECK(generator.GetPotionChance() == 40);
    CHECK(generator.GetAct() == 2);
}

TEST_CASE("A Prismatic Shard opens the pick up to the other colours")
{
    std::mt19937 rng(29);
    RewardGenerator generator(CardColor::RED, rng);
    const Player player =
        MakePlayer(CardColor::RED, { RelicId::PRISMATIC_SHARD });

    std::map<CardColor, int> tally;

    for (int i = 0; i < 30; ++i)
    {
        const Reward cards =
            Find(generator.ForCombat(MonsterType::NORMAL, player, rng),
                 RewardKind::CARD_CHOICE);

        REQUIRE(cards.kind != RewardKind::INVALID);

        for (const CardId id : cards.cards)
        {
            ++tally[CardRegistry::Get(id).GetColor()];
        }
    }

    CHECK(tally[CardColor::RED] > 0);
    CHECK(tally[CardColor::GREEN] > 0);
    CHECK(tally[CardColor::BLUE] > 0);
    CHECK(tally[CardColor::COLORLESS] > 0);
}

TEST_CASE("The potion chance walks up and down as potions turn up")
{
    std::mt19937 rng(17);
    RewardGenerator generator(CardColor::RED, rng);
    const Player player = MakePlayer(CardColor::RED);

    CHECK(generator.GetPotionChance() == 40);

    int drops = 0;

    for (int i = 0; i < 12; ++i)
    {
        const int before = generator.GetPotionChance();
        const std::vector<Reward> rewards =
            generator.ForCombat(MonsterType::NORMAL, player, rng);
        const bool dropped = Has(rewards, RewardKind::POTION);

        if (dropped)
        {
            ++drops;
            CHECK(generator.GetPotionChance() == before - 10);
        }
        else
        {
            CHECK(generator.GetPotionChance() == before + 10);
        }
    }

    // Over a dozen fights some came and some did not.
    CHECK(drops > 0);
    CHECK(drops < 12);
}

TEST_CASE("A White Beast Statue makes sure of the potion and Sozu refuses it")
{
    std::mt19937 rng(19);
    RewardGenerator generator(CardColor::RED, rng);
    const Player sure = MakePlayer(CardColor::RED,
                                   { RelicId::WHITE_BEAST_STATUE });

    for (int i = 0; i < 5; ++i)
    {
        CHECK(Has(generator.ForCombat(MonsterType::NORMAL, sure, rng),
                  RewardKind::POTION) == true);
    }

    RewardGenerator other(CardColor::RED, rng);
    const Player refusing = MakePlayer(CardColor::RED, { RelicId::SOZU });

    for (int i = 0; i < 8; ++i)
    {
        CHECK(Has(other.ForCombat(MonsterType::NORMAL, refusing, rng),
                  RewardKind::POTION) == false);
    }

    // Sozu leaves the books alone rather than pushing the chance up.
    CHECK(other.GetPotionChance() == 40);
}

TEST_CASE("A Question Card widens the pick and a Busted Crown narrows it")
{
    std::mt19937 rng(23);
    RewardGenerator generator(CardColor::RED, rng);

    const Player curious = MakePlayer(CardColor::RED,
                                      { RelicId::QUESTION_CARD });
    const Reward wide =
        Find(generator.ForCombat(MonsterType::NORMAL, curious, rng),
             RewardKind::CARD_CHOICE);

    REQUIRE(wide.kind != RewardKind::INVALID);
    CHECK(wide.cards.size() == 4u);

    const Player crowned = MakePlayer(CardColor::RED,
                                      { RelicId::BUSTED_CROWN });
    const Reward narrow =
        Find(generator.ForCombat(MonsterType::NORMAL, crowned, rng),
             RewardKind::CARD_CHOICE);

    REQUIRE(narrow.kind != RewardKind::INVALID);
    CHECK(narrow.cards.size() == 1u);
}

TEST_CASE("A Prayer Wheel hands over a second pick after a plain fight")
{
    std::mt19937 rng(29);
    RewardGenerator generator(CardColor::RED, rng);
    const Player player = MakePlayer(CardColor::RED,
                                     { RelicId::PRAYER_WHEEL });

    const std::vector<Reward> rewards =
        generator.ForCombat(MonsterType::NORMAL, player, rng);

    CHECK(Count(rewards, RewardKind::CARD_CHOICE) == 2);

    // Only after a plain one.
    CHECK(Count(generator.ForCombat(MonsterType::ELITE, player, rng),
                RewardKind::CARD_CHOICE) == 1);
}

TEST_CASE("A Black Star leaves a second relic after an elite")
{
    std::mt19937 rng(31);
    RewardGenerator generator(CardColor::RED, rng);
    const Player player = MakePlayer(CardColor::RED, { RelicId::BLACK_STAR });

    CHECK(Count(generator.ForCombat(MonsterType::ELITE, player, rng),
                RewardKind::RELIC_CHOICE) == 2);
}

TEST_CASE("The cards offered come from the character and the right rarities")
{
    std::mt19937 rng(37);
    RewardGenerator generator(CardColor::GREEN, rng);
    const Player player = MakePlayer(CardColor::GREEN);

    std::map<CardRarity, int> tally;

    for (int i = 0; i < 60; ++i)
    {
        const Reward cards =
            Find(generator.ForCombat(MonsterType::NORMAL, player, rng),
                 RewardKind::CARD_CHOICE);

        REQUIRE(cards.kind != RewardKind::INVALID);

        for (const CardId id : cards.cards)
        {
            const Card card = CardRegistry::Get(id);

            CHECK(card.GetColor() == CardColor::GREEN);
            CHECK(card.GetRarity() != CardRarity::BASIC);
            CHECK(card.GetRarity() != CardRarity::SPECIAL);

            ++tally[card.GetRarity()];
        }
    }

    // Commons come up most often, and rares do turn up.
    CHECK(tally[CardRarity::COMMON] > tally[CardRarity::UNCOMMON]);
    CHECK(tally[CardRarity::UNCOMMON] > tally[CardRarity::RARE]);
    CHECK(tally[CardRarity::RARE] > 0);
}

TEST_CASE("An elite offers better odds on a rare card than a plain fight")
{
    std::mt19937 rng(41);
    const Player player = MakePlayer(CardColor::RED);

    int plainRares = 0;
    int eliteRares = 0;

    for (int i = 0; i < 120; ++i)
    {
        RewardGenerator plain(CardColor::RED, rng);
        RewardGenerator elite(CardColor::RED, rng);

        const Reward plainCards =
            Find(plain.ForCombat(MonsterType::NORMAL, player, rng),
                 RewardKind::CARD_CHOICE);
        const Reward eliteCards =
            Find(elite.ForCombat(MonsterType::ELITE, player, rng),
                 RewardKind::CARD_CHOICE);

        REQUIRE(plainCards.kind != RewardKind::INVALID);
        REQUIRE(eliteCards.kind != RewardKind::INVALID);

        for (const CardId id : plainCards.cards)
        {
            if (CardRegistry::Get(id).GetRarity() == CardRarity::RARE)
            {
                ++plainRares;
            }
        }

        for (const CardId id : eliteCards.cards)
        {
            if (CardRegistry::Get(id).GetRarity() == CardRarity::RARE)
            {
                ++eliteRares;
            }
        }
    }

    CHECK(eliteRares > plainRares);
}

TEST_CASE("A relic is never handed over twice in a run")
{
    std::mt19937 rng(43);
    RewardGenerator generator(CardColor::RED, rng);
    Player player = MakePlayer(CardColor::RED);

    std::vector<RelicId> seen;

    for (int i = 0; i < 40; ++i)
    {
        const std::vector<Reward> rewards =
            generator.ForCombat(MonsterType::ELITE, player, rng);
        const Reward relic = Find(rewards, RewardKind::RELIC_CHOICE);

        if (relic.kind == RewardKind::INVALID)
        {
            continue;
        }

        const RelicId id = relic.relics[0];

        for (const RelicId already : seen)
        {
            CHECK(already != id);
        }

        seen.emplace_back(id);
        player.AddRelic(RelicRegistry::Get(id));
    }

    CHECK(seen.size() > 20u);
}

TEST_CASE("A chest holds a relic of the kind its size allows")
{
    std::mt19937 rng(47);
    const Player player = MakePlayer(CardColor::RED);

    for (int i = 0; i < 30; ++i)
    {
        RewardGenerator generator(CardColor::RED, rng);

        const Reward small =
            Find(generator.ForChest(ChestSize::SMALL, player, rng),
                 RewardKind::RELIC_CHOICE);

        REQUIRE(small.kind != RewardKind::INVALID);

        // A small chest never holds a rare one.
        CHECK(RelicRegistry::Get(small.relics[0]).GetTier() !=
              RelicTier::RARE);

        RewardGenerator other(CardColor::RED, rng);

        const Reward large =
            Find(other.ForChest(ChestSize::LARGE, player, rng),
                 RewardKind::RELIC_CHOICE);

        REQUIRE(large.kind != RewardKind::INVALID);

        // A large one never holds a common.
        CHECK(RelicRegistry::Get(large.relics[0]).GetTier() !=
              RelicTier::COMMON);
    }
}

TEST_CASE("The gold in a chest matches its size")
{
    std::mt19937 rng(53);
    const Player player = MakePlayer(CardColor::RED);

    bool sawGold = false;

    for (int i = 0; i < 40; ++i)
    {
        RewardGenerator generator(CardColor::RED, rng);
        const Reward gold =
            Find(generator.ForChest(ChestSize::LARGE, player, rng),
                 RewardKind::GOLD);

        if (gold.kind == RewardKind::INVALID)
        {
            continue;
        }

        sawGold = true;
        CHECK(gold.amount >= 68);
        CHECK(gold.amount <= 82);
    }

    CHECK(sawGold == true);
}

TEST_CASE("Chest sizes come up as often as they should")
{
    std::mt19937 rng(59);
    std::map<ChestSize, int> tally;

    for (int i = 0; i < 600; ++i)
    {
        ++tally[RewardGenerator::RollChestSize(rng)];
    }

    // Half small, a third medium, the rest large.
    CHECK(tally[ChestSize::SMALL] > tally[ChestSize::MEDIUM]);
    CHECK(tally[ChestSize::MEDIUM] > tally[ChestSize::LARGE]);
    CHECK(tally[ChestSize::LARGE] > 0);
}

TEST_CASE("A Cursed Key puts a curse in the chest")
{
    std::mt19937 rng(61);
    RewardGenerator generator(CardColor::RED, rng);
    const Player player = MakePlayer(CardColor::RED, { RelicId::CURSED_KEY });

    const std::vector<Reward> rewards =
        generator.ForChest(ChestSize::MEDIUM, player, rng);
    const Reward curse = Find(rewards, RewardKind::CURSE);

    REQUIRE(curse.kind != RewardKind::INVALID);
    REQUIRE(curse.cards.size() == 1u);
    CHECK(CardRegistry::Get(curse.cards[0]).GetCardType() ==
          CardType::CURSE);
}

TEST_CASE("A Hungry Face leaves the next chest empty, once")
{
    std::mt19937 rng(67);
    RewardGenerator generator(CardColor::RED, rng);
    const Player player = MakePlayer(CardColor::RED,
                                     { RelicId::NLOTHS_HUNGRY_FACE });

    CHECK(generator.ForChest(ChestSize::LARGE, player, rng).empty() == true);
    CHECK(generator.ForChest(ChestSize::LARGE, player, rng).empty() == false);
}

TEST_CASE("A Matryoshka has a second relic tucked inside the next two chests")
{
    std::mt19937 rng(71);
    RewardGenerator generator(CardColor::RED, rng);
    const Player player = MakePlayer(CardColor::RED, { RelicId::MATRYOSHKA });

    CHECK(Count(generator.ForChest(ChestSize::SMALL, player, rng),
                RewardKind::RELIC_CHOICE) == 2);
    CHECK(Count(generator.ForChest(ChestSize::SMALL, player, rng),
                RewardKind::RELIC_CHOICE) == 2);

    // The third one is on its own again.
    CHECK(Count(generator.ForChest(ChestSize::SMALL, player, rng),
                RewardKind::RELIC_CHOICE) == 1);
}

TEST_CASE("A run collects what a won fight leaves behind")
{
    Run run(CardColor::RED, 101);

    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);
    REQUIRE(run.GetRewards().empty() == true);

    Battle battle = run.StartBattleHere();

    // Knock the fight over however long it takes.
    FightToTheEnd(battle);

    REQUIRE(battle.GetPhase() == BattlePhase::WON);

    const int goldBefore = run.GetGold();
    const std::size_t deckBefore = run.GetDeck().size();

    run.FinishBattle(battle);

    REQUIRE(run.HasUnclaimedRewards() == true);

    const Reward gold = Find(run.GetRewards(), RewardKind::GOLD);
    REQUIRE(gold.kind != RewardKind::INVALID);

    // Taking the gold puts it in the purse.
    for (std::size_t i = 0; i < run.GetRewards().size(); ++i)
    {
        if (run.GetRewards()[i].kind == RewardKind::GOLD)
        {
            REQUIRE(run.ClaimReward(i) == true);
            CHECK(run.GetGold() == goldBefore + gold.amount);

            // Twice is not allowed.
            CHECK(run.ClaimReward(i) == false);
        }
    }

    // Taking a card puts it in the deck.
    for (std::size_t i = 0; i < run.GetRewards().size(); ++i)
    {
        if (run.GetRewards()[i].kind == RewardKind::CARD_CHOICE)
        {
            const CardId wanted = run.GetRewards()[i].cards.front();

            REQUIRE(run.ClaimReward(i, 0) == true);
            CHECK(run.GetDeck().size() == deckBefore + 1);
            CHECK(run.GetDeck().back().GetId() == wanted);
        }
    }

    run.ClearRewards();
    CHECK(run.HasUnclaimedRewards() == false);
    CHECK(run.GetRewards().empty() == true);
}

TEST_CASE("Turning a card down with a Singing Bowl raises the health instead")
{
    Run run(CardColor::RED, 103);
    run.AddRelic(RelicId::SINGING_BOWL);

    std::mt19937 rng(5);
    RewardGenerator generator(CardColor::RED, rng);

    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);

    const int healthBefore = run.GetPlayer().GetMaxHealth();
    const std::size_t deckBefore = run.GetDeck().size();

    // A chest is the quickest way to something to turn down, so a card reward
    // is put together by hand here.
    Battle battle = run.StartBattleHere();

    FightToTheEnd(battle);

    run.FinishBattle(battle);

    bool turnedDown = false;

    for (std::size_t i = 0; i < run.GetRewards().size(); ++i)
    {
        if (run.GetRewards()[i].kind == RewardKind::CARD_CHOICE)
        {
            REQUIRE(run.SkipReward(i) == true);
            turnedDown = true;
            break;
        }
    }

    REQUIRE(turnedDown == true);

    CHECK(run.GetPlayer().GetMaxHealth() == healthBefore + 2);
    CHECK(run.GetDeck().size() == deckBefore);
}

TEST_CASE("A run opens the chest that waits where it stands")
{
    Run run(CardColor::RED, 107);

    const ChestSize size = run.OpenChest();

    CHECK(size != ChestSize::INVALID);
    REQUIRE(run.GetRewards().empty() == false);

    const Reward relic = Find(run.GetRewards(), RewardKind::RELIC_CHOICE);

    REQUIRE(relic.kind != RewardKind::INVALID);
    REQUIRE(relic.relics.size() == 1u);

    const int healthBefore = run.GetPlayer().GetMaxHealth();
    const RelicId wanted = relic.relics[0];

    for (std::size_t i = 0; i < run.GetRewards().size(); ++i)
    {
        if (run.GetRewards()[i].kind == RewardKind::RELIC_CHOICE)
        {
            REQUIRE(run.ClaimReward(i) == true);
        }
    }

    CHECK(run.GetPlayer().HasRelic(wanted) == true);

    // A relic that carries health hands it over on the way in.
    if (RelicRegistry::BonusMaxHealth(wanted) > 0)
    {
        CHECK(run.GetPlayer().GetMaxHealth() > healthBefore);
    }
}

TEST_CASE("A potion reward stays on the pile while the belt is full")
{
    Run run(CardColor::RED, 109);

    for (int i = 0; i < 3; ++i)
    {
        REQUIRE(run.AddPotion(PotionId::FIRE_POTION) == true);
    }

    std::mt19937 rng(2);
    RewardGenerator generator(CardColor::RED, rng);

    // Put a potion on the pile by hand: a full belt has to turn it away.
    REQUIRE(run.GetPlayer().GetPotions().size() == 3u);

    const Player& player = run.GetPlayer();
    const std::vector<Reward> rewards =
        generator.ForCombat(MonsterType::NORMAL,
                            MakePlayer(CardColor::RED,
                                       { RelicId::WHITE_BEAST_STATUE }),
                            rng);

    REQUIRE(Has(rewards, RewardKind::POTION) == true);
    CHECK(player.GetPotions().size() == 3u);
}
