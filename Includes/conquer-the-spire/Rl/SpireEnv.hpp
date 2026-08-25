// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_SPIRE_ENV_HPP
#define CONQUER_THE_SPIRE_SPIRE_ENV_HPP

#include <conquer-the-spire/Run/Run.hpp>
#include <conquer-the-spire/Run/RunStats.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace ConquerTheSpire
{
//! Where a run is standing, as far as an agent is concerned. Each phase
//! offers its own handful of moves and nothing else.
enum class EnvPhase
{
    INVALID = 0,

    //! Choosing where to walk next.
    MAP,

    //! In a fight.
    BATTLE,

    //! Standing over what a fight or a chest left.
    REWARD,

    //! In a room with something to decide.
    EVENT,

    //! At the merchant.
    SHOP,

    //! At a rest site.
    REST,

    //! At the top of an act, with the boss ahead.
    BOSS,

    //! The boss is down and the next act is waiting.
    ACT_DONE,

    //! The climber is dead, or the spire is done with.
    OVER,

    //! A card has been played that wants one of the climber's own cards
    //! picked out for it, and nothing else can happen until it is. Last in
    //! the list because the numbers are written into saved climbs and read
    //! back out of them by position.
    CHOOSING
};

//! What an agent can do. \p a and \p b are what the move needs: a hand slot
//! and a monster, a reward and which of its choices, a card to work on.
//!
//! A monster is named by where it stands among the ones still alive, not by
//! its slot in the fight, so that a slime splitting does not move everything
//! along. The state lists the living monsters in the same order.
enum class ActionKind
{
    INVALID = 0,
    TRAVEL,
    PLAY_CARD,
    END_TURN,
    USE_POTION,
    DISCARD_POTION,
    CLAIM_REWARD,
    SKIP_REWARD,
    LEAVE_REWARDS,
    CHOOSE_OPTION,
    BUY_CARD,
    BUY_RELIC,
    BUY_POTION,
    BUY_REMOVAL,
    LEAVE_SHOP,
    REST,
    SMITH,
    TOKE,
    DIG,
    LIFT,
    LEAVE_REST,
    FIGHT_BOSS,
    NEXT_ACT,

    //! Picks the card a played card was asking about. Last, so that every
    //! move that was already numbered keeps its number.
    CHOOSE_CARD,

    //! Says that is all of them, for a card that takes as many as are named.
    CHOOSE_DONE,

    //! How many kinds of move there are, for anything that has to keep a
    //! name for each of them.
    COUNT
};

//! Returns the name of \p kind, or nothing at all when there is no such
//! kind.
//!
//! Asked of the engine rather than kept in a list beside it: a list of names
//! read by position falls quietly out of step the moment a kind is added, and
//! a move whose name is not known is a move nothing can be said about. That
//! is what happened when the card-picking move was added and the reader
//! carried on calling it invalid.
const char* NameOfActionKind(ActionKind kind);

//!
//! \brief Action struct.
//!
struct Action
{
    ActionKind kind = ActionKind::INVALID;
    int a = 0;
    int b = 0;

    Action() = default;
    Action(ActionKind kind, int a = 0, int b = 0);
};

//!
//! \brief StepResult struct.
//!
struct StepResult
{
    //! Whether the move was a legal one at all.
    bool taken = false;

    //! What the move was worth, by the shaping below.
    float reward = 0.0f;

    //! Whether the run is over, either way.
    bool done = false;
};

//!
//! \brief SpireEnv class.
//!
//! A climb, wrapped up the way a learner wants it: one phase at a time, a
//! list of the moves that are legal right now, a step that takes one of them,
//! and a flat vector of numbers for the state. Nothing here reads a file or
//! prints anything, and the same seed always gives the same climb.
//!
class SpireEnv
{
 public:
    //! What a step is worth. A climber is paid for climbing, for the harder
    //! fights, and for the spire itself; and charged for the health it costs.
    static constexpr float FLOOR_REWARD = 1.0f;
    static constexpr float ELITE_REWARD = 5.0f;
    static constexpr float BOSS_REWARD = 20.0f;
    static constexpr float WIN_REWARD = 100.0f;
    static constexpr float DEATH_REWARD = -20.0f;
    static constexpr float HEALTH_WEIGHT = 0.05f;

    //! What a point of the health ceiling is worth, taken off or handed
    //! over.
    //!
    //! Health lost is health a fire can put back. A ceiling taken off is
    //! gone for the rest of the climb, and takes a little off every fire
    //! after it as well, so a point of it is worth at least what a point of
    //! health is worth - this is the floor of its price, not the ceiling of
    //! it. Charged and paid both ways, so that the five the fish offers is
    //! worth something and the third of a climber the vampires ask for
    //! costs something.
    static constexpr float MAX_HEALTH_WEIGHT = 0.05f;

    //! What a curse in the deck costs for every floor walked with it.
    //!
    //! The deck says which of its cards are curses, what holding one costs a
    //! turn and that it can never be played, so the learner can see them -
    //! but seeing is not paying. What a curse actually costs is a draw
    //! wasted, a fight gone worse and a floor not reached, which arrives
    //! late and mixed in with everything else; charged by the floor it
    //! arrives at once, and stops the moment the card is torn out.
    static constexpr float CURSE_A_FLOOR = 0.2f;

    //! What a point of health taken off actually costs this climb. It starts
    //! at HEALTH_WEIGHT and can be moved: how dearly health is held against
    //! how far the climb gets is the one number in here worth arguing about,
    //! so it is worth being able to try another.
    void SetHealthWeight(float weight);
    float GetHealthWeight() const;

    //! What a point of the health ceiling is worth. Nought charges nothing
    //! for one, which is how the climbs before this were scored: the option
    //! said plainly that it wanted a third of the ceiling, and nothing was
    //! ever taken for it.
    void SetMaxHealthWeight(float weight);
    float GetMaxHealthWeight() const;

    //! What a curse in the deck costs a floor. Nought charges nothing, which
    //! is how the climbs before this were scored.
    void SetCursePenalty(float penalty);
    float GetCursePenalty() const;

    //! How many monsters of a fight the state has room for, and how many
    //! cards of a hand, potions of a belt, rewards of a pile, options of a
    //! room, and cards of a deck a move may name.
    static constexpr std::size_t OBSERVED_MONSTERS = 8;
    static constexpr std::size_t HAND_SLOTS = 10;
    static constexpr std::size_t POTION_SLOTS = 5;
    static constexpr std::size_t REWARD_SLOTS = 6;
    static constexpr std::size_t REWARD_OPTIONS = 20;
    static constexpr std::size_t EVENT_OPTIONS = 6;
    static constexpr std::size_t DECK_SLOTS = 40;

    //! How many cards a played card may be offered to pick from. As deep as
    //! the deck, because a discard pile late in a fight is deeper than a
    //! hand.
    static constexpr std::size_t CHOICE_SLOTS = DECK_SLOTS;
    static constexpr std::size_t SHOP_CARD_SLOTS = 7;
    static constexpr std::size_t SHOP_RELIC_SLOTS = 3;
    static constexpr std::size_t SHOP_POTION_SLOTS = 3;
    static constexpr std::size_t MAP_COLUMNS = 7;

    //! What a hand slot says besides which card it is: what it costs, whether
    //! it is sharpened, and whether it can be played right now.
    static constexpr std::size_t HAND_EXTRAS = 3;

    //! How many choices of a reward the state shows. A pile with more than
    //! this - a library with its twenty books - shows the first few.
    static constexpr std::size_t OBSERVED_OPTIONS = 4;

    //! How many relics the id vector has room for.
    static constexpr std::size_t OBSERVED_RELICS = 25;

    SpireEnv() = default;

    //! Starts a climb of \p character laid out by \p seed.
    void Reset(CardColor character, unsigned int seed);

    EnvPhase GetPhase() const;
    const Run& GetRun() const;
    Run& GetRun();

    //! Returns the fight going on, or nullptr when there is none.
    const Battle* GetBattle() const;

    //! Returns every move that is legal right now, which doubles as the mask
    //! an agent needs.
    std::vector<Action> LegalActions() const;

    //! Takes \p action. An illegal move changes nothing and is reported as
    //! not taken, so that a mask can be checked or ignored.
    StepResult Step(const Action& action);

    //! Returns the state as a flat vector of numbers, mostly between zero and
    //! one. The layout is fixed and its size is ObservationSize().
    std::vector<float> Observe() const;

    static std::size_t ObservationSize();

    //! Ends a climb once \p acts of the spire have been cleared, or 0 to
    //! climb the whole thing. Kept over a reset: it is how the climb is set
    //! up rather than anything about the one going on.
    //!
    //! A learner walking one act at a time needs the climb to end when that
    //! act does. Taking the move that walks on off the table from outside
    //! does not do it: the climb would stand at the top with a move it is
    //! not allowed to make, going nowhere until it is called off.
    void SetActLimit(int acts);
    int GetActLimit() const;

    //! How many moves a climb is given before it is called off. No climb of
    //! the spire comes near this: it is there so that one that cannot get
    //! out of a room takes a batch of its own with it instead of sitting in
    //! a row of climbs for ever.
    static constexpr int MOVE_LIMIT = 3000;

    //!
    //! \brief Layout struct.
    //!
    //! Where each part of the state starts, so that whatever is reading it
    //! can slice it up without counting.
    //!
    struct Layout
    {
        std::size_t phase = 0;
        std::size_t run = 0;
        std::size_t deck = 0;
        std::size_t relics = 0;
        std::size_t battle = 0;
        std::size_t powers = 0;
        std::size_t monsters = 0;
        std::size_t hand = 0;
        std::size_t piles = 0;
        std::size_t total = 0;

        //! Where the parts added for the rooms outside a fight begin: what
        //! is on the reward pile, on the shelf, on offer in a room, in the
        //! belt, and what the monsters mean to do.
        std::size_t rewards = 0;
        std::size_t shop = 0;
        std::size_t event = 0;
        std::size_t potions = 0;
        std::size_t moves = 0;

        //! Where what is on offer begins: every card a pile is holding
        //! out, and every card on the shelf, with the same figures the deck
        //! carries. Kept in blocks of their own so that each is a run of
        //! numbers belonging to one card, which is what a learner reading
        //! by the card needs.
        //!
        //! Without these a card being chosen was an id and nothing else -
        //! what it costs and what it does only reached the state once it was
        //! already in the deck, which is after the choosing is over.
        std::size_t offers = 0;
        std::size_t shopCards = 0;

        //! Where the deck begins: what sits in each of its slots, which is
        //! what an action naming a slot is about. Which card it is comes
        //! from the ids beside the state.
        std::size_t deckCards = 0;

        //! Which card is doing the asking, and what it is worth. Being picked
        //! out by an Armaments is a card being sharpened and being picked out
        //! by a Burning Pact is a card being burnt; without this the state
        //! held out a row of cards to choose between and did not say what
        //! choosing one of them would do to it.
        std::size_t asking = 0;

        //! Where the cards a played card is asking about begin. The pile is
        //! whichever one that card picks out of - a hand, a discard pile, an
        //! exhaust pile - so this is the only place the last two are written
        //! down at all. Without it a choice would be made by its position
        //! rather than by what is standing in it.
        std::size_t choices = 0;

        //! Where the map ahead begins: what stands on the places that could
        //! be walked to next, what stands on the places those lead to, and
        //! what the rest of the act still holds.
        std::size_t map = 0;

        //! How wide each of the repeating parts is.
        std::size_t monsterStride = 0;
        std::size_t handStride = 0;
        std::size_t choiceStride = 0;
        std::size_t askingStride = 0;
        std::size_t pileStride = 0;
        std::size_t rewardStride = 0;
        std::size_t eventStride = 0;
        std::size_t moveStride = 0;
        std::size_t deckStride = 0;
        std::size_t offerStride = 0;
        std::size_t shopCardStride = 0;
    };

    //!
    //! \brief IdLayout struct.
    //!
    //! Where each part of the id vector begins. These are ids, not numbers to
    //! be added up: an empty slot is zero, and the rest are meant to be looked
    //! up in a table of their own on the other side.
    //!
    struct IdLayout
    {
        std::size_t hand = 0;
        std::size_t potions = 0;
        std::size_t relics = 0;
        std::size_t rewardKinds = 0;
        std::size_t rewardOptions = 0;
        std::size_t rewardOptionKinds = 0;
        std::size_t shopCards = 0;
        std::size_t shopRelics = 0;
        std::size_t shopPotions = 0;
        std::size_t event = 0;
        std::size_t monsters = 0;

        //! Which card sits in each slot of the deck. Every action that names
        //! a slot of the deck - sharpening one, tearing one up, handing one
        //! to a room - is blind without this.
        std::size_t deck = 0;

        //! Which card is doing the asking. \see Layout::asking
        std::size_t asking = 0;

        //! Which card sits in each place a played card is asking about.
        //! \see Layout::choices
        std::size_t choices = 0;
        std::size_t total = 0;
    };

    //! What a slot of the reward options holds, so that an id can be looked up
    //! in the right table.
    static constexpr int ITEM_NONE = 0;
    static constexpr int ITEM_CARD = 1;
    static constexpr int ITEM_RELIC = 2;
    static constexpr int ITEM_POTION = 3;

    static Layout GetLayout();
    static IdLayout GetIdLayout();

    //! Returns which card, relic, potion, room and monster each slot of the
    //! state is about. These are ids for looking up or embedding, not numbers
    //! to be weighed.
    //! Returns the cards a played card is waiting to be answered about, in
    //! the order the answers number them. Empty whenever nothing is asking.
    //!
    //! For a card picked out of the hand this is the hand without the card
    //! doing the asking, because that card has left the hand by the time its
    //! own effects run and the answers are numbered against what is left.
    std::vector<CardId> ChoosableCards() const;

    //! Returns the card that is doing the asking, or nothing at all when
    //! nothing is being asked.
    CardId AskingCard() const;

    std::vector<int> ObserveIds() const;

    static std::size_t IdCount();

    //! How many moves there are, counting the ones that are not legal just
    //! now. An agent with a fixed head wants this, along with the mask.
    static std::size_t ActionCount();

    //! Turns a slot of that fixed head into a move, and back. An index with
    //! no move behind it comes back invalid, and a move that no index names
    //! comes back as ActionCount().
    static Action ActionFromIndex(std::size_t index);
    static std::size_t IndexOfAction(const Action& action);

    //! Returns one byte a move, set where the move is legal right now.
    std::vector<unsigned char> ActionMask() const;

    //! Takes the move at \p index of that fixed head.
    StepResult StepIndex(std::size_t index);

    //! What the rest of this turn costs if \p index is taken now: the health
    //! gone by the time the monsters have had their go, and whether the
    //! fight was won or lost getting there.
    //!
    //! A copy of the fight is played out, so nothing here is kept. Only a
    //! fight can be looked into - out on the map there is no turn to end -
    //! and \p cost is left alone when there is nothing to look at.
    //!
    //! \p follow decides what fills the rest of the turn after \p index:
    //! FOLLOW_NOTHING ends it there, FOLLOW_CHEAPEST keeps playing whatever
    //! is playable. Neither is how a policy would play it, so a cost read
    //! off this is a comparison between moves rather than a prediction.
    struct TurnCost
    {
        //! Whether a fight was there to look into at all.
        bool looked = false;

        //! Health gone by the end of the turn, and the health left.
        int healthLost = 0;
        int healthLeft = 0;

        //! What the monsters have left, added up, and how many still stand.
        int monsterHealth = 0;
        int monstersLeft = 0;

        //! Whether the fight ended, and which way.
        bool over = false;
        bool won = false;
    };

    static constexpr int FOLLOW_NOTHING = 0;
    static constexpr int FOLLOW_CHEAPEST = 1;

    //! Fills the rest of the fight with the rule of thumb below, and reports
    //! how the whole thing came out rather than how the turn did.
    static constexpr int FOLLOW_TO_THE_END = 2;

    TurnCost Peek(std::size_t index, int follow = FOLLOW_CHEAPEST) const;

    bool IsDone() const;

    //! Returns how many floors have been climbed over the whole run, which is
    //! what a score is usually counted in.
    int GetTotalFloors() const;

    //! What came of the choices, over every climb this has played. Starting a
    //! new climb does not clear it; ClearStats() does.
    const RunStats& GetStats() const;
    void ClearStats();

    //! Writes the climb out, for picking up later. A fight is not written
    //! out: a save is taken between rooms, and mid-fight this returns an
    //! empty string.
    std::string Save() const;
    bool Load(const std::string& text);

 private:
    //! Turns the \p ordinal th living monster into its slot in the fight.
    std::size_t TargetOf(int ordinal) const;

    //! Ends the climb and counts it into the table, once however many steps
    //! come after it. Every way out of Step() goes through here.
    void Close();

    //! Plays \p battle out to its end with a rule of thumb: block enough to
    //! cover what is coming, then spend the rest on damage. Crude on
    //! purpose - it has to run in microseconds, because a search calls it
    //! thousands of times for one decision, and asking the policy would cost
    //! four hundred times as much per move.
    static void PlayOut(Battle& battle, int turnLimit = 60);

    //! How much damage the monsters mean to do to the climber next.
    static int IncomingDamage(const Battle& battle);

    //! Walks the phase on once whatever was going on has finished.
    void Settle();

    //! Opens whatever room the climber has just walked into.
    void EnterRoom();

    Run m_run;
    std::unique_ptr<Battle> m_battle;
    EnvPhase m_phase = EnvPhase::INVALID;
    int m_totalFloors = 0;

    //! Whether the fight going on is the boss of the act, and what the
    //! climber's health was when the last step began.
    bool m_bossFight = false;
    int m_healthBefore = 0;

    //! Whether the climb going on has been counted yet, so that it is counted
    //! once however many steps come after it ended.
    bool m_counted = false;

    //! How many moves this climb has taken, which the limit above is
    //! measured against.
    int m_moves = 0;

    //! How many acts a climb is asked for, 0 being all of them.
    int m_actLimit = 0;

    //! The card played that is waiting on an answer, where it sits in the
    //! hand, and what it was aimed at. Only meaningful in CHOOSING, and never
    //! written to a saved climb: a load drops the fight altogether.
    std::size_t m_chosen = 0;
    int m_chosenTarget = 0;

    //! The cards named so far, for a card that takes as many as are named.
    //! Empty the rest of the time.
    std::vector<std::size_t> m_answers;

    //! What a point of health is worth against a floor.
    float m_healthWeight = HEALTH_WEIGHT;

    //! What a point of the health ceiling is worth against a floor.
    float m_maxHealthWeight = MAX_HEALTH_WEIGHT;

    //! What a curse in the deck costs a floor.
    float m_cursePenalty = CURSE_A_FLOOR;
    RunStats m_stats;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_SPIRE_ENV_HPP
