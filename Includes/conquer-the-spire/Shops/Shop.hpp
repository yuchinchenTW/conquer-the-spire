// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_SHOP_HPP
#define CONQUER_THE_SPIRE_SHOP_HPP

#include <conquer-the-spire/Models/Player.hpp>
#include <conquer-the-spire/Rewards/RewardGenerator.hpp>

#include <istream>
#include <random>
#include <string>
#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief ShopCard struct.
//!
//! A card on the shelf, what it costs, and whether it has been bought.
//!
struct ShopCard
{
    CardId id = CardId::INVALID;
    int price = 0;
    bool sold = false;

    //! Which of the five coloured slots this is, or true when it is one of
    //! the two colourless ones the shop always keeps.
    bool colorless = false;
};

//!
//! \brief ShopRelic struct.
//!
struct ShopRelic
{
    RelicId id = RelicId::INVALID;
    int price = 0;
    bool sold = false;
};

//!
//! \brief ShopPotion struct.
//!
struct ShopPotion
{
    PotionId id = PotionId::INVALID;
    int price = 0;
    bool sold = false;
};

//!
//! \brief Shop class.
//!
//! What a merchant has laid out: five cards of the character, two colourless
//! ones, three relics, three potions, and the offer to take a card out of the
//! deck. The prices are rolled once when the shop opens and the discounts a
//! Membership Card or a Courier bring are already in them.
//!
class Shop
{
 public:
    //! How many cards of the character are on the shelf, and the types they
    //! come in: two attacks, two skills and a power.
    static constexpr int COLORED_CARDS = 5;

    //! How many colourless cards are on the shelf: one uncommon, one rare.
    static constexpr int COLORLESS_CARDS = 2;

    //! How much more a colourless card costs, out of a hundred.
    static constexpr int COLORLESS_MARKUP = 120;

    static constexpr int RELIC_SLOTS = 3;
    static constexpr int POTION_SLOTS = 3;

    //! What taking a card out of the deck costs the first time, and how much
    //! dearer it gets with every one after that.
    static constexpr int FIRST_REMOVAL_PRICE = 75;
    static constexpr int REMOVAL_PRICE_STEP = 25;

    //! What a Smiling Mask settles the price at, for good.
    static constexpr int MASK_REMOVAL_PRICE = 50;

    Shop() = default;

    //! Lays out a shop for \p player, drawing the relics out of \p books so
    //! that nothing turns up twice in a run. \p removalPrice is what taking a
    //! card out costs by now.
    Shop(CardColor character, const Player& player, RewardGenerator& books,
         int removalPrice, std::mt19937& rng);

    //! Writes the shelf out as one line, and reads it back.
    std::string Serialize() const;
    bool Load(std::istream& in);

    //! Returns the cards on the shelf: the five of the character first, then
    //! the two colourless ones.
    const std::vector<ShopCard>& GetCards() const;

    const std::vector<ShopRelic>& GetRelics() const;
    const std::vector<ShopPotion>& GetPotions() const;

    //! Returns what taking a card out of the deck costs here.
    int GetRemovalPrice() const;

    //! Returns true once the one card this shop will take out is gone.
    bool IsRemovalSpent() const;

    //! Marks the card at \p index as bought. A Courier puts something else in
    //! its place. Returns false when there is nothing there to buy.
    bool TakeCard(std::size_t index, const Player& player,
                  std::mt19937& rng);

    //! Marks the relic at \p index as bought, restocking as above.
    bool TakeRelic(std::size_t index, const Player& player,
                   RewardGenerator& books, std::mt19937& rng);

    //! Marks the potion at \p index as bought, restocking as above.
    bool TakePotion(std::size_t index, const Player& player,
                    RewardGenerator& books, std::mt19937& rng);

    //! Marks the one card removal this shop offers as used up.
    void SpendRemoval();

    //! Returns \p price with the discounts \p player carries taken off. A
    //! Membership Card halves it and a Courier takes a fifth off, and both
    //! together do both.
    static int Discounted(int price, const Player& player);

 private:
    //! Fills a coloured slot with a card of \p type, and a colourless one
    //! with a card of \p rarity.
    void StockColoredSlot(std::size_t index, CardType type,
                          const Player& player, std::mt19937& rng);
    void StockColorlessSlot(std::size_t index, CardRarity rarity,
                            const Player& player, std::mt19937& rng);

    //! Rolls a card of \p color and \p rarity that is not on the shelf yet.
    //! \p type filters the pool further when it is not INVALID.
    CardId RollCard(CardColor color, CardRarity rarity, CardType type,
                    std::mt19937& rng) const;

    //! Rolls a rarity the way a shop does: 54 common, 37 uncommon, 9 rare.
    static CardRarity RollRarity(std::mt19937& rng);

    //! What a card of \p rarity is worth, before any discount.
    static int CardPrice(CardRarity rarity, bool colorless,
                         std::mt19937& rng);
    static int RelicPrice(RelicTier tier, std::mt19937& rng);
    static int PotionPrice(PotionRarity rarity, std::mt19937& rng);

    CardColor m_character = CardColor::INVALID;
    std::vector<ShopCard> m_cards;
    std::vector<ShopRelic> m_relics;
    std::vector<ShopPotion> m_potions;
    int m_removalPrice = FIRST_REMOVAL_PRICE;
    bool m_removalSpent = false;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_SHOP_HPP
