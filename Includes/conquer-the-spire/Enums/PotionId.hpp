// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_POTION_ID_HPP
#define CONQUER_THE_SPIRE_POTION_ID_HPP

namespace ConquerTheSpire
{
//! When a potion may be drunk. Most of them are of use in a fight and nowhere
//! else; three are of use anywhere; and one is never drunk on purpose at all.
enum class PotionUse
{
    IN_BATTLE = 0,
    ANYWHERE,
    PASSIVE
};

//! How often a potion turns up.
enum class PotionRarity
{
    INVALID = 0,
    COMMON,
    UNCOMMON,
    RARE
};

//! Every potion the registry can build.
enum class PotionId
{
    INVALID = 0,

    // Common
    ANCIENT_POTION,
    ATTACK_POTION,
    BLOCK_POTION,
    BLOOD_POTION,
    BOTTLED_MIRACLE,
    COLORLESS_POTION,
    DEXTERITY_POTION,
    ENERGY_POTION,
    EXPLOSIVE_POTION,
    FEAR_POTION,
    FIRE_POTION,
    FLEX_POTION,
    POWER_POTION,
    SKILL_POTION,
    SPEED_POTION,
    STRENGTH_POTION,
    SWIFT_POTION,
    WEAK_POTION,

    // Uncommon
    BLESSING_OF_THE_FORGE,
    CULTIST_POTION,
    DISTILLED_CHAOS,
    DUPLICATION_POTION,
    ELIXIR,
    ESSENCE_OF_STEEL,
    FOCUS_POTION,
    GAMBLERS_BREW,
    LIQUID_BRONZE,
    LIQUID_MEMORIES,
    POISON_POTION,
    POTION_OF_CAPACITY,
    REGEN_POTION,

    // Rare
    CUNNING_POTION,
    ENTROPIC_BREW,
    ESSENCE_OF_DARKNESS,
    FAIRY_IN_A_BOTTLE,
    FRUIT_JUICE,
    GHOST_IN_A_JAR,
    HEART_OF_IRON,
    SMOKE_BOMB,
    SNECKO_OIL
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_POTION_ID_HPP
