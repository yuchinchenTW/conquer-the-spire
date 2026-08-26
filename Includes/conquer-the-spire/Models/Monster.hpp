// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_MONSTER_HPP
#define CONQUER_THE_SPIRE_MONSTER_HPP

#include <conquer-the-spire/Enums/MonsterEnums.hpp>
#include <conquer-the-spire/Models/Card.hpp>
#include <conquer-the-spire/Models/Creature.hpp>

#include <cstddef>
#include <random>
#include <string>
#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief MonsterEffect struct.
//!
//! One step of a monster move. A move is a list of these, so a move that hits
//! and debuffs, or blocks and buffs, needs no special case.
//!
struct MonsterEffect
{
    //! Hits the player for \p amount, \p times times.
    static MonsterEffect Damage(int amount, int times = 1);

    //! Hits the player \p times times, each for their health divided by
    //! \p divisor and one more, which is how Hexaghost opens.
    static MonsterEffect DamageByPlayerHealth(int divisor, int times);

    //! Blocks \p amount for itself.
    static MonsterEffect Block(int amount);

    //! Blocks \p amount for another monster, or itself when it is alone.
    static MonsterEffect BlockAlly(int amount);

    //! Applies \p amount of \p power to itself.
    static MonsterEffect Buff(PowerType power, int amount);

    //! Applies \p amount of \p power to the player.
    static MonsterEffect Debuff(PowerType power, int amount);

    //! Puts \p count copies of \p id into the player's discard pile.
    static MonsterEffect AddCard(CardId id, int count = 1,
                                 bool upgraded = false);

    //! Applies \p amount of \p power to every monster still standing.
    static MonsterEffect BuffAll(PowerType power, int amount);

    //! Blocks \p amount for the others, which is what a leader shouting
    //! orders does for its gremlins.
    static MonsterEffect BlockAllies(int amount);

    //! Heals every monster still standing by \p amount.
    static MonsterEffect HealAll(int amount);

    //! Calls in \p count more of \p id, so long as no more than \p cap of
    //! them are already about.
    static MonsterEffect Summon(MonsterId id, int count, int cap);

    //! Takes a card out of the player's piles until this monster dies.
    static MonsterEffect Stasis();

    //! Throws off every debuff standing on the monster. What a Champ does
    //! when he stops fighting fair: everything the climber spent on slowing
    //! him down goes at once.
    static MonsterEffect ShakeOff();

    //! Steps aside for \p first and \p second, each with the health this
    //! monster has left.
    static MonsterEffect Split(MonsterId first, MonsterId second);

    //! Comes back with \p percent of its health, which is what a darkling
    //! that has been put down does while one of its own still stands.
    static MonsterEffect Revive(int percent);

    //! Goes off for \p amount and goes with it.
    static MonsterEffect SelfDestruct(int amount);

    //! Walks out of the fight.
    static MonsterEffect Escape();

    MonsterEffectType type = MonsterEffectType::NOTHING;
    int amount = 0;
    int times = 1;
    PowerType power = PowerType::INVALID;
    bool toPlayer = false;
    CardId cardId = CardId::INVALID;
    bool upgradedCard = false;
    MonsterId splitFirst = MonsterId::INVALID;
    MonsterId splitSecond = MonsterId::INVALID;

    //! What a summon calls in, and how many of them there may be at once.
    MonsterId summon = MonsterId::INVALID;
    int cap = 0;
};

//!
//! \brief MoveContext struct.
//!
//! What a monster is allowed to know when it picks its next move: which turn
//! it is, how many of its fellows are still standing, how badly hurt the
//! worst of them is, and which phase of the fight it is in.
//!
struct MoveContext
{
    int turn = 1;
    int allies = 0;
    int allyMissing = 0;
    int phase = 1;
};

//!
//! \brief MonsterMove struct.
//!
//! One thing a monster can decide to do, and the intent the player gets to see
//! beforehand. \p weight is how often the move comes up when the monster picks
//! at random; a weight of zero keeps it out of the draw, which is where the
//! moves reached only by a rule of the monster's own sit.
//!
struct MonsterMove
{
    //! A move that hits for \p damage, \p times times.
    static MonsterMove Attack(std::string name, int damage, int times = 1);

    //! A move that blocks.
    static MonsterMove Defend(std::string name, int block);

    //! A move that hits and blocks in one go.
    static MonsterMove AttackAndDefend(std::string name, int damage, int block);

    //! A move that buffs itself.
    static MonsterMove Buff(std::string name, PowerType power, int amount,
                            int block = 0);

    //! A move that debuffs the player.
    static MonsterMove Debuff(std::string name, PowerType power, int amount);

    //! A move built out of \p effects, for everything else.
    static MonsterMove Of(std::string name, Intent intent,
                          std::vector<MonsterEffect> effects);

    //! A move that does nothing, which is what a monster charging up shows.
    static MonsterMove Nothing(std::string name,
                               Intent intent = Intent::UNKNOWN);

    //! Sets how often this move comes up, and how many times in a row it may
    //! be used at most. A limit of zero means no limit.
    MonsterMove& Chance(int weight, int maxInARow = 0);

    //! Marks this move as the one the monster always opens with.
    MonsterMove& Opener();

    //! Keeps this move out of the opening turn.
    MonsterMove& NotFirst();

    //! Makes this the move of every \p turns th turn, whatever the weights
    //! say, which is how the Champ finds time to taunt.
    MonsterMove& Every(int turns);

    //! Counts \p Every from the turn the monster changed phase rather than
    //! from the first turn of the fight. A Champ executes the turn after he
    //! stops fighting fair and every third turn from there, which has nothing
    //! to do with how long the fight had been going when he turned.
    MonsterMove& SincePhase();

    //! Hands this move's share to \p other when it may not be repeated,
    //! rather than leaving it to be shared out among everything else.
    //!
    //! A Champ that has just gloated does not gloat again, and the fifteen it
    //! would have had goes to the face slap and nowhere else - so a face slap
    //! at twenty-five becomes one at forty while the rest stand still. Sharing
    //! it out instead moved every other share a little, which is a different
    //! monster.
    MonsterMove& SpillsTo(const std::string& other);

    //! Allows this move \p many times in a fight, and turns it into \p other
    //! after that. A Champ may take his stance twice and gloats instead from
    //! then on.
    MonsterMove& AtMost(int many, const std::string& other);

    //! Makes this the move of turn \p turn exactly.
    MonsterMove& OnTurn(int turn);

    //! Keeps this move to one phase of the fight.
    MonsterMove& InPhase(int phase);

    //! Keeps this move for when the monster is the last one standing, or for
    //! when it is not.
    MonsterMove& Alone();
    MonsterMove& WithAlly();

    //! Only while fewer than \p many allies are standing, and only while at
    //! least \p many are. What a Collector may do depends on how many of her
    //! torch heads are alive, which is a count rather than the yes or no that
    //! Alone and WithAlly ask.
    MonsterMove& WhenAlliesUnder(int many);
    MonsterMove& WhenAlliesAtLeast(int many);

    //! Makes this the move whenever one of the monsters is missing \p amount
    //! of health or more, which is what a healer waits for.
    MonsterMove& WhenAllyMissing(int amount);

    std::string name;
    Intent intent = Intent::UNKNOWN;
    std::vector<MonsterEffect> effects;
    int weight = 0;
    int maxInARow = 0;
    bool opener = false;
    bool notFirst = false;
    int everyTurns = 0;
    int onTurn = 0;
    int phase = 0;
    //! Whether Every counts from the turn the phase changed.
    bool sincePhase = false;

    //! Who gets this move's share when it may not be repeated, and who it
    //! turns into once it has been used as often as it is allowed.
    std::string spillsTo;
    std::string insteadAfter;

    //! How often it may be used in one fight, and how often it has been.
    int atMost = 0;
    int used = 0;

    bool alone = false;
    bool withAlly = false;

    //! How many allies standing this move wants, above and below. Nought
    //! either way asks nothing.
    int alliesUnder = 0;
    int alliesAtLeast = 0;
    int allyMissing = 0;
};

//!
//! \brief Monster class.
//!
//! An enemy. A monster either walks a fixed script, which keeps a battle
//! reproducible for a test, or picks its next move at random from the weights
//! on its moves while respecting how often each may repeat.
//!
class Monster : public Creature
{
 public:
    Monster() = default;

    //! Constructs a monster that walks \p moveScript in order. When
    //! \p loopMoves is false it stays on the last move once it gets there.
    Monster(std::string name, int maxHealth,
            std::vector<MonsterMove> moveScript, bool loopMoves = true,
            std::size_t loopFrom = 0);

    //! Constructs a monster that picks from \p moves by their weights.
    Monster(MonsterId id, std::string name, MonsterType type, int maxHealth,
            std::vector<MonsterMove> moves);

    //! Records which monster this is and what kind of fight it belongs to,
    //! for the ones built from a fixed pattern.
    void SetIdentity(MonsterId id, MonsterType type);

    //! Returns which monster this is, for the ones the library builds.
    MonsterId GetMonsterId() const;

    //! Returns what kind of fight this monster belongs to.
    MonsterType GetMonsterType() const;

    //! Returns the move this monster will make on its next turn.
    const MonsterMove& GetCurrentMove() const;

    //! Returns the intent of the move this monster will make next.
    Intent GetIntent() const;

    //! Returns every move this monster knows.
    const std::vector<MonsterMove>& GetMoves() const;

    //! Steps to the next move: along the script, or picked from the weights.
    //! \p context is what the monster is allowed to know about the fight.
    void AdvanceMove(std::mt19937& rng);
    void AdvanceMove(std::mt19937& rng, const MoveContext& context);

    //! Picks the move this monster opens the fight with.
    void ChooseOpeningMove(std::mt19937& rng);
    void ChooseOpeningMove(std::mt19937& rng, const MoveContext& context);

    //! Forces the next move to be the one named \p name, which is how a rule
    //! of a monster's own - a split, a wake-up, a phase change - overrides the
    //! usual choice. Returns false when there is no such move.
    bool ForceMove(const std::string& name);

    //! Whether this monster is down but not out, waiting to come back.
    bool IsRegrowing() const;
    void SetRegrowing(bool regrowing);

    //! How much more health this monster can lose this turn, for the one
    //! that can only be brought down so far at a time.
    int GetDamageCapLeft() const;
    void SetDamageCapLeft(int amount);

    //! Returns true once this monster has walked out of the fight.
    bool HasEscaped() const;

    //! Notes that this monster has left.
    void MarkEscaped();

    //! Returns true when this monster is out of the fight, dead or fled.
    bool IsGone() const;

    //! Which phase of its fight this monster is in. A boss that changes its
    //! ways part way through moves itself on.
    int GetPhase() const;
    void SetPhase(int phase);

    //! Writes down that the move standing has been used once more, so that a
    //! move allowed only so often in a fight knows when it has had its turns.
    void CountMoveUsed();

    //! Returns whether the move at \p at is the same move as the one
    //! standing, which is a question about its name and not about where it
    //! sits in the list.
    bool SameMoveAs(std::size_t other) const;

    //! Returns whether the move at \p at could be made right now: allowed by
    //! the company and the phase, not just made, and not out of its turns.
    bool MoveDrawable(std::size_t at, const MoveContext& context) const;

    //! Returns who ends up with the share of the move at \p at, following the
    //! redirects until one lands somewhere it can actually be drawn.
    std::size_t HeirOfMove(std::size_t at, const MoveContext& context) const;

    //! Returns where \p name sits in the list, or the size of it.
    std::size_t IndexOfMove(const std::string& name) const;

    //! Returns how many moves the monster had made when its phase last
    //! changed.
    int GetPhaseTurn() const;

    //! How much flight and how much malleable armour this monster goes back
    //! to at the start of a turn.
    int GetFlightBase() const;
    void SetFlightBase(int amount);
    int GetMalleableBase() const;
    void SetMalleableBase(int amount);

    //! How much gold this monster has taken off the climber. It keeps it if
    //! it walks away, and drops it if it is killed.
    int GetStolenGold() const;
    void StealGold(int amount);

    //! The card this monster is keeping in stasis, if any.
    bool HasStasisCard() const;
    const Card& GetStasisCard() const;
    void HoldStasisCard(Card card);
    Card ReleaseStasisCard();

 private:
    //! Returns the index of a move picked by weight, honouring the repeat
    //! limits, the opening rules and whatever \p context rules out.
    std::size_t PickWeightedMove(std::mt19937& rng,
                                 const MoveContext& context) const;

    //! Returns true when \p move is one this monster may make at all in
    //! \p context.
    bool MoveAllowed(const MonsterMove& move,
                     const MoveContext& context) const;

    MonsterId m_id = MonsterId::INVALID;
    MonsterType m_type = MonsterType::NORMAL;
    std::vector<MonsterMove> m_moves;
    std::size_t m_moveIndex = 0;
    bool m_scripted = true;
    bool m_loopMoves = true;
    std::size_t m_loopFrom = 0;
    bool m_escaped = false;
    int m_movesMade = 0;
    int m_sameMoveRun = 0;
    int m_phase = 1;

    //! The turn the phase last changed, so that a move counting turns from
    //! there has something to count from.
    int m_phaseTurn = 0;
    int m_flightBase = 0;
    int m_malleableBase = 0;
    int m_stolenGold = 0;
    bool m_regrowing = false;
    int m_damageCapLeft = 0;
    Card m_stasisCard;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_MONSTER_HPP
