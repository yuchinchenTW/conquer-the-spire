// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_ENCOUNTER_LIBRARY_HPP
#define CONQUER_THE_SPIRE_ENCOUNTER_LIBRARY_HPP

#include <conquer-the-spire/Enums/MapEnums.hpp>
#include <conquer-the-spire/Monsters/MonsterRoster.hpp>

#include <string>
#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief Encounter struct.
//!
//! One group of monsters a place on the map can hold.
//!
struct Encounter
{
    std::string name;
    MonsterType type = MonsterType::NORMAL;
    std::vector<MonsterId> monsters;

    //! How often this group comes up against the others of its pool. The
    //! spire does not weigh them all alike. The second act's are published
    //! and written down here; the pools that carry no source for theirs still
    //! weigh every group alike, which is a thing to come back to rather than a
    //! thing that is right.
    int weight = 1;
};

//!
//! \brief EncounterLibrary class.
//!
//! What waits in the fights of an act. The opening fights come from the weak
//! list, the rest from the strong one, the same way the spire hands them out.
//!
class EncounterLibrary
{
 public:
    //! How many fights an act opens with before it reaches for the harder
    //! groups. The first act eases a climber in for one fight longer than
    //! the others do.
    static constexpr int WEAK_FIGHTS = 3;
    static constexpr int LATER_WEAK_FIGHTS = 2;

    //! Returns how many opening fights of \p act come from its weak list.
    static int WeakFightsOf(int act);

    //! Returns the groups the first fights of act one are drawn from.
    static const std::vector<Encounter>& GetAct1Weak();

    //! Returns the groups the later fights of act one are drawn from.
    static const std::vector<Encounter>& GetAct1Strong();

    //! Returns the elites of act one.
    static const std::vector<Encounter>& GetAct1Elites();

    //! Returns the bosses of act one.
    static const std::vector<Encounter>& GetAct1Bosses();

    //! The same four lists for act three, and the two rooms of the last
    //! act.
    static const std::vector<Encounter>& GetAct3Weak();
    static const std::vector<Encounter>& GetAct3Strong();
    static const std::vector<Encounter>& GetAct3Elites();
    static const std::vector<Encounter>& GetAct3Bosses();
    static const std::vector<Encounter>& GetAct4Elites();
    static const std::vector<Encounter>& GetAct4Bosses();

    //! The same four lists for act two.
    static const std::vector<Encounter>& GetAct2Weak();
    static const std::vector<Encounter>& GetAct2Strong();
    static const std::vector<Encounter>& GetAct2Elites();
    static const std::vector<Encounter>& GetAct2Bosses();

    //! Picks a group of \p act for \p node. \p fightsSoFar decides whether a
    //! normal fight is still drawn from the weak list.
    //!
    //! \p lately holds the fights just had, newest first. The same one does
    //! not come round again within two of itself - three fights running can
    //! never hold two alike - so the two newest are out of the draw. It is
    //! asked of plain monster rooms only, which is where the rule sits, and a
    //! pool with nothing else left to offer lets it go rather than handing
    //! back no fight at all.
    static Encounter Pick(int act, MapNodeType node, int fightsSoFar,
                          std::mt19937& rng,
                          const std::vector<std::string>& lately = {});

    //! Builds the monsters of \p encounter.
    static std::vector<Monster> Build(const Encounter& encounter,
                                      std::mt19937& rng);
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_ENCOUNTER_LIBRARY_HPP
