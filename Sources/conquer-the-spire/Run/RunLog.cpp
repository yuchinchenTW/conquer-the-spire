// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Run/RunLog.hpp>

#include <algorithm>
#include <cstddef>

namespace ConquerTheSpire
{
const char* NameOf(LogEntry entry)
{
    // In the order of the enum, and one for every value of it: a test counts
    // them.
    static const char* names[] = {
        "invalid",       "card_taken",     "card_passed",  "relic_passed",
        "potion_passed", "card_removed",   "card_upgraded",
        "card_transformed",
        "relic_taken",   "relic_lost",     "potion_taken", "potion_drunk",
        "potion_thrown", "gold_earned",    "gold_spent",   "fight_won",
        "floor_walked",  "act_started",    "room_entered", "room_answered",
        "rested",        "died",           "spire_done",   "room_passed",
        "path_passed",   "curse_chosen",   "curse_refused"};

    static_assert(sizeof(names) / sizeof(names[0]) ==
                      static_cast<std::size_t>(LogEntry::COUNT),
                  "a kind of line was added without a name");

    const auto at = static_cast<std::size_t>(entry);

    return at < static_cast<std::size_t>(LogEntry::COUNT) ? names[at]
                                                          : names[0];
}

void RunLog::Add(LogEntry entry, LogSource source, int id, int extra, int act,
                 int floor, int stage)
{
    LogLine line;

    line.entry = entry;
    line.source = source;
    line.id = id;
    line.extra = extra;
    line.act = act;
    line.floor = floor;
    line.stage = stage;

    m_lines.emplace_back(line);

    m_summary.act = act;
    m_summary.deepestAct = std::max(m_summary.deepestAct, act);

    switch (entry)
    {
        case LogEntry::CARD_TAKEN:
            ++m_summary.cardsTaken;

            if (source == LogSource::SHOP)
            {
                ++m_summary.cardsBought;
            }

            break;

        case LogEntry::CARD_PASSED:
            ++m_summary.cardsPassed;
            break;

        case LogEntry::CARD_REMOVED:
            ++m_summary.cardsRemoved;
            break;

        case LogEntry::CARD_UPGRADED:
            ++m_summary.cardsUpgraded;
            break;

        case LogEntry::CARD_TRANSFORMED:
            ++m_summary.cardsTransformed;
            break;

        case LogEntry::RELIC_TAKEN:
            ++m_summary.relicsTaken;

            if (source == LogSource::SHOP)
            {
                ++m_summary.relicsBought;
            }

            break;

        case LogEntry::POTION_TAKEN:
            ++m_summary.potionsTaken;

            if (source == LogSource::SHOP)
            {
                ++m_summary.potionsBought;
            }

            break;

        case LogEntry::POTION_DRUNK:
            ++m_summary.potionsDrunk;
            break;

        case LogEntry::GOLD_EARNED:
            m_summary.goldEarned += extra;
            break;

        case LogEntry::GOLD_SPENT:
            m_summary.goldSpent += extra;
            break;

        case LogEntry::FIGHT_WON:
            ++m_summary.fightsWon;

            // The kind of fight is the id of the line.
            if (id == static_cast<int>(MonsterType::ELITE))
            {
                ++m_summary.elitesWon;
            }
            else if (id == static_cast<int>(MonsterType::BOSS))
            {
                ++m_summary.bossesWon;
            }

            break;

        case LogEntry::FLOOR_WALKED:
            ++m_summary.floors;
            break;

        case LogEntry::ROOM_ENTERED:
            ++m_summary.roomsEntered;
            break;

        case LogEntry::RESTED:
            ++m_summary.rested;
            break;

        case LogEntry::DIED:
            m_summary.died = 1;
            break;

        case LogEntry::SPIRE_DONE:
            m_summary.wonTheSpire = 1;
            break;

        case LogEntry::CURSE_CHOSEN:
            ++m_summary.cursesChosen;
            break;

        case LogEntry::CURSE_REFUSED:
            ++m_summary.cursesRefused;
            break;

        default:
            break;
    }
}

void RunLog::Clear()
{
    m_lines.clear();
    m_summary = Summary();
}

const std::vector<LogLine>& RunLog::GetLines() const
{
    return m_lines;
}

const RunLog::Summary& RunLog::GetSummary() const
{
    return m_summary;
}

void RunLog::ReadSummary(int* out) const
{
    if (out == nullptr)
    {
        return;
    }

    out[0] = m_summary.floors;
    out[1] = m_summary.act;
    out[2] = m_summary.deepestAct;
    out[3] = m_summary.fightsWon;
    out[4] = m_summary.elitesWon;
    out[5] = m_summary.bossesWon;
    out[6] = m_summary.cardsTaken;
    out[7] = m_summary.cardsPassed;
    out[8] = m_summary.cardsBought;
    out[9] = m_summary.cardsRemoved;
    out[10] = m_summary.cardsUpgraded;
    out[11] = m_summary.cardsTransformed;
    out[12] = m_summary.relicsTaken;
    out[13] = m_summary.relicsBought;
    out[14] = m_summary.potionsTaken;
    out[15] = m_summary.potionsBought;
    out[16] = m_summary.potionsDrunk;
    out[17] = m_summary.goldEarned;
    out[18] = m_summary.goldSpent;
    out[19] = m_summary.roomsEntered;
    out[20] = m_summary.rested;
    out[21] = m_summary.died;
    out[22] = m_summary.wonTheSpire;
    out[23] = m_summary.cursesChosen;
    out[24] = m_summary.cursesRefused;
}
}  // namespace ConquerTheSpire
