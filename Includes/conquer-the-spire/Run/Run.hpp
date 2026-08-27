// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_RUN_HPP
#define CONQUER_THE_SPIRE_RUN_HPP

#include <conquer-the-spire/Battle/Battle.hpp>
#include <conquer-the-spire/Enums/PotionId.hpp>
#include <conquer-the-spire/Enums/RelicId.hpp>
#include <conquer-the-spire/Map/Map.hpp>
#include <conquer-the-spire/Events/EventLibrary.hpp>
#include <conquer-the-spire/Monsters/EncounterLibrary.hpp>
#include <conquer-the-spire/Rewards/RewardGenerator.hpp>
#include <conquer-the-spire/Run/RunLog.hpp>
#include <conquer-the-spire/Shops/Shop.hpp>
#include <conquer-the-spire/Models/Player.hpp>

#include <cstddef>
#include <random>
#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief Run class.
//!
//! One climb up the spire. It holds what a battle does not: the map, where the
//! player is on it, the deck between fights, the gold, and the relics and
//! potions that carry over. A battle is handed a copy of the player and what
//! it changed is copied back when it is over.
//!
class Run
{
 public:
    //! How much of the maximum health a rest site gives back.
    static constexpr int REST_HEAL_PERCENT = 30;

    //! What a question mark starts out being likely to hold, and by how much
    //! each of them climbs while it goes unseen.
    static constexpr int MONSTER_CHANCE = 10;
    static constexpr int SHOP_CHANCE = 3;
    static constexpr int TREASURE_CHANCE = 2;

    //! The gold a run opens with.
    static constexpr int STARTING_GOLD = 99;

    //! Opens a run with \p character. \p seed lays out the map, so the same
    //! seed gives the same act.
    //! An empty run, for reading a save into.
    Run() = default;

    Run(CardColor character, unsigned int seed = 0);

    //! Writes the whole run out as text: the state of the dice included, so
    //! that a run read back carries on exactly as it would have.
    std::string Serialize() const;

    //! Reads a run back over this one. Returns false when the text is not a
    //! save of this shape of the engine.
    bool Load(const std::string& text);

    //! Returns what this climb did, line by line, with the counts that go
    //! with it.
    const RunLog& GetLog() const;

    //! Takes \p amount off the climber's health, a point less if they are
    //! carrying a tungsten rod. The rod takes a point off every loss of
    //! health there is, and a toll at a bridge or a hand pushed into a pool
    //! is a loss of health - those were taking the full amount.
    void LoseHealth(int amount);

    //! Writes a line of the log, at wherever the climber is standing. This is
    //! open to whoever is driving the run, for the things only they see: a
    //! potion drunk in a fight, say.
    void Note(LogEntry entry, int id, int extra = 0);

    //! Returns the character this run is climbing with.
    CardColor GetCharacter() const;

    //! Returns the act laid out for this run.
    const Map& GetMap() const;

    Player& GetPlayer();
    const Player& GetPlayer() const;

    //! Returns how many places have been walked to, so zero before the run
    //! steps onto the map and ROWS + 1 once it reaches the boss.
    int GetFloor() const;

    //! Returns the column the run is standing in, or -1 before it starts.
    int GetColumn() const;

    //! Returns what waits where the run is standing.
    MapNodeType GetCurrentNodeType() const;

    int GetGold() const;
    void AddGold(int amount);

    //! Spends \p amount and returns false when there is not that much.
    bool SpendGold(int amount);

    //! Returns the columns the run may walk to next.
    std::vector<int> GetAvailableColumns() const;

    //! Walks to \p column of the next row, or to the boss from the last one.
    //! Returns false when no path goes there. A climber in wing boots may set
    //! \p ignorePaths and step anywhere, three times in a run.
    bool Travel(int column, bool ignorePaths = false);

    //! Returns how many times the wing boots can still ignore a path.
    int GetPathSkips() const;

    //! Returns true once the run is standing in front of the boss.
    bool IsAtBoss() const;

    //! Returns true once the boss has been seen off.
    bool IsFinished() const;

    //! Notes that the boss has been seen off.
    void FinishBoss();

    //! Builds a started battle against \p monsters from the run's player.
    Battle StartBattle(std::vector<Monster> monsters);

    //! Builds the fight that waits where the run is standing, picking the
    //! group the same way the spire does: the opening fights come from the
    //! weak list, the rest from the strong one.
    Battle StartBattleHere();

    //! Returns the group the last call to StartBattleHere() picked.
    const Encounter& GetCurrentEncounter() const;

    //! Returns how many fights the run has walked into.
    int GetFightCount() const;

    //! Copies the health and the potions a battle changed back into the run,
    //! then pays out the relics that heal once a fight is over. When the fight
    //! was won it also rolls what it hands over, which is then waiting in
    //! GetRewards().
    void FinishBattle(const Battle& battle);

    //! Starts act \p act: a map of it is laid out, the climber is put back
    //! at the bottom of it, the fights start counting again, and the rewards
    //! go back to what an act starts them at.
    void BeginAct(int act);

    //! Returns which act the run is climbing.
    int GetAct() const;

    //!
    //! \brief UnknownOdds struct.
    //!
    //! How likely a question mark is to turn out to be each of the things it
    //! can be, out of a hundred. Whatever is left over is a room with
    //! something to decide in it.
    //!
    struct UnknownOdds
    {
        int monster = MONSTER_CHANCE;
        int shop = SHOP_CHANCE;
        int treasure = TREASURE_CHANCE;
    };

    //! Returns how likely each of them is just now. Anything a question mark
    //! does not turn out to be gets likelier by its own step, and goes back
    //! to where it started once it is seen.
    UnknownOdds GetUnknownOdds() const;

    //! Walks on to the next act once its boss is down. The door of the last
    //! act only opens for a climber carrying all three keys, so this returns
    //! false when one is missing.
    bool AdvanceAct();

    //! Whether there is an act above this one that the climber may walk into.
    //! False at the top of the spire, and false at the third act's door
    //! without the three keys to open it.
    //!
    //! This is what says a climb is finished rather than merely over, so the
    //! answer has to be the same one AdvanceAct gives - a climb that is paid
    //! for finishing the spire and not written down as having finished it is
    //! two different notions of winning under one name.
    bool CanClimbHigher() const;

    //! Gives up the relic waiting at \p index for the key \p key, which is
    //! the trade the last two elites and the last chest offer. Returns false
    //! when there is no relic there to give up.
    bool TakeKeyInsteadOf(std::size_t index, KeyType key);

    //! Takes the ruby key at a rest site, instead of resting or smithing.
    void RecallKey();

    //! Takes the key \p key, which is what a climber gives up a reward for.
    void TakeKey(KeyType key);

    bool HasKey(KeyType key) const;
    bool HasAllKeys() const;

    //! Opens the chest that waits where the run is standing, rolling its size.
    //! Returns the size it turned out to be.
    ChestSize OpenChest();

    //! Returns what is waiting to be claimed.
    const std::vector<Reward>& GetRewards() const;

    //! Returns true while something is still waiting to be claimed.
    bool HasUnclaimedRewards() const;

    //! Takes the reward at \p index. \p option names which of the choices is
    //! wanted, for the rewards that offer more than one. Returns false when
    //! there is nothing there, it has been taken already, or the belt is full.
    bool ClaimReward(std::size_t index, std::size_t option = 0);

    //! Turns down the reward at \p index. A card turned down with a Singing
    //! Bowl to hand raises the maximum health instead.
    bool SkipReward(std::size_t index);

    //! Clears whatever is left on the pile, which is what walking on does.
    void ClearRewards();

    //! Returns the books the rewards are rolled from, for a look at how likely
    //! the next potion is and what relics are left.
    const RewardGenerator& GetRewardGenerator() const;

    //! Opens the room that waits where the run is standing, drawing one that
    //! this run has not had yet. Returns what is in it.
    const Event& StartEvent();

    //! Opens the room \p id, whichever one the run is standing in. A room
    //! that turns up once in a run counts as had.
    const Event& StartEvent(EventId id);

    //! Builds the four blessings that come before the first step.
    const Event& StartNeow();

    //! Returns the room the run is standing in.
    const Event& GetEvent() const;

    //! Returns true while a room still has something to offer.
    bool HasEvent() const;

    //! Returns true when the option at \p index can be taken: the gold is
    //! there, and the deck holds whatever it asks for.
    bool CanChooseEventOption(std::size_t index) const;

    //! Takes the option at \p index. \p picks names the cards the option
    //! works on, in the order its effects ask for them; anything it does not
    //! name is picked at random, so an option can always be taken blind.
    bool ChooseEventOption(std::size_t index,
                           const std::vector<std::size_t>& picks = {});

    //! Returns true while a fight a room started is waiting to be had.
    bool HasPendingFight() const;

    //! Starts the fight a room picked. What winning it hands over comes
    //! through FinishBattle().
    Battle StartPendingBattle();

    //! Opens the shop that waits where the run is standing and lays out its
    //! shelf. Returns what is on it.
    const Shop& OpenShop();

    //! Returns the shop last opened.
    const Shop& GetShop() const;

    //! Walks out of the shop. Whatever was left on the shelf is written down
    //! as looked at and left, which is what a pick rate is counted from.
    void CloseShop();

    //! Buys the card at \p index off the shelf and puts it in the deck.
    //! Returns false when the slot is empty or the gold is not there.
    bool BuyCard(std::size_t index);

    //! Buys the relic at \p index off the shelf.
    bool BuyRelic(std::size_t index);

    //! Buys the potion at \p index off the shelf. Returns false when the
    //! belt is full, and the gold stays put.
    bool BuyPotion(std::size_t index);

    //! Pays the merchant to take the card at \p deckIndex out of the deck.
    //! One card per shop, and the price goes up with every one bought.
    bool BuyCardRemoval(std::size_t deckIndex);

    //! Returns what taking a card out will cost at the next shop, before any
    //! discount a relic brings.
    int GetCardRemovalPrice() const;

    //! Heals at a rest site. Returns false for a climber carrying a coffee
    //! dripper, who cannot rest at all.
    bool Rest();

    //! Takes a card out of the deck at a rest site, which is what a peace
    //! pipe is for.
    bool Toke(std::size_t index);

    //! Digs a relic up at a rest site, which is what a shovel is for.
    //! Returns what was dug up.
    RelicId Dig();

    //! Lifts at a rest site for a point of strength in every fight to come,
    //! which is what a girya is for and only three times over.
    bool Lift();

    //! Returns how many times the girya has been lifted.
    int GetLifts() const;

    //! Bottles the card at \p index, so that every fight opens with it in
    //! hand. Returns false when the card is not of a kind a bottle in hand
    //! will hold.
    bool BottleCard(std::size_t index);

    //! Upgrades the deck card at \p index, which is the other thing a rest
    //! site is for.
    bool Smith(std::size_t index);

    //! Returns the deck the run carries between fights.
    const std::vector<Card>& GetDeck() const;

    void AddCardToDeck(Card card);

    //! Takes the deck card at \p index out of the run for good.
    bool RemoveCardFromDeck(std::size_t index);

    //! Hands over the relic \p id, along with the maximum health a few of them
    //! carry.
    void AddRelic(RelicId id);

    //! Puts the potion \p id in the belt.
    bool AddPotion(PotionId id);

    //! Returns whether a potion would be kept if one were handed over. A
    //! full belt turns it away, and so does a Sozu; anything offering one
    //! has to ask first, or the offer is one that cannot be taken.
    bool CanAddPotion() const;

    //! Throws the potion at \p index away, which is what a full belt asks
    //! for.
    bool DiscardPotion(std::size_t index);

    //! Drinks the potion at \p index between fights. Only the three that are
    //! as good on the map as in a fight can be drunk here; the rest return
    //! false and stay in the belt.
    bool DrinkPotion(std::size_t index);

    //! Returns true when the card in slot \p index may be taken out of the
    //! deck, and when a whetstone would change it. Whatever draws up the
    //! moves on offer asks these, so that nothing is offered that would then
    //! be turned down.
    bool IsRemovable(std::size_t index) const;
    bool IsUpgradeable(std::size_t index) const;

    //! Writes down the cards that could have been chosen instead of the one
    //! in \p slot and were not - one line for each different card, not for
    //! each copy, so that the rate reads as how often that card is the one
    //! picked when it is there to pick.
    void NoteCardsLeft(LogEntry entry, std::size_t slot);

    //! Returns true when the potion at \p index can be drunk right here.
    bool CanDrinkPotion(std::size_t index) const;

 private:
    //! Whether answering a room this way would hand over a curse. A chance
    //! counts: the option put the curse on the table whether or not the roll
    //! goes on to land it in the deck.
    static bool OptionCurses(const EventOption& option);

    //! Puts \p room in front of the climber, setting up whatever it keeps
    //! count of.
    const Event& OpenRoom(Event room);

    //! Pays out whatever taking the relic \p id brings with it.
    void OnRelicTaken(RelicId id);

    //! Pays out whatever walking into the room the climber now stands in
    //! brings with it.
    void OnFloorEntered();

    //! Settles what the question mark the climber is standing on turns out to
    //! be, writing it into the map.
    void ResolveUnknownRoom();

    //! Hands over \p reward, whatever kind it is. The one that calls this
    //! says where it came from first.
    bool ClaimRewardNow(Reward& reward, std::size_t option);

    //! Writes down everything \p reward was offering that was left where it
    //! was. \p taken is the choice that was made, or the size of the list
    //! when none was.
    void NotePassedOver(const Reward& reward, std::size_t taken);

    //! Says where whatever happens next came from, and puts it back.
    LogSource Blame(LogSource source);

    //! Heals \p amount, unless the climber carries something that says no.
    void Heal(int amount);

    //! Returns a deck slot that may be touched, or the size of the deck.
    std::size_t RandomDeckSlot();

    //! Turns the card at \p index into another of the character's, sharpened
    //! when \p upgrade is set.
    void TransformAt(std::size_t index, bool upgrade);

    //! Sharpens \p count random cards of \p type, or of any type when it is
    //! invalid.
    void UpgradeRandomOfType(CardType type, int count);

    //! Bottles the first card of \p type the deck holds.
    void BottleFirstOfType(CardType type);

    //! Carries out one thing an option does. \p cursor walks along \p picks
    //! as the effects ask for cards.
    void ResolveEventEffect(const EventEffect& effect,
                            const std::vector<std::size_t>& picks,
                            std::size_t& cursor);

    //! Returns the deck slot an effect should work on: the next one named in
    //! \p picks, or one at random when nothing is named. Returns the size of
    //! the deck when there is nothing it may touch.
    std::size_t PickDeckSlot(const std::vector<std::size_t>& picks,
                             std::size_t& cursor, bool upgradeable);

    //! Rolls a card of \p color and \p rarity. An invalid rarity rolls one
    //! the way a fight does.
    CardId RollEventCard(CardColor color, CardRarity rarity);

    CardColor m_character = CardColor::INVALID;
    std::mt19937 m_rng;
    Map m_map;
    Player m_player;
    int m_floor = 0;
    int m_column = -1;
    int m_gold = STARTING_GOLD;
    bool m_bossDone = false;
    int m_fights = 0;
    Encounter m_encounter;

    //! The plain fights just had, newest first, so that the same one does not
    //! come round again within two of itself. Only ever two long.
    std::vector<std::string> m_lately;
    RewardGenerator m_rewardGenerator;
    std::vector<Reward> m_rewards;
    Shop m_shop;
    bool m_shopOpen = false;
    int m_removalPrice = Shop::FIRST_REMOVAL_PRICE;
    int m_act = 1;

    //! What the relics that watch the climbing keep count of: whether the
    //! bank is still paying out, how many paths the boots can still ignore,
    //! how many curses the charm can still turn away, how often the girya has
    //! been lifted, and how many question marks have been walked into.
    bool m_mawBankOpen = true;
    int m_monsterChance = MONSTER_CHANCE;
    int m_shopChance = SHOP_CHANCE;
    int m_treasureChance = TREASURE_CHANCE;
    int m_pathSkips = 0;
    int m_curseWards = 0;
    int m_lifts = 0;
    int m_questionRooms = 0;
    RunLog m_log;

    //! What whatever is happening just now is to be blamed on.
    LogSource m_source = LogSource::UNKNOWN;
    std::vector<KeyType> m_keys;
    Event m_event;
    std::vector<EventId> m_seenEvents;
    Encounter m_pendingEncounter;
    bool m_pendingFight = false;
    RelicId m_pendingPrize = RelicId::INVALID;

    //! Which option of the room is being carried out, which is what a skull
    //! charges by.
    std::size_t m_eventOption = 0;

    //! Set by an effect that closes the room it is in, such as the ooze
    //! finally handing something over.
    bool m_eventEnds = false;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_RUN_HPP
