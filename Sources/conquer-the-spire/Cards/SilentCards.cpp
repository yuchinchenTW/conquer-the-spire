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
    return Card(id, name, CardColor::GREEN, CardType::ATTACK, rarity, target,
                cost, std::move(effects), flags);
}

Card Skill(CardId id, const char* name, CardRarity rarity, CardTarget target,
           int cost, std::vector<CardEffect> effects,
           CardFlag flags = CardFlag::NONE)
{
    return Card(id, name, CardColor::GREEN, CardType::SKILL, rarity, target,
                cost, std::move(effects), flags);
}

Card Power(CardId id, const char* name, CardRarity rarity, int cost,
           std::vector<CardEffect> effects, CardFlag flags = CardFlag::NONE)
{
    return Card(id, name, CardColor::GREEN, CardType::POWER, rarity,
                CardTarget::SELF, cost, std::move(effects), flags);
}
}  // namespace

Card MakeSilentCard(CardId id, int upgradeCount)
{
    const bool up = upgradeCount > 0;

    switch (id)
    {
        // Basic

        case CardId::STRIKE_GREEN:
            return Attack(id, "Strike", CardRarity::BASIC,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 9 : 6) });

        case CardId::DEFEND_GREEN:
            return Skill(id, "Defend", CardRarity::BASIC, CardTarget::SELF, 1,
                         { CE::Block(up ? 8 : 5) });

        case CardId::NEUTRALIZE:
            return Attack(id, "Neutralize", CardRarity::BASIC,
                          CardTarget::SINGLE_ENEMY, 0,
                          { CE::Damage(up ? 4 : 3),
                            CE::ApplyPower(PowerType::WEAK, up ? 2 : 1) });

        case CardId::SURVIVOR:
            return Skill(id, "Survivor", CardRarity::BASIC, CardTarget::SELF, 1,
                         { CE::Block(up ? 11 : 8),
                           CE::DiscardCards(1, false) });

        // Common

        case CardId::ACROBATICS:
            return Skill(id, "Acrobatics", CardRarity::COMMON, CardTarget::SELF,
                         1,
                         { CE::Draw(up ? 4 : 3), CE::DiscardCards(1, false) });

        case CardId::BACKFLIP:
            return Skill(id, "Backflip", CardRarity::COMMON, CardTarget::SELF,
                         1, { CE::Block(up ? 8 : 5), CE::Draw(2) });

        case CardId::BANE:
            // The second hit only lands on something already poisoned.
            return Attack(id, "Bane", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 10 : 7),
                            CE::Damage(up ? 10 : 7)
                                .If(EffectCondition::TARGET_POISONED) });

        case CardId::BLADE_DANCE:
            return Skill(id, "Blade Dance", CardRarity::COMMON,
                         CardTarget::SELF, 1,
                         { CE::AddCard(CardId::SHIV, CardPile::HAND,
                                       up ? 4 : 3) });

        case CardId::CLOAK_AND_DAGGER:
            return Skill(id, "Cloak and Dagger", CardRarity::COMMON,
                         CardTarget::SELF, 1,
                         { CE::Block(6), CE::AddCard(CardId::SHIV,
                                                     CardPile::HAND,
                                                     up ? 2 : 1) });

        case CardId::DAGGER_SPRAY:
            return Attack(id, "Dagger Spray", CardRarity::COMMON,
                          CardTarget::ALL_ENEMIES, 1,
                          { CE::DamageAll(up ? 6 : 4, 2) });

        case CardId::DAGGER_THROW:
            return Attack(id, "Dagger Throw", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 12 : 9), CE::Draw(1),
                            CE::DiscardCards(1, false) });

        case CardId::DEADLY_POISON:
            return Skill(id, "Deadly Poison", CardRarity::COMMON,
                         CardTarget::SINGLE_ENEMY, 1,
                         { CE::ApplyPower(PowerType::POISON, up ? 7 : 5) });

        case CardId::DEFLECT:
            return Skill(id, "Deflect", CardRarity::COMMON, CardTarget::SELF, 0,
                         { CE::Block(up ? 7 : 4) });

        case CardId::DODGE_AND_ROLL:
            return Skill(id, "Dodge and Roll", CardRarity::COMMON,
                         CardTarget::SELF, 1,
                         { CE::Block(up ? 6 : 4),
                           CE::ApplyPowerToSelf(PowerType::NEXT_TURN_BLOCK,
                                                up ? 6 : 4) });

        case CardId::FLYING_KNEE:
            return Attack(id, "Flying Knee", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 11 : 8),
                            CE::ApplyPowerToSelf(PowerType::ENERGIZED, 1) });

        case CardId::OUTMANEUVER:
            return Skill(id, "Outmaneuver", CardRarity::COMMON,
                         CardTarget::SELF, 1,
                         { CE::ApplyPowerToSelf(PowerType::ENERGIZED,
                                                up ? 3 : 2) });

        case CardId::PIERCING_WAIL:
            // The Strength comes back at the end of the enemy's turn.
            return Skill(id, "Piercing Wail", CardRarity::COMMON,
                         CardTarget::ALL_ENEMIES, 1,
                         { CE::ApplyPowerToAll(PowerType::STRENGTH,
                                               up ? -8 : -6),
                           CE::ApplyPowerToAll(PowerType::STRENGTH_UP,
                                               up ? 8 : 6) },
                         CardFlag::EXHAUST);

        case CardId::POISONED_STAB:
            return Attack(id, "Poisoned Stab", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 8 : 6),
                            CE::ApplyPower(PowerType::POISON, up ? 4 : 3) });

        case CardId::PREPARED:
            return Skill(id, "Prepared", CardRarity::COMMON, CardTarget::SELF,
                         0,
                         { CE::Draw(up ? 2 : 1),
                           CE::DiscardCards(up ? 2 : 1, false) });

        case CardId::QUICK_SLASH:
            return Attack(id, "Quick Slash", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 12 : 8), CE::Draw(1) });

        case CardId::SLICE:
            return Attack(id, "Slice", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 0,
                          { CE::Damage(up ? 9 : 6) });

        case CardId::SNEAKY_STRIKE:
            return Attack(
                id, "Sneaky Strike", CardRarity::COMMON,
                CardTarget::SINGLE_ENEMY, 2,
                { CE::Damage(up ? 16 : 12),
                  CE::Energy(2).If(EffectCondition::DISCARDED_THIS_TURN) });

        case CardId::SUCKER_PUNCH:
            return Attack(id, "Sucker Punch", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 9 : 7),
                            CE::ApplyPower(PowerType::WEAK, up ? 2 : 1) });

        // Uncommon

        case CardId::ACCURACY:
            return Power(id, "Accuracy", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::ACCURACY,
                                                up ? 6 : 4) });

        case CardId::ALL_OUT_ATTACK:
            return Attack(id, "All-Out Attack", CardRarity::UNCOMMON,
                          CardTarget::ALL_ENEMIES, 1,
                          { CE::DamageAll(up ? 14 : 10),
                            CE::DiscardCards(1, true) });

        case CardId::BACKSTAB:
            return Attack(id, "Backstab", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 0,
                          { CE::Damage(up ? 15 : 11) },
                          CardFlag::INNATE | CardFlag::EXHAUST);

        case CardId::BLUR:
            return Skill(id, "Blur", CardRarity::UNCOMMON, CardTarget::SELF, 1,
                         { CE::Block(up ? 8 : 5),
                           CE::ApplyPowerToSelf(PowerType::BLUR, 1) });

        case CardId::BOUNCING_FLASK:
            return Skill(id, "Bouncing Flask", CardRarity::UNCOMMON,
                         CardTarget::RANDOM_ENEMY, 2,
                         { CE::ApplyPower(PowerType::POISON, 3)
                               .Times(up ? 4 : 3) });

        case CardId::CALCULATED_GAMBLE:
            return Skill(id, "Calculated Gamble", CardRarity::UNCOMMON,
                         CardTarget::SELF, 0,
                         { CE::DiscardHand(),
                           CE::Draw(0).From(ValueSource::CARDS_DISCARDED, 1) },
                         up ? CardFlag::NONE : CardFlag::EXHAUST);

        case CardId::CALTROPS:
            return Power(id, "Caltrops", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::THORNS,
                                                up ? 5 : 3) });

        case CardId::CATALYST:
            return Skill(id, "Catalyst", CardRarity::UNCOMMON,
                         CardTarget::SINGLE_ENEMY, 1,
                         { CE::MultiplyPower(PowerType::POISON, up ? 3 : 2) },
                         CardFlag::EXHAUST);

        case CardId::CHOKE:
            return Attack(id, "Choke", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 2,
                          { CE::Damage(12),
                            CE::ApplyPower(PowerType::CHOKED,
                                           up ? 5 : 3) });

        case CardId::CONCENTRATE:
            return Skill(id, "Concentrate", CardRarity::UNCOMMON,
                         CardTarget::SELF, 0,
                         { CE::DiscardCards(up ? 2 : 3, false),
                           CE::Energy(2) });

        case CardId::CRIPPLING_CLOUD:
            return Skill(id, "Crippling Cloud", CardRarity::UNCOMMON,
                         CardTarget::ALL_ENEMIES, 2,
                         { CE::ApplyPowerToAll(PowerType::POISON, up ? 7 : 4),
                           CE::ApplyPowerToAll(PowerType::WEAK, 2) },
                         CardFlag::EXHAUST);

        case CardId::DASH:
            return Attack(id, "Dash", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 2,
                          { CE::Block(up ? 13 : 10),
                            CE::Damage(up ? 13 : 10) });

        case CardId::DISTRACTION:
            return Skill(id, "Distraction", CardRarity::UNCOMMON,
                         CardTarget::SELF, up ? 0 : 1,
                         { CE::AddRandomSkill() }, CardFlag::EXHAUST);

        case CardId::ENDLESS_AGONY:
            // Copying itself when drawn is handled by the battle.
            return Attack(id, "Endless Agony", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 0,
                          { CE::Damage(up ? 6 : 4) }, CardFlag::EXHAUST);

        case CardId::ESCAPE_PLAN:
            return Skill(id, "Escape Plan", CardRarity::UNCOMMON,
                         CardTarget::SELF, 0,
                         { CE::Draw(1), CE::Block(up ? 5 : 3)
                                            .If(EffectCondition::DREW_SKILL) });

        case CardId::EVISCERATE:
        {
            Card card = Attack(id, "Eviscerate", CardRarity::UNCOMMON,
                               CardTarget::SINGLE_ENEMY, 3,
                               { CE::Damage(up ? 9 : 7, 3) });
            card.SetCostModifier(CostModifier::CARDS_DISCARDED_THIS_TURN);

            return card;
        }

        case CardId::EXPERTISE:
            return Skill(id, "Expertise", CardRarity::UNCOMMON,
                         CardTarget::SELF, 2, { CE::DrawUntil(up ? 7 : 6) });

        case CardId::FINISHER:
            return Attack(id, "Finisher", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(0).From(ValueSource::ATTACKS_PLAYED,
                                               up ? 8 : 6) });

        case CardId::FLECHETTES:
            return Attack(id, "Flechettes", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(0).From(ValueSource::SKILLS_IN_HAND,
                                               up ? 6 : 4) });

        case CardId::FOOTWORK:
            return Power(id, "Footwork", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::DEXTERITY,
                                                up ? 3 : 2) });

        case CardId::HEEL_HOOK:
            return Attack(id, "Heel Hook", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 8 : 5),
                            CE::Energy(1).If(EffectCondition::TARGET_WEAK),
                            CE::Draw(1).If(EffectCondition::TARGET_WEAK) });

        case CardId::INFINITE_BLADES:
            return Power(id, "Infinite Blades", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::INFINITE_BLADES,
                                                1) },
                         up ? CardFlag::INNATE : CardFlag::NONE);

        case CardId::LEG_SWEEP:
            return Skill(id, "Leg Sweep", CardRarity::UNCOMMON,
                         CardTarget::SINGLE_ENEMY, 2,
                         { CE::ApplyPower(PowerType::WEAK, up ? 3 : 2),
                           CE::Block(up ? 14 : 11) });

        case CardId::MASTERFUL_STAB:
        {
            Card card = Attack(id, "Masterful Stab", CardRarity::UNCOMMON,
                               CardTarget::SINGLE_ENEMY, 0,
                               { CE::Damage(up ? 16 : 12) });
            card.SetCostModifier(CostModifier::HEALTH_LOST_RAISES_COST);

            return card;
        }

        case CardId::NOXIOUS_FUMES:
            return Power(id, "Noxious Fumes", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::NOXIOUS_FUMES,
                                                up ? 3 : 2) });

        case CardId::PREDATOR:
            return Attack(id, "Predator", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 2,
                          { CE::Damage(up ? 20 : 15),
                            CE::ApplyPowerToSelf(PowerType::DRAW_NEXT_TURN,
                                                 2) });

        case CardId::REFLEX:
            // Only does anything when it is thrown away.
            return Skill(id, "Reflex", CardRarity::UNCOMMON, CardTarget::NONE,
                         Card::COST_UNPLAYABLE, {}, CardFlag::UNPLAYABLE);

        case CardId::RIDDLE_WITH_HOLES:
            return Attack(id, "Riddle with Holes", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 2,
                          { CE::Damage(up ? 4 : 3, 5) });

        case CardId::SETUP:
            return Skill(id, "Setup", CardRarity::UNCOMMON, CardTarget::SELF,
                         up ? 0 : 1, { CE::SetupCard() });

        case CardId::SKEWER:
            return Attack(id, "Skewer", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, Card::COST_X,
                          { CE::Damage(up ? 10 : 7).XTimes() });

        case CardId::TACTICIAN:
            // Only does anything when it is thrown away.
            return Skill(id, "Tactician", CardRarity::UNCOMMON,
                         CardTarget::NONE, Card::COST_UNPLAYABLE, {},
                         CardFlag::UNPLAYABLE);

        case CardId::TERROR:
            return Skill(id, "Terror", CardRarity::UNCOMMON,
                         CardTarget::SINGLE_ENEMY, up ? 0 : 1,
                         { CE::ApplyPower(PowerType::VULNERABLE, 99) },
                         CardFlag::EXHAUST);

        case CardId::WELL_LAID_PLANS:
            return Power(id, "Well-Laid Plans", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::WELL_LAID_PLANS,
                                                up ? 2 : 1) });

        // Rare

        case CardId::A_THOUSAND_CUTS:
            return Power(id, "A Thousand Cuts", CardRarity::RARE, 2,
                         { CE::ApplyPowerToSelf(PowerType::THOUSAND_CUTS,
                                                up ? 2 : 1) });

        case CardId::ADRENALINE:
            return Skill(id, "Adrenaline", CardRarity::RARE, CardTarget::SELF,
                         0, { CE::Energy(up ? 2 : 1), CE::Draw(2) },
                         CardFlag::EXHAUST);

        case CardId::AFTER_IMAGE:
            return Power(id, "After Image", CardRarity::RARE, 1,
                         { CE::ApplyPowerToSelf(PowerType::AFTER_IMAGE, 1) },
                         up ? CardFlag::INNATE : CardFlag::NONE);

        case CardId::BULLET_TIME:
            return Skill(id, "Bullet Time", CardRarity::RARE, CardTarget::SELF,
                         up ? 2 : 3,
                         { CE::ApplyPowerToSelf(PowerType::FREE_CARDS, 1),
                           CE::ApplyPowerToSelf(PowerType::NO_DRAW, 1) });

        case CardId::BURST:
            return Skill(id, "Burst", CardRarity::RARE, CardTarget::SELF, 1,
                         { CE::ApplyPowerToSelf(PowerType::BURST,
                                                up ? 2 : 1) });

        case CardId::CORPSE_EXPLOSION:
            return Skill(id, "Corpse Explosion", CardRarity::RARE,
                         CardTarget::SINGLE_ENEMY, 2,
                         { CE::ApplyPower(PowerType::POISON, up ? 9 : 6),
                           CE::ApplyPower(PowerType::CORPSE_EXPLOSION, 1) });

        case CardId::DIE_DIE_DIE:
            return Attack(id, "Die Die Die", CardRarity::RARE,
                          CardTarget::ALL_ENEMIES, 1,
                          { CE::DamageAll(up ? 17 : 13) }, CardFlag::EXHAUST);

        case CardId::DOPPELGANGER:
            return Skill(id, "Doppelganger", CardRarity::RARE,
                         CardTarget::SELF, Card::COST_X,
                         { CE::ApplyPowerToSelf(PowerType::DRAW_NEXT_TURN,
                                                up ? 1 : 0)
                               .From(ValueSource::ENERGY_SPENT),
                           CE::ApplyPowerToSelf(PowerType::ENERGIZED,
                                                up ? 1 : 0)
                               .From(ValueSource::ENERGY_SPENT) });

        case CardId::ENVENOM:
            return Power(id, "Envenom", CardRarity::RARE, up ? 1 : 2,
                         { CE::ApplyPowerToSelf(PowerType::ENVENOM, 1) });

        case CardId::GLASS_KNIFE:
            return Attack(id, "Glass Knife", CardRarity::RARE,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 12 : 8, 2),
                            CE::IncreaseSelfDamage(-2) });

        case CardId::GRAND_FINALE:
        {
            Card card = Attack(id, "Grand Finale", CardRarity::RARE,
                               CardTarget::ALL_ENEMIES, 0,
                               { CE::DamageAll(up ? 60 : 50) });
            card.SetPlayCondition(PlayCondition::DRAW_PILE_EMPTY);

            return card;
        }

        case CardId::MALAISE:
            return Skill(id, "Malaise", CardRarity::RARE,
                         CardTarget::SINGLE_ENEMY, Card::COST_X,
                         { CE::ApplyPower(PowerType::STRENGTH, up ? 1 : 0)
                               .From(ValueSource::ENERGY_SPENT, -1),
                           CE::ApplyPower(PowerType::WEAK, up ? 1 : 0)
                               .From(ValueSource::ENERGY_SPENT) },
                         CardFlag::EXHAUST);

        case CardId::NIGHTMARE:
            return Skill(id, "Nightmare", CardRarity::RARE, CardTarget::SELF,
                         up ? 2 : 3, { CE::RememberCard(3) },
                         CardFlag::EXHAUST);

        case CardId::PHANTASMAL_KILLER:
            return Skill(id, "Phantasmal Killer", CardRarity::RARE,
                         CardTarget::SELF, up ? 0 : 1,
                         { CE::ApplyPowerToSelf(PowerType::PHANTASMAL, 1) });

        case CardId::STORM_OF_STEEL:
            return Skill(id, "Storm of Steel", CardRarity::RARE,
                         CardTarget::SELF, 1,
                         { CE::DiscardHand(),
                           CE::AddCard(CardId::SHIV, CardPile::HAND, 0, up)
                               .From(ValueSource::CARDS_DISCARDED, 1) });

        case CardId::TOOLS_OF_THE_TRADE:
            return Power(id, "Tools of the Trade", CardRarity::RARE,
                         up ? 0 : 1,
                         { CE::ApplyPowerToSelf(PowerType::TOOLS_OF_THE_TRADE,
                                                1) });

        case CardId::UNLOAD:
            return Attack(id, "Unload", CardRarity::RARE,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 18 : 14),
                            CE::DiscardHand(CardFilter::NON_ATTACK) });

        case CardId::WRAITH_FORM:
            return Power(id, "Wraith Form", CardRarity::RARE, 3,
                         { CE::ApplyPowerToSelf(PowerType::INTANGIBLE,
                                                up ? 3 : 2),
                           CE::ApplyPowerToSelf(PowerType::WRAITH_FORM, 1) });

        case CardId::ALCHEMIZE:
            return Skill(id, "Alchemize", CardRarity::RARE, CardTarget::SELF,
                         up ? 0 : 1, { CE::ObtainPotion() },
                         CardFlag::EXHAUST);

        // Made by other cards

        case CardId::SHIV:
            return Attack(id, "Shiv", CardRarity::SPECIAL,
                          CardTarget::SINGLE_ENEMY, 0,
                          { CE::Damage(up ? 6 : 4) }, CardFlag::EXHAUST);

        default:
            return Card();
    }
}
}  // namespace ConquerTheSpire::Detail
