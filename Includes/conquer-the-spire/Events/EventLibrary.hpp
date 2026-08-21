// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_EVENT_LIBRARY_HPP
#define CONQUER_THE_SPIRE_EVENT_LIBRARY_HPP

#include <conquer-the-spire/Events/Event.hpp>
#include <conquer-the-spire/Models/Player.hpp>

#include <random>
#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief EventLibrary class.
//!
//! Every room a question mark can turn out to be, and the rule for which of
//! them a run has not had yet.
//!
class EventLibrary
{
 public:
    //! Returns the rooms of act one, each of which turns up once in a run.
    static const std::vector<EventId>& GetAct1Rooms();

    //! The same for acts two and three.
    static const std::vector<EventId>& GetAct2Rooms();
    static const std::vector<EventId>& GetAct3Rooms();

    //! Returns the shrines, which any act can hold and which can turn up
    //! again later.
    static const std::vector<EventId>& GetShrines();

    //! Builds the event \p id, at its opening stage.
    static Event Get(EventId id);

    //! Builds the four blessings Neow offers a climber of \p character before
    //! the first step.
    static Event MakeNeow(CardColor character, std::mt19937& rng);

    //! Returns true when \p id can turn up for \p player: a fountain only
    //! flows for a deck that has a curse in it.
    static bool CanAppear(EventId id, const Player& player);

    //! Picks a room for \p act that \p seen does not already hold and that
    //! \p player can make something of.
    static Event Pick(int act, const Player& player,
                      const std::vector<EventId>& seen, std::mt19937& rng);
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_EVENT_LIBRARY_HPP
