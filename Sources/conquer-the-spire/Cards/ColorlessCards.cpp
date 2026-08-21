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
    return Card(id, name, CardColor::COLORLESS, CardType::ATTACK, rarity,
                target, cost, std::move(effects), flags);
}

Card Skill(CardId id, const char* name, CardRarity rarity, CardTarget target,
           int cost, std::vector<CardEffect> effects,
           CardFlag flags = CardFlag::NONE)
{
    return Card(id, name, CardColor::COLORLESS, CardType::SKILL, rarity, target,
                cost, std::move(effects), flags);
}

Card Power(CardId id, const char* name, CardRarity rarity, int cost,
           std::vector<CardEffect> effects, CardFlag flags = CardFlag::NONE)
{
    return Card(id, name, CardColor::COLORLESS, CardType::POWER, rarity,
                CardTarget::SELF, cost, std::move(effects), flags);
}
}  // namespace

Card MakeColorlessCard(CardId id, int upgradeCount)
{
    const bool up = upgradeCount > 0;

    switch (id)
    {
        // Uncommon

        case CardId::BANDAGE_UP:
            return Skill(id, "Bandage Up", CardRarity::UNCOMMON,
                         CardTarget::SELF, 0, { CE::Heal(up ? 6 : 4) },
                         CardFlag::EXHAUST);

        case CardId::BLIND:
            // Upgrading it spreads the Weak over everything.
            return Skill(id, "Blind", CardRarity::UNCOMMON,
                         up ? CardTarget::ALL_ENEMIES
                            : CardTarget::SINGLE_ENEMY,
                         0, { CE::ApplyPower(PowerType::WEAK, 2) });

        case CardId::DARK_SHACKLES:
            return Skill(id, "Dark Shackles", CardRarity::UNCOMMON,
                         CardTarget::SINGLE_ENEMY, 0,
                         { CE::ApplyPower(PowerType::STRENGTH, up ? -15 : -9),
                           CE::ApplyPower(PowerType::STRENGTH_UP,
                                          up ? 15 : 9) },
                         CardFlag::EXHAUST);

        case CardId::DEEP_BREATH:
            return Skill(id, "Deep Breath", CardRarity::UNCOMMON,
                         CardTarget::SELF, 0,
                         { CE::ReshuffleAll(up ? 2 : 1)
                               .From(ValueSource::FIXED, 1) });

        case CardId::DRAMATIC_ENTRANCE:
            return Attack(id, "Dramatic Entrance", CardRarity::UNCOMMON,
                          CardTarget::ALL_ENEMIES, 0,
                          { CE::DamageAll(up ? 12 : 8) },
                          CardFlag::INNATE | CardFlag::EXHAUST);

        case CardId::ENLIGHTENMENT:
            // The real card keeps the cost down all battle when upgraded; this
            // one only holds it for the turn either way.
            return Skill(id, "Enlightenment", CardRarity::UNCOMMON,
                         CardTarget::SELF, 0, { CE::SetHandCost(1) });

        case CardId::FINESSE:
            return Skill(id, "Finesse", CardRarity::UNCOMMON, CardTarget::SELF,
                         0, { CE::Block(up ? 4 : 2), CE::Draw(1) });

        case CardId::FLASH_OF_STEEL:
            return Attack(id, "Flash of Steel", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 0,
                          { CE::Damage(up ? 6 : 3), CE::Draw(1) });

        case CardId::FORETHOUGHT:
            return Skill(id, "Forethought", CardRarity::UNCOMMON,
                         CardTarget::SELF, 0,
                         { CE::SetupCard(CardPile::DRAW_BOTTOM) });

        case CardId::GOOD_INSTINCTS:
            return Skill(id, "Good Instincts", CardRarity::UNCOMMON,
                         CardTarget::SELF, 0, { CE::Block(up ? 9 : 6) });

        case CardId::IMPATIENCE:
            return Skill(id, "Impatience", CardRarity::UNCOMMON,
                         CardTarget::SELF, 0,
                         { CE::Draw(up ? 3 : 2)
                               .If(EffectCondition::NO_ATTACKS_IN_HAND) });

        case CardId::JACK_OF_ALL_TRADES:
            return Skill(id, "Jack of All Trades", CardRarity::UNCOMMON,
                         CardTarget::SELF, 0,
                         { CE::AddRandomCard(up ? 2 : 1) },
                         CardFlag::EXHAUST);

        case CardId::MADNESS:
            return Skill(id, "Madness", CardRarity::UNCOMMON, CardTarget::SELF,
                         up ? 0 : 1, { CE::SetHandCost(0, true) },
                         CardFlag::EXHAUST);

        case CardId::MIND_BLAST:
            return Attack(id, "Mind Blast", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, up ? 1 : 2,
                          { CE::Damage(0).From(ValueSource::DRAW_PILE_SIZE,
                                               1) },
                          CardFlag::INNATE);

        case CardId::PANACEA:
            return Skill(id, "Panacea", CardRarity::UNCOMMON, CardTarget::SELF,
                         0,
                         { CE::ApplyPowerToSelf(PowerType::ARTIFACT,
                                                up ? 2 : 1) },
                         CardFlag::EXHAUST);

        case CardId::PANIC_BUTTON:
            return Skill(id, "Panic Button", CardRarity::UNCOMMON,
                         CardTarget::SELF, 0,
                         { CE::Block(up ? 40 : 30),
                           CE::ApplyPowerToSelf(PowerType::NO_BLOCK, 2) },
                         CardFlag::EXHAUST);

        case CardId::PURITY:
            return Skill(id, "Purity", CardRarity::UNCOMMON, CardTarget::SELF,
                         0, { CE::ExhaustHandCard(up ? 5 : 3, false) },
                         CardFlag::EXHAUST);

        case CardId::SWIFT_STRIKE:
            return Attack(id, "Swift Strike", CardRarity::UNCOMMON,
                          CardTarget::SINGLE_ENEMY, 0,
                          { CE::Damage(up ? 10 : 7) });

        case CardId::TRIP:
            return Skill(id, "Trip", CardRarity::UNCOMMON,
                         up ? CardTarget::ALL_ENEMIES
                            : CardTarget::SINGLE_ENEMY,
                         0, { CE::ApplyPower(PowerType::VULNERABLE, 2) });

        // Rare

        case CardId::APOTHEOSIS:
            return Skill(id, "Apotheosis", CardRarity::RARE, CardTarget::SELF,
                         up ? 1 : 2,
                         { CE::UpgradeHandCard(true)
                               .From(ValueSource::FIXED, 1) },
                         CardFlag::EXHAUST);

        case CardId::CHRYSALIS:
            return Skill(id, "Chrysalis", CardRarity::RARE, CardTarget::SELF, 2,
                         { CE::AddRandomSkill(up ? 5 : 3,
                                             CardPile::DRAW_SHUFFLED) },
                         CardFlag::EXHAUST);

        case CardId::HAND_OF_GREED:
            // The gold it takes is outside a battle, so only the damage is
            // here.
            return Attack(id, "Hand of Greed", CardRarity::RARE,
                          CardTarget::SINGLE_ENEMY, 2,
                          { CE::Damage(up ? 25 : 20) });

        case CardId::MAGNETISM:
            return Power(id, "Magnetism", CardRarity::RARE, up ? 1 : 2,
                         { CE::ApplyPowerToSelf(PowerType::MAGNETISM, 1) });

        case CardId::MASTER_OF_STRATEGY:
            return Skill(id, "Master of Strategy", CardRarity::RARE,
                         CardTarget::SELF, 0, { CE::Draw(up ? 4 : 3) },
                         CardFlag::EXHAUST);

        case CardId::MAYHEM:
            return Power(id, "Mayhem", CardRarity::RARE, up ? 1 : 2,
                         { CE::ApplyPowerToSelf(PowerType::MAYHEM, 1) });

        case CardId::METAMORPHOSIS:
            return Skill(id, "Metamorphosis", CardRarity::RARE,
                         CardTarget::SELF, 2,
                         { CE::AddRandomAttack(up ? 5 : 3,
                                              CardPile::DRAW_SHUFFLED) },
                         CardFlag::EXHAUST);

        case CardId::PANACHE:
            return Power(id, "Panache", CardRarity::RARE, 0,
                         { CE::ApplyPowerToSelf(PowerType::PANACHE,
                                                up ? 14 : 10) });

        case CardId::SADISTIC_NATURE:
            return Power(id, "Sadistic Nature", CardRarity::RARE, 0,
                         { CE::ApplyPowerToSelf(PowerType::SADISTIC,
                                                up ? 7 : 5) });

        case CardId::SECRET_TECHNIQUE:
            return Skill(id, "Secret Technique", CardRarity::RARE,
                         CardTarget::SELF, 0,
                         { CE::TakeFromDrawPileByType(CardType::SKILL, 1) },
                         up ? CardFlag::NONE : CardFlag::EXHAUST);

        case CardId::SECRET_WEAPON:
            return Skill(id, "Secret Weapon", CardRarity::RARE,
                         CardTarget::SELF, 0,
                         { CE::TakeFromDrawPileByType(CardType::ATTACK, 1) },
                         up ? CardFlag::NONE : CardFlag::EXHAUST);

        case CardId::THE_BOMB:
            return Skill(id, "The Bomb", CardRarity::RARE, CardTarget::SELF, 2,
                         { CE::ApplyPowerToSelf(PowerType::THE_BOMB, 3)
                               .From(ValueSource::FIXED, up ? 50 : 40) });

        case CardId::THINKING_AHEAD:
            return Skill(id, "Thinking Ahead", CardRarity::RARE,
                         CardTarget::SELF, 0,
                         { CE::Draw(2), CE::HandToDrawTop() },
                         up ? CardFlag::NONE : CardFlag::EXHAUST);

        case CardId::TRANSMUTATION:
            return Skill(id, "Transmutation", CardRarity::RARE,
                         CardTarget::SELF, Card::COST_X,
                         { up ? CE::AddRandomCard(0)
                                    .From(ValueSource::ENERGY_SPENT)
                                    .Upgraded()
                              : CE::AddRandomCard(0)
                                    .From(ValueSource::ENERGY_SPENT) },
                         CardFlag::EXHAUST);

        case CardId::VIOLENCE:
            return Skill(id, "Violence", CardRarity::RARE, CardTarget::SELF, 0,
                         { CE::TakeFromDrawPileByType(CardType::ATTACK,
                                                      up ? 4 : 3) },
                         CardFlag::EXHAUST);

        // Handed out by relics and events

        case CardId::BITE:
            return Attack(id, "Bite", CardRarity::SPECIAL,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(up ? 8 : 7), CE::Heal(up ? 3 : 2) });

        case CardId::RITUAL_DAGGER:
            return Attack(id, "Ritual Dagger", CardRarity::SPECIAL,
                          CardTarget::SINGLE_ENEMY, 1,
                          { CE::Damage(15),
                            CE::IncreaseSelfDamage(up ? 5 : 3)
                                .If(EffectCondition::KILLED_TARGET) },
                          CardFlag::EXHAUST);

        case CardId::JAX:
            return Skill(id, "J.A.X.", CardRarity::SPECIAL, CardTarget::SELF, 0,
                         { CE::LoseHealth(3),
                           CE::ApplyPowerToSelf(PowerType::STRENGTH,
                                                up ? 3 : 2) });

        case CardId::INSIGHT:
            return Skill(id, "Insight", CardRarity::SPECIAL, CardTarget::SELF,
                         0, { CE::Draw(up ? 3 : 2) },
                         CardFlag::EXHAUST | CardFlag::RETAIN);

        case CardId::MIRACLE:
            return Skill(id, "Miracle", CardRarity::SPECIAL, CardTarget::SELF,
                         0, { CE::Energy(up ? 2 : 1) },
                         CardFlag::EXHAUST | CardFlag::RETAIN);

        case CardId::APPARITION:
            // The ghosts of a council hand these over: a turn out of reach,
            // and gone from the hand if it is not spent.
            return Skill(id, "Apparition", CardRarity::SPECIAL,
                         CardTarget::SELF, 1,
                         { CE::ApplyPowerToSelf(PowerType::INTANGIBLE, 1) },
                         up ? CardFlag::EXHAUST
                            : CardFlag::EXHAUST | CardFlag::ETHEREAL);

        case CardId::SAFETY:
            return Skill(id, "Safety", CardRarity::SPECIAL, CardTarget::SELF, 1,
                         { CE::Block(up ? 16 : 12) },
                         CardFlag::EXHAUST | CardFlag::RETAIN);

        default:
            return Card();
    }
}
}  // namespace ConquerTheSpire::Detail
