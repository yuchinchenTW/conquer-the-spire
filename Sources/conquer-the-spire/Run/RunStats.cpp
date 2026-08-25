// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Run/RunStats.hpp>

#include <algorithm>

namespace ConquerTheSpire
{
namespace
{
//! One key a kind and an id, so that the rows sort by kind and then by what
//! they are about.
long long KeyOf(StatKind kind, int id)
{
    return static_cast<long long>(kind) * 100000LL +
           static_cast<long long>(id);
}
}  // namespace

void RunStats::Note(StatKind kind, int id, bool won, bool died, int floors,
                    std::vector<long long>& seen)
{
    const long long key = KeyOf(kind, id);
    Row& row = m_rows[key];

    row.kind = static_cast<int>(kind);
    row.id = id;
    ++row.picks;

    // The rest is counted once a climb, however often it was chosen in it.
    if (std::find(seen.begin(), seen.end(), key) != seen.end())
    {
        return;
    }

    seen.emplace_back(key);
    ++row.runs;
    row.wins += won ? 1 : 0;
    row.deaths += died ? 1 : 0;
    row.floors += floors;
}

void RunStats::NotePassed(StatKind kind, int id)
{
    Row& row = m_rows[KeyOf(kind, id)];

    row.kind = static_cast<int>(kind);
    row.id = id;
    ++row.passes;
}

void RunStats::Ingest(const RunLog& log)
{
    const RunLog::Summary& counts = log.GetSummary();
    const bool won = counts.wonTheSpire != 0;
    const bool died = counts.died != 0;

    ++m_runs;
    m_wins += won ? 1 : 0;
    m_deaths += died ? 1 : 0;
    m_floors += counts.floors;

    std::vector<long long> seen;

    // The fight being stood in, until it is won or the climb ends in it. A
    // climb dies at most once, so at most one fight goes down as lost; one
    // that was walked out of is simply replaced by the next and counted
    // neither way.
    int inFight = 0;
    StatKind fightKind = StatKind::FIGHT_FOUGHT;

    for (const auto& line : log.GetLines())
    {
        const bool bought = line.source == LogSource::SHOP;

        switch (line.entry)
        {
            case LogEntry::CARD_TAKEN:
                Note(StatKind::CARD_TAKEN, line.id, won, died, counts.floors,
                     seen);

                if (bought)
                {
                    Note(StatKind::CARD_BOUGHT, line.id, won, died,
                         counts.floors, seen);
                }

                break;

            case LogEntry::CARD_PASSED:
                NotePassed(StatKind::CARD_TAKEN, line.id);

                // Something left on a shelf was also something not bought.
                if (bought)
                {
                    NotePassed(StatKind::CARD_BOUGHT, line.id);
                }

                break;

            case LogEntry::RELIC_PASSED:
                NotePassed(StatKind::RELIC_TAKEN, line.id);

                if (bought)
                {
                    NotePassed(StatKind::RELIC_BOUGHT, line.id);
                }

                break;

            case LogEntry::POTION_PASSED:
                NotePassed(StatKind::POTION_TAKEN, line.id);

                if (bought)
                {
                    NotePassed(StatKind::POTION_BOUGHT, line.id);
                }

                break;

            case LogEntry::CARD_REMOVED:
                Note(StatKind::CARD_REMOVED, line.id, won, died,
                     counts.floors, seen);
                break;

            case LogEntry::CARD_UPGRADED:
                Note(StatKind::CARD_UPGRADED, line.id, won, died,
                     counts.floors, seen);
                break;

            case LogEntry::CARD_TRANSFORMED:
                Note(StatKind::CARD_TRANSFORMED, line.id, won, died,
                     counts.floors, seen);
                break;

            case LogEntry::RELIC_TAKEN:
                Note(StatKind::RELIC_TAKEN, line.id, won, died,
                     counts.floors, seen);

                if (bought)
                {
                    Note(StatKind::RELIC_BOUGHT, line.id, won, died,
                         counts.floors, seen);
                }

                break;

            case LogEntry::POTION_TAKEN:
                Note(StatKind::POTION_TAKEN, line.id, won, died,
                     counts.floors, seen);

                if (bought)
                {
                    Note(StatKind::POTION_BOUGHT, line.id, won, died,
                         counts.floors, seen);
                }

                break;

            case LogEntry::POTION_DRUNK:
                Note(StatKind::POTION_DRUNK, line.id, won, died,
                     counts.floors, seen);
                break;

            case LogEntry::ROOM_ENTERED:
                Note(StatKind::ROOM_ENTERED, line.id, won, died,
                     counts.floors, seen);
                break;

            case LogEntry::ROOM_ANSWERED:
                // The room and the option it was answered with, together.
                Note(StatKind::ROOM_ANSWERED, line.id * 100 + line.extra, won,
                     died, counts.floors, seen);
                break;

            case LogEntry::FLOOR_WALKED:
                Note(StatKind::NODE_WALKED, line.id, won, died,
                     counts.floors, seen);
                break;

            case LogEntry::PATH_PASSED:
                NotePassed(StatKind::NODE_WALKED, line.id);
                break;

            case LogEntry::CARD_NOT_REMOVED:
                NotePassed(StatKind::CARD_REMOVED, line.id);
                break;

            case LogEntry::CARD_NOT_UPGRADED:
                NotePassed(StatKind::CARD_UPGRADED, line.id);
                break;

            case LogEntry::ROOM_PASSED:
                NotePassed(StatKind::ROOM_ANSWERED,
                           line.id * 100 + line.extra);
                break;

            case LogEntry::CURSE_CHOSEN:
                Note(StatKind::CURSE_OPTION, line.id, won, died,
                     counts.floors, seen);
                break;

            case LogEntry::CURSE_REFUSED:
                NotePassed(StatKind::CURSE_OPTION, line.id);
                break;

            case LogEntry::FIGHT_STARTED:
                inFight = line.id;
                fightKind =
                    line.extra == static_cast<int>(MonsterType::BOSS)
                        ? StatKind::BOSS_FOUGHT
                        : (line.extra == static_cast<int>(MonsterType::ELITE)
                               ? StatKind::ELITE_FOUGHT
                               : StatKind::FIGHT_FOUGHT);
                break;

            case LogEntry::CARD_PLAYED:
                // Played is a pick, and carried through a fight without
                // being played is a pass, so the rate is how much use the
                // card turned out to be rather than what it says it does.
                if (line.extra > 0)
                {
                    for (int again = 0; again < line.extra; ++again)
                    {
                        Note(StatKind::CARD_PLAYED, line.id, won, died,
                             counts.floors, seen);
                    }
                }
                else
                {
                    NotePassed(StatKind::CARD_PLAYED, line.id);
                }

                break;

            case LogEntry::FIGHT_WON:
                if (inFight != 0)
                {
                    Note(fightKind, inFight, won, died, counts.floors, seen);
                    inFight = 0;
                }

                break;

            default:
                break;
        }
    }

    // A fight still being stood in when the log runs out, on a climb that
    // died, is the fight that killed it.
    if (died && inFight != 0)
    {
        NotePassed(fightKind, inFight);
    }
}

void RunStats::Clear()
{
    m_rows.clear();
    m_runs = 0;
    m_wins = 0;
    m_deaths = 0;
    m_floors = 0;
}

std::size_t RunStats::GetRowCount() const
{
    return m_rows.size();
}

void RunStats::ReadRows(int* out) const
{
    if (out == nullptr)
    {
        return;
    }

    std::size_t at = 0;

    for (const auto& row : m_rows)
    {
        out[at + 0] = row.second.kind;
        out[at + 1] = row.second.id;
        out[at + 2] = row.second.picks;
        out[at + 3] = row.second.passes;
        out[at + 4] = row.second.runs;
        out[at + 5] = row.second.wins;
        out[at + 6] = row.second.deaths;
        out[at + 7] = row.second.floors;
        at += Row::SLOTS;
    }
}

const std::map<long long, RunStats::Row>& RunStats::GetRows() const
{
    return m_rows;
}

int RunStats::GetRuns() const
{
    return m_runs;
}

int RunStats::GetWins() const
{
    return m_wins;
}

int RunStats::GetDeaths() const
{
    return m_deaths;
}

int RunStats::GetFloors() const
{
    return m_floors;
}

void RunStats::ReadTotals(int* out) const
{
    if (out != nullptr)
    {
        out[0] = m_runs;
        out[1] = m_wins;
        out[2] = m_deaths;
        out[3] = m_floors;
    }
}
}  // namespace ConquerTheSpire
