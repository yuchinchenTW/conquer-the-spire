// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_MAP_GENERATOR_HPP
#define CONQUER_THE_SPIRE_MAP_GENERATOR_HPP

#include <conquer-the-spire/Map/Map.hpp>

#include <random>

namespace ConquerTheSpire
{
//!
//! \brief MapGenerator class.
//!
//! Lays out one act. Paths are carved from the bottom row upwards, each step
//! going straight on or one column to either side, and then every place a path
//! runs through is given something to do. The first row is always a fight, the
//! ninth always the treasure and the last always a rest site; elites and rest
//! sites stay out of the opening rows, and neither they nor a merchant follow
//! straight after one of their own kind.
//!
class MapGenerator
{
 public:
    //! Lays out an act with \p pathCount ways up, using \p rng for every
    //! choice. The same generator state lays out the same act.
    static Map Generate(std::mt19937& rng, int pathCount = 6);

    //! Lays out the last act, which is the same handful of rooms every time:
    //! a rest, a merchant and the pair that guard the door.
    static Map GenerateFinalAct();

 private:
    //! Walks \p pathCount paths from the bottom row to the top.
    static void CarvePaths(Map& map, std::mt19937& rng, int pathCount);

    //! Gives every place a path runs through something to do.
    static void AssignTypes(Map& map, std::mt19937& rng);
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_MAP_GENERATOR_HPP
