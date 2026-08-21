#include "doctest.h"

#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Rl/VecSpireEnv.hpp>
#include <conquer-the-spire/Run/Run.hpp>
#include <conquer-the-spire/Run/RunStats.hpp>

#include <algorithm>
#include <cstddef>
#include <random>
#include <string>
#include <vector>

using namespace ConquerTheSpire;

namespace
{
//! Counts the lines of \p entry in the log.
int Count(const Run& run, LogEntry entry)
{
    int count = 0;

    for (const auto& line : run.GetLog().GetLines())
    {
        if (line.entry == entry)
        {
            ++count;
        }
    }

    return count;
}

//! Returns the last line of \p entry, or an empty one.
LogLine Last(const Run& run, LogEntry entry)
{
    LogLine found;

    for (const auto& line : run.GetLog().GetLines())
    {
        if (line.entry == entry)
        {
            found = line;
        }
    }

    return found;
}
}  // namespace

TEST_CASE("A climb writes down the cards it takes and tears up")
{
    Run run(CardColor::RED, 5);

    // The starting deck is not a choice, so it is not written down.
    CHECK(Count(run, LogEntry::CARD_TAKEN) == 0);

    run.AddCardToDeck(CardRegistry::Get(CardId::CLEAVE));

    CHECK(Count(run, LogEntry::CARD_TAKEN) == 1);
    CHECK(Last(run, LogEntry::CARD_TAKEN).id ==
          static_cast<int>(CardId::CLEAVE));

    REQUIRE(run.RemoveCardFromDeck(0) == true);

    CHECK(Count(run, LogEntry::CARD_REMOVED) == 1);

    REQUIRE(run.Smith(0) == true);

    CHECK(Count(run, LogEntry::CARD_UPGRADED) == 1);

    const RunLog::Summary& counts = run.GetLog().GetSummary();

    CHECK(counts.cardsTaken == 1);
    CHECK(counts.cardsRemoved == 1);
    CHECK(counts.cardsUpgraded == 1);
}

TEST_CASE("A climb writes down what it bought and where from")
{
    Run run(CardColor::RED, 7);

    run.AddGold(2000);
    run.OpenShop();

    REQUIRE(run.BuyCard(0) == true);

    const LogLine card = Last(run, LogEntry::CARD_TAKEN);

    CHECK(card.source == LogSource::SHOP);
    CHECK(run.GetLog().GetSummary().cardsBought == 1);

    REQUIRE(run.BuyRelic(2) == true);

    const LogLine relic = Last(run, LogEntry::RELIC_TAKEN);

    CHECK(relic.source == LogSource::SHOP);
    CHECK(run.GetLog().GetSummary().relicsBought == 1);
    CHECK(run.GetLog().GetSummary().goldSpent > 0);
}

TEST_CASE("A chest is told apart from a pile a fight left")
{
    Run run(CardColor::RED, 9);

    run.OpenChest();

    bool took = false;

    for (std::size_t i = 0; i < run.GetRewards().size(); ++i)
    {
        if (run.GetRewards()[i].kind == RewardKind::RELIC_CHOICE &&
            run.ClaimReward(i))
        {
            took = true;
            break;
        }
    }

    if (took)
    {
        CHECK(Last(run, LogEntry::RELIC_TAKEN).source == LogSource::CHEST);
    }
}

TEST_CASE("A climb writes down the rooms it walks into and how it answers")
{
    Run run(CardColor::RED, 5);

    run.StartEvent(EventId::THE_CLERIC);

    CHECK(Count(run, LogEntry::ROOM_ENTERED) == 1);
    CHECK(Last(run, LogEntry::ROOM_ENTERED).id ==
          static_cast<int>(EventId::THE_CLERIC));

    REQUIRE(run.ChooseEventOption(0) == true);

    const LogLine answered = Last(run, LogEntry::ROOM_ANSWERED);

    CHECK(answered.id == static_cast<int>(EventId::THE_CLERIC));
    CHECK(answered.extra == 0);
    CHECK(answered.stage == 0);

    // A room with a second stage says which one the answer was to.
    Run idol(CardColor::RED, 5);

    idol.StartEvent(EventId::GOLDEN_IDOL);

    REQUIRE(idol.ChooseEventOption(0) == true);
    REQUIRE(idol.ChooseEventOption(1) == true);

    CHECK(Last(idol, LogEntry::ROOM_ANSWERED).stage == 1);
}

TEST_CASE("A climb writes down the fights it wins and the floors it walks")
{
    Run run(CardColor::RED, 8);

    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);

    CHECK(Count(run, LogEntry::FLOOR_WALKED) == 1);
    CHECK(Last(run, LogEntry::FLOOR_WALKED).id ==
          static_cast<int>(MapNodeType::MONSTER));

    Battle battle = run.StartBattleHere();

    for (auto& monster : battle.GetMonsters())
    {
        monster.SetHealth(0);
    }

    battle.EndTurn();
    run.FinishBattle(battle);

    CHECK(Count(run, LogEntry::FIGHT_WON) == 1);
    CHECK(run.GetLog().GetSummary().fightsWon == 1);
    CHECK(run.GetLog().GetSummary().floors == 1);
}

TEST_CASE("A climb that ends says so")
{
    Run run(CardColor::RED, 8);

    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);

    Battle battle = run.StartBattleHere();

    battle.GetPlayer().SetHealth(0);
    run.FinishBattle(battle);

    CHECK(Count(run, LogEntry::DIED) == 1);
    CHECK(run.GetLog().GetSummary().died == 1);
    CHECK(run.GetLog().GetSummary().wonTheSpire == 0);
}

TEST_CASE("The counts read out as a row of numbers, in the order they are in")
{
    Run run(CardColor::RED, 7);

    run.AddGold(500);
    run.AddCardToDeck(CardRegistry::Get(CardId::CLEAVE));
    run.AddRelic(RelicId::AKABEKO);
    run.AddPotion(PotionId::FIRE_POTION);

    std::vector<int> row(RunLog::Summary::SLOTS, -1);

    run.GetLog().ReadSummary(row.data());

    const RunLog::Summary& counts = run.GetLog().GetSummary();

    CHECK(row[0] == counts.floors);
    CHECK(row[1] == counts.act);
    CHECK(row[6] == counts.cardsTaken);
    CHECK(row[7] == counts.cardsPassed);
    CHECK(row[12] == counts.relicsTaken);
    CHECK(row[14] == counts.potionsTaken);
    CHECK(row[17] == counts.goldEarned);
    CHECK(row[21] == counts.died);
    CHECK(row[22] == counts.wonTheSpire);

    // Nothing was left unwritten.
    for (const int value : row)
    {
        CHECK(value >= 0);
    }
}

TEST_CASE("A row of climbs keeps the counts of the one that just ended")
{
    VecSpireEnv row(2);

    row.Reset(CardColor::RED, 8);

    std::mt19937 rng(3);
    std::vector<unsigned char> mask(2u * SpireEnv::ActionCount(), 0u);
    std::vector<std::size_t> actions(2u, 0u);
    std::vector<unsigned char> dones(2u, 0u);
    std::vector<int> last(2u * RunLog::Summary::SLOTS, 0);

    bool sawOne = false;

    for (int tick = 0; tick < 3000 && !sawOne; ++tick)
    {
        row.ActionMask(mask.data());

        for (std::size_t i = 0; i < 2u; ++i)
        {
            std::vector<std::size_t> open;
            const std::size_t stride = SpireEnv::ActionCount();

            for (std::size_t slot = 0; slot < stride; ++slot)
            {
                if (mask[i * stride + slot] != 0u)
                {
                    open.emplace_back(slot);
                }
            }

            REQUIRE(open.empty() == false);

            std::uniform_int_distribution<std::size_t> pick(
                0, open.size() - 1);

            actions[i] = open[pick(rng)];
        }

        row.Step(actions.data(), nullptr, dones.data(), nullptr, nullptr,
                 nullptr);

        for (std::size_t i = 0; i < 2u && !sawOne; ++i)
        {
            if (dones[i] == 0u)
            {
                continue;
            }

            row.ReadLastSummaries(last.data());

            const int* counts = last.data() + i * RunLog::Summary::SLOTS;

            // The climb that just ended walked somewhere and then stopped.
            CHECK(counts[0] > 0);
            CHECK((counts[20] == 1 || counts[21] == 1));

            // And the one standing in its place has a clean sheet.
            std::vector<int> now(2u * RunLog::Summary::SLOTS, 0);

            row.ReadSummaries(now.data());

            CHECK(now[i * RunLog::Summary::SLOTS] == 0);

            sawOne = true;
        }
    }

    CHECK(sawOne == true);
}

TEST_CASE("The table counts a choice once a climb and every time it is made")
{
    RunStats stats;

    // Two climbs: one that took Cleave twice and died, one that took it once
    // and came out the top.
    Run first(CardColor::RED, 5);

    first.AddCardToDeck(CardRegistry::Get(CardId::CLEAVE));
    first.AddCardToDeck(CardRegistry::Get(CardId::CLEAVE));
    first.Note(LogEntry::DIED, 1);
    stats.Ingest(first.GetLog());

    Run second(CardColor::RED, 6);

    second.AddCardToDeck(CardRegistry::Get(CardId::CLEAVE));
    second.Note(LogEntry::SPIRE_DONE, 4);
    stats.Ingest(second.GetLog());

    CHECK(stats.GetRuns() == 2);
    CHECK(stats.GetWins() == 1);
    CHECK(stats.GetDeaths() == 1);

    bool found = false;

    for (const auto& row : stats.GetRows())
    {
        if (row.second.kind != static_cast<int>(StatKind::CARD_TAKEN) ||
            row.second.id != static_cast<int>(CardId::CLEAVE))
        {
            continue;
        }

        found = true;

        // Three picks over two climbs, one of which was won.
        CHECK(row.second.picks == 3);
        CHECK(row.second.runs == 2);
        CHECK(row.second.wins == 1);
        CHECK(row.second.deaths == 1);
    }

    CHECK(found == true);
}

TEST_CASE("A card bought is counted as taken and as bought")
{
    RunStats stats;
    Run run(CardColor::RED, 7);

    run.AddGold(2000);
    run.OpenShop();

    const CardId bought = run.GetShop().GetCards().front().id;

    REQUIRE(run.BuyCard(0) == true);

    stats.Ingest(run.GetLog());

    int taken = 0;
    int paid = 0;

    for (const auto& row : stats.GetRows())
    {
        if (row.second.id != static_cast<int>(bought))
        {
            continue;
        }

        if (row.second.kind == static_cast<int>(StatKind::CARD_TAKEN))
        {
            taken = row.second.picks;
        }
        else if (row.second.kind == static_cast<int>(StatKind::CARD_BOUGHT))
        {
            paid = row.second.picks;
        }
    }

    CHECK(taken == 1);
    CHECK(paid == 1);
}

TEST_CASE("The table reads out as rows of whole numbers")
{
    RunStats stats;
    Run run(CardColor::RED, 5);

    run.AddRelic(RelicId::AKABEKO);
    run.Note(LogEntry::DIED, 1);
    stats.Ingest(run.GetLog());

    const std::size_t rows = stats.GetRowCount();

    REQUIRE(rows > 0u);

    std::vector<int> flat(rows * RunStats::Row::SLOTS, -1);

    stats.ReadRows(flat.data());

    for (const int value : flat)
    {
        CHECK(value >= 0);
    }

    std::vector<int> totals(RunStats::TOTAL_SLOTS, -1);

    stats.ReadTotals(totals.data());

    CHECK(totals[0] == 1);
    CHECK(totals[1] == 0);
    CHECK(totals[2] == 1);
}

TEST_CASE("A row of climbs counts every one that finishes")
{
    VecSpireEnv row(4);

    row.Reset(CardColor::RED, 30);

    std::mt19937 rng(6);
    std::vector<unsigned char> mask(4u * SpireEnv::ActionCount(), 0u);
    std::vector<std::size_t> actions(4u, 0u);
    std::vector<unsigned char> dones(4u, 0u);

    int ended = 0;

    for (int tick = 0; tick < 4000 && ended < 8; ++tick)
    {
        row.ActionMask(mask.data());

        for (std::size_t i = 0; i < 4u; ++i)
        {
            std::vector<std::size_t> open;
            const std::size_t stride = SpireEnv::ActionCount();

            for (std::size_t slot = 0; slot < stride; ++slot)
            {
                if (mask[i * stride + slot] != 0u)
                {
                    open.emplace_back(slot);
                }
            }

            REQUIRE(open.empty() == false);

            std::uniform_int_distribution<std::size_t> pick(
                0, open.size() - 1);

            actions[i] = open[pick(rng)];
        }

        row.Step(actions.data(), nullptr, dones.data(), nullptr, nullptr,
                 nullptr);

        for (std::size_t i = 0; i < 4u; ++i)
        {
            ended += dones[i] != 0u ? 1 : 0;
        }
    }

    CHECK(row.GetStats().GetRuns() == ended);
    CHECK(row.GetStats().GetFloors() > 0);
    CHECK(row.GetStats().GetRowCount() > 0u);

    row.ClearStats();

    CHECK(row.GetStats().GetRuns() == 0);
    CHECK(row.GetStats().GetRowCount() == 0u);
}

TEST_CASE("A die playing whole climbs counts them into the table")
{
    VecSpireEnv row(1);
    std::vector<float> returns(30, 0.0f);

    row.RollRandomHere(CardColor::RED, 1, 30, returns.data(), nullptr,
                       nullptr);

    CHECK(row.GetStats().GetRuns() == 30);
    CHECK(row.GetStats().GetRuns() ==
          row.GetStats().GetWins() + row.GetStats().GetDeaths());
    CHECK(row.GetStats().GetRowCount() > 5u);
}

TEST_CASE("What was offered and left is counted as well as what was taken")
{
    Run run(CardColor::RED, 8);

    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);

    Battle battle = run.StartBattleHere();

    for (auto& monster : battle.GetMonsters())
    {
        monster.SetHealth(0);
    }

    battle.EndTurn();
    run.FinishBattle(battle);

    // Find the card choice a won fight left.
    std::size_t which = run.GetRewards().size();

    for (std::size_t i = 0; i < run.GetRewards().size(); ++i)
    {
        if (run.GetRewards()[i].kind == RewardKind::CARD_CHOICE)
        {
            which = i;
            break;
        }
    }

    REQUIRE(which < run.GetRewards().size());

    const std::vector<CardId> offered = run.GetRewards()[which].cards;

    REQUIRE(offered.size() >= 2u);
    REQUIRE(run.ClaimReward(which, 0) == true);

    RunStats stats;

    stats.Ingest(run.GetLog());

    // The one that was taken has a pick and no pass; the rest are the other
    // way round.
    for (std::size_t i = 0; i < offered.size(); ++i)
    {
        bool found = false;

        for (const auto& row : stats.GetRows())
        {
            if (row.second.kind != static_cast<int>(StatKind::CARD_TAKEN) ||
                row.second.id != static_cast<int>(offered[i]))
            {
                continue;
            }

            found = true;

            if (i == 0u)
            {
                CHECK(row.second.picks == 1);
                CHECK(row.second.passes == 0);
            }
            else
            {
                CHECK(row.second.picks == 0);
                CHECK(row.second.passes == 1);
                CHECK(row.second.runs == 0);
            }
        }

        CHECK(found == true);
    }
}

TEST_CASE("A pile walked away from counts as everything passed over")
{
    Run run(CardColor::RED, 9);

    run.OpenChest();

    std::vector<RelicId> offered;

    for (const auto& reward : run.GetRewards())
    {
        offered.insert(offered.end(), reward.relics.begin(),
                       reward.relics.end());
    }

    run.ClearRewards();

    RunStats stats;

    stats.Ingest(run.GetLog());

    for (const RelicId id : offered)
    {
        for (const auto& row : stats.GetRows())
        {
            if (row.second.kind == static_cast<int>(StatKind::RELIC_TAKEN) &&
                row.second.id == static_cast<int>(id))
            {
                CHECK(row.second.picks == 0);
                CHECK(row.second.passes == 1);
            }
        }
    }
}

TEST_CASE("Turning a card down counts every card of the pick as passed")
{
    Run run(CardColor::RED, 8);

    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);

    Battle battle = run.StartBattleHere();

    for (auto& monster : battle.GetMonsters())
    {
        monster.SetHealth(0);
    }

    battle.EndTurn();
    run.FinishBattle(battle);

    for (std::size_t i = 0; i < run.GetRewards().size(); ++i)
    {
        if (run.GetRewards()[i].kind != RewardKind::CARD_CHOICE)
        {
            continue;
        }

        const std::size_t offered = run.GetRewards()[i].cards.size();

        REQUIRE(run.SkipReward(i) == true);

        CHECK(run.GetLog().GetSummary().cardsPassed ==
              static_cast<int>(offered));
        break;
    }
}

TEST_CASE("What was left on a shelf is counted as not bought")
{
    Run run(CardColor::RED, 7);

    run.AddGold(2000);
    run.OpenShop();

    const CardId boughtCard = run.GetShop().GetCards().front().id;
    const RelicId boughtRelic = run.GetShop().GetRelics()[2].id;

    REQUIRE(run.BuyCard(0) == true);
    REQUIRE(run.BuyRelic(2) == true);

    // What is still on the shelf when the door shuts was passed over.
    std::vector<CardId> leftCards;
    std::vector<RelicId> leftRelics;

    for (const auto& slot : run.GetShop().GetCards())
    {
        if (!slot.sold)
        {
            leftCards.emplace_back(slot.id);
        }
    }

    for (const auto& slot : run.GetShop().GetRelics())
    {
        if (!slot.sold)
        {
            leftRelics.emplace_back(slot.id);
        }
    }

    REQUIRE(leftCards.empty() == false);
    REQUIRE(leftRelics.empty() == false);

    run.CloseShop();

    RunStats stats;

    stats.Ingest(run.GetLog());

    // The one that was bought reads as bought once and passed never.
    for (const auto& row : stats.GetRows())
    {
        if (row.second.kind == static_cast<int>(StatKind::CARD_BOUGHT) &&
            row.second.id == static_cast<int>(boughtCard))
        {
            CHECK(row.second.picks == 1);
        }

        if (row.second.kind == static_cast<int>(StatKind::RELIC_BOUGHT) &&
            row.second.id == static_cast<int>(boughtRelic))
        {
            CHECK(row.second.picks == 1);
        }
    }

    // And what was left reads the other way round, under both tables.
    for (const RelicId id : leftRelics)
    {
        bool asBought = false;
        bool asTaken = false;

        for (const auto& row : stats.GetRows())
        {
            if (row.second.id != static_cast<int>(id))
            {
                continue;
            }

            if (row.second.kind == static_cast<int>(StatKind::RELIC_BOUGHT))
            {
                asBought = true;
                CHECK(row.second.passes == 1);
                CHECK(row.second.picks == 0);
            }

            if (row.second.kind == static_cast<int>(StatKind::RELIC_TAKEN))
            {
                asTaken = true;
                CHECK(row.second.passes == 1);
            }
        }

        CHECK(asBought == true);
        CHECK(asTaken == true);
    }

    // Shutting the door twice counts nothing twice.
    const std::size_t lines = run.GetLog().GetLines().size();

    run.CloseShop();

    CHECK(run.GetLog().GetLines().size() == lines);
}

TEST_CASE("Opening a shop shuts the one before it")
{
    Run run(CardColor::RED, 7);

    run.OpenShop();

    const std::size_t before = run.GetLog().GetLines().size();

    run.OpenShop();

    // The first shelf was written off as it was replaced.
    CHECK(run.GetLog().GetLines().size() > before);
}

TEST_CASE("Every kind of line has a name")
{
    // What Python reads the log with. A kind added without a name here would
    // leave everything after it mislabelled on that side.
    for (int entry = 1; entry < static_cast<int>(LogEntry::COUNT); ++entry)
    {
        const char* name = NameOf(static_cast<LogEntry>(entry));

        REQUIRE(name != nullptr);
        CHECK(std::string(name).empty() == false);
        CHECK(std::string(name) != "invalid");
    }

    CHECK(std::string(NameOf(LogEntry::ROOM_ANSWERED)) == "room_answered");
    CHECK(std::string(NameOf(LogEntry::CARD_PASSED)) == "card_passed");
    CHECK(std::string(NameOf(LogEntry::COUNT)) == "invalid");
}

TEST_CASE("Answering a room counts what it turned down")
{
    Run run(CardColor::RED, 11);

    run.StartEvent(EventId::GOLDEN_SHRINE);

    REQUIRE(run.HasEvent() == true);

    const std::size_t options = run.GetEvent().GetOptions().size();

    REQUIRE(options == 3);

    // Desecrate: 275 gold and a Regret for it.
    REQUIRE(run.ChooseEventOption(1) == true);

    CHECK(Count(run, LogEntry::ROOM_ANSWERED) == 1);

    // Everything else that could have been taken instead.
    CHECK(Count(run, LogEntry::ROOM_PASSED) == static_cast<int>(options) - 1);

    RunStats stats;

    stats.Ingest(run.GetLog());

    int picked = 0;
    int left = 0;

    for (const auto& row : stats.GetRows())
    {
        if (row.second.kind == static_cast<int>(StatKind::ROOM_ANSWERED))
        {
            picked += row.second.picks;
            left += row.second.passes;
        }
    }

    CHECK(picked == 1);
    CHECK(left == static_cast<int>(options) - 1);
}

TEST_CASE("Walking counts the places it walked past")
{
    Run run(CardColor::RED, 23);

    // A fork wide enough to have something to turn down.
    std::size_t choices = 0;
    int walked = 0;

    while (walked < 6)
    {
        const std::vector<int> columns = run.GetAvailableColumns();

        REQUIRE(columns.empty() == false);

        choices += columns.size() - 1u;

        REQUIRE(run.Travel(columns.front()) == true);

        // Whatever the floor turned into is left where it is: only the
        // walking is what this counts.
        ++walked;
    }

    CHECK(Count(run, LogEntry::FLOOR_WALKED) == walked);
    CHECK(Count(run, LogEntry::PATH_PASSED) == static_cast<int>(choices));

    RunStats stats;

    stats.Ingest(run.GetLog());

    int picked = 0;
    int left = 0;

    for (const auto& row : stats.GetRows())
    {
        if (row.second.kind == static_cast<int>(StatKind::NODE_WALKED))
        {
            picked += row.second.picks;
            left += row.second.passes;

            // A row is one kind of place, and it is a real one.
            CHECK(std::string(NameOf(
                      static_cast<MapNodeType>(row.second.id))) != "empty");
        }
    }

    CHECK(picked == walked);
    CHECK(left == static_cast<int>(choices));
}
