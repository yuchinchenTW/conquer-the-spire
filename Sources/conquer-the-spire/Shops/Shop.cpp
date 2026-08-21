// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Potions/PotionRegistry.hpp>
#include <conquer-the-spire/Relics/RelicRegistry.hpp>
#include <conquer-the-spire/Shops/Shop.hpp>

#include <algorithm>
#include <sstream>

namespace ConquerTheSpire
{
namespace
{
//! Rolls a number between \p low and \p high, both included.
int Roll(std::mt19937& rng, int low, int high)
{
    std::uniform_int_distribution<int> pick(low, high);

    return pick(rng);
}

//! What a coloured slot sells: two attacks, two skills and a power.
CardType SlotType(std::size_t index)
{
    if (index < 2)
    {
        return CardType::ATTACK;
    }

    return index < 4 ? CardType::SKILL : CardType::POWER;
}

//! What a colourless slot sells: one uncommon and one rare.
CardRarity SlotRarity(std::size_t index)
{
    return index == static_cast<std::size_t>(Shop::COLORED_CARDS)
               ? CardRarity::UNCOMMON
               : CardRarity::RARE;
}
}  // namespace

Shop::Shop(CardColor character, const Player& player, RewardGenerator& books,
           int removalPrice, std::mt19937& rng)
    : m_character(character), m_removalPrice(removalPrice)
{
    // A Smiling Mask settles what taking a card out costs, for good.
    if (player.HasRelic(RelicId::SMILING_MASK))
    {
        m_removalPrice = MASK_REMOVAL_PRICE;
    }

    m_removalPrice = Discounted(m_removalPrice, player);

    m_cards.resize(COLORED_CARDS + COLORLESS_CARDS);

    for (std::size_t i = 0; i < static_cast<std::size_t>(COLORED_CARDS); ++i)
    {
        StockColoredSlot(i, SlotType(i), player, rng);
    }

    for (std::size_t i = 0; i < static_cast<std::size_t>(COLORLESS_CARDS); ++i)
    {
        const std::size_t slot = static_cast<std::size_t>(COLORED_CARDS) + i;

        StockColorlessSlot(slot, SlotRarity(slot), player, rng);
    }

    m_relics.resize(RELIC_SLOTS);

    for (std::size_t i = 0; i < static_cast<std::size_t>(RELIC_SLOTS); ++i)
    {
        // The rightmost slot is always a relic only a shop carries.
        const RelicTier tier = i + 1 == static_cast<std::size_t>(RELIC_SLOTS)
                                   ? RelicTier::SHOP
                                   : books.RollTier(rng);
        const RelicId id = books.TakeRelic(tier, player, rng);

        m_relics[i].id = id;
        m_relics[i].sold = id == RelicId::INVALID;
        m_relics[i].price =
            Discounted(RelicPrice(RelicRegistry::Get(id).GetTier(), rng),
                       player);
    }

    m_potions.resize(POTION_SLOTS);

    for (auto& slot : m_potions)
    {
        const PotionId id = books.TakePotion(rng);

        slot.id = id;
        slot.sold = id == PotionId::INVALID;
        slot.price = Discounted(
            PotionPrice(PotionRegistry::Get(id).GetRarity(), rng), player);
    }
}

std::string Shop::Serialize() const
{
    std::ostringstream out;

    out << static_cast<int>(m_character) << ' ' << m_removalPrice << ' '
        << (m_removalSpent ? 1 : 0) << ' ' << m_cards.size();

    for (const auto& slot : m_cards)
    {
        out << ' ' << static_cast<int>(slot.id) << ' ' << slot.price << ' '
            << (slot.sold ? 1 : 0) << ' ' << (slot.colorless ? 1 : 0);
    }

    out << ' ' << m_relics.size();

    for (const auto& slot : m_relics)
    {
        out << ' ' << static_cast<int>(slot.id) << ' ' << slot.price << ' '
            << (slot.sold ? 1 : 0);
    }

    out << ' ' << m_potions.size();

    for (const auto& slot : m_potions)
    {
        out << ' ' << static_cast<int>(slot.id) << ' ' << slot.price << ' '
            << (slot.sold ? 1 : 0);
    }

    return out.str();
}

bool Shop::Load(std::istream& in)
{
    int character = 0;
    int spent = 0;
    std::size_t count = 0;

    in >> character >> m_removalPrice >> spent >> count;

    m_character = static_cast<CardColor>(character);
    m_removalSpent = spent != 0;
    m_cards.clear();

    for (std::size_t i = 0; i < count; ++i)
    {
        ShopCard slot;
        int id = 0;
        int sold = 0;
        int colorless = 0;

        in >> id >> slot.price >> sold >> colorless;

        slot.id = static_cast<CardId>(id);
        slot.sold = sold != 0;
        slot.colorless = colorless != 0;
        m_cards.emplace_back(slot);
    }

    in >> count;
    m_relics.clear();

    for (std::size_t i = 0; i < count; ++i)
    {
        ShopRelic slot;
        int id = 0;
        int sold = 0;

        in >> id >> slot.price >> sold;

        slot.id = static_cast<RelicId>(id);
        slot.sold = sold != 0;
        m_relics.emplace_back(slot);
    }

    in >> count;
    m_potions.clear();

    for (std::size_t i = 0; i < count; ++i)
    {
        ShopPotion slot;
        int id = 0;
        int sold = 0;

        in >> id >> slot.price >> sold;

        slot.id = static_cast<PotionId>(id);
        slot.sold = sold != 0;
        m_potions.emplace_back(slot);
    }

    return static_cast<bool>(in);
}

const std::vector<ShopCard>& Shop::GetCards() const
{
    return m_cards;
}

const std::vector<ShopRelic>& Shop::GetRelics() const
{
    return m_relics;
}

const std::vector<ShopPotion>& Shop::GetPotions() const
{
    return m_potions;
}

int Shop::GetRemovalPrice() const
{
    return m_removalPrice;
}

bool Shop::IsRemovalSpent() const
{
    return m_removalSpent;
}

bool Shop::TakeCard(std::size_t index, const Player& player,
                    std::mt19937& rng)
{
    if (index >= m_cards.size() || m_cards[index].sold)
    {
        return false;
    }

    m_cards[index].sold = true;

    // A Courier has another one out of the cart before the door shuts.
    if (player.HasRelic(RelicId::THE_COURIER))
    {
        if (m_cards[index].colorless)
        {
            StockColorlessSlot(index, SlotRarity(index), player, rng);
        }
        else
        {
            StockColoredSlot(index, SlotType(index), player, rng);
        }
    }

    return true;
}

bool Shop::TakeRelic(std::size_t index, const Player& player,
                     RewardGenerator& books, std::mt19937& rng)
{
    if (index >= m_relics.size() || m_relics[index].sold)
    {
        return false;
    }

    m_relics[index].sold = true;

    if (player.HasRelic(RelicId::THE_COURIER))
    {
        const RelicTier tier = index + 1 == m_relics.size()
                                   ? RelicTier::SHOP
                                   : books.RollTier(rng);
        const RelicId id = books.TakeRelic(tier, player, rng);

        if (id != RelicId::INVALID)
        {
            m_relics[index].id = id;
            m_relics[index].sold = false;
            m_relics[index].price =
                Discounted(RelicPrice(RelicRegistry::Get(id).GetTier(), rng),
                           player);
        }
    }

    return true;
}

bool Shop::TakePotion(std::size_t index, const Player& player,
                      RewardGenerator& books, std::mt19937& rng)
{
    if (index >= m_potions.size() || m_potions[index].sold)
    {
        return false;
    }

    m_potions[index].sold = true;

    if (player.HasRelic(RelicId::THE_COURIER))
    {
        const PotionId id = books.TakePotion(rng);

        if (id != PotionId::INVALID)
        {
            m_potions[index].id = id;
            m_potions[index].sold = false;
            m_potions[index].price = Discounted(
                PotionPrice(PotionRegistry::Get(id).GetRarity(), rng),
                player);
        }
    }

    return true;
}

void Shop::SpendRemoval()
{
    m_removalSpent = true;
}

int Shop::Discounted(int price, const Player& player)
{
    int out = price;

    // A Membership Card halves everything and a Courier takes a fifth off
    // what is left.
    if (player.HasRelic(RelicId::MEMBERSHIP_CARD))
    {
        out = out / 2;
    }

    if (player.HasRelic(RelicId::THE_COURIER))
    {
        out = out * 80 / 100;
    }

    return std::max(1, out);
}

void Shop::StockColoredSlot(std::size_t index, CardType type,
                            const Player& player, std::mt19937& rng)
{
    CardRarity rarity = RollRarity(rng);
    CardId id = RollCard(m_character, rarity, type, rng);

    // A character short of that type at the rarity rolled sells whatever it
    // does have.
    const CardRarity fallbacks[] = { CardRarity::COMMON, CardRarity::UNCOMMON,
                                    CardRarity::RARE };

    for (const CardRarity other : fallbacks)
    {
        if (id != CardId::INVALID)
        {
            break;
        }

        rarity = other;
        id = RollCard(m_character, other, type, rng);
    }

    m_cards[index].id = id;
    m_cards[index].colorless = false;
    m_cards[index].sold = id == CardId::INVALID;
    m_cards[index].price =
        Discounted(CardPrice(rarity, false, rng), player);
}

void Shop::StockColorlessSlot(std::size_t index, CardRarity rarity,
                              const Player& player, std::mt19937& rng)
{
    const CardId id =
        RollCard(CardColor::COLORLESS, rarity, CardType::INVALID, rng);

    m_cards[index].id = id;
    m_cards[index].colorless = true;
    m_cards[index].sold = id == CardId::INVALID;
    m_cards[index].price = Discounted(CardPrice(rarity, true, rng), player);
}

CardId Shop::RollCard(CardColor color, CardRarity rarity, CardType type,
                      std::mt19937& rng) const
{
    std::vector<CardId> pool = CardRegistry::GetPool(color, rarity);
    std::vector<CardId> wanted;

    for (const CardId id : pool)
    {
        if (type != CardType::INVALID &&
            CardRegistry::Get(id).GetCardType() != type)
        {
            continue;
        }

        // Nothing is laid out twice.
        const bool onShelf =
            std::any_of(m_cards.begin(), m_cards.end(),
                        [id](const ShopCard& card) { return card.id == id; });

        if (!onShelf)
        {
            wanted.emplace_back(id);
        }
    }

    if (wanted.empty())
    {
        return CardId::INVALID;
    }

    std::uniform_int_distribution<std::size_t> pick(0, wanted.size() - 1);

    return wanted[pick(rng)];
}

CardRarity Shop::RollRarity(std::mt19937& rng)
{
    // A shop sells at 54 common, 37 uncommon and 9 rare.
    const int roll = Roll(rng, 1, 100);

    if (roll <= 54)
    {
        return CardRarity::COMMON;
    }

    return roll <= 91 ? CardRarity::UNCOMMON : CardRarity::RARE;
}

int Shop::CardPrice(CardRarity rarity, bool colorless, std::mt19937& rng)
{
    int price = Roll(rng, 45, 55);

    if (rarity == CardRarity::UNCOMMON)
    {
        price = Roll(rng, 68, 83);
    }
    else if (rarity == CardRarity::RARE)
    {
        price = Roll(rng, 135, 165);
    }

    // A colourless card is a fifth dearer than one of the character's own.
    return colorless ? price * COLORLESS_MARKUP / 100 : price;
}

int Shop::RelicPrice(RelicTier tier, std::mt19937& rng)
{
    switch (tier)
    {
        case RelicTier::UNCOMMON:
            return Roll(rng, 238, 263);

        case RelicTier::RARE:
            return Roll(rng, 285, 315);

        default:
            // A common one and one only a shop carries go for the same.
            return Roll(rng, 143, 158);
    }
}

int Shop::PotionPrice(PotionRarity rarity, std::mt19937& rng)
{
    switch (rarity)
    {
        case PotionRarity::UNCOMMON:
            return Roll(rng, 71, 79);

        case PotionRarity::RARE:
            return Roll(rng, 95, 105);

        default:
            return Roll(rng, 48, 53);
    }
}
}  // namespace ConquerTheSpire
