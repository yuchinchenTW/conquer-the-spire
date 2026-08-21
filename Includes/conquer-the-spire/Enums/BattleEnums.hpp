// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_BATTLE_ENUMS_HPP
#define CONQUER_THE_SPIRE_BATTLE_ENUMS_HPP

namespace ConquerTheSpire
{
//! Which character a card belongs to.
enum class CardColor
{
    INVALID = 0,
    RED,        //!< Ironclad
    GREEN,      //!< Silent
    BLUE,       //!< Defect
    COLORLESS,  //!< Available to everyone
    CURSE,
    STATUS
};

//! How often a card shows up in a run.
enum class CardRarity
{
    INVALID = 0,
    BASIC,  //!< Starting deck
    COMMON,
    UNCOMMON,
    RARE,
    SPECIAL  //!< Statuses, curses and cards only made by other cards
};

//! Kind of a card.
enum class CardType
{
    INVALID = 0,
    ATTACK,
    SKILL,
    POWER,
    STATUS,
    CURSE
};

//! What a card needs before it can be played.
enum class CardTarget
{
    INVALID = 0,
    NONE,  //!< Unplayable
    SELF,
    SINGLE_ENEMY,
    ALL_ENEMIES,
    RANDOM_ENEMY
};

//! Where an effect aims, when it does not follow the card's own target.
enum class EffectTarget
{
    DEFAULT = 0,  //!< Follow the target of the card being played
    SELF,
    SINGLE_ENEMY,
    ALL_ENEMIES,
    RANDOM_ENEMY
};

//! One of the piles a card can sit in.
enum class CardPile
{
    INVALID = 0,
    DRAW_TOP,
    DRAW_BOTTOM,
    DRAW_SHUFFLED,
    HAND,
    DISCARD,
    EXHAUST
};

//! One of the four orbs the Defect can put into orbit.
enum class OrbType
{
    INVALID = 0,
    LIGHTNING,  //!< Damages an enemy each turn, harder when evoked.
    FROST,      //!< Blocks each turn, more when evoked.
    DARK,       //!< Builds damage up each turn and unloads it when evoked.
    PLASMA      //!< Hands over energy.
};

//! Buffs and debuffs a creature can carry.
enum class PowerType
{
    INVALID = 0,

    // Shared
    STRENGTH,        //!< Adds its amount to attack damage.
    DEXTERITY,       //!< Adds its amount to block gained.
    VULNERABLE,      //!< Takes 50% more attack damage.
    WEAK,            //!< Deals 25% less attack damage.
    FRAIL,           //!< Gains 25% less block.
    POISON,          //!< Loses health at the start of its turn, then decays.
    THORNS,          //!< Damages whoever attacks it.
    ARTIFACT,        //!< Negates the next debuff, then decays.
    INTANGIBLE,      //!< Reduces all damage taken to 1, then decays.
    STRENGTH_DOWN,   //!< Loses that much Strength at the end of the turn.
    DEXTERITY_DOWN,  //!< Loses that much Dexterity at the end of the turn.
    NO_DRAW,         //!< Cannot draw for the rest of the turn.

    // Ironclad
    BARRICADE,       //!< Block no longer expires.
    BERSERK,         //!< Gains that much extra energy each turn.
    BRUTALITY,       //!< Turn start: lose 1 health and draw a card, per stack.
    COMBUST,         //!< Turn end: lose health and damage every enemy.
    CORRUPTION,      //!< Skills cost no energy but exhaust.
    DARK_EMBRACE,    //!< Draws a card whenever a card is exhausted.
    DEMON_FORM,      //!< Turn start: gain that much Strength.
    DOUBLE_TAP,      //!< The next attack this turn is played twice.
    EVOLVE,          //!< Draws a card whenever a status card is drawn.
    FEEL_NO_PAIN,    //!< Gains block whenever a card is exhausted.
    FIRE_BREATHING,  //!< Damages every enemy when a status or curse is drawn.
    FLAME_BARRIER,   //!< Damages attackers this turn only.
    JUGGERNAUT,      //!< Damages a random enemy whenever block is gained.
    METALLICIZE,     //!< Turn end: gain that much block.
    RAGE,            //!< Gains block whenever an attack is played this turn.
    RUPTURE,         //!< Gains Strength whenever a card costs health.

    // Silent
    ACCURACY,            //!< Shivs deal that much more damage.
    AFTER_IMAGE,         //!< Gains block whenever a card is played.
    BLUR,                //!< Block does not expire next turn.
    BURST,               //!< The next skills this turn are played twice.
    CHOKED,              //!< Loses health whenever the player plays a card.
    CORPSE_EXPLOSION,    //!< On death, hits every enemy for its maximum health.
    DOUBLE_DAMAGE,       //!< Attacks deal double damage this turn.
    DRAW_NEXT_TURN,      //!< Draws that many extra cards next turn.
    ENERGIZED,           //!< Gains that much extra energy next turn.
    ENVENOM,             //!< Unblocked attack damage also applies poison.
    FREE_CARDS,          //!< Every card costs no energy this turn.
    INFINITE_BLADES,     //!< Adds a Shiv to the hand each turn.
    NEXT_TURN_BLOCK,     //!< Gains that much block next turn.
    NOXIOUS_FUMES,       //!< Poisons every enemy at the start of the turn.
    PHANTASMAL,          //!< Turns on Double Damage next turn.
    STRENGTH_UP,         //!< Gains that much Strength back at the turn end.
    THOUSAND_CUTS,       //!< Damages every enemy whenever a card is played.
    TOOLS_OF_THE_TRADE,  //!< Draws and discards a card each turn.
    WELL_LAID_PLANS,     //!< Keeps that many cards at the end of the turn.
    WRAITH_FORM,         //!< Loses that much Dexterity each turn.

    // Defect
    FOCUS,              //!< Raises what every orb does.
    LOCK_ON,            //!< Takes 50% more orb damage, then decays.
    BIASED_COGNITION,   //!< Loses that much Focus each turn.
    BUFFER,             //!< Shrugs off that many hits entirely.
    CREATIVE_AI,        //!< Adds a random power to the hand each turn.
    ECHO_FORM,          //!< The first card each turn is played twice.
    ELECTRO,            //!< Lightning orbs hit every enemy.
    HEATSINKS,          //!< Draws when a power is played.
    HELLO_WORLD,        //!< Adds a random common card to the hand each turn.
    LOOP,               //!< Sets off the first orb again at the turn start.
    MACHINE_LEARNING,   //!< Draws that many extra cards each turn.
    SELF_REPAIR,        //!< Heals when the battle is over.
    STATIC_DISCHARGE,   //!< Channels Lightning when the player is attacked.
    STORM,              //!< Channels Lightning when a power is played.
    RETAIN_HAND,        //!< Keeps the whole hand, for this turn only.

    // Colourless
    NO_BLOCK,    //!< Cannot gain block, and wears off each turn.
    MAYHEM,      //!< Plays the top card of the draw pile each turn.
    PANACHE,     //!< Every fifth card played hits everything.
    SADISTIC,    //!< Damages an enemy whenever it takes a debuff.
    MAGNETISM,   //!< Adds a random colourless card to the hand each turn.
    THE_BOMB,    //!< Goes off in that many turns.

    // Monsters
    RITUAL,        //!< Gains that much Strength at the end of its turn.
    CURL_UP,       //!< Blocks that much the first time it is attacked.
    ANGRY,         //!< Gains that much Strength whenever it is attacked.
    ENRAGE,        //!< Gains that much Strength when the player plays a skill.
    SPORE_CLOUD,   //!< Leaves that much Vulnerable behind when it dies.
    ENTANGLED,     //!< The player cannot play attacks this turn.
    SHARP_HIDE,    //!< Hurts the player for that much per attack played.
    MODE_SHIFT,    //!< Switches to its other shape once that much is dealt.
    ASLEEP,        //!< Does nothing until it is woken.

    // Potions
    //! Takes half of what an attack would do, and is knocked down by
    //! enough separate hits in one turn.
    FLIGHT,

    //! Every non-attack card played shuffles a Dazed into the draw pile.
    HEX,

    //! Blocks for itself at the end of its turn, and loses a layer to every
    //! unblocked hit.
    PLATED_ARMOR,

    //! Answers every attack with block, and by a little more each time.
    MALLEABLE,

    //! Every hit it lands puts Wounds in the discard pile.
    PAINFUL_STABS,

    //! Marks a monster that was summoned by another.
    MINION,

    //! Randomises what the cards drawn cost.
    CONFUSED,

    //! Takes gold with every hit it lands, and keeps it if it gets away.
    THIEVERY,

    //! Comes back as long as one of its own is still standing.
    LIFE_LINK,

    //! Hurts the climber at the end of every turn.
    CONSTRICTED,

    //! Counts the turns it has left before it goes.
    FADING,

    //! Loses as much strength as the health an attack takes off it, until
    //! the turn is over.
    SHIFTING,
    SHIFTING_LOSS,

    //! Thinks again about what it was going to do whenever it is hit.
    REACTIVE,

    //! Takes a tenth more from every card played this turn.
    SLOW,

    //! Ends the turn of a climber who plays too many cards, and grows for
    //! it.
    TIME_WARP,

    //! Draws that much less next turn.
    DRAW_REDUCTION,

    //! Grows whenever a power is played.
    CURIOSITY,

    //! Can only be brought down so far in one turn.
    INVINCIBLE,

    //! Hurts the climber for every card played.
    BEAT_OF_DEATH,

    //! Slips out of reach every other turn.
    INTANGIBLE_CYCLE,
    DUPLICATION,   //!< The next card played this turn goes twice.
    REGENERATION   //!< Heals at the end of the turn, then wears down.
};

//! A single thing a card does when it resolves.
enum class EffectType
{
    INVALID = 0,
    DEAL_DAMAGE,
    GAIN_BLOCK,
    APPLY_POWER,
    GAIN_ENERGY,
    DRAW_CARD,
    LOSE_HEALTH,
    HEAL,
    INCREASE_MAX_HEALTH,
    ADD_CARD,              //!< Makes a new card in a pile.
    COPY_SELF_TO_DISCARD,  //!< Anger.
    COPY_HAND_CARD,        //!< Dual Wield.
    ADD_RANDOM_ATTACK,     //!< Infernal Blade.
    UPGRADE_HAND_CARD,     //!< Armaments.
    EXHAUST_HAND_CARD,     //!< True Grit, Burning Pact.
    EXHAUST_HAND,          //!< Fiend Fire, Second Wind, Sever Soul.
    RETURN_FROM_EXHAUST,   //!< Exhume.
    DISCARD_TO_DRAW_TOP,   //!< Headbutt.
    HAND_TO_DRAW_TOP,      //!< Warcry.
    PLAY_TOP_CARD,         //!< Havoc.
    DOUBLE_BLOCK,          //!< Entrench.
    DOUBLE_STRENGTH,       //!< Limit Break.
    INCREASE_SELF_DAMAGE,  //!< Rampage, Glass Knife.
    DISCARD_CARDS,         //!< Prepared, All-Out Attack, Concentrate.
    DISCARD_HAND,          //!< Unload, Storm of Steel, Calculated Gamble.
    MULTIPLY_TARGET_POWER,  //!< Catalyst.
    DRAW_UNTIL,            //!< Expertise.
    ADD_RANDOM_SKILL,      //!< Distraction.
    SETUP_CARD,            //!< Setup.
    REMEMBER_CARD,         //!< Nightmare.
    CHANNEL_ORB,           //!< Zap, Cold Snap, Chaos.
    EVOKE_ORB,             //!< Dualcast, Recursion, Multi-Cast.
    EVOKE_ALL_ORBS,        //!< Fission.
    ADD_ORB_SLOTS,         //!< Capacitor, Consume.
    ADD_RANDOM_POWER,      //!< White Noise.
    ADD_RANDOM_COMMON,     //!< Hello World.
    DOUBLE_ENERGY,         //!< Double Energy.
    REMOVE_BLOCK,          //!< Melter.
    RETURN_FROM_DISCARD,   //!< Hologram, All for One.
    DRAW_TO_HAND_FROM_TOP, //!< Seek.
    RESHUFFLE_ALL,         //!< Reboot.
    EXHAUST_FOR_ENERGY,    //!< Recycle.
    INCREASE_SELF_BLOCK,   //!< Genetic Algorithm, Steam Barrier.
    INCREASE_CLAW_DAMAGE,  //!< Claw.
    REDUCE_SELF_COST,      //!< Streamline.
    ADD_RANDOM_CARD,       //!< Jack of All Trades, Transmutation.
    SET_HAND_COST,         //!< Enlightenment, Madness.
    TAKE_FROM_DRAW_BY_TYPE,  //!< Secret Technique, Secret Weapon, Violence.
    HEAL_PERCENT,            //!< Blood Potion.
    REMOVE_ALL_ORBS,         //!< Fission.
    TRIGGER_DARK_ORBS,       //!< Darkness+.
    OBTAIN_POTION            //!< Alchemize.
};

//! Which cards an effect that touches the hand is allowed to take.
enum class CardFilter
{
    ANY = 0,
    NON_ATTACK,       //!< Second Wind, Sever Soul.
    ATTACK_OR_POWER,  //!< Dual Wield.
    ATTACK_ONLY,      //!< Secret Weapon, Violence.
    SKILL_ONLY        //!< Secret Technique.
};

//! Where an effect reads its amount from, when it is not a plain number.
enum class ValueSource
{
    FIXED = 0,
    CURRENT_BLOCK,      //!< Body Slam.
    STRENGTH_MULTIPLE,  //!< Heavy Blade: extra copies of Strength.
    STRIKE_COUNT,       //!< Perfected Strike: extra per Strike owned.
    CARDS_EXHAUSTED,    //!< Fiend Fire, Second Wind: per card exhausted.
    UNBLOCKED_DAMAGE,   //!< Reaper: the health actually taken off.
    CARDS_DISCARDED,    //!< Storm of Steel: per card discarded.
    ATTACKS_PLAYED,     //!< Finisher: per attack played this turn.
    SKILLS_IN_HAND,     //!< Flechettes: per skill still held.
    ENERGY_SPENT,       //!< Malaise, Doppelganger: the X that was paid.
    ORB_COUNT,          //!< Barrage: per orb in orbit.
    ORB_TYPES,          //!< Compile Driver: per kind of orb in orbit.
    DISCARD_PILE_SIZE,  //!< Stack.
    DRAW_PILE_SIZE,     //!< Aggregate.
    FROST_CHANNELED,    //!< Blizzard: per Frost channelled this battle.
    LIGHTNING_CHANNELED,  //!< Thunder Strike: per Lightning this battle.
    ENEMY_COUNT           //!< Chill: per enemy still standing.
};

//! An extra requirement on a single effect.
enum class EffectCondition
{
    NONE = 0,
    TARGET_VULNERABLE,  //!< Dropkick.
    TARGET_ATTACKING,   //!< Spot Weakness.
    KILLED_TARGET,      //!< Feed.
    TARGET_POISONED,    //!< Bane.
    TARGET_WEAK,        //!< Heel Hook.
    DISCARDED_THIS_TURN,  //!< Sneaky Strike.
    DREW_SKILL,         //!< Escape Plan.
    PLAYER_HAS_NO_BLOCK,  //!< Auto Shields.
    NO_ATTACKS_IN_HAND,   //!< Impatience.
    FEW_CARDS_PLAYED      //!< FTL: fewer than the threshold played this turn.
};

//! An extra requirement on playing the card at all.
enum class PlayCondition
{
    NONE = 0,
    HAND_ALL_ATTACKS,  //!< Clash.
    DRAW_PILE_EMPTY    //!< Grand Finale.
};

//! How a card's cost changes during the battle.
enum class CostModifier
{
    NONE = 0,
    HEALTH_LOST_THIS_BATTLE,  //!< Blood for Blood: cheaper as health goes.
    HEALTH_LOST_RAISES_COST,  //!< Masterful Stab: dearer as health goes.
    CARDS_DISCARDED_THIS_TURN,  //!< Eviscerate.
    POWERS_PLAYED_THIS_BATTLE   //!< Force Field.
};

//! Extra rules a card carries. Combined with the operators below.
enum class CardFlag : unsigned int
{
    NONE = 0u,
    EXHAUST = 1u << 0,     //!< Leaves the battle after being played.
    ETHEREAL = 1u << 1,    //!< Exhausts if it is still in hand at turn end.
    INNATE = 1u << 2,      //!< Starts the battle in hand.
    RETAIN = 1u << 3,      //!< Is not discarded at the end of the turn.
    UNPLAYABLE = 1u << 4,  //!< Cannot be played at all.
    SENTINEL = 1u << 5,    //!< Gives energy when exhausted.
    LOSE_ENERGY_ON_DRAW = 1u << 6  //!< Void.
};

constexpr CardFlag operator|(CardFlag lhs, CardFlag rhs)
{
    return static_cast<CardFlag>(static_cast<unsigned int>(lhs) |
                                 static_cast<unsigned int>(rhs));
}

constexpr bool HasFlag(CardFlag flags, CardFlag flag)
{
    return (static_cast<unsigned int>(flags) &
            static_cast<unsigned int>(flag)) != 0u;
}

//! What a monster telegraphs for its next turn.
enum class Intent
{
    UNKNOWN = 0,
    ATTACK,
    DEFEND,
    BUFF,
    DEBUFF,

    //! The ones that do two things at once, which is what the spire shows a
    //! climber deciding what to block.
    ATTACK_DEBUFF,
    ATTACK_DEFEND,
    ATTACK_BUFF,
    DEFEND_BUFF,

    //! Standing there, and walking away.
    STUN,
    ESCAPE
};

//! Where the battle currently is.
enum class BattlePhase
{
    NOT_STARTED = 0,
    PLAYER_TURN,
    MONSTER_TURN,
    WON,
    LOST
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_BATTLE_ENUMS_HPP
