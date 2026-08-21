// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_EVENT_HPP
#define CONQUER_THE_SPIRE_EVENT_HPP

#include <conquer-the-spire/Enums/CardId.hpp>
#include <conquer-the-spire/Enums/EventEnums.hpp>
#include <conquer-the-spire/Enums/MonsterEnums.hpp>
#include <conquer-the-spire/Enums/RelicEnums.hpp>
#include <conquer-the-spire/Enums/RelicId.hpp>
#include <conquer-the-spire/Enums/BattleEnums.hpp>

#include <string>
#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief EventEffect struct.
//!
//! One thing that happens when an option is taken. The named constructors
//! read the way the room does: EventEffect::Gold(75), EventEffect::Damage(11).
//!
struct EventEffect
{
    EventEffectType type = EventEffectType::NONE;

    //! The amount, or the low end of a range.
    int amount = 0;

    //! The high end of a range, when the amount is one.
    int high = 0;

    //! A share of the maximum health, out of a hundred.
    int percent = 0;

    //! How many cards or potions it is about.
    int count = 1;

    CardId card = CardId::INVALID;
    CardColor color = CardColor::INVALID;
    CardRarity rarity = CardRarity::INVALID;
    RelicId relic = RelicId::INVALID;
    RelicTier tier = RelicTier::INVALID;

    //! What waits when an option turns into a fight, and what winning it
    //! hands over.
    std::vector<MonsterId> monsters;
    RelicId prize = RelicId::INVALID;
    RelicTier prizeTier = RelicTier::INVALID;

    //! The relics an option may hand over one of, and the type of card it
    //! works on.
    std::vector<RelicId> relics;
    CardType cardType = CardType::INVALID;

    static EventEffect Gold(int amount);
    static EventEffect GoldRange(int low, int high);
    static EventEffect LoseGold(int low, int high);
    static EventEffect LoseAllGold();
    static EventEffect Heal(int amount);
    static EventEffect HealPercent(int percent);
    static EventEffect HealFull();
    static EventEffect Damage(int amount);
    static EventEffect DamagePercent(int percent);
    static EventEffect GainMaxHealth(int amount);
    static EventEffect LoseMaxHealth(int amount);
    static EventEffect LoseMaxHealthPercent(int percent);
    static EventEffect DamageCurrentPercent(int percent);
    static EventEffect GainRelic(RelicId id);
    static EventEffect RandomRelic();
    static EventEffect RandomRelic(RelicTier tier);
    static EventEffect BossSwap();
    static EventEffect GainCurse(CardId id);
    static EventEffect RandomCurse();
    static EventEffect GainCard(CardId id);

    //! Hands over \p count copies of \p id.
    static EventEffect GainCards(CardId id, int count);

    //! Hands over the curse \p id \p chance times in a hundred.
    static EventEffect MaybeCurse(CardId id, int chance);
    static EventEffect RandomCards(int count, CardColor color,
                                   CardRarity rarity);
    static EventEffect CardReward(int count, CardColor color,
                                  CardRarity rarity);
    static EventEffect Potions(int count);
    static EventEffect RemoveCards(int count = 1);
    static EventEffect UpgradeCards(int count = 1);
    static EventEffect UpgradeRandom(int count);
    static EventEffect TransformCards(int count = 1);
    static EventEffect DuplicateCard();
    static EventEffect CleanseCurses();
    static EventEffect LosePotion();
    static EventEffect LoseCard();
    static EventEffect BurnOffering();
    static EventEffect Fight(std::vector<MonsterId> monsters,
                             RelicId prize = RelicId::INVALID);

    //! A fight whose prize is a relic of \p tier rather than a named one.
    static EventEffect FightFor(std::vector<MonsterId> monsters,
                                RelicTier tier);

    static EventEffect RemoveRandomOfType(CardType type);
    static EventEffect UpgradeAll();
    static EventEffect UpgradeAllBasic();

    //! Takes every card of \p type out and hands over \p count copies of
    //! \p card, which is the bargain the vampires offer.
    static EventEffect ReplaceAllOfType(CardType type, CardId card,
                                        int count);

    //! Gives up \p id, or any relic at all when it is invalid.
    static EventEffect LoseRelic(RelicId id);

    //! Hands over one of \p choices.
    static EventEffect OneOfRelics(std::vector<RelicId> choices);

    //! Pays what a skull asks of whoever keeps asking.
    static EventEffect SkullToll();

    //! Stakes \p stake gold on something that comes in \p chance times in a
    //! hundred and pays \p payout.
    static EventEffect Wager(int stake, int chance, int payout);

    static EventEffect ToTheBoss();
    static EventEffect FightOldBoss();
    static EventEffect Spin();
    static EventEffect Reach();
    static EventEffect Search();
    static EventEffect TradeFace();
};

//!
//! \brief EventOption struct.
//!
//! One line the room offers. \p nextStage is where the event goes on choosing
//! it: -1 closes the room, and its own number keeps it open, which is how the
//! ooze and the dead adventurer let a climber press their luck.
//!
struct EventOption
{
    std::string label;
    std::vector<EventEffect> effects;
    EventRequirement requirement = EventRequirement::NONE;

    //! What the requirement is measured against, and what taking the option
    //! costs in gold.
    int requirementValue = 0;
    int goldCost = 0;

    //! The relic an option wants before it can be taken.
    RelicId requirementRelic = RelicId::INVALID;

    int nextStage = -1;

    EventOption() = default;
    EventOption(std::string label, std::vector<EventEffect> effects,
                int nextStage = -1);

    //! Asks for \p gold before the option can be taken, and takes it.
    EventOption& Costs(int gold);

    //! Asks for \p requirement before the option can be taken.
    EventOption& Needs(EventRequirement requirement, int value = 0);

    //! Asks for the relic \p id before the option can be taken.
    EventOption& NeedsRelic(RelicId id);
};

//!
//! \brief Event class.
//!
//! A room with something in it. An event can hold more than one stage: the
//! idol drops a boulder once it is off its pedestal, and the ooze lets a
//! climber reach in again.
//!
class Event
{
 public:
    Event() = default;
    Event(EventId id, std::string name, EventKind kind);

    //! Adds a stage of options. The first one added is where the room opens.
    void AddStage(std::vector<EventOption> options);

    EventId GetId() const;
    const std::string& GetName() const;
    EventKind GetKind() const;

    //! Returns the options of the stage the room is at.
    const std::vector<EventOption>& GetOptions() const;

    int GetStage() const;

    //! Returns true once the room has nothing more to offer.
    bool IsDone() const;

    //! Moves to \p stage, or closes the room when it is below zero.
    void GoTo(int stage);

    //! Returns how many times the option that can be taken again has been.
    int GetTries() const;
    void CountTry();

    //! Counts how often each option of the stage has been taken, which is
    //! what a skull charges by.
    int GetOptionTries(std::size_t index) const;
    void CountOption(std::size_t index);

    //! The outcomes an event deals out one at a time, in the order they were
    //! shuffled into.
    std::vector<int>& GetBag();
    const std::vector<int>& GetBag() const;

 private:
    EventId m_id = EventId::INVALID;
    std::string m_name;
    EventKind m_kind = EventKind::INVALID;
    std::vector<std::vector<EventOption>> m_stages;
    std::vector<EventOption> m_none;
    std::vector<int> m_bag;
    std::vector<int> m_optionTries;
    int m_stage = 0;
    int m_tries = 0;
    bool m_done = false;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_EVENT_HPP
