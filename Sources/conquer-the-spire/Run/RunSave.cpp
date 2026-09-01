// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Potions/PotionRegistry.hpp>
#include <conquer-the-spire/Relics/RelicRegistry.hpp>
#include <conquer-the-spire/Run/Run.hpp>

#include <sstream>

namespace ConquerTheSpire
{
namespace
{
//! What a saved run starts with, so that a file from another shape of the
//! engine is turned away rather than half read.
constexpr int SAVE_VERSION = 4;

//! Writes a name as one word, since a save is read back word by word.
std::string Packed(const std::string& name)
{
    std::string out = name;

    for (char& letter : out)
    {
        if (letter == ' ')
        {
            letter = '_';
        }
    }

    return out.empty() ? "-" : out;
}

std::string Unpacked(const std::string& name)
{
    if (name == "-")
    {
        return std::string();
    }

    std::string out = name;

    for (char& letter : out)
    {
        if (letter == '_')
        {
            letter = ' ';
        }
    }

    return out;
}

//! Reads and writes an encounter, which is a name and a list of monsters.
void WriteEncounter(std::ostream& out, const Encounter& encounter)
{
    out << Packed(encounter.name) << ' '
        << static_cast<int>(encounter.type) << ' ' << encounter.weight << ' '
        << encounter.monsters.size();

    for (const MonsterId id : encounter.monsters)
    {
        out << ' ' << static_cast<int>(id);
    }

    out << '\n';
}

Encounter ReadEncounter(std::istream& in)
{
    Encounter encounter;
    std::string name;
    int type = 0;
    std::size_t count = 0;

    in >> name >> type >> encounter.weight >> count;

    encounter.name = Unpacked(name);
    encounter.type = static_cast<MonsterType>(type);

    for (std::size_t i = 0; i < count; ++i)
    {
        int id = 0;

        in >> id;
        encounter.monsters.emplace_back(static_cast<MonsterId>(id));
    }

    return encounter;
}

void WriteCards(std::ostream& out, const std::vector<Card>& cards)
{
    out << cards.size();

    for (const auto& card : cards)
    {
        out << ' ' << static_cast<int>(card.GetId()) << ' '
            << card.GetUpgradeCount();
    }

    out << '\n';
}

std::vector<Card> ReadCards(std::istream& in)
{
    std::vector<Card> cards;
    std::size_t count = 0;

    in >> count;

    for (std::size_t i = 0; i < count; ++i)
    {
        int id = 0;
        int upgrades = 0;

        in >> id >> upgrades;
        cards.emplace_back(
            CardRegistry::Get(static_cast<CardId>(id), upgrades));
    }

    return cards;
}

void WriteIds(std::ostream& out, const std::vector<CardId>& ids)
{
    out << ids.size();

    for (const CardId id : ids)
    {
        out << ' ' << static_cast<int>(id);
    }

    out << '\n';
}
}  // namespace

std::string Run::Serialize() const
{
    std::ostringstream out;

    out << "cts " << SAVE_VERSION << '\n';

    // The state of the dice, which is what makes a saved run pick up exactly
    // where it left off.
    out << m_rng << '\n';

    out << static_cast<int>(m_character) << ' ' << m_act << ' ' << m_floor
        << ' ' << m_column << ' ' << m_gold << ' ' << (m_bossDone ? 1 : 0)
        << ' ' << m_fights << ' ' << m_removalPrice << ' '
        << (m_mawBankOpen ? 1 : 0) << ' ' << m_pathSkips << ' '
        << m_curseWards << ' ' << m_lifts << ' ' << m_questionRooms << ' '
        << (m_pendingFight ? 1 : 0) << ' '
        << static_cast<int>(m_pendingPrize) << ' ' << (m_eventEnds ? 1 : 0)
        << ' ' << m_monsterChance << ' ' << m_shopChance << ' '
        << m_treasureChance << '\n';

    // The map, row by row, with what each node holds and where it leads.
    out << m_map.GetRows() << '\n';

    for (int row = 0; row < Map::ROWS; ++row)
    {
        for (int column = 0; column < Map::COLUMNS; ++column)
        {
            const MapNode& node = m_map.GetNode(row, column);

            out << static_cast<int>(node.type) << ' '
                << (node.exists ? 1 : 0) << ' ' << node.nextColumns.size();

            for (const int next : node.nextColumns)
            {
                out << ' ' << next;
            }

            out << '\n';
        }
    }

    // The climber.
    out << Packed(m_player.GetName()) << ' ' << m_player.GetHealth() << ' '
        << m_player.GetMaxHealth() << ' ' << m_player.GetMaxEnergy() << ' '
        << m_player.GetCardsPerTurn() << ' '
        << static_cast<int>(m_player.GetColor()) << ' '
        << m_player.GetLiftedStrength() << ' ' << m_player.GetBonusEnergy()
        << '\n';

    WriteCards(out, m_player.GetDeck());

    out << m_player.GetRelics().size();

    for (const auto& relic : m_player.GetRelics())
    {
        out << ' ' << static_cast<int>(relic.GetId()) << ' '
            << relic.GetCounter() << ' ' << (relic.IsUsed() ? 1 : 0);
    }

    out << '\n' << m_player.GetPotions().size();

    for (const auto& potion : m_player.GetPotions())
    {
        out << ' ' << static_cast<int>(potion.GetId());
    }

    out << '\n';
    WriteIds(out, m_player.GetBottledCards());

    // The books the rewards are rolled from.
    out << m_rewardGenerator.Serialize() << '\n';

    // What is waiting to be claimed.
    out << m_rewards.size() << '\n';

    for (const auto& reward : m_rewards)
    {
        out << static_cast<int>(reward.kind) << ' ' << reward.amount << ' '
            << static_cast<int>(reward.potion) << ' '
            << (reward.claimed ? 1 : 0) << ' '
            << (reward.fromChest ? 1 : 0) << ' ' << reward.cards.size();

        for (const CardId id : reward.cards)
        {
            out << ' ' << static_cast<int>(id);
        }

        out << ' ' << reward.relics.size();

        for (const RelicId id : reward.relics)
        {
            out << ' ' << static_cast<int>(id);
        }

        out << '\n';
    }

    // The shelf, whatever is left of it.
    out << m_shop.Serialize() << '\n';

    WriteEncounter(out, m_encounter);
    WriteEncounter(out, m_pendingEncounter);

    // The fights just had. A climb picked up without them draws its next
    // room from the wrong pool, because the bar on repeating one is gone.
    out << m_lately.size();

    for (const std::string& had : m_lately)
    {
        out << ' ' << Packed(had);
    }

    out << '\n';

    // The room the climber is standing in.
    out << static_cast<int>(m_event.GetId()) << ' ' << m_event.GetStage()
        << ' ' << (m_event.IsDone() ? 1 : 0) << ' ' << m_event.GetTries()
        << ' ' << m_event.GetBag().size();

    for (const int found : m_event.GetBag())
    {
        out << ' ' << found;
    }

    out << '\n' << m_seenEvents.size();

    for (const EventId id : m_seenEvents)
    {
        out << ' ' << static_cast<int>(id);
    }

    out << '\n' << m_keys.size();

    for (const KeyType key : m_keys)
    {
        out << ' ' << static_cast<int>(key);
    }

    out << '\n';

    return out.str();
}

bool Run::Load(const std::string& text)
{
    std::istringstream in(text);
    std::string tag;
    int version = 0;

    in >> tag >> version;

    if (!in || tag != "cts" || version != SAVE_VERSION)
    {
        return false;
    }

    // The log is not written into a save, so whatever is in it belongs to
    // whichever climb this object was playing before - a different climb.
    // Without this the floors and the fights of the one being dropped carry
    // over onto the one being picked up, and every table reading the summary
    // reads both climbs added together.
    m_log.Clear();

    in >> m_rng;

    int character = 0;
    int bossDone = 0;
    int mawBank = 0;
    int pendingFight = 0;
    int pendingPrize = 0;
    int eventEnds = 0;

    in >> character >> m_act >> m_floor >> m_column >> m_gold >> bossDone >>
        m_fights >> m_removalPrice >> mawBank >> m_pathSkips >>
        m_curseWards >> m_lifts >> m_questionRooms >> pendingFight >>
        pendingPrize >> eventEnds >> m_monsterChance >> m_shopChance >>
        m_treasureChance;

    m_character = static_cast<CardColor>(character);
    m_bossDone = bossDone != 0;
    m_mawBankOpen = mawBank != 0;
    m_pendingFight = pendingFight != 0;
    m_pendingPrize = static_cast<RelicId>(pendingPrize);
    m_eventEnds = eventEnds != 0;

    int rows = Map::ROWS;

    in >> rows;

    m_map = Map();
    m_map.SetRows(rows);

    for (int row = 0; row < Map::ROWS; ++row)
    {
        for (int column = 0; column < Map::COLUMNS; ++column)
        {
            int type = 0;
            int exists = 0;
            std::size_t count = 0;

            in >> type >> exists >> count;

            MapNode& node = m_map.GetNode(row, column);

            node.type = static_cast<MapNodeType>(type);
            node.exists = exists != 0;
            node.row = row;
            node.column = column;
            node.nextColumns.clear();

            for (std::size_t i = 0; i < count; ++i)
            {
                int next = 0;

                in >> next;
                node.nextColumns.emplace_back(next);
            }
        }
    }

    std::string name;
    int health = 0;
    int maxHealth = 0;
    int maxEnergy = 3;
    int cardsPerTurn = 5;
    int color = 0;
    int lifted = 0;
    int bonusEnergy = 0;

    in >> name >> health >> maxHealth >> maxEnergy >> cardsPerTurn >> color >>
        lifted >> bonusEnergy;

    m_player = Player(Unpacked(name), maxHealth, maxEnergy, cardsPerTurn);
    m_player.SetHealth(health);
    m_player.SetColor(static_cast<CardColor>(color));
    m_player.Lift(lifted);
    m_player.SetBonusEnergy(bonusEnergy);

    for (auto& card : ReadCards(in))
    {
        m_player.GetDeck().emplace_back(std::move(card));
    }

    std::size_t relics = 0;

    in >> relics;

    for (std::size_t i = 0; i < relics; ++i)
    {
        int id = 0;
        int counter = 0;
        int used = 0;

        in >> id >> counter >> used;

        Relic relic = RelicRegistry::Get(static_cast<RelicId>(id));

        for (int step = 0; step < counter; ++step)
        {
            relic.CountUp(counter + 1);
        }

        if (used != 0)
        {
            relic.MarkUsed();
        }

        m_player.GetRelics().emplace_back(std::move(relic));
    }

    std::size_t potions = 0;

    in >> potions;

    for (std::size_t i = 0; i < potions; ++i)
    {
        int id = 0;

        in >> id;
        m_player.GetPotions().emplace_back(
            PotionRegistry::Get(static_cast<PotionId>(id)));
    }

    std::size_t bottled = 0;

    in >> bottled;

    for (std::size_t i = 0; i < bottled; ++i)
    {
        int id = 0;

        in >> id;
        m_player.BottleCard(static_cast<CardId>(id));
    }

    if (!m_rewardGenerator.Load(in))
    {
        return false;
    }

    std::size_t rewards = 0;

    in >> rewards;
    m_rewards.clear();

    for (std::size_t i = 0; i < rewards; ++i)
    {
        Reward reward;
        int kind = 0;
        int potion = 0;
        int claimed = 0;
        int fromChest = 0;
        std::size_t cards = 0;

        in >> kind >> reward.amount >> potion >> claimed >> fromChest >>
            cards;

        reward.kind = static_cast<RewardKind>(kind);
        reward.potion = static_cast<PotionId>(potion);
        reward.claimed = claimed != 0;
        reward.fromChest = fromChest != 0;

        for (std::size_t card = 0; card < cards; ++card)
        {
            int id = 0;

            in >> id;
            reward.cards.emplace_back(static_cast<CardId>(id));
        }

        std::size_t held = 0;

        in >> held;

        for (std::size_t relic = 0; relic < held; ++relic)
        {
            int id = 0;

            in >> id;
            reward.relics.emplace_back(static_cast<RelicId>(id));
        }

        m_rewards.emplace_back(std::move(reward));
    }

    if (!m_shop.Load(in))
    {
        return false;
    }

    m_encounter = ReadEncounter(in);
    m_pendingEncounter = ReadEncounter(in);

    std::size_t lately = 0;

    in >> lately;
    m_lately.clear();

    for (std::size_t at = 0; at < lately && at < 2u; ++at)
    {
        std::string had;

        in >> had;
        m_lately.emplace_back(Unpacked(had));
    }

    int eventId = 0;
    int stage = 0;
    int done = 0;
    int tries = 0;
    std::size_t bag = 0;

    in >> eventId >> stage >> done >> tries >> bag;

    m_event = EventLibrary::Get(static_cast<EventId>(eventId));
    m_event.GetBag().clear();

    for (std::size_t i = 0; i < bag; ++i)
    {
        int found = 0;

        in >> found;
        m_event.GetBag().emplace_back(found);
    }

    // A room that was closed stays closed; one that was open is left where
    // it stood, and a stage of zero is where a room opens anyway.
    if (done != 0)
    {
        m_event.GoTo(-1);
    }
    else if (stage > 0)
    {
        m_event.GoTo(stage);
    }

    for (int i = 0; i < tries; ++i)
    {
        m_event.CountTry();
    }

    std::size_t seen = 0;

    in >> seen;
    m_seenEvents.clear();

    for (std::size_t i = 0; i < seen; ++i)
    {
        int id = 0;

        in >> id;
        m_seenEvents.emplace_back(static_cast<EventId>(id));
    }

    std::size_t keys = 0;

    in >> keys;
    m_keys.clear();

    for (std::size_t i = 0; i < keys; ++i)
    {
        int key = 0;

        in >> key;
        m_keys.emplace_back(static_cast<KeyType>(key));
    }

    return static_cast<bool>(in);
}
}  // namespace ConquerTheSpire
