// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_BATTLE_HPP
#define CONQUER_THE_SPIRE_BATTLE_HPP

#include <conquer-the-spire/Enums/BattleEnums.hpp>
#include <conquer-the-spire/Models/Card.hpp>
#include <conquer-the-spire/Models/Monster.hpp>
#include <conquer-the-spire/Models/Player.hpp>
#include <conquer-the-spire/Relics/RelicRegistry.hpp>

#include <cstddef>
#include <map>
#include <random>
#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief Battle class.
//!
//! Runs one fight from start to finish. The flow of a single round is:
//!
//! 1. Start of the player turn: block expires unless Barricade holds it,
//!    poison ticks, the turn start powers fire, energy refills and a fresh
//!    hand is drawn.
//! 2. The caller plays cards through PlayCard() until it runs out of energy or
//!    decides to stop.
//! 3. EndTurn(): cards that hurt while held resolve, ethereal cards exhaust,
//!    the hand is discarded, the turn end powers fire and the player's debuffs
//!    decay. Then every living monster takes its turn.
//! 4. Unless somebody died, the next round starts.
//!
//! Nothing here blocks or prompts, so the same battle can drive a console
//! front end or a reinforcement learning agent. The random number generator is
//! seeded from the constructor, so a battle replays identically.
//!
class Battle
{
 public:
    //! How deep a card that plays another card may go, so that a Havoc that
    //! keeps hitting Havoc cannot run away.
    static constexpr int MAX_PLAY_DEPTH = 8;

    //! Constructs a battle between \p player and \p monsters. \p seed fixes
    //! every shuffle, so the same seed replays the same battle.
    Battle(Player player, std::vector<Monster> monsters, unsigned int seed = 0);

    //! Shuffles the deck, draws the innate cards and opens the first player
    //! turn. Does nothing if the battle already started.
    void Start();

    //! Plays the card at \p handIndex, aimed at the monster at \p monsterIndex
    //! when the card needs a single enemy. \p choiceIndex is the card a few
    //! cards need to be pointed at - the card Armaments upgrades, the one Dual
    //! Wield copies, the one Exhume brings back - and indexes the pile that
    //! card comes from. Returns false and changes nothing when the play is not
    //! legal.
    bool PlayCard(std::size_t handIndex, std::size_t monsterIndex = 0,
                  std::size_t choiceIndex = 0);

    //! Plays the card at \p handIndex with a whole list of chosen cards
    //! behind it, for the cards that work on as many as are named. The
    //! numbers are places in the pile the card picks out of, as it stands
    //! once the card being played has left the hand.
    bool PlayCard(std::size_t handIndex, std::size_t monsterIndex,
                  const std::vector<std::size_t>& choices);

    //! Drinks the potion at \p index, aimed at the monster at \p monsterIndex
    //! when the potion needs one. Returns false and changes nothing when the
    //! potion cannot be drunk here: wrong phase, bad index, dead target, or a
    //! potion whose work happens outside a battle.
    bool UsePotion(std::size_t index, std::size_t monsterIndex = 0,
                   std::size_t choiceIndex = 0);

    //! Drinks the potion at \p index with a whole list of named cards behind
    //! it, for the potions that work on as many as are named.
    bool UsePotion(std::size_t index, std::size_t monsterIndex,
                   const std::vector<std::size_t>& choices);

    //! Returns true when the potion at \p index can be drunk right now at
    //! \p monsterIndex. A potion that is only of use outside a fight, or one
    //! aimed at something that is not there, cannot.
    bool CanUsePotion(std::size_t index, std::size_t monsterIndex = 0) const;

    //! Ends the player turn, runs the monster turn and opens the next player
    //! turn. Returns false if it is not the player's turn.
    bool EndTurn();

    //! Returns where the battle currently is.
    BattlePhase GetPhase() const;

    //! Returns true once the battle has been won or lost.
    bool IsDone() const;

    //! Returns the current round, counting from one.
    int GetTurn() const;

    Player& GetPlayer();
    const Player& GetPlayer() const;

    //! Whether this fight holds a boss, or an elite.
    bool IsBossFight() const;
    bool IsEliteFight() const;

    //! Returns false while a runic dome keeps the climber from seeing what
    //! the monsters mean to do. The intents are still there to be read; this
    //! says whether they are meant to be looked at.
    bool AreIntentsVisible() const;

    //! Returns true when the draw pile may be read in order, which is what a
    //! frozen eye is for.
    bool IsDrawPileOrdered() const;

    //! Throws \p count cards away and draws as many back, which is the offer
    //! a gambling chip makes at the start of a fight. Returns false once the
    //! offer has been taken, or when there was never one.
    bool Gamble(int count);

    //! Returns true when the climber walked out of this fight rather than
    //! winning it, which is what a smoke bomb is for. A fight walked out of
    //! leaves nothing behind.
    bool WasEscaped() const;

    //! Returns the gold this fight turned up that the purse has not seen
    //! yet: what a Hand of Greed took off whatever it finished off. The run
    //! holds the purse and reads this when the fight is over.
    int GetGoldFound() const;

    //! Returns how much gold the thieves of this fight got away with. What a
    //! thief was carrying when it was killed does not count.
    int GetGoldStolen() const;

    std::vector<Monster>& GetMonsters();
    const std::vector<Monster>& GetMonsters() const;

    //! Returns the energy \p card costs right now, after Corruption, a cost
    //! forced for this turn and the card's own cost modifier. An X cost
    //! reports all the energy that is left.
    int GetEffectiveCost(const Card& card) const;

    //! Returns true if \p card asks for one of the climber's own cards to be
    //! picked out before it can do anything: an Armaments wants to know what
    //! to sharpen, a Warcry what to put back, an Exhume what to fetch. What
    //! PlayCard() is handed as its choice is that card's place in a pile.
    static bool NeedsCardChoice(const Card& card);
    static bool NeedsCardChoice(const Potion& potion);

    //! Returns where \p card picks from. Only worth asking when
    //! NeedsCardChoice() says yes.
    static ChoiceSource ChoiceSourceOf(const Card& card);
    static ChoiceSource ChoiceSourceOf(const Potion& potion);
    static ChoiceSource ChoiceSourceOf(
        const std::vector<CardEffect>& effects);

    //! Rolls up the handful \p card holds out, for a card that offers cards
    //! rather than picking among ones the climber already has. Must be called
    //! before the cards on offer can be seen or picked.
    void RollOffer(const Card& card);
    void RollOffer(const Potion& potion);

 private:
    void RollOffer(const Card& card,
                   const std::vector<CardEffect>& effects);

 public:

    //! Returns the handful last rolled up, which is empty except while one is
    //! being picked from.
    const std::vector<CardId>& GetOffered() const;

    //! Returns how many cards \p card has to pick from right now, which is
    //! how many of them are worth offering.
    std::size_t ChoiceCount(const Card& card) const;
    std::size_t ChoiceCount(const Potion& potion) const;
    std::size_t ChoiceCount(ChoiceSource source) const;

    //! Returns true if \p card works on as many of the picked cards as are
    //! named rather than on one of them.
    static bool ChoiceTakesMany(const Card& card);
    static bool ChoiceTakesMany(const Potion& potion);
    static bool ChoiceTakesMany(const std::vector<CardEffect>& effects);

    //! Returns true if PlayCard() would accept this play.
    //! Whether \p handIndex may be played at \p monsterIndex. Left out, the
    //! question is asked of the first monster that may be aimed at rather
    //! than of whoever stands first - which while a darkling lay at the front
    //! of the room answered no for every card in hand, and that answer is
    //! written into the state.
    static constexpr std::size_t ANY_MONSTER =
        static_cast<std::size_t>(-1);

    bool CanPlay(std::size_t handIndex,
                 std::size_t monsterIndex = ANY_MONSTER) const;

    //! Returns the indices of the monsters that are still alive.
    std::vector<std::size_t> GetLivingMonsterIndices() const;

    //! Who may be aimed at, which is not everybody who is in the room: a
    //! darkling lying there waiting to be pulled back cannot be. The state
    //! and the ordinals a move names are built from the other list, so that
    //! nothing shifts places while one of them is down.
    std::vector<std::size_t> GetTargetableMonsterIndices() const;

    //! Returns the hand indices that PlayCard() would currently accept. Handy
    //! for enumerating legal moves.
    std::vector<std::size_t> GetPlayableCardIndices() const;

    //! How many times each card was actually played this fight, and how many
    //! turns each was left holding when the turn was handed over. What a card
    //! is worth on paper and what it does in a hand are different questions,
    //! and only these two answer the second.
    const std::map<CardId, int>& GetPlayedCounts() const;
    const std::map<CardId, int>& GetStrandedCounts() const;

    //! The cards a monster put into the deck for keeps rather than into a
    //! pile of this fight. A parasite is not swept away with the rest of the
    //! fight's litter, so the climb takes these up when the fight ends.
    const std::vector<Card>& GetKeptCards() const;

    //! Returns how many times the player has lost health this battle, which is
    //! what Blood for Blood reads.
    int GetHealthLossCount() const;

    //! Returns how many cards the player has played this turn.
    int GetCardsPlayedThisTurn() const;

    //! Returns how many cards the player has discarded this turn, which is
    //! what Eviscerate and Sneaky Strike read.
    int GetCardsDiscardedThisTurn() const;

    //! Puts an orb of \p type into orbit. When there is no room the oldest orb
    //! is evoked to make some. An invalid type channels a random orb, which is
    //! what Chaos does.
    void ChannelOrb(OrbType type);

    //! Evokes the orb at the front \p times over, then lets it go.
    void EvokeFrontOrb(int times);

    //! Evokes every orb in orbit, front first.
    void EvokeAllOrbs();

    //! Returns what an orb of \p type is worth right now, after Focus.
    //! \p evoking asks for the bigger number an orb gives on the way out.
    int OrbPower(OrbType type, bool evoking) const;

 private:
    // Flow

    void BeginPlayerTurn();
    void EndPlayerTurn();
    void RunMonsterTurn();
    void UpdatePhase();

    // Playing cards

    //! Runs \p card after it has left the hand and been paid for.
    //! Set when something ends the turn before the climber meant to, which
    //! is what eating time does.
    bool m_turnCutShort = false;

    //! Whether the gambling chip has been used, and whether the attack a
    //! necronomicon doubles has come up this turn.
    bool m_gambleSpent = false;
    bool m_escaped = false;
    bool m_fairySpent = false;
    bool m_necronomiconSpent = false;

    void ResolvePlayedCard(Card card, std::size_t monsterIndex,
                           std::size_t choiceIndex, int energySpent);

    //! Runs the effect list of \p card. \p card is not const because Rampage
    //! writes its growing damage back into the copy that goes to the discard
    //! pile.
    void ResolveCardEffects(Card& card, Monster* target,
                            std::size_t choiceIndex, int energySpent);

    //! Runs a single effect of \p card.
    void ResolveEffect(const CardEffect& effect, Card& card, Monster* target,
                       std::size_t choiceIndex, int energySpent);

    //! Returns the amount \p effect works with, following its value source.
    int ResolveValue(const CardEffect& effect, const Card& card,
                     int energySpent) const;

    //! Returns true when the condition on \p effect holds.
    bool ConditionHolds(const CardEffect& effect, const Monster* target) const;

    //! Returns the monsters \p effect hits.
    std::vector<Monster*> GetEffectTargets(const CardEffect& effect,
                                           const Card& card, Monster* target);

    //! Returns the monster \p card is aimed at, or null when it needs none.
    Monster* ResolveTarget(const Card& card, std::size_t monsterIndex);

    //! Returns the first monster that is still alive, or null.
    Monster* FirstLivingMonster();

    // Triggers

    //! \brief What the "whenever you play a card" powers held before the card
    //!        being played had a chance to change them.
    //!
    //! A card that grants one of these powers must not set it off itself, so
    //! the amounts are read before the card resolves and used afterwards.
    struct PlayTriggers
    {
        int afterImage = 0;
        int thousandCuts = 0;
        int storm = 0;
        int heatsinks = 0;
        std::vector<int> choked;
    };

    //! Reads the powers that react to a card being played.
    PlayTriggers ReadPlayTriggers() const;

    void OnCardPlayed(const Card& card, const PlayTriggers& before);
    void OnCardDrawn(CardId id, CardType type, CardFlag flags);
    void OnCardExhausted(Card card);
    void OnCardDiscarded(Card card);
    void OnMonsterDied(Monster& monster);

    // Actions the effects and the monsters go through

    void DrawCards(int count);
    void ApplyOrbEvoke(const Orb& orb);
    void TriggerOrbPassive(Orb& orb);
    void TriggerOrbPassives();
    void TriggerPlasmaPassives();
    void DealOrbDamage(Monster& monster, int amount);

    //! What a thing that shifts gives up when it loses \p lost health, by
    //! whatever means. The page says upon losing HP, not upon being struck,
    //! so poison and an orb and a card that costs health all count.
    void NoteShifting(Creature& creature, int lost);
    void DealOrbDamageToTarget(int amount);
    Monster* LowestHealthMonster();

    //! Plays the card on top of the draw pile for nothing, then exhausts it,
    //! which is what Havoc and Mayhem do.
    void PlayTopCardOfDrawPile();

    //! Runs whatever the carried relics do when \p hook comes round.
    void FireRelics(RelicHook hook);

    //! Runs the effects a relic or a potion hands over, which have no card
    //! behind them. \p target is what the effects aim at, and \p color is the
    //! pool the ones that hand over a card draw from.
    void ResolveEffectsWithoutCard(const std::vector<CardEffect>& effects,
                                   std::size_t choiceIndex,
                                   CardTarget target, CardColor color,
                                   Monster* aimedAt);

    //! Runs the effects a relic hands over.
    void ResolveRelicEffects(const std::vector<CardEffect>& effects);

    //! Returns the pool a card handing over another card draws from.
    CardColor PoolColor(const CardEffect& effect, const Card& card) const;
    void DiscardCards(int count, bool random, std::size_t choiceIndex);

    //! Throws away every card the climber named, from the back forwards so
    //! that taking one out does not move the next along. \p exhaust says
    //! whether they burn or go to the discard pile, and it returns how many
    //! went - which is what a Gambler's Brew draws again.
    int ThrowAwayNamed(bool exhaust);
    void DiscardWholeHand(CardFilter filter);
    void GainBlock(int amount);
    void DealDamageToMonster(Monster& monster, int base, bool fromAttack);
    void DealDamageToPlayer(int base, Monster& source);
    void DealFlatDamage(Creature& creature, int amount);
    void DamageAllEnemies(int amount);
    void DamageRandomEnemy(int amount);
    void PlayerLoseHealth(int amount, bool fromCard);
    void PutCardInStasis(Monster& monster);
    Card TakeStasisCardFrom(std::vector<Card>& pile);
    void ApplyPowerTo(Creature& creature, PowerType power, int amount);
    void TickPoison(Creature& creature);
    void DecayTimedPowers(Creature& creature);
    //! Heals the climber by \p amount, unless something says no.
    void HealPlayer(int amount);

    //! Fills whatever room the belt has with potions rolled at random. A
    //! fight will not pour a fruit juice; the map will.
    void FillPotionBelt(bool includeJuice);

    //! Reads what \p monster is allowed to know when it picks its move.
    MoveContext ReadMoveContext(const Monster& monster) const;

    void ResolveMonsterMove(Monster& monster);

    //! Runs one step of a monster move.
    void ResolveMonsterEffect(const MonsterEffect& effect, Monster& monster);

    //! Checks the rules a monster carries of its own: splitting when it is
    //! halved, waking up, changing shape.
    void CheckMonsterRules(Monster& monster);

    //! Puts the monsters a split promised onto the field.
    void ApplyPendingSpawns();

    //! Keeps back the cards Well-Laid Plans holds on to, so the discard at the
    //! end of the turn leaves them in hand.
    void RetainPlannedCards();

    // Cards that do something while they are held

    void ResolveEndOfTurnHandCards();
    void ExhaustEtherealCards();
    void ClearTurnCosts();

    //! Returns how many cards named Strike the player owns, counting the card
    //! being played, which is what Perfected Strike reads.
    int CountStrikeCards(const Card& played) const;

    //! Returns the count the per unit value sources multiply, such as the
    //! cards exhausted this play or the attacks played this turn.
    int CountUnits(ValueSource source) const;

    Player m_player;
    std::vector<Monster> m_monsters;
    std::mt19937 m_rng;
    BattlePhase m_phase = BattlePhase::NOT_STARTED;
    int m_turn = 0;

    //! Counters the effects read. They are reset per play, not per turn.
    int m_cardsExhaustedThisPlay = 0;
    int m_cardsDiscardedThisPlay = 0;
    int m_unblockedDamageThisPlay = 0;
    bool m_killedTargetThisPlay = false;

    int m_healthLossCount = 0;
    int m_frostChanneled = 0;
    int m_lightningChanneled = 0;
    int m_powersPlayedThisBattle = 0;
    int m_panacheCounter = 0;
    int m_bombDamage = 0;

    //! What the relics that only fire once, or watch the turn before, need to
    //! remember.
    bool m_akabekoSpent = false;
    bool m_puzzleSpent = false;
    bool m_lizardTailSpent = false;
    bool m_kiteSpentThisTurn = false;
    bool m_pelletsSpentThisTurn = false;
    bool m_playedAttackThisTurn = false;
    bool m_playedSkillThisTurn = false;
    bool m_playedPowerThisTurn = false;
    bool m_playedAttackLastTurn = false;
    int m_lastShuffleCount = 0;

    //! The monsters a split has promised, put on the field once the current
    //! action is over so that nothing holds a stale pointer.
    std::vector<Monster> m_pendingSpawns;
    int m_cardsPlayedThisTurn = 0;

    //! Gold this fight has turned up that the purse has not seen yet.
    int m_goldFound = 0;

    //! Every card named for the card being played, for the one step that
    //! works on more than one of them. Empty the rest of the time, and the
    //! single choice is used instead.
    std::vector<std::size_t> m_choices;

    //! The handful a card is holding out to be picked from. Empty except
    //! while one is being picked.
    std::vector<CardId> m_offered;

    //! Puts a copy of every Pride still in hand on top of the draw pile,
    //! which is what holding one costs.
    void ResolvePrideInHand();

    //! Counted per card, and copied along with the rest of the fight - so a
    //! fight simulated by a search counts into its own copy and leaves the
    //! real one alone.
    std::map<CardId, int> m_playedCounts;
    std::vector<Card> m_kept;

    //! Whether a hexaghost's inferno has been through: from then on every
    //! burn the fight makes is the worse kind.
    bool m_burnsStoked = false;
    std::map<CardId, int> m_strandedCounts;
    int m_cardsDiscardedThisTurn = 0;
    int m_attacksPlayedThisTurn = 0;
    int m_playDepth = 0;

    //! What the last card drawn was, which is what Escape Plan looks at.
    CardType m_lastDrawnType = CardType::INVALID;

    //! The card Nightmare remembered, and how many copies it owes.
    CardId m_rememberedCard = CardId::INVALID;
    int m_rememberedCopies = 0;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_BATTLE_HPP
