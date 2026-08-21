// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Potions/PotionRegistry.hpp>
#include <conquer-the-spire/Relics/RelicRegistry.hpp>
#include <conquer-the-spire/Rewards/RewardGenerator.hpp>

#include <algorithm>
#include <sstream>
#include <utility>

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

//! Returns true when a roll out of a hundred comes in under \p chance.
bool Chance(std::mt19937& rng, int chance)
{
    return Roll(rng, 1, 100) <= chance;
}

//! What a fight of each kind pays out in gold.
struct GoldRange
{
    int low;
    int high;
};

GoldRange GoldFor(MonsterType type)
{
    switch (type)
    {
        case MonsterType::ELITE:
            return { 25, 35 };

        case MonsterType::BOSS:
            return { 95, 105 };

        default:
            return { 10, 20 };
    }
}

//! The listed card rarity chances. An elite hands over better odds. A boss
//! is not on the table because it hands over rare cards and nothing else.
struct CardOdds
{
    int rare;
    int common;
};

CardOdds OddsFor(MonsterType type)
{
    // A shop sells at 9 / 37 / 54, which is for a shop to say once there is
    // one.
    return type == MonsterType::ELITE ? CardOdds{ 10, 50 }
                                      : CardOdds{ 3, 60 };
}

//! The colours a card reward can reach into once a Prismatic Shard is in
//! hand: every character's, and the colourless cards as well.
const CardColor PRISM_COLORS[] = { CardColor::RED, CardColor::GREEN,
                                  CardColor::BLUE, CardColor::COLORLESS };
}  // namespace

RewardGenerator::RewardGenerator(CardColor character, std::mt19937& rng)
    : m_character(character)
{
    const RelicTier tiers[] = { RelicTier::COMMON, RelicTier::UNCOMMON,
                               RelicTier::RARE, RelicTier::BOSS,
                               RelicTier::SHOP };

    for (const RelicTier tier : tiers)
    {
        std::vector<RelicId> pool = RelicRegistry::GetPool(tier);

        std::shuffle(pool.begin(), pool.end(), rng);
        m_relicPools[tier] = std::move(pool);
    }
}

void RewardGenerator::BeginAct(int act)
{
    m_act = act;
    m_potionChance = BASE_POTION_CHANCE;
}

int RewardGenerator::GetAct() const
{
    return m_act;
}

std::string RewardGenerator::Serialize() const
{
    std::ostringstream out;

    out << static_cast<int>(m_character) << ' ' << m_act << ' '
        << m_potionChance << ' ' << m_rareHandicap << ' '
        << m_extraChestRelics << ' ' << (m_matryoshkaStarted ? 1 : 0) << ' '
        << (m_hungryFaceSpent ? 1 : 0) << ' ' << m_relicPools.size();

    for (const auto& pool : m_relicPools)
    {
        out << ' ' << static_cast<int>(pool.first) << ' '
            << pool.second.size();

        for (const RelicId id : pool.second)
        {
            out << ' ' << static_cast<int>(id);
        }
    }

    return out.str();
}

bool RewardGenerator::Load(std::istream& in)
{
    int character = 0;
    int matryoshka = 0;
    int hungry = 0;
    std::size_t pools = 0;

    in >> character >> m_act >> m_potionChance >> m_rareHandicap >>
        m_extraChestRelics >> matryoshka >> hungry >> pools;

    m_character = static_cast<CardColor>(character);
    m_matryoshkaStarted = matryoshka != 0;
    m_hungryFaceSpent = hungry != 0;
    m_relicPools.clear();

    for (std::size_t i = 0; i < pools; ++i)
    {
        int tier = 0;
        std::size_t count = 0;

        in >> tier >> count;

        std::vector<RelicId> pool;

        for (std::size_t held = 0; held < count; ++held)
        {
            int id = 0;

            in >> id;
            pool.emplace_back(static_cast<RelicId>(id));
        }

        m_relicPools[static_cast<RelicTier>(tier)] = std::move(pool);
    }

    return static_cast<bool>(in);
}

std::vector<Reward> RewardGenerator::ForCombat(MonsterType type,
                                               const Player& player,
                                               std::mt19937& rng)
{
    std::vector<Reward> rewards;

    const GoldRange gold = GoldFor(type);
    rewards.emplace_back(Reward::Gold(Roll(rng, gold.low, gold.high)));

    // A boss hands over three rare cards and a pick of its own relics rather
    // than rolling for any of it. The last two acts hand over neither cards
    // nor potions.
    if (type == MonsterType::BOSS)
    {
        const bool generous = m_act <= 2;

        if (generous)
        {
            rewards.emplace_back(
                Reward::CardChoice(RollCardChoices(type, player, rng, true)));
        }

        std::vector<RelicId> choices;

        for (int i = 0; i < 3; ++i)
        {
            const RelicId id = DrawRelic(RelicTier::BOSS, player, rng);

            if (id != RelicId::INVALID)
            {
                choices.emplace_back(id);
            }
        }

        if (!choices.empty())
        {
            rewards.emplace_back(Reward::RelicChoice(std::move(choices)));
        }

        if (generous)
        {
            AddPotionReward(player, rng, rewards);
        }

        return rewards;
    }

    rewards.emplace_back(
        Reward::CardChoice(RollCardChoices(type, player, rng)));

    // A Prayer Wheel hands over a second pick after a plain fight.
    if (type == MonsterType::NORMAL &&
        player.HasRelic(RelicId::PRAYER_WHEEL))
    {
        rewards.emplace_back(
            Reward::CardChoice(RollCardChoices(type, player, rng)));
    }

    if (type == MonsterType::ELITE)
    {
        const RelicId id = DrawRelic(RollRelicTier(rng), player, rng);

        if (id != RelicId::INVALID)
        {
            rewards.emplace_back(Reward::RelicChoice({ id }));
        }

        // A Black Star puts a second relic on the pile.
        if (player.HasRelic(RelicId::BLACK_STAR))
        {
            const RelicId extra = DrawRelic(RollRelicTier(rng), player, rng);

            if (extra != RelicId::INVALID)
            {
                rewards.emplace_back(Reward::RelicChoice({ extra }));
            }
        }
    }

    AddPotionReward(player, rng, rewards);

    return rewards;
}

void RewardGenerator::AddPotionReward(const Player& player,
                                      std::mt19937& rng,
                                      std::vector<Reward>& rewards)
{
    // Sozu turns potions away, and a White Beast Statue makes sure of them.
    const bool always = player.HasRelic(RelicId::WHITE_BEAST_STATUE);
    const bool never = player.HasRelic(RelicId::SOZU);

    if (!never && (always || Chance(rng, m_potionChance)))
    {
        rewards.emplace_back(Reward::Potion(RollPotion(rng)));
        m_potionChance -= 10;
    }
    else if (!never)
    {
        m_potionChance += 10;
    }

    m_potionChance = std::min(100, std::max(0, m_potionChance));
}

std::vector<Reward> RewardGenerator::ForChest(ChestSize size,
                                              const Player& player,
                                              std::mt19937& rng)
{
    std::vector<Reward> rewards;

    // A N'loth's Hungry Face leaves the next chest empty.
    if (!m_hungryFaceSpent && player.HasRelic(RelicId::NLOTHS_HUNGRY_FACE))
    {
        m_hungryFaceSpent = true;

        return rewards;
    }

    int commonChance = 75;
    int rareChance = 0;
    GoldRange gold = { 23, 27 };
    int goldChance = 50;

    switch (size)
    {
        case ChestSize::MEDIUM:
            commonChance = 35;
            rareChance = 15;
            gold = { 45, 55 };
            goldChance = 35;
            break;

        case ChestSize::LARGE:
            commonChance = 0;
            rareChance = 25;
            gold = { 68, 82 };
            goldChance = 50;
            break;

        default:
            break;
    }

    const int roll = Roll(rng, 1, 100);
    RelicTier tier = RelicTier::UNCOMMON;

    if (roll <= commonChance)
    {
        tier = RelicTier::COMMON;
    }
    else if (roll > 100 - rareChance)
    {
        tier = RelicTier::RARE;
    }

    const RelicId id = DrawRelic(tier, player, rng);

    if (id != RelicId::INVALID)
    {
        rewards.emplace_back(Reward::RelicChoice({ id }));
    }

    // A Matryoshka has a second one tucked inside, for two chests only.
    if (!m_matryoshkaStarted && player.HasRelic(RelicId::MATRYOSHKA))
    {
        m_matryoshkaStarted = true;
        m_extraChestRelics = 2;
    }

    if (m_extraChestRelics > 0)
    {
        --m_extraChestRelics;

        const RelicTier nested =
            Chance(rng, 75) ? RelicTier::COMMON : RelicTier::UNCOMMON;
        const RelicId extra = DrawRelic(nested, player, rng);

        if (extra != RelicId::INVALID)
        {
            rewards.emplace_back(Reward::RelicChoice({ extra }));
        }
    }

    // Whether a chest holds gold as well is rolled on its own, whatever
    // relic came out of it.
    if (Chance(rng, goldChance))
    {
        rewards.emplace_back(Reward::Gold(Roll(rng, gold.low, gold.high)));
    }

    // A Cursed Key buys what is inside with a curse.
    if (player.HasRelic(RelicId::CURSED_KEY))
    {
        const std::vector<CardId>& curses =
            CardRegistry::GetPool(CardColor::CURSE);

        if (!curses.empty())
        {
            std::uniform_int_distribution<std::size_t> pick(
                0, curses.size() - 1);
            rewards.emplace_back(Reward::Curse(curses[pick(rng)]));
        }
    }

    return rewards;
}

ChestSize RewardGenerator::RollChestSize(std::mt19937& rng)
{
    const int roll = Roll(rng, 1, 100);

    if (roll <= 50)
    {
        return ChestSize::SMALL;
    }

    return roll <= 83 ? ChestSize::MEDIUM : ChestSize::LARGE;
}

int RewardGenerator::GetPotionChance() const
{
    return m_potionChance;
}

std::size_t RewardGenerator::CountRemaining(RelicTier tier) const
{
    const auto iter = m_relicPools.find(tier);

    return iter == m_relicPools.end() ? 0u : iter->second.size();
}

RelicId RewardGenerator::TakeRelic(RelicTier tier, const Player& player,
                                   std::mt19937& rng)
{
    return DrawRelic(tier, player, rng);
}

PotionId RewardGenerator::TakePotion(std::mt19937& rng) const
{
    return RollPotion(rng);
}

RelicTier RewardGenerator::RollTier(std::mt19937& rng) const
{
    return RollRelicTier(rng);
}

RelicId RewardGenerator::DrawRelic(RelicTier tier, const Player& player,
                                   std::mt19937& rng)
{
    // The tiers to fall back on once the one asked for has run dry.
    const RelicTier order[] = { tier, RelicTier::COMMON, RelicTier::UNCOMMON,
                               RelicTier::RARE, RelicTier::SHOP };

    for (const RelicTier wanted : order)
    {
        auto iter = m_relicPools.find(wanted);

        if (iter == m_relicPools.end())
        {
            continue;
        }

        std::vector<RelicId>& pool = iter->second;

        while (!pool.empty())
        {
            const RelicId id = pool.back();
            pool.pop_back();

            if (!player.HasRelic(id))
            {
                return id;
            }
        }
    }

    static_cast<void>(rng);

    return RelicId::INVALID;
}

RelicTier RewardGenerator::RollRelicTier(std::mt19937& rng) const
{
    const int roll = Roll(rng, 1, 100);

    if (roll <= 50)
    {
        return RelicTier::COMMON;
    }

    return roll <= 83 ? RelicTier::UNCOMMON : RelicTier::RARE;
}

std::vector<CardId> RewardGenerator::RollCardChoices(MonsterType type,
                                                     const Player& player,
                                                     std::mt19937& rng,
                                                     bool rareOnly)
{
    int wanted = CARD_CHOICES;

    // A Question Card widens the pick and a Busted Crown narrows it.
    if (player.HasRelic(RelicId::QUESTION_CARD))
    {
        ++wanted;
    }

    if (player.HasRelic(RelicId::BUSTED_CROWN))
    {
        wanted -= 2;
    }

    wanted = std::max(1, wanted);

    std::vector<CardId> choices;

    for (int attempt = 0; attempt < wanted * 12 &&
                          static_cast<int>(choices.size()) < wanted;
         ++attempt)
    {
        const CardId id = RollCard(type, player, rng, rareOnly);

        if (id == CardId::INVALID)
        {
            break;
        }

        // The same card never turns up twice in one pick.
        if (std::find(choices.begin(), choices.end(), id) == choices.end())
        {
            choices.emplace_back(id);
        }
    }

    return choices;
}

CardId RewardGenerator::RollCard(MonsterType type, const Player& player,
                                 std::mt19937& rng, bool rareOnly)
{
    CardRarity rarity = CardRarity::RARE;

    // A pick that is rare by the fight it came from is handed out without
    // rolling, so it leaves the rare chance where it was.
    if (!rareOnly)
    {
        CardOdds odds = OddsFor(type);

        // A gift from N'loth makes a rare card three times as likely.
        if (player.HasRelic(RelicId::NLOTHS_GIFT))
        {
            odds.rare *= 3;
        }

        const int rare = std::max(0, odds.rare - m_rareHandicap);
        const int common = std::max(0, odds.common + m_rareHandicap);
        const int roll = Roll(rng, 1, 100);

        rarity = CardRarity::UNCOMMON;

        if (roll <= common)
        {
            rarity = CardRarity::COMMON;

            // Every common rolled brings the rare chance a little closer.
            --m_rareHandicap;
        }
        else if (roll > 100 - rare)
        {
            rarity = CardRarity::RARE;
            m_rareHandicap = RARE_CARD_HANDICAP;
        }
    }

    // A Prismatic Shard opens the reward up to every other character's cards
    // and to the colourless ones.
    CardColor color = m_character;

    if (player.HasRelic(RelicId::PRISMATIC_SHARD))
    {
        std::uniform_int_distribution<std::size_t> which(
            0, sizeof(PRISM_COLORS) / sizeof(PRISM_COLORS[0]) - 1);
        color = PRISM_COLORS[which(rng)];
    }

    std::vector<CardId> pool = CardRegistry::GetPool(color, rarity);

    if (pool.empty())
    {
        pool = CardRegistry::GetPool(m_character, rarity);
    }

    if (pool.empty())
    {
        pool = CardRegistry::GetPool(color, CardRarity::COMMON);
    }

    if (pool.empty())
    {
        return CardId::INVALID;
    }

    std::uniform_int_distribution<std::size_t> pick(0, pool.size() - 1);

    return pool[pick(rng)];
}

PotionId RewardGenerator::RollPotion(std::mt19937& rng) const
{
    const int roll = Roll(rng, 1, 100);
    PotionRarity rarity = PotionRarity::COMMON;

    if (roll > 90)
    {
        rarity = PotionRarity::RARE;
    }
    else if (roll > 65)
    {
        rarity = PotionRarity::UNCOMMON;
    }

    const std::vector<PotionId> pool = PotionRegistry::GetPool(rarity);

    if (pool.empty())
    {
        return PotionId::INVALID;
    }

    std::uniform_int_distribution<std::size_t> pick(0, pool.size() - 1);

    return pool[pick(rng)];
}
}  // namespace ConquerTheSpire
