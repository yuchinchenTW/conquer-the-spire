// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_RELIC_ENUMS_HPP
#define CONQUER_THE_SPIRE_RELIC_ENUMS_HPP

namespace ConquerTheSpire
{
//! How a relic is come by.
enum class RelicTier
{
    INVALID = 0,
    STARTER,   //!< Handed over with the character.
    COMMON,
    UNCOMMON,
    RARE,
    BOSS,      //!< Taken after a boss, usually with a price attached.
    SHOP,      //!< Bought.
    EVENT      //!< Given by an event.
};

//! When a relic does its work. A relic that only matters outside a battle -
//! on the map, in a shop, at a rest site - carries NONE and is only carried
//! around by the battle.
enum class RelicHook
{
    NONE = 0,
    BATTLE_START,    //!< Anchor, Vajra, Cracked Core.
    TURN_START,      //!< Mercury Hourglass, Happy Flower, Horn Cleat.
    TURN_END,        //!< Orichalcum, Mutagenic Strength.
    CARD_PLAYED,     //!< Ink Bottle.
    ATTACK_PLAYED,   //!< Kunai, Shuriken, Ornamental Fan, Nunchaku.
    SKILL_PLAYED,    //!< Letter Opener.
    POWER_PLAYED,    //!< Bird-Faced Urn, Mummified Hand.
    CARD_EXHAUSTED,  //!< Charon's Ashes, Dead Branch.
    SHUFFLED,        //!< Sundial, The Abacus.
    HEALTH_LOST,     //!< Centennial Puzzle, Runic Cube.
    ENEMY_KILLED     //!< Gremlin Horn.
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_RELIC_ENUMS_HPP
