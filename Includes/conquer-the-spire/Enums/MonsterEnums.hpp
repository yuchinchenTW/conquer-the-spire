// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_MONSTER_ENUMS_HPP
#define CONQUER_THE_SPIRE_MONSTER_ENUMS_HPP

namespace ConquerTheSpire
{
//! What kind of fight a monster turns up in.
enum class MonsterType
{
    INVALID = 0,
    NORMAL,
    ELITE,
    BOSS
};

//! One thing a monster move does. A move is a list of these, the same way a
//! card is a list of effects.
enum class MonsterEffectType
{
    NOTHING = 0,
    DAMAGE,        //!< Hits the player.
    DAMAGE_SCALED, //!< Hits for a share of the player's health, plus one.
    BLOCK,        //!< Blocks for itself.
    BLOCK_ALLY,   //!< Blocks for another monster, or itself when alone.
    APPLY_POWER,  //!< Buffs itself or debuffs the player.
    ADD_CARD,     //!< Puts a card into the player's discard pile.
    BUFF_ALL,     //!< Buffs every monster still standing.
    BLOCK_ALLIES, //!< Blocks for the others, but not for itself.
    HEAL_ALL,     //!< Heals every monster still standing.
    SUMMON,       //!< Calls more monsters in, up to a limit.
    REVIVE,       //!< Comes back with a share of its health.
    SELF_DESTRUCT, //!< Goes off, and goes with it.
    SPLIT,        //!< Steps aside for two smaller ones.
    ESCAPE,       //!< Leaves the fight.
    STASIS,       //!< Holds a card until the monster dies.
    DRAIN,        //!< Hits, and takes back what got through as health.
    RECOVER,      //!< Shakes off every debuff and comes back up to a share
                  //!< of its health.
    RAISE_WALL,   //!< Puts the wall it changes shape at back up, higher than
                  //!< it was.
    STOKE_BURNS,  //!< Sets fire to what is already burning, and to whatever
                  //!< catches after.
    REBIRTH,      //!< Stands the second body up whole, clean, and in the
                  //!< phase that goes with it.
    SHAKE_OFF     //!< Throws off every debuff standing on it.
};

//! Every monster the library can build.
enum class MonsterId
{
    INVALID = 0,

    // Act 1 - normal
    CULTIST,
    JAW_WORM,
    RED_LOUSE,
    GREEN_LOUSE,
    ACID_SLIME_S,
    ACID_SLIME_M,
    ACID_SLIME_L,
    SPIKE_SLIME_S,
    SPIKE_SLIME_M,
    SPIKE_SLIME_L,
    FUNGI_BEAST,
    LOOTER,
    BLUE_SLAVER,
    RED_SLAVER,
    MAD_GREMLIN,
    SNEAKY_GREMLIN,
    FAT_GREMLIN,
    SHIELD_GREMLIN,
    GREMLIN_WIZARD,

    // Act 1 - elite
    GREMLIN_NOB,
    LAGAVULIN,
    SENTRY,

    // Act 1 - boss
    THE_GUARDIAN,
    HEXAGHOST,
    SLIME_BOSS,

    // Act 2 - normal
    SPHERIC_GUARDIAN,
    CHOSEN,
    SHELLED_PARASITE,
    BYRD,
    MUGGER,
    CENTURION,
    MYSTIC,
    SNAKE_PLANT,
    SNECKO,

    // Act 2 - elite
    GREMLIN_LEADER,

    //! Not a monster of its own: asked for, it makes one of the five kinds of
    //! gremlin at random. A leader brings two of these and calls for more,
    //! and which kinds turn up is most of what the fight is.
    RANDOM_GREMLIN,
    TASKMASTER,
    BOOK_OF_STABBING,

    // Act 2 - boss
    BRONZE_AUTOMATON,
    BRONZE_ORB,
    THE_CHAMP,
    THE_COLLECTOR,
    TORCH_HEAD,

    // Act 3 - normal
    DARKLING,
    ORB_WALKER,
    SPIKER,
    REPULSOR,
    EXPLODER,
    THE_MAW,
    SPIRE_GROWTH,
    TRANSIENT,
    WRITHING_MASS,

    // Act 3 - elite
    GIANT_HEAD,
    NEMESIS,
    REPTOMANCER,
    DAGGER,

    // Act 3 - boss
    AWAKENED_ONE,
    TIME_EATER,
    DONU,
    DECA,

    // Act 4
    SPIRE_SHIELD,
    SPIRE_SPEAR,
    CORRUPT_HEART,

    // The ones that only turn up in a room of their own
    POINTY,
    ROMEO,
    BEAR,

    // For tests and practice
    TRAINING_DUMMY,

    // Appended, and only ever appended: the number a monster has here is the
    // row it gets in the learner's table of monsters, so putting one in the
    // middle moves every monster after it onto somebody else's row and throws
    // away what a running climb has learned about all of them.

    //! Not a monster of its own: asked for, it makes one of a repulsor, an
    //! exploder or a spiker. Which three or four turn up is what a room of
    //! shapes is, and the rooms are built so that no more than two are alike.
    RANDOM_SHAPE,

    //! A jaw worm of the third act, which is the same worm that has already
    //! bellowed once before the fight starts.
    JAW_WORM_HARD,

    //! Not monsters of their own: a louse of either colour, a medium slime of
    //! either kind, a large slime of either kind. The spire writes several of
    //! its rooms as "a louse" and "a slime" and settles the colour when the
    //! room is built, so the rooms say that too.
    RANDOM_LOUSE,
    RANDOM_MEDIUM_SLIME,
    RANDOM_LARGE_SLIME
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_MONSTER_ENUMS_HPP
