// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_EVENT_ENUMS_HPP
#define CONQUER_THE_SPIRE_EVENT_ENUMS_HPP

namespace ConquerTheSpire
{
//! What stands behind a question mark on the map.
enum class EventId
{
    INVALID = 0,

    // The rooms of act one, each of which turns up once in a run.
    BIG_FISH,
    DEAD_ADVENTURER,
    FACE_TRADER,
    GOLDEN_IDOL,
    HYPNOTIZING_COLORED_MUSHROOMS,
    LIVING_WALL,
    SCRAP_OOZE,
    SHINING_LIGHT,
    THE_CLERIC,
    THE_SSSSSERPENT,
    WING_STATUE,
    WORLD_OF_GOOP,

    // The rooms of act two.
    ANCIENT_WRITING,
    AUGMENTER,
    COUNCIL_OF_GHOSTS,
    CURSED_TOME,
    FORGOTTEN_ALTAR,
    MASKED_BANDITS,
    OLD_BEGGAR,
    PLEADING_VAGRANT,
    THE_COLOSSEUM,
    THE_LIBRARY,
    THE_MAUSOLEUM,
    THE_NEST,
    VAMPIRES,

    // The rooms of act three.
    FALLING,
    MIND_BLOOM,
    MYSTERIOUS_SPHERE,
    SENSORY_STONE,
    THE_MOAI_HEAD,
    TOMB_OF_LORD_RED_MASK,
    WINDING_HALLS,

    // The shrines, which any act can hold.
    BONFIRE_SPIRITS,
    DUPLICATOR,
    GOLDEN_SHRINE,
    LAB,
    OMINOUS_FORGE,
    PURIFIER,
    THE_DIVINE_FOUNTAIN,
    THE_WOMAN_IN_BLUE,
    TRANSMOGRIFIER,
    UPGRADE_SHRINE,
    WE_MEET_AGAIN,
    WHEEL_OF_CHANGE,

    // The shrines of the later acts.
    DESIGNER_IN_SPIRE,
    KNOWING_SKULL,
    NLOTH,
    SECRET_PORTAL,
    THE_JOUST,

    // The one that comes before the first step.
    NEOW
};

//! Whether an event is one of an act's own rooms or a shrine that any act can
//! hold. A room only turns up once in a run; a shrine can turn up again.
enum class EventKind
{
    INVALID = 0,
    ONE_TIME,
    SHRINE
};

//! What choosing an option does.
enum class EventEffectType
{
    NONE = 0,
    GAIN_GOLD,
    LOSE_GOLD,
    LOSE_ALL_GOLD,
    HEAL,
    HEAL_PERCENT,
    HEAL_FULL,
    LOSE_HEALTH,
    LOSE_HEALTH_PERCENT,
    GAIN_MAX_HEALTH,
    LOSE_MAX_HEALTH,
    LOSE_MAX_HEALTH_PERCENT,

    //! A share of the health left rather than of the whole, which is what
    //! Neow asks of a climber who wants something for it.
    LOSE_HEALTH_PERCENT_CURRENT,
    GAIN_RELIC,
    GAIN_RANDOM_RELIC,
    BOSS_RELIC_SWAP,
    GAIN_CURSE,
    GAIN_RANDOM_CURSE,
    GAIN_CARD,
    GAIN_RANDOM_CARDS,

    //! A pick of cards left on the pile, the way a fight leaves one.
    CARD_REWARD,
    GAIN_POTIONS,
    REMOVE_CARDS,
    UPGRADE_CARDS,
    UPGRADE_RANDOM_CARDS,
    TRANSFORM_CARDS,
    DUPLICATE_CARD,
    CLEANSE_CURSES,
    LOSE_POTION,
    LOSE_CARD,

    //! The offer a bonfire wants, which pays by what was burned.
    BURN_OFFERING,

    //! A fight the event picks, with a relic for winning it.
    FIGHT,

    //! The wheel, whose six faces are all as likely as each other.
    SPIN_WHEEL,

    //! Reaching into the ooze: it costs more and pays out more often with
    //! every try.
    REACH_INTO_OOZE,

    //! Searching the body: every search is likelier than the last to wake
    //! something up.
    SEARCH_BODY,

    //! The faces on offer: two in five are kind, two in five are not.
    TRADE_FACE,

    //! Takes a card of a kind out of the deck at random, which is what
    //! falling costs.
    REMOVE_RANDOM_OF_TYPE,

    //! Sharpens every card of the deck, or only the ones a starting deck is
    //! made of.
    UPGRADE_ALL,
    UPGRADE_ALL_BASIC,

    //! Takes every card of a kind out, and hands over copies of another.
    REPLACE_EVERY,

    //! Gives up a relic: the one named, or one at random.
    LOSE_RELIC,

    //! Hands over one of a few relics, which one being the surprise a book
    //! keeps.
    GAIN_ONE_OF_RELICS,

    //! Pays the toll a skull asks, which climbs with every answer given to
    //! the same question.
    SKULL_TOLL,

    //! Puts gold on an outcome: takes the stake, and pays out when it comes
    //! in.
    WAGER,

    //! Steps straight to the boss of the act.
    TO_THE_BOSS,

    //! A fight with one of the bosses of the first act, for a rare relic.
    FIGHT_OLD_BOSS
};

//! What an option needs before it can be taken.
enum class EventRequirement
{
    NONE = 0,
    GOLD,
    HAS_CURSE,
    HAS_POTION,
    HAS_REMOVABLE_CARD,
    HAS_UPGRADEABLE_CARD,

    //! Wants a relic the climber may not have.
    HAS_RELIC,

    //! An attack that lands ten or more in one hit, which is what the statue
    //! wants breaking.
    HAS_HEAVY_ATTACK
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_EVENT_ENUMS_HPP
