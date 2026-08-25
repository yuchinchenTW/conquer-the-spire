// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_CARD_HPP
#define CONQUER_THE_SPIRE_CARD_HPP

#include <conquer-the-spire/Enums/BattleEnums.hpp>
#include <conquer-the-spire/Enums/CardId.hpp>
#include <conquer-the-spire/Models/Orb.hpp>

#include <string>
#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief CardEffect struct.
//!
//! One step of what a card does. A card is a list of these, so cards stay
//! plain data instead of one class per card. Build one with a named
//! constructor and adjust it with the chained modifiers:
//!
//! \code
//! CardEffect::Damage(5).XTimes()                    // Whirlwind
//! CardEffect::Damage(14).From(ValueSource::STRENGTH_MULTIPLE, 3)
//! CardEffect::Energy(1).If(EffectCondition::TARGET_VULNERABLE)
//! \endcode
//!
struct CardEffect
{
    //! Repeat count meaning "as many times as the energy spent on an X cost".
    static constexpr int TIMES_X = -1;

    //! Deals \p value damage to the card's target, \p times times.
    static CardEffect Damage(int value, int times = 1);

    //! Deals \p value damage to every living enemy, \p times times.
    static CardEffect DamageAll(int value, int times = 1);

    //! Deals \p value damage to a random living enemy, \p times times.
    static CardEffect DamageRandom(int value, int times = 1);

    //! Gains \p value block.
    static CardEffect Block(int value);

    //! Applies \p amount stacks of \p power to the card's target.
    static CardEffect ApplyPower(PowerType power, int amount);

    //! Applies \p amount stacks of \p power to the player.
    static CardEffect ApplyPowerToSelf(PowerType power, int amount);

    //! Applies \p amount stacks of \p power to every living enemy.
    static CardEffect ApplyPowerToAll(PowerType power, int amount);

    //! Gains \p value energy this turn.
    static CardEffect Energy(int value);

    //! Draws \p value cards.
    static CardEffect Draw(int value);

    //! Loses \p value health, ignoring block.
    static CardEffect LoseHealth(int value);

    //! Restores \p value health.
    static CardEffect Heal(int value);

    //! Restores \p percent of the maximum health.
    static CardEffect HealPercent(int percent);

    //! Raises the maximum health by \p value.
    static CardEffect IncreaseMaxHealth(int value);

    //! Puts \p count new copies of \p id into \p pile.
    static CardEffect AddCard(CardId id, CardPile pile, int count = 1,
                              bool upgraded = false);

    //! Puts a copy of the card being played into the discard pile.
    static CardEffect CopySelfToDiscard();

    //! Copies a chosen card in hand \p count times.
    static CardEffect CopyHandCard(int count);

    //! Puts \p count random attacks of the player's colour into \p pile, each
    //! costing no energy this turn.
    static CardEffect AddRandomAttack(int count = 1,
                                      CardPile pile = CardPile::HAND);

    //! Upgrades a chosen card in hand, or every card when \p all is true.
    static CardEffect UpgradeHandCard(bool all = false);

    //! Upgrades one card in hand, picked at random.
    static CardEffect UpgradeRandomHandCard();

    //! Exhausts \p count cards from hand, randomly when \p random is true and
    //! by choice otherwise.
    static CardEffect ExhaustHandCard(int count, bool random);

    //! Exhausts every card in hand that passes \p filter.
    static CardEffect ExhaustHand(CardFilter filter = CardFilter::ANY);

    //! Returns a chosen card from the exhaust pile to the hand.
    static CardEffect ReturnFromExhaust();

    //! Puts a chosen card from the discard pile on top of the draw pile.
    static CardEffect DiscardToDrawTop();

    //! Puts a chosen card from the hand on top of the draw pile.
    static CardEffect HandToDrawTop();

    //! Plays the top card of the draw pile, then exhausts it.
    static CardEffect PlayTopCard();

    //! Doubles the block the player currently has.
    static CardEffect DoubleBlock();

    //! Doubles the Strength the player currently has.
    static CardEffect DoubleStrength();

    //! Raises the damage of this copy of the card for the rest of the battle.
    //! A negative amount lowers it, which is what Glass Knife does.
    static CardEffect IncreaseSelfDamage(int value);

    //! Discards \p count cards, at random when \p random is true and by
    //! choice otherwise.
    static CardEffect DiscardCards(int count, bool random);

    //! Discards every card in hand that passes \p filter.
    static CardEffect DiscardHand(CardFilter filter = CardFilter::ANY);

    //! Multiplies the \p power the target carries by \p factor, which is what
    //! Catalyst does to poison.
    static CardEffect MultiplyPower(PowerType power, int factor);

    //! Puts \p count random skills of the player's colour into \p pile, each
    //! costing no energy this turn.
    static CardEffect AddRandomSkill(int count = 1,
                                     CardPile pile = CardPile::HAND);

    //! Puts a chosen card from the hand into \p pile, costing no energy this
    //! turn. With \p manyCards it takes as many as are chosen rather than one,
    //! which is the difference a sharpened Forethought buys - a wider choice
    //! rather than a bigger number.
    static CardEffect SetupCard(CardPile pile = CardPile::DRAW_TOP,
                                bool manyCards = false);

    //! Draws until the hand holds \p size cards.
    static CardEffect DrawUntil(int size);

    //! Remembers a chosen card in hand, and hands over \p copies of it at the
    //! start of the next turn.
    static CardEffect RememberCard(int copies);

    //! Puts \p count orbs of \p type into orbit. An invalid type channels a
    //! random orb, which is what Chaos does.
    static CardEffect ChannelOrb(OrbType type, int count = 1);

    //! Evokes the orb at the front \p times over.
    static CardEffect EvokeOrb(int times = 1);

    //! Evokes every orb in orbit.
    static CardEffect EvokeAllOrbs();

    //! Evokes the orb at the front and puts another of the same kind back into
    //! orbit, which is what Recursion does.
    static CardEffect EvokeAndChannel();

    //! Lets every orb go without evoking it, which is what Fission does.
    static CardEffect RemoveAllOrbs();

    //! Sets off the passive of every Dark orb, which is what an upgraded
    //! Darkness does.
    static CardEffect TriggerDarkOrbs();

    //! Puts a random potion in the belt.
    static CardEffect ObtainPotion();

    //! Adds \p amount orb slots, or takes them away when negative.
    static CardEffect AddOrbSlots(int amount);

    //! Puts a random power of the player's colour into the hand, costing no
    //! energy this turn.
    static CardEffect AddRandomPower();

    //! Puts \p count random common cards of the player's colour into the hand.
    static CardEffect AddRandomCommon(int count);

    //! Doubles the energy left this turn.
    static CardEffect DoubleEnergy();

    //! Strips the block off the target.
    static CardEffect RemoveBlock();

    //! Returns a chosen card from the discard pile to the hand, or every card
    //! that costs nothing when \p everyZeroCost is true.
    static CardEffect ReturnFromDiscard(bool everyZeroCost = false);

    //! Moves \p count cards off the top of the draw pile into the hand.
    static CardEffect TakeFromDrawPile(int count);

    //! Shuffles hand and discard pile back into the draw pile, then draws
    //! \p draw cards.
    static CardEffect ReshuffleAll(int draw);

    //! Exhausts a chosen card in hand and hands over its cost in energy.
    static CardEffect ExhaustForEnergy();

    //! Raises the block this copy of the card gives for the rest of the
    //! battle.
    static CardEffect IncreaseSelfBlock(int value);

    //! Raises the damage of every Claw for the rest of the battle.
    static CardEffect IncreaseClawDamage(int value);

    //! Makes this copy of the card cheaper for the rest of the battle.
    static CardEffect ReduceSelfCost(int value);

    //! Puts \p count random cards of the player's colour into \p pile, each
    //! costing no energy this turn.
    static CardEffect AddRandomCard(int count,
                                    CardPile pile = CardPile::HAND);

    //! Holds out \p count random cards of the player's colour and takes the
    //! one the climber picks into the hand, costing no energy this turn.
    //! Which of them it is is a choice rather than a roll, which is the
    //! difference between a Discovery and a Jack of All Trades.
    static CardEffect OfferRandomCards(int count);

    //! Forces the cards in hand to cost \p cost. Only one card is touched,
    //! picked at random, when \p onlyOne is true. The cost goes back at the
    //! end of the turn unless \p wholeBattle, which holds it down for the
    //! rest of the fight - the difference between Enlightenment and an
    //! upgraded one.
    static CardEffect SetHandCost(int cost, bool onlyOne = false,
                                  bool wholeBattle = false);

    //! Moves \p count cards of \p type out of the draw pile into the hand.
    static CardEffect TakeFromDrawPileByType(CardType type, int count);

    //! Hands over \p amount gold if this damage is what kills the target.
    CardEffect& IfFatalGold(int amount);

    //! Repeats this effect \p count times.
    CardEffect& Times(int count);

    //! Repeats this effect once per energy spent on an X cost.
    CardEffect& XTimes();

    //! Reads the amount from \p source instead of a plain number. \p extra is
    //! the per unit amount for the sources that count something, and the
    //! multiplier for ENERGY_SPENT.
    CardEffect& From(ValueSource source, int extra = 0);

    //! Runs this effect only when \p condition holds.
    CardEffect& If(EffectCondition condition);

    //! Restricts which cards this effect may touch.
    CardEffect& Only(CardFilter filter);

    //! Sets the number a condition compares against, such as how many cards
    //! FTL allows to have been played.
    CardEffect& Threshold(int amount);

    //! Lands the whole amount as one hit rather than one per unit counted,
    //! which is what Blizzard does.
    CardEffect& AsOneHit();

    //! Hands over upgraded copies of whatever card this effect makes.
    CardEffect& Upgraded();

    //! Draws the card this effect hands over from \p color rather than from
    //! whatever played it.
    CardEffect& FromPool(CardColor color);

    EffectType type = EffectType::INVALID;
    int value = 0;
    int times = 1;
    int extra = 0;
    PowerType power = PowerType::INVALID;
    EffectTarget target = EffectTarget::DEFAULT;
    ValueSource valueSource = ValueSource::FIXED;
    EffectCondition condition = EffectCondition::NONE;
    CardFilter filter = CardFilter::ANY;
    CardId cardId = CardId::INVALID;
    CardPile pile = CardPile::DISCARD;
    OrbType orb = OrbType::INVALID;
    CardColor colorOverride = CardColor::INVALID;
    bool upgradedCard = false;
    bool randomPick = false;
    bool singleHit = false;

    //! Whether a cost forced on the hand holds for the rest of the fight
    //! rather than the turn.
    bool wholeBattle = false;

    //! Gold handed over when this damage is what kills the target, which is
    //! what a Hand of Greed is for.
    int goldIfFatal = 0;

    //! Whether this works on as many of the climber's own cards as are
    //! chosen, rather than on one.
    bool manyCards = false;
};

//!
//! \brief Card class.
//!
//! Cards are values: they are copied between the piles, and a copy carries the
//! state it picked up during the battle, such as the damage Rampage builds up.
//!
class Card
{
 public:
    //! Cost of a card whose cost is X, meaning it spends all the energy left.
    static constexpr int COST_X = -1;

    //! Cost of a card that cannot be played at all.
    static constexpr int COST_UNPLAYABLE = -2;

    //! Marks that no cost has been forced for this turn.
    static constexpr int NO_COST_OVERRIDE = -3;

    Card() = default;
    Card(CardId id, std::string name, CardColor color, CardType type,
         CardRarity rarity, CardTarget target, int cost,
         std::vector<CardEffect> effects, CardFlag flags = CardFlag::NONE);

    //! Returns the id the registry builds this card from.
    CardId GetId() const;

    //! Returns the display name, with a plus when upgraded.
    const std::string& GetName() const;

    //! Returns the character this card belongs to.
    CardColor GetColor() const;

    //! Returns the kind of this card.
    CardType GetCardType() const;

    //! Returns how often this card shows up in a run.
    CardRarity GetRarity() const;

    //! Returns what this card needs to be played at.
    CardTarget GetTarget() const;

    //! Returns the printed energy cost, or COST_X.
    int GetCost() const;

    //! Returns the steps this card runs when it resolves.
    const std::vector<CardEffect>& GetEffects() const;

    //! Returns the extra rules this card carries.
    CardFlag GetFlags() const;

    //! Returns true if this card carries \p flag.
    bool Has(CardFlag flag) const;

    //! Adds \p flag to this copy, which is how Well-Laid Plans marks the
    //! cards it holds on to.
    void AddFlag(CardFlag flag);

    //! Drops \p flag from this copy.
    void RemoveFlag(CardFlag flag);

    //! Returns how many times this card has been upgraded.
    int GetUpgradeCount() const;

    //! Returns true if this card has been upgraded at least once.
    bool IsUpgraded() const;

    //! Returns the extra requirement on playing this card.
    PlayCondition GetPlayCondition() const;

    //! Returns how this card's cost changes during a battle.
    CostModifier GetCostModifier() const;

    //! Returns the damage this copy has built up during the battle.
    int GetBonusDamage() const;

    //! Returns the block this copy has built up during the battle.
    int GetBonusBlock() const;

    //! Adds \p amount to the block this copy gives for the rest of the battle.
    void AddBonusBlock(int amount);

    //! Returns how much this copy has knocked off its own cost.
    int GetCostReduction() const;

    //! Makes this copy \p amount cheaper for the rest of the battle.
    void AddCostReduction(int amount);

    //! Adds \p amount to the damage this copy deals for the rest of the
    //! battle.
    void AddBonusDamage(int amount);

    //! Returns true if the card is the kind that can be played at all. The
    //! battle still checks energy, targets and play conditions.
    bool IsPlayable() const;

    //! Records that the registry built this card with \p upgradeCount
    //! upgrades, which also puts a plus on its name.
    void MarkUpgraded(int upgradeCount);

    //! Sets the extra requirement on playing this card.
    void SetPlayCondition(PlayCondition condition);

    //! Sets how this card's cost changes during a battle.
    void SetCostModifier(CostModifier modifier);

    //! Forces this copy to cost \p cost until the end of the turn, which is
    //! what Infernal Blade hands out.
    void SetCostThisTurn(int cost);

    //! Drops a cost forced by SetCostThisTurn().
    void ClearCostThisTurn();

    //! Returns true while a cost has been forced for this turn.
    bool HasCostThisTurn() const;

    //! Returns the cost forced for this turn, if there is one.
    int GetCostThisTurn() const;

 private:
    CardId m_id = CardId::INVALID;
    std::string m_name;
    CardColor m_color = CardColor::INVALID;
    CardType m_cardType = CardType::INVALID;
    CardRarity m_rarity = CardRarity::INVALID;
    CardTarget m_target = CardTarget::INVALID;
    int m_cost = 0;
    std::vector<CardEffect> m_effects;
    CardFlag m_flags = CardFlag::NONE;
    int m_upgradeCount = 0;
    PlayCondition m_playCondition = PlayCondition::NONE;
    CostModifier m_costModifier = CostModifier::NONE;
    int m_bonusDamage = 0;
    int m_bonusBlock = 0;
    int m_costReduction = 0;
    int m_costThisTurn = NO_COST_OVERRIDE;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_CARD_HPP
