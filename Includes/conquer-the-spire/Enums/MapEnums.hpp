// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_MAP_ENUMS_HPP
#define CONQUER_THE_SPIRE_MAP_ENUMS_HPP

namespace ConquerTheSpire
{
//! What waits at a place on the map.
enum class MapNodeType
{
    EMPTY = 0,  //!< Nothing is here; no path goes through it.
    MONSTER,
    ELITE,
    EVENT,
    REST,
    MERCHANT,
    TREASURE,
    BOSS,

    //! How many kinds there are, which is what anything counting them
    //! measures itself against.
    COUNT
};

//! What a kind of place is called.
const char* NameOf(MapNodeType type);
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_MAP_ENUMS_HPP
