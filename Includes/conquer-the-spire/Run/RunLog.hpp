// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_RUN_LOG_HPP
#define CONQUER_THE_SPIRE_RUN_LOG_HPP

#include <conquer-the-spire/Enums/CardId.hpp>
#include <conquer-the-spire/Enums/MonsterEnums.hpp>
#include <conquer-the-spire/Enums/PotionId.hpp>
#include <conquer-the-spire/Enums/RelicId.hpp>

#include <cstddef>
#include <vector>

namespace ConquerTheSpire
{
//! What a line of the log is about.
enum class LogEntry
{
    INVALID = 0,
    CARD_TAKEN,

    //! A card, relic or potion that was offered and left where it was. What
    //! was taken and what was passed over together say how often a thing is
    //! picked when it does turn up.
    CARD_PASSED,
    RELIC_PASSED,
    POTION_PASSED,
    CARD_REMOVED,
    CARD_UPGRADED,
    CARD_TRANSFORMED,
    RELIC_TAKEN,
    RELIC_LOST,
    POTION_TAKEN,
    POTION_DRUNK,
    POTION_THROWN,
    GOLD_EARNED,
    GOLD_SPENT,
    FIGHT_WON,
    FLOOR_WALKED,
    ACT_STARTED,
    ROOM_ENTERED,
    ROOM_ANSWERED,
    RESTED,
    DIED,
    SPIRE_DONE,

    //! An option of a room that was on the table and not taken. One of these
    //! for every other option the room offered, which is what makes the
    //! answer a choice among alternatives rather than the only thing seen.
    ROOM_PASSED,

    //! A place the map led to and the climber walked past. \p id is what
    //! stood on it, the way FLOOR_WALKED says what was walked onto.
    PATH_PASSED,

    //! An option that hands over a curse, taken; and one that was on the
    //! table, could have been afforded, and was left alone. \p id is the room
    //! in both, so that the rooms it keeps giving in to can be told from the
    //! ones it has learnt to walk past.
    //!
    //! A curse otherwise only ever reads as taken - it is handed over rather
    //! than offered, so the deck says nothing about the times the danger was
    //! seen and turned down. These two are the pair that does.
    CURSE_CHOSEN,
    CURSE_REFUSED,

    //! A card played in a fight, once a line for each time. \p extra is how
    //! many times, so a Strike played four times is one line and not four.
    //! What a card is worth on paper and how often it is actually played are
    //! different things - a Clash reads as nought energy for fourteen damage
    //! and is unplayable two turns in three - and only this says the second.
    CARD_PLAYED,

    //! A fight walked into. \p id is the monster leading it and \p extra its
    //! MonsterType, so a fight that was lost can be told from one that was
    //! never met: FIGHT_WON says only that one was won, and a climb that
    //! died left nothing at all behind to say what killed it.
    FIGHT_STARTED,

    //! A card of the deck that could have been taken out, or put to the
    //! whetstone, and was not. One of these for every other card the deck
    //! held at the time, which is what makes taking one out a choice among
    //! the rest rather than the only thing that happened.
    CARD_NOT_REMOVED,
    CARD_NOT_UPGRADED,

    //! How many kinds of line there are, which is what anything reading the
    //! log from outside counts against so that the two cannot drift apart.
    COUNT
};

//! What a kind of line is called. Anything reading the log from outside asks
//! here rather than keeping a list of its own.
const char* NameOf(LogEntry entry);

//! Where a line of the log came from, which is what tells a pick from a
//! purchase.
enum class LogSource
{
    UNKNOWN = 0,
    REWARD,
    SHOP,
    ROOM,
    RELIC,
    REST,
    CHEST,
    FIGHT,
    BOSS
};

//!
//! \brief LogLine struct.
//!
//! One thing that happened. \p id is the card, relic, potion, room or monster
//! it was about, and \p extra is whatever else the line needs: the amount of
//! gold, the card a transform turned into, the option a room was answered
//! with.
//!
struct LogLine
{
    LogEntry entry = LogEntry::INVALID;
    LogSource source = LogSource::UNKNOWN;
    int id = 0;
    int extra = 0;
    int act = 1;
    int floor = 0;

    //! Which stage of a room the line is about, for the rooms that have more
    //! than one.
    int stage = 0;
};

//!
//! \brief RunLog class.
//!
//! What a climb did, line by line, and the counts that go with it. This is
//! how a run is read back afterwards: which cards were taken and which were
//! torn up, what was bought, how deep it got, and whether it ever came out
//! the top.
//!
class RunLog
{
 public:
    //!
    //! \brief Summary struct.
    //!
    //! The counts a run is judged by. Every one of them is a whole number, so
    //! that they can be handed over as one row of a table.
    //!
    struct Summary
    {
        int floors = 0;
        int act = 1;
        int deepestAct = 1;
        int fightsWon = 0;
        int elitesWon = 0;
        int bossesWon = 0;
        int cardsTaken = 0;
        int cardsPassed = 0;
        int cardsBought = 0;
        int cardsRemoved = 0;
        int cardsUpgraded = 0;
        int cardsTransformed = 0;
        int relicsTaken = 0;
        int relicsBought = 0;
        int potionsTaken = 0;
        int potionsBought = 0;
        int potionsDrunk = 0;
        int goldEarned = 0;
        int goldSpent = 0;
        int roomsEntered = 0;
        int rested = 0;
        int died = 0;
        int wonTheSpire = 0;

        //! Curse options taken, and curse options turned down. The second
        //! over the two together is how often the danger was walked into
        //! when it was there to be walked into.
        int cursesChosen = 0;
        int cursesRefused = 0;

        //! How many numbers a summary is, for whatever hands it over as a
        //! row.
        static constexpr std::size_t SLOTS = 25;
    };

    //! Writes a line, and counts it.
    void Add(LogEntry entry, LogSource source, int id, int extra, int act,
             int floor, int stage = 0);

    //! Forgets everything, which is what starting a climb does.
    void Clear();

    const std::vector<LogLine>& GetLines() const;
    const Summary& GetSummary() const;

    //! Reads the summary as a row of whole numbers, in the order it is
    //! written above. \p out takes Summary::SLOTS of them.
    void ReadSummary(int* out) const;

 private:
    std::vector<LogLine> m_lines;
    Summary m_summary;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_RUN_LOG_HPP
