// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Potions/PotionRegistry.hpp>

#include <utility>

namespace ConquerTheSpire
{
namespace
{
using CE = CardEffect;

Potion Drink(PotionId id, const char* name, PotionRarity rarity,
             std::vector<CardEffect> effects,
             CardTarget target = CardTarget::SELF)
{
    return Potion(id, name, rarity, target, std::move(effects));
}

//! A potion that is as good on the map as it is in a fight.
Potion Anywhere(PotionId id, const char* name, PotionRarity rarity,
                std::vector<CardEffect> effects)
{
    return Potion(id, name, rarity, CardTarget::SELF, std::move(effects),
                  PotionUse::ANYWHERE);
}

//! A potion that is never drunk on purpose: it waits for a climber to die.
Potion Waiting(PotionId id, const char* name, PotionRarity rarity)
{
    return Potion(id, name, rarity, CardTarget::SELF, {},
                  PotionUse::PASSIVE);
}

//! A potion the fight itself sees to, which carries no effects of its own.
Potion Special(PotionId id, const char* name, PotionRarity rarity)
{
    return Potion(id, name, rarity, CardTarget::SELF, {});
}

bool AvailableTo(PotionId id, CardColor character)
{
    switch (id)
    {
        case PotionId::BLOOD_POTION:
        case PotionId::ELIXIR:
        case PotionId::HEART_OF_IRON:
            return character == CardColor::RED;

        case PotionId::CUNNING_POTION:
        case PotionId::GHOST_IN_A_JAR:
        case PotionId::POISON_POTION:
            return character == CardColor::GREEN;

        case PotionId::ESSENCE_OF_DARKNESS:
        case PotionId::FOCUS_POTION:
        case PotionId::POTION_OF_CAPACITY:
            return character == CardColor::BLUE;

        case PotionId::BOTTLED_MIRACLE:
            return false;

        default:
            return true;
    }
}

constexpr PotionId LAST_POTION_ID = PotionId::SNECKO_OIL;
}  // namespace

Potion PotionRegistry::Get(PotionId id)
{
    switch (id)
    {
        // Common

        case PotionId::ANCIENT_POTION:
            return Drink(id, "Ancient Potion", PotionRarity::UNCOMMON,
                         { CE::ApplyPowerToSelf(PowerType::ARTIFACT, 1) });

        case PotionId::ATTACK_POTION:
            return Drink(id, "Attack Potion", PotionRarity::COMMON,
                         { CE::AddRandomAttack() });

        case PotionId::BLOCK_POTION:
            return Drink(id, "Block Potion", PotionRarity::COMMON,
                         { CE::Block(12) });

        case PotionId::BLOOD_POTION:
            return Anywhere(id, "Blood Potion", PotionRarity::COMMON,
                            { CE::HealPercent(20) });

        case PotionId::BOTTLED_MIRACLE:
            return Drink(id, "Bottled Miracle", PotionRarity::COMMON,
                         { CE::AddCard(CardId::MIRACLE, CardPile::HAND, 2) });

        case PotionId::COLORLESS_POTION:
            return Drink(id, "Colorless Potion", PotionRarity::COMMON,
                         { CE::AddRandomCard(1) });

        case PotionId::DEXTERITY_POTION:
            return Drink(id, "Dexterity Potion", PotionRarity::COMMON,
                         { CE::ApplyPowerToSelf(PowerType::DEXTERITY, 2) });

        case PotionId::ENERGY_POTION:
            return Drink(id, "Energy Potion", PotionRarity::COMMON,
                         { CE::Energy(2) });

        case PotionId::EXPLOSIVE_POTION:
            return Drink(id, "Explosive Potion", PotionRarity::COMMON,
                         { CE::DamageAll(10) }, CardTarget::ALL_ENEMIES);

        case PotionId::FEAR_POTION:
            return Drink(id, "Fear Potion", PotionRarity::COMMON,
                         { CE::ApplyPower(PowerType::VULNERABLE, 3) },
                         CardTarget::SINGLE_ENEMY);

        case PotionId::FIRE_POTION:
            return Drink(id, "Fire Potion", PotionRarity::COMMON,
                         { CE::Damage(20) }, CardTarget::SINGLE_ENEMY);

        case PotionId::FLEX_POTION:
            return Drink(id, "Flex Potion", PotionRarity::COMMON,
                         { CE::ApplyPowerToSelf(PowerType::STRENGTH, 5),
                           CE::ApplyPowerToSelf(PowerType::STRENGTH_DOWN, 5) });

        case PotionId::POWER_POTION:
            return Drink(id, "Power Potion", PotionRarity::COMMON,
                         { CE::AddRandomPower() });

        case PotionId::SKILL_POTION:
            return Drink(id, "Skill Potion", PotionRarity::COMMON,
                         { CE::AddRandomSkill() });

        case PotionId::SPEED_POTION:
            return Drink(id, "Speed Potion", PotionRarity::COMMON,
                         { CE::ApplyPowerToSelf(PowerType::DEXTERITY, 5),
                           CE::ApplyPowerToSelf(PowerType::DEXTERITY_DOWN,
                                                5) });

        case PotionId::STRENGTH_POTION:
            return Drink(id, "Strength Potion", PotionRarity::COMMON,
                         { CE::ApplyPowerToSelf(PowerType::STRENGTH, 2) });

        case PotionId::SWIFT_POTION:
            return Drink(id, "Swift Potion", PotionRarity::COMMON,
                         { CE::Draw(3) });

        case PotionId::WEAK_POTION:
            return Drink(id, "Weak Potion", PotionRarity::COMMON,
                         { CE::ApplyPower(PowerType::WEAK, 3) },
                         CardTarget::SINGLE_ENEMY);

        // Uncommon

        case PotionId::BLESSING_OF_THE_FORGE:
            return Drink(id, "Blessing of the Forge", PotionRarity::COMMON,
                         { CE::UpgradeHandCard(true) });

        case PotionId::CULTIST_POTION:
            // Ritual is the same thing Demon Form does: Strength every turn.
            return Drink(id, "Cultist Potion", PotionRarity::RARE,
                         { CE::ApplyPowerToSelf(PowerType::DEMON_FORM, 1) });

        case PotionId::DISTILLED_CHAOS:
            return Drink(id, "Distilled Chaos", PotionRarity::UNCOMMON,
                         { CE::PlayTopCard(), CE::PlayTopCard(),
                           CE::PlayTopCard() });

        case PotionId::DUPLICATION_POTION:
            return Drink(id, "Duplication Potion", PotionRarity::UNCOMMON,
                         { CE::ApplyPowerToSelf(PowerType::DUPLICATION, 1) });

        case PotionId::ELIXIR:
            // The real potion exhausts as many cards as the player likes; this
            // one takes up to three.
            return Drink(id, "Elixir", PotionRarity::UNCOMMON,
                         { CE::ExhaustHandCard(3, false) });

        case PotionId::ESSENCE_OF_STEEL:
            // Plated Armor is not modelled, so this hands over block.
            return Drink(id, "Essence of Steel", PotionRarity::UNCOMMON,
                         { CE::Block(4) });

        case PotionId::FOCUS_POTION:
            return Drink(id, "Focus Potion", PotionRarity::COMMON,
                         { CE::ApplyPowerToSelf(PowerType::FOCUS, 2) });

        case PotionId::GAMBLERS_BREW:
            return Drink(id, "Gambler's Brew", PotionRarity::UNCOMMON,
                         { CE::DiscardHand(),
                           CE::Draw(0).From(ValueSource::CARDS_DISCARDED,
                                            1) });

        case PotionId::LIQUID_BRONZE:
            return Drink(id, "Liquid Bronze", PotionRarity::UNCOMMON,
                         { CE::ApplyPowerToSelf(PowerType::THORNS, 3) });

        case PotionId::LIQUID_MEMORIES:
            return Drink(id, "Liquid Memories", PotionRarity::UNCOMMON,
                         { CE::ReturnFromDiscard() });

        case PotionId::POISON_POTION:
            return Drink(id, "Poison Potion", PotionRarity::COMMON,
                         { CE::ApplyPower(PowerType::POISON, 6) },
                         CardTarget::SINGLE_ENEMY);

        case PotionId::POTION_OF_CAPACITY:
            return Drink(id, "Potion of Capacity", PotionRarity::UNCOMMON,
                         { CE::AddOrbSlots(2) });

        case PotionId::REGEN_POTION:
            return Drink(id, "Regen Potion", PotionRarity::UNCOMMON,
                         { CE::ApplyPowerToSelf(PowerType::REGENERATION, 5) });

        // Rare

        case PotionId::CUNNING_POTION:
            return Drink(id, "Cunning Potion", PotionRarity::UNCOMMON,
                         { CE::AddCard(CardId::SHIV, CardPile::HAND, 3,
                                       true) });

        case PotionId::ENTROPIC_BREW:
            // Fills the belt, which is seen to where it is drunk.
            return Anywhere(id, "Entropic Brew", PotionRarity::RARE, {});

        case PotionId::ESSENCE_OF_DARKNESS:
            // One Dark orb for every slot; three is the usual number.
            return Drink(id, "Essence of Darkness", PotionRarity::RARE,
                         { CE::ChannelOrb(OrbType::DARK, 3) });

        case PotionId::FAIRY_IN_A_BOTTLE:
            // Drinks itself when the climber would die.
            return Waiting(id, "Fairy in a Bottle", PotionRarity::RARE);

        case PotionId::FRUIT_JUICE:
            return Anywhere(id, "Fruit Juice", PotionRarity::RARE,
                            { CE::IncreaseMaxHealth(5) });

        case PotionId::GHOST_IN_A_JAR:
            return Drink(id, "Ghost in a Jar", PotionRarity::RARE,
                         { CE::ApplyPowerToSelf(PowerType::INTANGIBLE, 1) });

        case PotionId::HEART_OF_IRON:
            return Drink(id, "Heart of Iron", PotionRarity::RARE,
                         { CE::ApplyPowerToSelf(PowerType::METALLICIZE, 6) });

        case PotionId::SMOKE_BOMB:
            // Walks out of the fight, which the fight sees to.
            return Special(id, "Smoke Bomb", PotionRarity::RARE);

        case PotionId::SNECKO_OIL:
            // The random costs are not modelled; the draw is.
            return Drink(id, "Snecko Oil", PotionRarity::RARE,
                         { CE::Draw(5) });

        case PotionId::INVALID:
            break;
    }

    return Potion();
}

const std::vector<PotionId>& PotionRegistry::GetAll()
{
    static const std::vector<PotionId> all = [] {
        std::vector<PotionId> built;

        for (int i = 1; i <= static_cast<int>(LAST_POTION_ID); ++i)
        {
            const PotionId id = static_cast<PotionId>(i);

            if (Get(id).GetId() != PotionId::INVALID)
            {
                built.emplace_back(id);
            }
        }

        return built;
    }();

    return all;
}

std::vector<PotionId> PotionRegistry::GetAll(CardColor character)
{
    std::vector<PotionId> matching;

    for (const PotionId id : GetAll())
    {
        if (AvailableTo(id, character))
        {
            matching.emplace_back(id);
        }
    }

    return matching;
}

std::vector<PotionId> PotionRegistry::GetPool(PotionRarity rarity)
{
    std::vector<PotionId> matching;

    for (const PotionId id : GetAll())
    {
        if (Get(id).GetRarity() == rarity)
        {
            matching.emplace_back(id);
        }
    }

    return matching;
}

std::vector<PotionId> PotionRegistry::GetPool(PotionRarity rarity,
                                              CardColor character)
{
    std::vector<PotionId> matching;

    for (const PotionId id : GetPool(rarity))
    {
        if (AvailableTo(id, character))
        {
            matching.emplace_back(id);
        }
    }

    return matching;
}

bool PotionRegistry::IsDoubledBySacredBark(PotionId id)
{
    switch (id)
    {
        case PotionId::BLESSING_OF_THE_FORGE:
        case PotionId::ELIXIR:
        case PotionId::ENTROPIC_BREW:
        case PotionId::GAMBLERS_BREW:
        case PotionId::SMOKE_BOMB:
            return false;

        default:
            return true;
    }
}
}  // namespace ConquerTheSpire
