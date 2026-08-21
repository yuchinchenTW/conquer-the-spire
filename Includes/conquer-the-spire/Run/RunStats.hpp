// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_RUN_STATS_HPP
#define CONQUER_THE_SPIRE_RUN_STATS_HPP

#include <conquer-the-spire/Run/RunLog.hpp>

#include <cstddef>
#include <map>
#include <vector>

namespace ConquerTheSpire
{
//! What a row of the table is about: a card taken, a card bought, a relic
//! bought, and so on. A card bought is counted twice over, once as taken and
//! once as bought, because it was both.
enum class StatKind
{
    INVALID = 0,
    CARD_TAKEN,
    CARD_BOUGHT,
    CARD_REMOVED,
    CARD_UPGRADED,
    CARD_TRANSFORMED,
    RELIC_TAKEN,
    RELIC_BOUGHT,
    POTION_TAKEN,
    POTION_BOUGHT,
    POTION_DRUNK,
    ROOM_ENTERED,
    ROOM_ANSWERED,

    //! Which kind of place on the map it walks onto, against the kinds it
    //! walked past to get there.
    NODE_WALKED
};

//!
//! \brief RunStats class.
//!
//! What came of the choices, counted over as many climbs as are put through
//! it. Every card, relic, potion and room keeps its own row: how often it was
//! chosen, how many climbs it was in, how many of those came out the top of
//! the spire, how many died, and how far they got altogether.
//!
//! A row says nothing on its own about what is worth taking - a card that
//! only ever turns up in a deck that was already winning will read well - but
//! over enough climbs it is what tells one pick from another.
//!
class RunStats
{
 public:
    //!
    //! \brief Row struct.
    //!
    struct Row
    {
        int kind = 0;
        int id = 0;

        //! How many times this was chosen altogether, counting more than once
        //! in the same climb.
        int picks = 0;

        //! How many times it was offered and left where it was. Taken over
        //! taken plus passed is how often it is picked when it turns up.
        int passes = 0;

        //! How many climbs it turned up in, and how those climbs ended.
        int runs = 0;
        int wins = 0;
        int deaths = 0;

        //! The floors of all those climbs added up, for an average.
        int floors = 0;

        //! How many numbers a row is.
        static constexpr std::size_t SLOTS = 8;
    };

    //! Takes in one finished climb.
    void Ingest(const RunLog& log);

    void Clear();

    //! How many rows the table holds, and the rows themselves as whole
    //! numbers: \p out takes GetRowCount() times Row::SLOTS of them, in the
    //! order the fields are written above.
    std::size_t GetRowCount() const;
    void ReadRows(int* out) const;

    //! The whole table as it stands, keyed by the kind and the id.
    const std::map<long long, Row>& GetRows() const;

    //! How many climbs went through here, how many were won, how many ended
    //! in a death, and the floors of all of them added up.
    int GetRuns() const;
    int GetWins() const;
    int GetDeaths() const;
    int GetFloors() const;

    //! Reads those four as a row of numbers, in that order.
    static constexpr std::size_t TOTAL_SLOTS = 4;
    void ReadTotals(int* out) const;

 private:
    //! Notes that \p kind and \p id were chosen in the climb being taken in.
    void Note(StatKind kind, int id, bool won, bool died, int floors,
              std::vector<long long>& seen);

    //! Notes that \p kind and \p id were offered and not taken.
    void NotePassed(StatKind kind, int id);

    std::map<long long, Row> m_rows;
    int m_runs = 0;
    int m_wins = 0;
    int m_deaths = 0;
    int m_floors = 0;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_RUN_STATS_HPP
