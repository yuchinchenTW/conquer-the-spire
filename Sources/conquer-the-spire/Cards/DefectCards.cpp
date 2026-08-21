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
    return Card(id, name, CardColor::BLUE, CardType::ATTACK, rarity, target,
                cost, std::move(effects), flags);
}

Card Skill(CardId id, const char* name, CardRarity rarity, CardTarget target,
           int cost, std::vector<CardEffect> effects,
           CardFlag flags = CardFlag::NONE)
{
    return Card(id, name, CardColor::BLUE, CardType::SKILL, rarity, target,
                cost, std::move(effects), flags);
}

Card Power(CardId id, const char* name, CardRarity rarity, int cost,
           std::vector<CardEffect> effects, CardFlag flags = CardFlag::NONE)
{
    return Card(id, name, CardColor::BLUE, CardType::POWER, rarity,
                CardTarget::SELF, cost, std::move(effects), flags);
}
}  // namespace

Card MakeDefectCard(CardId id, int upgradeCount)
{
    const bool up = upgradeCount > 0;

    switch (id)
    {
        // Basic

        case CardId::STRIKE_BLUE:
            return Attack(id, "Strike", CardRarity::BASIC,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 9 : 6) });

        case CardId::DEFEND_BLUE:
            return Skill(id, "Defend", CardRarity::BASIC, CardTarget::SELF, 1,
                         { CE::Block(up ? 8 : 5) });

        case CardId::ZAP:
            return Skill(id, "Zap", CardRarity::BASIC, CardTarget::SELF,
                         up ? 0 : 1,
                         { CE::ChannelOrb(OrbType::LIGHTNING) });

        case CardId::DUALCAST:
            return Skill(id, "Dualcast", CardRarity::BASIC, CardTarget::SELF,
                         up ? 0 : 1, { CE::EvokeOrb(2) });

        // Common

        case CardId::BALL_LIGHTNING:
            return Attack(id, "Ball Lightning", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 10 : 7),
                            CE::ChannelOrb(OrbType::LIGHTNING) });

        case CardId::BARRAGE:
            return Attack(id, "Barrage", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(0).From(ValueSource::ORB_COUNT,
                                               up ? 6 : 4) });

        case CardId::BEAM_CELL:
            return Attack(id, "Beam Cell", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 0,
                          { CE::Damage(up ? 4 : 3),
                            CE::ApplyPower(PowerType::VULNERABLE,
                                           up ? 2 : 1) });

        case CardId::CHARGE_BATTERY:
            return Skill(id, "Charge Battery", CardRarity::COMMON,
                         CardTarget::SELF, 1,
                         { CE::Block(up ? 10 : 7),
                           CE::ApplyPowerToSelf(PowerType::ENERGIZED, 1) });

        case CardId::CLAW:
            return Attack(id, "Claw", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 0,
                          { CE::Damage(up ? 5 : 3),
                            CE::IncreaseClawDamage(2) });

        case CardId::COLD_SNAP:
            return Attack(id, "Cold Snap", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 9 : 6),
                            CE::ChannelOrb(OrbType::FROST) });

        case CardId::COMPILE_DRIVER:
            return Attack(id, "Compile Driver", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 10 : 7),
                            CE::Draw(0).From(ValueSource::ORB_TYPES, 1) });

        case CardId::COOLHEADED:
            return Skill(id, "Coolheaded", CardRarity::COMMON, CardTarget::SELF,
                         1,
                         { CE::ChannelOrb(OrbType::FROST),
                           CE::Draw(up ? 2 : 1) });

        case CardId::GO_FOR_THE_EYES:
            return Attack(id, "Go for the Eyes", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 0,
                          { CE::Damage(up ? 4 : 3),
                            CE::ApplyPower(PowerType::WEAK, up ? 2 : 1)
                                .If(EffectCondition::TARGET_ATTACKING) });

        case CardId::HOLOGRAM:
            return Skill(id, "Hologram", CardRarity::COMMON, CardTarget::SELF,
                         1,
                         { CE::Block(up ? 5 : 3), CE::ReturnFromDiscard() },
                         up ? CardFlag::NONE : CardFlag::EXHAUST);

        case CardId::LEAP:
            return Skill(id, "Leap", CardRarity::COMMON, CardTarget::SELF, 1,
                         { CE::Block(up ? 12 : 9) });

        case CardId::REBOUND:
            // The card played next going back on top of the draw pile is not
            // modelled; the damage is.
            return Attack(id, "Rebound", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 12 : 9) });

        case CardId::RECURSION:
            return Skill(id, "Recursion", CardRarity::COMMON, CardTarget::SELF,
                         up ? 0 : 1, { CE::EvokeAndChannel() });

        case CardId::STACK:
            return Skill(id, "Stack", CardRarity::COMMON, CardTarget::SELF, 1,
                         { CE::Block(up ? 3 : 0)
                               .From(ValueSource::DISCARD_PILE_SIZE) });

        case CardId::STEAM_BARRIER:
            return Skill(id, "Steam Barrier", CardRarity::COMMON,
                         CardTarget::SELF, 0,
                         { CE::Block(up ? 8 : 6),
                           CE::IncreaseSelfBlock(-1) });

        case CardId::STREAMLINE:
            return Attack(id, "Streamline", CardRarity::COMMON,
                          CardTarget::SINGLE_ENEMY, 2,
                          { CE::Damage(up ? 20 : 15),
                            CE::ReduceSelfCost(1) });

        case CardId::SWEEPING_BEAM:
            return Attack(id, "Sweeping Beam", CardRarity::COMMON,
                          CardTarget::ALL_ENEMIES, 1,
                          { CE::DamageAll(up ? 9 : 6), CE::Draw(1) });

        case CardId::TURBO:
            return Skill(id, "TURBO", CardRarity::COMMON, CardTarget::SELF, 0,
                         { CE::Energy(up ? 3 : 2),
                           CE::AddCard(CardId::VOID, CardPile::DISCARD) });

        // Uncommon

        case CardId::AGGREGATE:
            return Skill(id, "Aggregate", CardRarity::UNCOMMON,
                         CardTarget::SELF, 1,
                         { CE::Energy(0).From(ValueSource::DRAW_PILE_SIZE,
                                              up ? 3 : 4) });

        case CardId::AUTO_SHIELDS:
            return Skill(id, "Auto-Shields", CardRarity::UNCOMMON,
                         CardTarget::SELF, 1,
                         { CE::Block(up ? 15 : 11)
                               .If(EffectCondition::PLAYER_HAS_NO_BLOCK) });

        case CardId::BLIZZARD:
            return Attack(id, "Blizzard", CardRarity::UNCOMMON,
                          CardTarget::ALL_ENEMIES, 1,
                          { CE::DamageAll(0)
                                .From(ValueSource::FROST_CHANNELED, up ? 3 : 2)
                                .AsOneHit() });

        case CardId::BOOT_SEQUENCE:
            return Skill(id, "Boot Sequence", CardRarity::UNCOMMON,
                         CardTarget::SELF, 0, { CE::Block(up ? 13 : 10) },
                         CardFlag::INNATE | CardFlag::EXHAUST);

        case CardId::BULLSEYE:
            return Attack(id, "Bullseye", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 11 : 8),
                            CE::ApplyPower(PowerType::LOCK_ON, up ? 3 : 2) });

        case CardId::CAPACITOR:
            return Power(id, "Capacitor", CardRarity::UNCOMMON, 1,
                         { CE::AddOrbSlots(up ? 3 : 2) });

        case CardId::CHAOS:
            return Skill(id, "Chaos", CardRarity::UNCOMMON, CardTarget::SELF, 1,
                         { CE::ChannelOrb(OrbType::INVALID, up ? 2 : 1) });

        case CardId::CHILL:
            // One Frost for each enemy in the fight.
            return Skill(id, "Chill", CardRarity::UNCOMMON, CardTarget::SELF, 0,
                         { CE::ChannelOrb(OrbType::FROST)
                               .From(ValueSource::ENEMY_COUNT, 1) },
                         up ? CardFlag::EXHAUST | CardFlag::INNATE
                            : CardFlag::EXHAUST);

        case CardId::CONSUME:
            return Skill(id, "Consume", CardRarity::UNCOMMON, CardTarget::SELF,
                         2,
                         { CE::ApplyPowerToSelf(PowerType::FOCUS, up ? 3 : 2),
                           CE::AddOrbSlots(-1) });

        case CardId::DARKNESS:
            return Skill(id, "Darkness", CardRarity::UNCOMMON,
                         CardTarget::SELF, 1,
                         up ? std::vector<CardEffect>{
                                  CE::ChannelOrb(OrbType::DARK),
                                  CE::TriggerDarkOrbs() }
                            : std::vector<CardEffect>{
                                  CE::ChannelOrb(OrbType::DARK) });

        case CardId::DEFRAGMENT:
            return Power(id, "Defragment", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::FOCUS,
                                                up ? 2 : 1) });

        case CardId::DOOM_AND_GLOOM:
            return Attack(id, "Doom and Gloom", CardRarity::UNCOMMON,
                          CardTarget::ALL_ENEMIES, 2,
                          { CE::DamageAll(up ? 14 : 10),
                            CE::ChannelOrb(OrbType::DARK) });

        case CardId::DOUBLE_ENERGY:
            return Skill(id, "Double Energy", CardRarity::UNCOMMON,
                         CardTarget::SELF, up ? 0 : 1,
                         { CE::DoubleEnergy() }, CardFlag::EXHAUST);

        case CardId::EQUILIBRIUM:
            return Skill(id, "Equilibrium", CardRarity::UNCOMMON,
                         CardTarget::SELF, 2,
                         { CE::Block(up ? 16 : 13),
                           CE::ApplyPowerToSelf(PowerType::RETAIN_HAND, 1) });

        case CardId::FORCE_FIELD:
        {
            Card card = Skill(id, "Force Field", CardRarity::UNCOMMON,
                              CardTarget::SELF, 4,
                              { CE::Block(up ? 16 : 12) });
            card.SetCostModifier(CostModifier::POWERS_PLAYED_THIS_BATTLE);

            return card;
        }

        case CardId::FTL:
            return Attack(id, "FTL", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 0,
                          { CE::Damage(up ? 6 : 5),
                            CE::Draw(1)
                                .If(EffectCondition::FEW_CARDS_PLAYED)
                                .Threshold(up ? 4 : 3) });

        case CardId::FUSION:
            return Skill(id, "Fusion", CardRarity::UNCOMMON, CardTarget::SELF,
                         up ? 1 : 2, { CE::ChannelOrb(OrbType::PLASMA) });

        case CardId::GENETIC_ALGORITHM:
            return Skill(id, "Genetic Algorithm", CardRarity::UNCOMMON,
                         CardTarget::SELF, 1,
                         { CE::Block(1), CE::IncreaseSelfBlock(up ? 3 : 2) },
                         CardFlag::EXHAUST);

        case CardId::GLACIER:
            return Skill(id, "Glacier", CardRarity::UNCOMMON, CardTarget::SELF,
                         2,
                         { CE::Block(up ? 10 : 7),
                           CE::ChannelOrb(OrbType::FROST, 2) });

        case CardId::HEATSINKS:
            return Power(id, "Heatsinks", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::HEATSINKS,
                                                up ? 2 : 1) });

        case CardId::HELLO_WORLD:
            return Power(id, "Hello World", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::HELLO_WORLD, 1) },
                         up ? CardFlag::INNATE : CardFlag::NONE);

        case CardId::LOOP:
            return Power(id, "Loop", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::LOOP,
                                                up ? 2 : 1) });

        case CardId::MELTER:
            return Attack(id, "Melter", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::RemoveBlock(), CE::Damage(up ? 14 : 10) });

        case CardId::OVERCLOCK:
            return Skill(id, "Overclock", CardRarity::UNCOMMON,
                         CardTarget::SELF, 0,
                         { CE::Draw(up ? 3 : 2),
                           CE::AddCard(CardId::BURN, CardPile::DISCARD) });

        case CardId::RECYCLE:
            return Skill(id, "Recycle", CardRarity::UNCOMMON, CardTarget::SELF,
                         up ? 0 : 1, { CE::ExhaustForEnergy() });

        case CardId::REINFORCED_BODY:
            return Skill(id, "Reinforced Body", CardRarity::UNCOMMON,
                         CardTarget::SELF, Card::COST_X,
                         { CE::Block(up ? 9 : 7).XTimes() });

        case CardId::REPROGRAM:
            return Skill(id, "Reprogram", CardRarity::UNCOMMON,
                         CardTarget::SELF, 1,
                         { CE::ApplyPowerToSelf(PowerType::FOCUS,
                                                up ? -2 : -1),
                           CE::ApplyPowerToSelf(PowerType::STRENGTH,
                                                up ? 2 : 1),
                           CE::ApplyPowerToSelf(PowerType::DEXTERITY,
                                                up ? 2 : 1) });

        case CardId::RIP_AND_TEAR:
            return Attack(id, "Rip and Tear", CardRarity::UNCOMMON,
                          CardTarget::RANDOM_ENEMY, 1,
                          { CE::DamageRandom(up ? 9 : 7, 2) });

        case CardId::SCRAPE:
            // Throwing the drawn cards that cost something back away is not
            // modelled; the draw is.
            return Attack(id, "Scrape", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 10 : 7), CE::Draw(up ? 5 : 4) });

        case CardId::SELF_REPAIR:
            // The healing lands when the battle is over, which the battle does
            // not model, so the power only records the amount.
            return Power(id, "Self Repair", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::SELF_REPAIR,
                                                up ? 10 : 7) });

        case CardId::SKIM:
            return Skill(id, "Skim", CardRarity::UNCOMMON, CardTarget::SELF, 1,
                         { CE::Draw(up ? 4 : 3) });

        case CardId::STATIC_DISCHARGE:
            return Power(id, "Static Discharge", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::STATIC_DISCHARGE,
                                                up ? 2 : 1) });

        case CardId::STORM:
            return Power(id, "Storm", CardRarity::UNCOMMON, 1,
                         { CE::ApplyPowerToSelf(PowerType::STORM, 1) },
                         up ? CardFlag::INNATE : CardFlag::NONE);

        case CardId::SUNDER:
            return Attack(id, "Sunder", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 3,
                          { CE::Damage(up ? 32 : 24),
                            CE::Energy(3).If(
                                EffectCondition::KILLED_TARGET) });

        case CardId::TEMPEST:
            return Skill(id, "Tempest", CardRarity::UNCOMMON, CardTarget::SELF,
                         Card::COST_X,
                         { CE::ChannelOrb(OrbType::LIGHTNING, up ? 1 : 0)
                               .From(ValueSource::ENERGY_SPENT) },
                         CardFlag::EXHAUST);

        case CardId::WHITE_NOISE:
            return Skill(id, "White Noise", CardRarity::UNCOMMON,
                         CardTarget::SELF, up ? 0 : 1,
                         { CE::AddRandomPower() }, CardFlag::EXHAUST);

        // Rare

        case CardId::ALL_FOR_ONE:
            return Attack(id, "All for One", CardRarity::RARE,
                          CardTarget::SINGLE_ENEMY, 2,
                          { CE::Damage(up ? 14 : 10),
                            CE::ReturnFromDiscard(true) });

        case CardId::BIASED_COGNITION:
            return Power(id, "Biased Cognition", CardRarity::RARE, 1,
                         { CE::ApplyPowerToSelf(PowerType::FOCUS,
                                                up ? 5 : 4),
                           CE::ApplyPowerToSelf(PowerType::BIASED_COGNITION,
                                                1) });

        case CardId::BUFFER:
            return Power(id, "Buffer", CardRarity::RARE, 2,
                         { CE::ApplyPowerToSelf(PowerType::BUFFER,
                                                up ? 2 : 1) });

        case CardId::CORE_SURGE:
            return Attack(id, "Core Surge", CardRarity::RARE,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 15 : 11),
                            CE::ApplyPowerToSelf(PowerType::ARTIFACT, 1) },
                          CardFlag::EXHAUST);

        case CardId::CREATIVE_AI:
            return Power(id, "Creative AI", CardRarity::RARE, up ? 2 : 3,
                         { CE::ApplyPowerToSelf(PowerType::CREATIVE_AI, 1) });

        case CardId::ECHO_FORM:
            // The base card is the ethereal one; upgrading takes that away.
            return Power(id, "Echo Form", CardRarity::RARE, 3,
                         { CE::ApplyPowerToSelf(PowerType::ECHO_FORM, 1) },
                         up ? CardFlag::NONE : CardFlag::ETHEREAL);

        case CardId::ELECTRODYNAMICS:
            return Power(id, "Electrodynamics", CardRarity::RARE, 2,
                         { CE::ApplyPowerToSelf(PowerType::ELECTRO, 1),
                           CE::ChannelOrb(OrbType::LIGHTNING, up ? 3 : 2) });

        case CardId::FISSION:
            // The energy and the cards are counted before the orbs go, and
            // only the upgraded card sets them off on the way out.
            return Skill(
                id, "Fission", CardRarity::RARE, CardTarget::SELF, 0,
                { CE::Energy(0).From(ValueSource::ORB_COUNT, 1),
                  CE::Draw(0).From(ValueSource::ORB_COUNT, 1),
                  up ? CE::EvokeAllOrbs() : CE::RemoveAllOrbs() },
                CardFlag::EXHAUST);

        case CardId::HYPERBEAM:
            return Attack(id, "Hyperbeam", CardRarity::RARE,
                          CardTarget::ALL_ENEMIES, 2,
                          { CE::DamageAll(up ? 34 : 26),
                            CE::ApplyPowerToSelf(PowerType::FOCUS, -3) });

        case CardId::MACHINE_LEARNING:
            return Power(id, "Machine Learning", CardRarity::RARE, 1,
                         { CE::ApplyPowerToSelf(PowerType::MACHINE_LEARNING,
                                                1) },
                         up ? CardFlag::INNATE : CardFlag::NONE);

        case CardId::METEOR_STRIKE:
            return Attack(id, "Meteor Strike", CardRarity::RARE,
                          CardTarget::SINGLE_ENEMY, 5,
                          { CE::Damage(up ? 30 : 24),
                            CE::ChannelOrb(OrbType::PLASMA, 3) });

        case CardId::MULTI_CAST:
            return Skill(id, "Multi-Cast", CardRarity::RARE, CardTarget::SELF,
                         Card::COST_X,
                         { CE::EvokeOrb(up ? 1 : 0)
                               .From(ValueSource::ENERGY_SPENT) });

        case CardId::RAINBOW:
            return Skill(id, "Rainbow", CardRarity::RARE, CardTarget::SELF, 2,
                         { CE::ChannelOrb(OrbType::LIGHTNING),
                           CE::ChannelOrb(OrbType::FROST),
                           CE::ChannelOrb(OrbType::DARK) },
                         up ? CardFlag::NONE : CardFlag::EXHAUST);

        case CardId::REBOOT:
            return Skill(id, "Reboot", CardRarity::RARE, CardTarget::SELF, 0,
                         { CE::ReshuffleAll(up ? 6 : 4) }, CardFlag::EXHAUST);

        case CardId::SEEK:
            return Skill(id, "Seek", CardRarity::RARE, CardTarget::SELF, 0,
                         { CE::TakeFromDrawPile(up ? 2 : 1) },
                         CardFlag::EXHAUST);

        case CardId::THUNDER_STRIKE:
            return Attack(id, "Thunder Strike", CardRarity::RARE,
                          CardTarget::RANDOM_ENEMY, 3,
                          { CE::Damage(0).From(
                              ValueSource::LIGHTNING_CHANNELED,
                              up ? 9 : 7) });

        default:
            return Card();
    }
}
}  // namespace ConquerTheSpire::Detail
