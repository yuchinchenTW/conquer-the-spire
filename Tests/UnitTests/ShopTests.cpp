#include "doctest.h"

#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Potions/PotionRegistry.hpp>
#include <conquer-the-spire/Relics/RelicRegistry.hpp>
#include <conquer-the-spire/Run/Run.hpp>
#include <conquer-the-spire/Shops/Shop.hpp>

#include <algorithm>
#include <cstddef>
#include <map>
#include <random>
#include <set>
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

//! Lays out a shop for a player of \p color carrying \p relics.
Shop MakeShop(std::mt19937& rng, CardColor color,
              const std::vector<RelicId>& relics = {},
              int removalPrice = Shop::FIRST_REMOVAL_PRICE)
{
    const Player player = MakePlayer(color, relics);
    RewardGenerator books(color, rng);

    return Shop(color, player, books, removalPrice, rng);
}

//! Walks a run to a shop with enough gold to spend there.
Run MakeRichRun(CardColor color, int gold,
                const std::vector<RelicId>& relics = {})
{
    Run run(color, 7);

    for (const RelicId id : relics)
    {
        run.AddRelic(id);
    }

    run.AddGold(gold);
    run.OpenShop();

    return run;
}
}  // namespace

TEST_CASE("A shop lays out five cards of the character and two colourless")
{
    std::mt19937 rng(11);
    const Shop shop = MakeShop(rng, CardColor::RED);

    const std::vector<ShopCard>& cards = shop.GetCards();

    REQUIRE(cards.size() == 7u);

    // Two attacks, two skills and a power, all of the character.
    const CardType wanted[] = { CardType::ATTACK, CardType::ATTACK,
                                CardType::SKILL, CardType::SKILL,
                                CardType::POWER };

    for (std::size_t i = 0; i < 5u; ++i)
    {
        const Card card = CardRegistry::Get(cards[i].id);

        CHECK(cards[i].colorless == false);
        CHECK(card.GetColor() == CardColor::RED);
        CHECK(card.GetCardType() == wanted[i]);
        CHECK(card.GetRarity() != CardRarity::BASIC);
        CHECK(card.GetRarity() != CardRarity::SPECIAL);
    }

    // One uncommon colourless card and one rare one.
    CHECK(cards[5].colorless == true);
    CHECK(cards[6].colorless == true);
    CHECK(CardRegistry::Get(cards[5].id).GetColor() == CardColor::COLORLESS);
    CHECK(CardRegistry::Get(cards[6].id).GetColor() == CardColor::COLORLESS);
    CHECK(CardRegistry::Get(cards[5].id).GetRarity() == CardRarity::UNCOMMON);
    CHECK(CardRegistry::Get(cards[6].id).GetRarity() == CardRarity::RARE);

    // Nothing is laid out twice.
    std::set<CardId> seen;

    for (const ShopCard& card : cards)
    {
        CHECK(seen.insert(card.id).second == true);
    }
}

TEST_CASE("What is on the shelf is priced by what it is")
{
    std::mt19937 rng(13);

    for (int i = 0; i < 20; ++i)
    {
        const Shop shop = MakeShop(rng, CardColor::GREEN);

        for (const ShopCard& slot : shop.GetCards())
        {
            const CardRarity rarity = CardRegistry::Get(slot.id).GetRarity();
            int low = 45;
            int high = 55;

            if (rarity == CardRarity::UNCOMMON)
            {
                low = 68;
                high = 83;
            }
            else if (rarity == CardRarity::RARE)
            {
                low = 135;
                high = 165;
            }

            if (slot.colorless)
            {
                low = low * 120 / 100;
                high = high * 120 / 100;
            }

            CHECK(slot.price >= low);
            CHECK(slot.price <= high);
        }

        for (const ShopRelic& slot : shop.GetRelics())
        {
            const RelicTier tier = RelicRegistry::Get(slot.id).GetTier();
            int low = 143;
            int high = 158;

            if (tier == RelicTier::UNCOMMON)
            {
                low = 238;
                high = 263;
            }
            else if (tier == RelicTier::RARE)
            {
                low = 285;
                high = 315;
            }

            CHECK(slot.price >= low);
            CHECK(slot.price <= high);
        }

        for (const ShopPotion& slot : shop.GetPotions())
        {
            const PotionRarity rarity =
                PotionRegistry::Get(slot.id).GetRarity();
            int low = 48;
            int high = 53;

            if (rarity == PotionRarity::UNCOMMON)
            {
                low = 71;
                high = 79;
            }
            else if (rarity == PotionRarity::RARE)
            {
                low = 95;
                high = 105;
            }

            CHECK(slot.price >= low);
            CHECK(slot.price <= high);
        }
    }
}

TEST_CASE("The rightmost relic is one only a shop carries")
{
    std::mt19937 rng(17);

    for (int i = 0; i < 10; ++i)
    {
        const Shop shop = MakeShop(rng, CardColor::BLUE);
        const std::vector<ShopRelic>& relics = shop.GetRelics();

        REQUIRE(relics.size() == 3u);
        CHECK(RelicRegistry::Get(relics[2].id).GetTier() == RelicTier::SHOP);
    }
}

TEST_CASE("The card rarities a shop sells lean common")
{
    std::mt19937 rng(19);
    std::map<CardRarity, int> tally;
    std::map<CardRarity, int> powers;

    for (int i = 0; i < 40; ++i)
    {
        const Shop shop = MakeShop(rng, CardColor::RED);
        const std::vector<ShopCard>& cards = shop.GetCards();

        // The attack and skill slots are the ones the weights show in.
        for (std::size_t slot = 0; slot < 4u; ++slot)
        {
            ++tally[CardRegistry::Get(cards[slot].id).GetRarity()];
        }

        ++powers[CardRegistry::Get(cards[4].id).GetRarity()];
    }

    CHECK(tally[CardRarity::COMMON] > tally[CardRarity::UNCOMMON]);
    CHECK(tally[CardRarity::UNCOMMON] > tally[CardRarity::RARE]);
    CHECK(tally[CardRarity::RARE] > 0);

    // The Ironclad has no common power, so that slot sells what he does have
    // rather than nothing at all.
    CHECK(powers[CardRarity::COMMON] == 0);
    CHECK(powers[CardRarity::UNCOMMON] + powers[CardRarity::RARE] == 40);
}

TEST_CASE("Buying a card takes the gold and empties the slot")
{
    Run run = MakeRichRun(CardColor::RED, 500);

    const std::size_t before = run.GetDeck().size();
    const int purse = run.GetGold();
    const ShopCard slot = run.GetShop().GetCards().front();

    REQUIRE(run.BuyCard(0) == true);

    CHECK(run.GetGold() == purse - slot.price);
    CHECK(run.GetDeck().size() == before + 1);
    CHECK(run.GetDeck().back().GetId() == slot.id);
    CHECK(run.GetShop().GetCards().front().sold == true);

    // The same slot cannot be bought twice.
    CHECK(run.BuyCard(0) == false);
}

TEST_CASE("A shop turns down what the purse cannot cover")
{
    Run run(CardColor::RED, 7);
    run.OpenShop();

    const std::size_t before = run.GetDeck().size();
    const int purse = run.GetGold();

    // The starting purse is nowhere near a rare relic.
    CHECK(run.BuyRelic(2) == false);
    CHECK(run.GetGold() == purse);
    CHECK(run.GetDeck().size() == before);
}

TEST_CASE("Buying a relic hands it over and takes it off the shelf")
{
    Run run = MakeRichRun(CardColor::RED, 900);

    const ShopRelic slot = run.GetShop().GetRelics()[2];
    const int purse = run.GetGold();

    REQUIRE(run.BuyRelic(2) == true);

    CHECK(run.GetPlayer().HasRelic(slot.id) == true);
    CHECK(run.GetGold() == purse - slot.price);
    CHECK(run.GetShop().GetRelics()[2].sold == true);
}

TEST_CASE("A full belt leaves the potion on the shelf and the gold in hand")
{
    Run run = MakeRichRun(CardColor::RED, 900);

    // Fill the belt from the shelf, then try once more.
    int bought = 0;

    while (run.BuyPotion(0) || run.BuyPotion(1) || run.BuyPotion(2))
    {
        ++bought;
    }

    CHECK(bought > 0);
    CHECK(static_cast<int>(run.GetPlayer().GetPotions().size()) ==
          run.GetPlayer().GetPotionSlots());

    const int purse = run.GetGold();
    const std::vector<ShopPotion>& potions = run.GetShop().GetPotions();
    const bool anyLeft =
        std::any_of(potions.begin(), potions.end(),
                    [](const ShopPotion& slot) { return !slot.sold; });

    if (anyLeft)
    {
        CHECK(run.GetGold() == purse);
    }
}

TEST_CASE("A merchant takes one card out of the deck, for a rising price")
{
    Run run = MakeRichRun(CardColor::RED, 500);

    REQUIRE(run.GetCardRemovalPrice() == 75);

    const std::size_t before = run.GetDeck().size();
    const int purse = run.GetGold();

    REQUIRE(run.BuyCardRemoval(0) == true);

    CHECK(run.GetDeck().size() == before - 1);
    CHECK(run.GetGold() == purse - 75);

    // One card per shop, and the next one costs more.
    CHECK(run.BuyCardRemoval(0) == false);
    CHECK(run.GetCardRemovalPrice() == 100);

    run.OpenShop();

    CHECK(run.GetShop().GetRemovalPrice() == 100);
    CHECK(run.BuyCardRemoval(0) == true);
    CHECK(run.GetCardRemovalPrice() == 125);
}

TEST_CASE("A Smiling Mask settles the removal price at fifty")
{
    Run run = MakeRichRun(CardColor::RED, 500, { RelicId::SMILING_MASK });

    CHECK(run.GetShop().GetRemovalPrice() == 50);

    REQUIRE(run.BuyCardRemoval(0) == true);

    run.OpenShop();

    CHECK(run.GetShop().GetRemovalPrice() == 50);
}

TEST_CASE("A Membership Card and a Courier come off the price together")
{
    const Player plain = MakePlayer(CardColor::RED);
    const Player member =
        MakePlayer(CardColor::RED, { RelicId::MEMBERSHIP_CARD });
    const Player courier = MakePlayer(CardColor::RED, { RelicId::THE_COURIER });
    const Player both = MakePlayer(
        CardColor::RED, { RelicId::MEMBERSHIP_CARD, RelicId::THE_COURIER });

    CHECK(Shop::Discounted(100, plain) == 100);
    CHECK(Shop::Discounted(100, member) == 50);
    CHECK(Shop::Discounted(100, courier) == 80);
    CHECK(Shop::Discounted(100, both) == 40);

    // Nothing is ever free.
    CHECK(Shop::Discounted(1, both) >= 1);
}

TEST_CASE("A Courier puts something else out when a slot empties")
{
    Run run = MakeRichRun(CardColor::RED, 2000, { RelicId::THE_COURIER });

    const CardId first = run.GetShop().GetCards().front().id;

    REQUIRE(run.BuyCard(0) == true);

    const ShopCard slot = run.GetShop().GetCards().front();

    CHECK(slot.sold == false);
    CHECK(slot.id != first);

    // And the shelf is still a fifth cheaper than it would be.
    const ShopPotion potion = run.GetShop().GetPotions().front();

    CHECK(potion.price <= 105 * 80 / 100);
}

TEST_CASE("A shop draws from the same relic books as the rest of the run")
{
    Run run = MakeRichRun(CardColor::RED, 20000);

    std::set<RelicId> seen;

    for (int i = 0; i < 6; ++i)
    {
        run.OpenShop();

        for (const ShopRelic& slot : run.GetShop().GetRelics())
        {
            if (slot.id != RelicId::INVALID)
            {
                CHECK(seen.insert(slot.id).second == true);
            }
        }
    }

    CHECK(seen.size() >= 12u);
}
