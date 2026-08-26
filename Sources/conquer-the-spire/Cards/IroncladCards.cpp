// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Cards/CardBuilders.hpp>

#include <utility>

namespace ConquerTheSpire::Detail
{
namespace
{
using CE = CardEffect;

Card Attack(CardId id, const char* name, CardRarity rarity, CardTarget target,
            int cost, std::vector<CardEffect> effects,
            CardFlag flags = CardFlag::NONE)
{
    return Card(id, name, CardColor::RED, CardType::ATTACK, rarity, target,
                cost, std::move(effects), flags);
}

Card Skill(CardId id, const char* name, CardRarity rarity, CardTarget target,
           int cost, std::vector<CardEffect> effects,
           CardFlag flags = CardFlag::NONE)
{
    return Card(id, name, CardColor::RED, CardType::SKILL, rarity, target, cost,
                std::move(effects), flags);
}

Card Power(CardId id, const char* name, CardRarity rarity, int cost,
           std::vector<CardEffect> effects, CardFlag flags = CardFlag::NONE)
{
    return Card(id, name, CardColor::RED, CardType::POWER, rarity,
                CardTarget::SELF, cost, std::move(effects), flags);
}

//! Searing Blow keeps growing with every upgrade: 12, 16, 21, 27, 34, ...
int SearingBlowDamage(int upgradeCount)
{
    return 12 + upgradeCount * (upgradeCount + 7) / 2;
}
}  // namespace

Card MakeIroncladCard(CardId id, int upgradeCount)
{
    const bool up = upgradeCount > 0;

    switch (id)
    {
        // Basic

        case CardId::STRIKE_RED:
            return Attack(id, "Strike", CardRarity::BASIC,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 9 : 6) });

        case CardId::DEFEND_RED:
            return Skill(id, "Defend", CardRarity::BASIC, CardTarget::SELF, 1,
                         { CE::Block(up ? 8 : 5) });

        case CardId::BASH:
            return Attack(id, "Bash", CardRarity::BASIC,
                          CardTarget::SINGLE_ENEMY, 2,
                          { CE::Damage(up ? 10 : 8),
                            CE::ApplyPower(PowerType::VULNERABLE,
                                           up ? 3 : 2) });

        // Common

        case CardId::ANGER:
            return Attack(id, "Anger", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 0,
                          { CE::Damage(up ? 8 : 6), CE::CopySelfToDiscard() });

        case CardId::ARMAMENTS:
            return Skill(id, "Armaments", CardRarity::COMMON, CardTarget::SELF,
                         1, { CE::Block(5), CE::UpgradeHandCard(up) });

        case CardId::BODY_SLAM:
            return Attack(id, "Body Slam", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, up ? 0 : 1,
                          { CE::Damage(0).From(ValueSource::CURRENT_BLOCK) });

        case CardId::CLASH:
        {
            Card card = Attack(id, "Clash", CardRarity::COMMON,
                               CardTarget::SINGLE_ENEMY, 0,
                               { CE::Damage(up ? 18 : 14) });
            card.SetPlayCondition(PlayCondition::HAND_ALL_ATTACKS);

            return card;
        }

        case CardId::CLEAVE:
            return Attack(id, "Cleave", CardRarity::COMMON,
                          CardTarget::ALL_ENEMIES, 1,
                          { CE::DamageAll(up ? 11 : 8) });

        case CardId::CLOTHESLINE:
            return Attack(
                id, "Clothesline", CardRarity::COMMON,
                CardTarget::SINGLE_ENEMY, 2,
                { CE::Damage(up ? 14 : 12),
                  CE::ApplyPower(PowerType::WEAK, up ? 3 : 2) });

        case CardId::FLEX:
            return Skill(
                id, "Flex", CardRarity::COMMON, CardTarget::SELF, 0,
                { CE::ApplyPowerToSelf(PowerType::STRENGTH, up ? 4 : 2),
                  CE::ApplyPowerToSelf(PowerType::STRENGTH_DOWN,
                                       up ? 4 : 2) });

        case CardId::HAVOC:
            return Skill(id, "Havoc", CardRarity::COMMON, CardTarget::SELF,
                         up ? 0 : 1, { CE::PlayTopCard() });

        case CardId::HEADBUTT:
            return Attack(id, "Headbutt", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 12 : 9), CE::DiscardToDrawTop() });

        case CardId::HEAVY_BLADE:
            return Attack(id, "Heavy Blade", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 2,
                          { CE::Damage(14).From(ValueSource::STRENGTH_MULTIPLE,
                                                up ? 5 : 3) });

        case CardId::IRON_WAVE:
            return Attack(id, "Iron Wave", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 7 : 5), CE::Block(up ? 7 : 5) });

        case CardId::PERFECTED_STRIKE:
            return Attack(id, "Perfected Strike", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 2,
                          { CE::Damage(6).From(ValueSource::STRIKE_COUNT,
                                               up ? 3 : 2) });

        case CardId::POMMEL_STRIKE:
            return Attack(id, "Pommel Strike", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 10 : 9), CE::Draw(up ? 2 : 1) });

        case CardId::SHRUG_IT_OFF:
            return Skill(id, "Shrug It Off", CardRarity::COMMON,
                         CardTarget::SELF, 1,
                         { CE::Block(up ? 11 : 8), CE::Draw(1) });

        case CardId::SWORD_BOOMERANG:
            return Attack(id, "Sword Boomerang", CardRarity::COMMON,
                          CardTarget::RANDOM_ENEMY, 1,
                          { CE::DamageRandom(3, up ? 4 : 3) });

        case CardId::THUNDERCLAP:
            return Attack(
                id, "Thunderclap", CardRarity::COMMON, CardTarget::ALL_ENEMIES,
                1,
                { CE::DamageAll(up ? 7 : 4),
                  CE::ApplyPowerToAll(PowerType::VULNERABLE, 1) });

        case CardId::TRUE_GRIT:
            return Skill(id, "True Grit", CardRarity::COMMON, CardTarget::SELF,
                         1,
                         { CE::Block(up ? 9 : 7),
                           CE::ExhaustHandCard(1, !up) });

        case CardId::TWIN_STRIKE:
            return Attack(id, "Twin Strike", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 7 : 5, 2) });

        case CardId::WARCRY:
            return Skill(id, "Warcry", CardRarity::COMMON, CardTarget::SELF, 0,
                         { CE::Draw(up ? 2 : 1), CE::HandToDrawTop() },
                         CardFlag::EXHAUST);

        case CardId::WILD_STRIKE:
            return Attack(id, "Wild Strike", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 17 : 12),
                            CE::AddCard(CardId::WOUND,
                                        CardPile::DRAW_SHUFFLED) });

        // Uncommon

        case CardId::BATTLE_TRANCE:
            return Skill(id, "Battle Trance", CardRarity::UNCOMMON,
                         CardTarget::SELF, 0,
                         { CE::Draw(up ? 4 : 3),
                           CE::ApplyPowerToSelf(PowerType::NO_DRAW, 1) });

        case CardId::BLOOD_FOR_BLOOD:
        {
            Card card = Attack(id, "Blood for Blood", CardRarity::UNCOMMON,
                               CardTarget::SINGLE_ENEMY, up ? 3 : 4,
                               { CE::Damage(up ? 22 : 18) });
            card.SetCostModifier(CostModifier::HEALTH_LOST_THIS_BATTLE);

            return card;
        }

        case CardId::BLOODLETTING:
            return Skill(id, "Bloodletting", CardRarity::UNCOMMON,
                         CardTarget::SELF, 0,
                         { CE::LoseHealth(3), CE::Energy(up ? 3 : 2) });

        case CardId::BURNING_PACT:
            return Skill(id, "Burning Pact", CardRarity::UNCOMMON,
                         CardTarget::SELF, 1,
                         { CE::ExhaustHandCard(1, false),
                           CE::Draw(up ? 3 : 2) });

        case CardId::CARNAGE:
            return Attack(id, "Carnage", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 2,
                          { CE::Damage(up ? 28 : 20) }, CardFlag::ETHEREAL);

        case CardId::COMBUST:
            // Two numbers, because the damage stacks and the health it costs
            // stacks separately: five or seven of the one, and one of the
            // other for every copy played.
            return Power(id, "Combust", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::COMBUST,
                                                up ? 7 : 5),
                           CE::ApplyPowerToSelf(PowerType::COMBUST_COPIES,
                                                1) });

        case CardId::DARK_EMBRACE:
            return Power(id, "Dark Embrace", CardRarity::UNCOMMON, up ? 1 : 2,
                         { CE::ApplyPowerToSelf(PowerType::DARK_EMBRACE, 1) });

        case CardId::DISARM:
            return Skill(id, "Disarm", CardRarity::UNCOMMON,
                         CardTarget::SINGLE_ENEMY, 1,
                         { CE::ApplyPower(PowerType::STRENGTH, up ? -3 : -2) },
                         CardFlag::EXHAUST);

        case CardId::DROPKICK:
            return Attack(
                id, "Dropkick", CardRarity::UNCOMMON,
                CardTarget::SINGLE_ENEMY, 1,
                { CE::Damage(up ? 8 : 5),
                  CE::Energy(1).If(EffectCondition::TARGET_VULNERABLE),
                  CE::Draw(1).If(EffectCondition::TARGET_VULNERABLE) });

        case CardId::DUAL_WIELD:
            return Skill(id, "Dual Wield", CardRarity::UNCOMMON,
                         CardTarget::SELF, 1,
                         { CE::CopyHandCard(up ? 2 : 1) });

        case CardId::ENTRENCH:
            return Skill(id, "Entrench", CardRarity::UNCOMMON, CardTarget::SELF,
                         up ? 1 : 2, { CE::DoubleBlock() });

        case CardId::EVOLVE:
            return Power(id, "Evolve", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::EVOLVE,
                                                up ? 2 : 1) });

        case CardId::FEEL_NO_PAIN:
            return Power(id, "Feel No Pain", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::FEEL_NO_PAIN,
                                                up ? 4 : 3) });

        case CardId::FIRE_BREATHING:
            return Power(id, "Fire Breathing", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::FIRE_BREATHING,
                                                up ? 10 : 6) });

        case CardId::FLAME_BARRIER:
            return Skill(id, "Flame Barrier", CardRarity::UNCOMMON,
                         CardTarget::SELF, 2,
                         { CE::Block(up ? 16 : 12),
                           CE::ApplyPowerToSelf(PowerType::FLAME_BARRIER,
                                                up ? 6 : 4) });

        case CardId::GHOSTLY_ARMOR:
            return Skill(id, "Ghostly Armor", CardRarity::UNCOMMON,
                         CardTarget::SELF, 1, { CE::Block(up ? 13 : 10) },
                         CardFlag::ETHEREAL);

        case CardId::HEMOKINESIS:
            return Attack(id, "Hemokinesis", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::LoseHealth(2), CE::Damage(up ? 20 : 15) });

        case CardId::INFERNAL_BLADE:
            return Skill(id, "Infernal Blade", CardRarity::UNCOMMON,
                         CardTarget::SELF, up ? 0 : 1,
                         { CE::AddRandomAttack() }, CardFlag::EXHAUST);

        case CardId::INFLAME:
            return Power(id, "Inflame", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::STRENGTH,
                                                up ? 3 : 2) });

        case CardId::INTIMIDATE:
            return Skill(id, "Intimidate", CardRarity::UNCOMMON,
                         CardTarget::ALL_ENEMIES, 0,
                         { CE::ApplyPowerToAll(PowerType::WEAK, up ? 2 : 1) },
                         CardFlag::EXHAUST);

        case CardId::METALLICIZE:
            return Power(id, "Metallicize", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::METALLICIZE,
                                                up ? 4 : 3) });

        case CardId::POWER_THROUGH:
            return Skill(id, "Power Through", CardRarity::UNCOMMON,
                         CardTarget::SELF, 1,
                         { CE::AddCard(CardId::WOUND, CardPile::HAND, 2),
                           CE::Block(up ? 20 : 15) });

        case CardId::PUMMEL:
            return Attack(id, "Pummel", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(2, up ? 5 : 4) }, CardFlag::EXHAUST);

        case CardId::RAGE:
            return Skill(id, "Rage", CardRarity::UNCOMMON, CardTarget::SELF, 0,
                         { CE::ApplyPowerToSelf(PowerType::RAGE, up ? 5 : 3) });

        case CardId::RAMPAGE:
            return Attack(id, "Rampage", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(8),
                            CE::IncreaseSelfDamage(up ? 8 : 5) });

        case CardId::RECKLESS_CHARGE:
            return Attack(id, "Reckless Charge", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 0,
                          { CE::Damage(up ? 10 : 7),
                            CE::AddCard(CardId::DAZED,
                                        CardPile::DRAW_SHUFFLED) });

        case CardId::RUPTURE:
            return Power(id, "Rupture", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::RUPTURE,
                                                up ? 2 : 1) });

        case CardId::SEARING_BLOW:
            return Attack(id, "Searing Blow", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 2,
                          { CE::Damage(SearingBlowDamage(upgradeCount)) });

        case CardId::SECOND_WIND:
            return Skill(id, "Second Wind", CardRarity::UNCOMMON,
                         CardTarget::SELF, 1,
                         { CE::ExhaustHand(CardFilter::NON_ATTACK),
                           CE::Block(0).From(ValueSource::CARDS_EXHAUSTED,
                                             up ? 7 : 5) });

        case CardId::SEEING_RED:
            return Skill(id, "Seeing Red", CardRarity::UNCOMMON,
                         CardTarget::SELF, up ? 0 : 1, { CE::Energy(2) },
                         CardFlag::EXHAUST);

        case CardId::SENTINEL:
            return Skill(id, "Sentinel", CardRarity::UNCOMMON, CardTarget::SELF,
                         1, { CE::Block(up ? 8 : 5) }, CardFlag::SENTINEL);

        case CardId::SEVER_SOUL:
            return Attack(id, "Sever Soul", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 2,
                          { CE::ExhaustHand(CardFilter::NON_ATTACK),
                            CE::Damage(up ? 22 : 16) });

        case CardId::SHOCKWAVE:
            return Skill(
                id, "Shockwave", CardRarity::UNCOMMON, CardTarget::ALL_ENEMIES,
                2,
                { CE::ApplyPowerToAll(PowerType::WEAK, up ? 5 : 3),
                  CE::ApplyPowerToAll(PowerType::VULNERABLE, up ? 5 : 3) },
                CardFlag::EXHAUST);

        case CardId::SPOT_WEAKNESS:
            return Skill(id, "Spot Weakness", CardRarity::UNCOMMON,
                         CardTarget::SINGLE_ENEMY, 1,
                         { CE::ApplyPowerToSelf(PowerType::STRENGTH,
                                                up ? 4 : 3)
                               .If(EffectCondition::TARGET_ATTACKING) });

        case CardId::UPPERCUT:
            return Attack(
                id, "Uppercut", CardRarity::UNCOMMON,
                CardTarget::SINGLE_ENEMY, 2,
                { CE::Damage(13), CE::ApplyPower(PowerType::WEAK, up ? 2 : 1),
                  CE::ApplyPower(PowerType::VULNERABLE, up ? 2 : 1) });

        case CardId::WHIRLWIND:
            return Attack(id, "Whirlwind", CardRarity::UNCOMMON,
                          CardTarget::ALL_ENEMIES, Card::COST_X,
                          { CE::DamageAll(up ? 8 : 5).XTimes() });

        // Rare

        case CardId::BARRICADE:
            return Power(id, "Barricade", CardRarity::RARE, up ? 2 : 3,
                         { CE::ApplyPowerToSelf(PowerType::BARRICADE, 1) });

        case CardId::BERSERK:
            return Power(id, "Berserk", CardRarity::RARE, 0,
                         { CE::ApplyPowerToSelf(PowerType::VULNERABLE,
                                                up ? 1 : 2),
                           CE::ApplyPowerToSelf(PowerType::BERSERK, 1) });

        case CardId::BLUDGEON:
            return Attack(id, "Bludgeon", CardRarity::RARE,
                          CardTarget::SINGLE_ENEMY, 3,
                          { CE::Damage(up ? 42 : 32) });

        case CardId::BRUTALITY:
            return Power(id, "Brutality", CardRarity::RARE, 0,
                         { CE::ApplyPowerToSelf(PowerType::BRUTALITY, 1) },
                         up ? CardFlag::INNATE : CardFlag::NONE);

        case CardId::CORRUPTION:
            return Power(id, "Corruption", CardRarity::RARE, up ? 2 : 3,
                         { CE::ApplyPowerToSelf(PowerType::CORRUPTION, 1) });

        case CardId::DEMON_FORM:
            return Power(id, "Demon Form", CardRarity::RARE, 3,
                         { CE::ApplyPowerToSelf(PowerType::DEMON_FORM,
                                                up ? 3 : 2) });

        case CardId::DOUBLE_TAP:
            return Skill(id, "Double Tap", CardRarity::RARE, CardTarget::SELF,
                         1,
                         { CE::ApplyPowerToSelf(PowerType::DOUBLE_TAP,
                                                up ? 2 : 1) });

        case CardId::EXHUME:
            return Skill(id, "Exhume", CardRarity::RARE, CardTarget::SELF,
                         up ? 0 : 1, { CE::ReturnFromExhaust() },
                         CardFlag::EXHAUST);

        case CardId::FEED:
            return Attack(id, "Feed", CardRarity::RARE,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 12 : 10),
                            CE::IncreaseMaxHealth(up ? 4 : 3)
                                .If(EffectCondition::KILLED_TARGET) },
                          CardFlag::EXHAUST);

        case CardId::FIEND_FIRE:
            return Attack(id, "Fiend Fire", CardRarity::RARE,
                          CardTarget::SINGLE_ENEMY, 2,
                          { CE::ExhaustHand(CardFilter::ANY),
                            CE::Damage(0).From(ValueSource::CARDS_EXHAUSTED,
                                               up ? 10 : 7) },
                          CardFlag::EXHAUST);

        case CardId::IMMOLATE:
            return Attack(id, "Immolate", CardRarity::RARE,
                          CardTarget::ALL_ENEMIES, 2,
                          { CE::DamageAll(up ? 28 : 21),
                            CE::AddCard(CardId::BURN, CardPile::DISCARD) });

        case CardId::IMPERVIOUS:
            return Skill(id, "Impervious", CardRarity::RARE, CardTarget::SELF,
                         2, { CE::Block(up ? 40 : 30) }, CardFlag::EXHAUST);

        case CardId::JUGGERNAUT:
            return Power(id, "Juggernaut", CardRarity::RARE, 2,
                         { CE::ApplyPowerToSelf(PowerType::JUGGERNAUT,
                                                up ? 7 : 5) });

        case CardId::LIMIT_BREAK:
            return Skill(id, "Limit Break", CardRarity::RARE, CardTarget::SELF,
                         1, { CE::DoubleStrength() },
                         up ? CardFlag::NONE : CardFlag::EXHAUST);

        case CardId::OFFERING:
            return Skill(id, "Offering", CardRarity::RARE, CardTarget::SELF, 0,
                         { CE::LoseHealth(6), CE::Energy(2),
                           CE::Draw(up ? 5 : 3) },
                         CardFlag::EXHAUST);

        case CardId::REAPER:
            return Attack(id, "Reaper", CardRarity::RARE,
                          CardTarget::ALL_ENEMIES, 2,
                          { CE::DamageAll(up ? 5 : 4),
                            CE::Heal(0).From(ValueSource::UNBLOCKED_DAMAGE) },
                          CardFlag::EXHAUST);

        default:
            return Card();
    }
}
}  // namespace ConquerTheSpire::Detail
