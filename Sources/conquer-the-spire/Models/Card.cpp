// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Models/Card.hpp>

#include <utility>

namespace ConquerTheSpire
{
CardEffect CardEffect::Damage(int value, int times)
{
    CardEffect effect;
    effect.type = EffectType::DEAL_DAMAGE;
    effect.value = value;
    effect.times = times;

    return effect;
}

CardEffect CardEffect::DamageAll(int value, int times)
{
    CardEffect effect = Damage(value, times);
    effect.target = EffectTarget::ALL_ENEMIES;

    return effect;
}

CardEffect CardEffect::DamageRandom(int value, int times)
{
    CardEffect effect = Damage(value, times);
    effect.target = EffectTarget::RANDOM_ENEMY;

    return effect;
}

CardEffect CardEffect::Block(int value)
{
    CardEffect effect;
    effect.type = EffectType::GAIN_BLOCK;
    effect.value = value;

    return effect;
}

CardEffect CardEffect::ApplyPower(PowerType power, int amount)
{
    CardEffect effect;
    effect.type = EffectType::APPLY_POWER;
    effect.value = amount;
    effect.power = power;

    return effect;
}

CardEffect CardEffect::ApplyPowerToSelf(PowerType power, int amount)
{
    CardEffect effect = ApplyPower(power, amount);
    effect.target = EffectTarget::SELF;

    return effect;
}

CardEffect CardEffect::ApplyPowerToAll(PowerType power, int amount)
{
    CardEffect effect = ApplyPower(power, amount);
    effect.target = EffectTarget::ALL_ENEMIES;

    return effect;
}

CardEffect CardEffect::Energy(int value)
{
    CardEffect effect;
    effect.type = EffectType::GAIN_ENERGY;
    effect.value = value;

    return effect;
}

CardEffect CardEffect::Draw(int value)
{
    CardEffect effect;
    effect.type = EffectType::DRAW_CARD;
    effect.value = value;

    return effect;
}

CardEffect CardEffect::LoseHealth(int value)
{
    CardEffect effect;
    effect.type = EffectType::LOSE_HEALTH;
    effect.value = value;

    return effect;
}

CardEffect CardEffect::Heal(int value)
{
    CardEffect effect;
    effect.type = EffectType::HEAL;
    effect.value = value;

    return effect;
}

CardEffect CardEffect::HealPercent(int percent)
{
    CardEffect effect;
    effect.type = EffectType::HEAL_PERCENT;
    effect.value = percent;

    return effect;
}

CardEffect CardEffect::IncreaseMaxHealth(int value)
{
    CardEffect effect;
    effect.type = EffectType::INCREASE_MAX_HEALTH;
    effect.value = value;

    return effect;
}

CardEffect CardEffect::AddCard(CardId id, CardPile pile, int count,
                               bool upgraded)
{
    CardEffect effect;
    effect.type = EffectType::ADD_CARD;
    effect.cardId = id;
    effect.pile = pile;
    effect.value = count;
    effect.upgradedCard = upgraded;

    return effect;
}

CardEffect CardEffect::CopySelfToDiscard()
{
    CardEffect effect;
    effect.type = EffectType::COPY_SELF_TO_DISCARD;
    effect.value = 1;

    return effect;
}

CardEffect CardEffect::CopyHandCard(int count)
{
    CardEffect effect;
    effect.type = EffectType::COPY_HAND_CARD;
    effect.value = count;
    effect.filter = CardFilter::ATTACK_OR_POWER;

    return effect;
}

CardEffect CardEffect::AddRandomAttack(int count, CardPile pile)
{
    CardEffect effect;
    effect.type = EffectType::ADD_RANDOM_ATTACK;
    effect.value = count;
    effect.pile = pile;

    return effect;
}

CardEffect CardEffect::UpgradeHandCard(bool all)
{
    CardEffect effect;
    effect.type = EffectType::UPGRADE_HAND_CARD;
    effect.value = all ? 0 : 1;  //!< 0 means every card in hand.

    return effect;
}

CardEffect CardEffect::UpgradeRandomHandCard()
{
    CardEffect effect;
    effect.type = EffectType::UPGRADE_HAND_CARD;
    effect.value = 1;
    effect.randomPick = true;

    return effect;
}

CardEffect CardEffect::ExhaustHandCard(int count, bool random)
{
    CardEffect effect;
    effect.type = EffectType::EXHAUST_HAND_CARD;
    effect.value = count;
    effect.randomPick = random;

    return effect;
}

CardEffect CardEffect::ExhaustHand(CardFilter filter)
{
    CardEffect effect;
    effect.type = EffectType::EXHAUST_HAND;
    effect.filter = filter;

    return effect;
}

CardEffect CardEffect::ReturnFromExhaust()
{
    CardEffect effect;
    effect.type = EffectType::RETURN_FROM_EXHAUST;
    effect.value = 1;

    return effect;
}

CardEffect CardEffect::DiscardToDrawTop()
{
    CardEffect effect;
    effect.type = EffectType::DISCARD_TO_DRAW_TOP;
    effect.value = 1;

    return effect;
}

CardEffect CardEffect::HandToDrawTop()
{
    CardEffect effect;
    effect.type = EffectType::HAND_TO_DRAW_TOP;
    effect.value = 1;

    return effect;
}

CardEffect CardEffect::PlayTopCard()
{
    CardEffect effect;
    effect.type = EffectType::PLAY_TOP_CARD;
    effect.value = 1;

    return effect;
}

CardEffect CardEffect::DoubleBlock()
{
    CardEffect effect;
    effect.type = EffectType::DOUBLE_BLOCK;

    return effect;
}

CardEffect CardEffect::DoubleStrength()
{
    CardEffect effect;
    effect.type = EffectType::DOUBLE_STRENGTH;

    return effect;
}

CardEffect CardEffect::IncreaseSelfDamage(int value)
{
    CardEffect effect;
    effect.type = EffectType::INCREASE_SELF_DAMAGE;
    effect.value = value;

    return effect;
}

CardEffect CardEffect::DiscardCards(int count, bool random)
{
    CardEffect effect;
    effect.type = EffectType::DISCARD_CARDS;
    effect.value = count;
    effect.randomPick = random;

    return effect;
}

CardEffect CardEffect::DiscardHand(CardFilter filter)
{
    CardEffect effect;
    effect.type = EffectType::DISCARD_HAND;
    effect.filter = filter;

    return effect;
}

CardEffect CardEffect::MultiplyPower(PowerType power, int factor)
{
    CardEffect effect;
    effect.type = EffectType::MULTIPLY_TARGET_POWER;
    effect.power = power;
    effect.value = factor;

    return effect;
}

CardEffect CardEffect::AddRandomSkill(int count, CardPile pile)
{
    CardEffect effect;
    effect.type = EffectType::ADD_RANDOM_SKILL;
    effect.value = count;
    effect.pile = pile;

    return effect;
}

CardEffect CardEffect::SetupCard(CardPile pile, bool manyCards)
{
    CardEffect effect;
    effect.type = EffectType::SETUP_CARD;
    effect.value = 1;
    effect.pile = pile;
    effect.manyCards = manyCards;

    return effect;
}

CardEffect CardEffect::DrawUntil(int size)
{
    CardEffect effect;
    effect.type = EffectType::DRAW_UNTIL;
    effect.value = size;

    return effect;
}

CardEffect CardEffect::RememberCard(int copies)
{
    CardEffect effect;
    effect.type = EffectType::REMEMBER_CARD;
    effect.value = copies;

    return effect;
}

CardEffect CardEffect::ChannelOrb(OrbType type, int count)
{
    CardEffect effect;
    effect.type = EffectType::CHANNEL_ORB;
    effect.orb = type;
    effect.value = count;

    return effect;
}

CardEffect CardEffect::EvokeOrb(int times)
{
    CardEffect effect;
    effect.type = EffectType::EVOKE_ORB;
    effect.value = times;

    return effect;
}

CardEffect CardEffect::EvokeAllOrbs()
{
    CardEffect effect;
    effect.type = EffectType::EVOKE_ALL_ORBS;

    return effect;
}

CardEffect CardEffect::EvokeAndChannel()
{
    CardEffect effect;
    effect.type = EffectType::EVOKE_ORB;
    effect.value = 1;
    effect.extra = 1;  //!< Puts the orb back afterwards.

    return effect;
}

CardEffect CardEffect::RemoveAllOrbs()
{
    CardEffect effect;
    effect.type = EffectType::REMOVE_ALL_ORBS;

    return effect;
}

CardEffect CardEffect::TriggerDarkOrbs()
{
    CardEffect effect;
    effect.type = EffectType::TRIGGER_DARK_ORBS;

    return effect;
}

CardEffect CardEffect::ObtainPotion()
{
    CardEffect effect;
    effect.type = EffectType::OBTAIN_POTION;
    effect.value = 1;

    return effect;
}

CardEffect CardEffect::AddOrbSlots(int amount)
{
    CardEffect effect;
    effect.type = EffectType::ADD_ORB_SLOTS;
    effect.value = amount;

    return effect;
}

CardEffect CardEffect::AddRandomPower()
{
    CardEffect effect;
    effect.type = EffectType::ADD_RANDOM_POWER;
    effect.value = 1;
    effect.pile = CardPile::HAND;

    return effect;
}

CardEffect CardEffect::AddRandomCommon(int count)
{
    CardEffect effect;
    effect.type = EffectType::ADD_RANDOM_COMMON;
    effect.value = count;
    effect.pile = CardPile::HAND;

    return effect;
}

CardEffect CardEffect::DoubleEnergy()
{
    CardEffect effect;
    effect.type = EffectType::DOUBLE_ENERGY;

    return effect;
}

CardEffect CardEffect::RemoveBlock()
{
    CardEffect effect;
    effect.type = EffectType::REMOVE_BLOCK;

    return effect;
}

CardEffect CardEffect::ReturnFromDiscard(bool everyZeroCost)
{
    CardEffect effect;
    effect.type = EffectType::RETURN_FROM_DISCARD;
    effect.value = 1;
    effect.extra = everyZeroCost ? 1 : 0;

    return effect;
}

CardEffect CardEffect::TakeFromDrawPile(int count)
{
    CardEffect effect;
    effect.type = EffectType::DRAW_TO_HAND_FROM_TOP;
    effect.value = count;

    return effect;
}

CardEffect CardEffect::ReshuffleAll(int draw)
{
    CardEffect effect;
    effect.type = EffectType::RESHUFFLE_ALL;
    effect.value = draw;

    return effect;
}

CardEffect CardEffect::ExhaustForEnergy()
{
    CardEffect effect;
    effect.type = EffectType::EXHAUST_FOR_ENERGY;
    effect.value = 1;

    return effect;
}

CardEffect CardEffect::IncreaseSelfBlock(int value)
{
    CardEffect effect;
    effect.type = EffectType::INCREASE_SELF_BLOCK;
    effect.value = value;

    return effect;
}

CardEffect CardEffect::IncreaseClawDamage(int value)
{
    CardEffect effect;
    effect.type = EffectType::INCREASE_CLAW_DAMAGE;
    effect.value = value;

    return effect;
}

CardEffect CardEffect::ReduceSelfCost(int value)
{
    CardEffect effect;
    effect.type = EffectType::REDUCE_SELF_COST;
    effect.value = value;

    return effect;
}

CardEffect CardEffect::AddRandomCard(int count, CardPile pile)
{
    CardEffect effect;
    effect.type = EffectType::ADD_RANDOM_CARD;
    effect.value = count;
    effect.pile = pile;

    return effect;
}

CardEffect CardEffect::OfferRandomCards(int count)
{
    CardEffect effect;
    effect.type = EffectType::OFFER_CARDS;
    effect.value = count;
    effect.pile = CardPile::HAND;

    return effect;
}

CardEffect CardEffect::SetHandCost(int cost, bool onlyOne, bool wholeBattle)
{
    CardEffect effect;
    effect.type = EffectType::SET_HAND_COST;
    effect.value = cost;
    effect.randomPick = onlyOne;
    effect.wholeBattle = wholeBattle;

    return effect;
}

CardEffect CardEffect::TakeFromDrawPileByType(CardType type, int count)
{
    CardEffect effect;
    effect.type = EffectType::TAKE_FROM_DRAW_BY_TYPE;
    effect.value = count;
    effect.filter = type == CardType::ATTACK ? CardFilter::ATTACK_ONLY
                                             : CardFilter::SKILL_ONLY;

    return effect;
}

CardEffect& CardEffect::IfFatalGold(int amount)
{
    goldIfFatal = amount;

    return *this;
}

CardEffect& CardEffect::Times(int count)
{
    times = count;

    return *this;
}

CardEffect& CardEffect::XTimes()
{
    times = TIMES_X;

    return *this;
}

CardEffect& CardEffect::From(ValueSource source, int extraAmount)
{
    valueSource = source;
    extra = extraAmount;

    return *this;
}

CardEffect& CardEffect::If(EffectCondition wanted)
{
    condition = wanted;

    return *this;
}

CardEffect& CardEffect::Only(CardFilter wanted)
{
    filter = wanted;

    return *this;
}

CardEffect& CardEffect::Threshold(int amount)
{
    extra = amount;

    return *this;
}

CardEffect& CardEffect::AsOneHit()
{
    singleHit = true;

    return *this;
}

CardEffect& CardEffect::Upgraded()
{
    upgradedCard = true;

    return *this;
}

CardEffect& CardEffect::FromPool(CardColor color)
{
    colorOverride = color;

    return *this;
}

Card::Card(CardId id, std::string name, CardColor color, CardType type,
           CardRarity rarity, CardTarget target, int cost,
           std::vector<CardEffect> effects, CardFlag flags)
    : m_id(id),
      m_name(std::move(name)),
      m_color(color),
      m_cardType(type),
      m_rarity(rarity),
      m_target(target),
      m_cost(cost),
      m_effects(std::move(effects)),
      m_flags(flags)
{
    // Do nothing
}

CardId Card::GetId() const
{
    return m_id;
}

const std::string& Card::GetName() const
{
    return m_name;
}

CardColor Card::GetColor() const
{
    return m_color;
}

CardType Card::GetCardType() const
{
    return m_cardType;
}

CardRarity Card::GetRarity() const
{
    return m_rarity;
}

CardTarget Card::GetTarget() const
{
    return m_target;
}

int Card::GetCost() const
{
    return m_cost;
}

const std::vector<CardEffect>& Card::GetEffects() const
{
    return m_effects;
}

CardFlag Card::GetFlags() const
{
    return m_flags;
}

bool Card::Has(CardFlag flag) const
{
    return HasFlag(m_flags, flag);
}

void Card::AddFlag(CardFlag flag)
{
    m_flags = m_flags | flag;
}

void Card::RemoveFlag(CardFlag flag)
{
    m_flags = static_cast<CardFlag>(static_cast<unsigned int>(m_flags) &
                                    ~static_cast<unsigned int>(flag));
}

int Card::GetUpgradeCount() const
{
    return m_upgradeCount;
}

bool Card::IsUpgraded() const
{
    return m_upgradeCount > 0;
}

PlayCondition Card::GetPlayCondition() const
{
    return m_playCondition;
}

CostModifier Card::GetCostModifier() const
{
    return m_costModifier;
}

int Card::GetBonusDamage() const
{
    return m_bonusDamage;
}

void Card::AddBonusDamage(int amount)
{
    m_bonusDamage += amount;
}

int Card::GetBonusBlock() const
{
    return m_bonusBlock;
}

void Card::AddBonusBlock(int amount)
{
    m_bonusBlock += amount;
}

int Card::GetCostReduction() const
{
    return m_costReduction;
}

void Card::AddCostReduction(int amount)
{
    m_costReduction += amount;
}

bool Card::IsPlayable() const
{
    // Statuses and curses carry the unplayable flag one by one, because a few
    // of them - Slimed, Pride - can be played.
    return !Has(CardFlag::UNPLAYABLE) && m_cardType != CardType::INVALID;
}

void Card::MarkUpgraded(int upgradeCount)
{
    if (upgradeCount <= 0)
    {
        return;
    }

    m_upgradeCount = upgradeCount;
    m_name += "+";
}

void Card::SetPlayCondition(PlayCondition condition)
{
    m_playCondition = condition;
}

void Card::SetCostModifier(CostModifier modifier)
{
    m_costModifier = modifier;
}

void Card::SetCostThisTurn(int cost)
{
    m_costThisTurn = cost;
}

void Card::ClearCostThisTurn()
{
    m_costThisTurn = NO_COST_OVERRIDE;
}

bool Card::HasCostThisTurn() const
{
    return m_costThisTurn != NO_COST_OVERRIDE;
}

int Card::GetCostThisTurn() const
{
    return m_costThisTurn;
}
}  // namespace ConquerTheSpire
