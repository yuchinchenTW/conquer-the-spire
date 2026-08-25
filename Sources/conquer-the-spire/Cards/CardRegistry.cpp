// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Cards/CardBuilders.hpp>
#include <conquer-the-spire/Cards/CardRegistry.hpp>

#include <map>

namespace ConquerTheSpire
{
namespace
{
//! The last id the registry walks when it collects the pools. Ids are
//! contiguous, so this is the whole table.
constexpr CardId LAST_CARD_ID = CardId::WRITHE;

//! Returns every known id grouped by colour, built once on first use.
const std::map<CardColor, std::vector<CardId>>& AllPools()
{
    static const std::map<CardColor, std::vector<CardId>> pools = [] {
        std::map<CardColor, std::vector<CardId>> built;

        for (int i = 1; i <= static_cast<int>(LAST_CARD_ID); ++i)
        {
            const CardId id = static_cast<CardId>(i);
            const Card card = CardRegistry::Get(id);

            if (card.GetId() == CardId::INVALID)
            {
                continue;
            }

            built[card.GetColor()].emplace_back(id);
        }

        return built;
    }();

    return pools;
}
}  // namespace

std::size_t CardRegistry::IdCount()
{
    // The last card of the list is a curse; a test makes sure every card of
    // every pool fits under this.
    return static_cast<std::size_t>(CardId::WRITHE) + 1u;
}

namespace
{
//! Adds up what a card hands over: damage, block, and one number for
//! everything else it does.
//!
//! The everything-else used to cover five kinds of effect out of fifty-two,
//! so a card built from any of the rest came out as damage 0, block 0,
//! power 0 - the same three numbers a Slimed reads as. Worse, the state puts
//! the difference between a card's worth and its sharpened worth beside it,
//! and nought minus nought says sharpening buys nothing. Entrench, Limit
//! Break, Havoc, Second Wind and Exhume were all worth nothing to look at,
//! and were drafted accordingly - Limit Break offered 2690 times and taken
//! none of them.
//!
//! The ones that multiply what is already there cannot be given a number
//! that is true in every state; they are counted as a modest something,
//! which is nearer than nothing.
//! What holding a power does for whoever holds it.
enum class PowerWorth
{
    //! Handed over once and done with: Strength, Dexterity, a heal.
    ONCE,

    //! Handed over again every turn, or on every trigger, until the fight
    //! ends.
    LASTING,

    //! Changes what the cards themselves do or cost, which is not a number
    //! at all.
    RULE,

    //! Bad to hold. Good to put on somebody else, and a price to put on
    //! yourself.
    HARM
};

//! What kind of thing \p power is to hold.
PowerWorth WorthOfPower(PowerType power)
{
    switch (power)
    {
        // Every turn, or on every trigger, for the rest of the fight.
        case PowerType::DEMON_FORM:
        case PowerType::METALLICIZE:
        case PowerType::PLATED_ARMOR:
        case PowerType::COMBUST:
        case PowerType::BRUTALITY:
        case PowerType::RITUAL:
        case PowerType::REGENERATION:
        case PowerType::THORNS:
        case PowerType::JUGGERNAUT:
        case PowerType::DARK_EMBRACE:
        case PowerType::FEEL_NO_PAIN:
        case PowerType::EVOLVE:
        case PowerType::FIRE_BREATHING:
        case PowerType::RUPTURE:
        case PowerType::NOXIOUS_FUMES:
        case PowerType::BERSERK:
        case PowerType::ENVENOM:
        case PowerType::THOUSAND_CUTS:
        case PowerType::INFINITE_BLADES:
        case PowerType::AFTER_IMAGE:
        case PowerType::ECHO_FORM:
        case PowerType::STORM:
        case PowerType::CREATIVE_AI:
        case PowerType::MACHINE_LEARNING:
        case PowerType::SELF_REPAIR:
        case PowerType::STATIC_DISCHARGE:
        case PowerType::HEATSINKS:
        case PowerType::LOOP:
        case PowerType::MAGNETISM:
        case PowerType::PANACHE:
        case PowerType::SADISTIC:
        case PowerType::TOOLS_OF_THE_TRADE:
        case PowerType::ACCURACY:
            return PowerWorth::LASTING;

        // The rules of the fight, not a number.
        case PowerType::CORRUPTION:
        case PowerType::BARRICADE:
        case PowerType::WRAITH_FORM:
        case PowerType::MAYHEM:
        case PowerType::RETAIN_HAND:
        case PowerType::WELL_LAID_PLANS:
        case PowerType::FREE_CARDS:
        case PowerType::DOUBLE_TAP:
        case PowerType::BURST:
        case PowerType::PHANTASMAL:
        case PowerType::BIASED_COGNITION:
        case PowerType::HELLO_WORLD:
        case PowerType::DUPLICATION:
        case PowerType::INTANGIBLE:
            return PowerWorth::RULE;

        // Bad to hold, whoever ends up holding it. The Bomb is not one of
        // these: it is put on the climber and goes off in the enemies' faces
        // three turns later, and calling it harm made a rare card read as a
        // price paid for nothing.
        case PowerType::VULNERABLE:
        case PowerType::WEAK:
        case PowerType::FRAIL:
        case PowerType::POISON:
        case PowerType::STRENGTH_DOWN:
        case PowerType::DEXTERITY_DOWN:
        case PowerType::NO_DRAW:
        case PowerType::NO_BLOCK:
        case PowerType::CONFUSED:
        case PowerType::ENTANGLED:
        case PowerType::CHOKED:
        case PowerType::CONSTRICTED:
        case PowerType::HEX:
        case PowerType::SLOW:
        case PowerType::LOCK_ON:
        case PowerType::DRAW_REDUCTION:
            return PowerWorth::HARM;

        default:
            return PowerWorth::ONCE;
    }
}

//! How many cards a hand thrown away is reckoned to hold. The real answer
//! is however many are left when it is played, which is not knowable from
//! the card.
constexpr int A_HAND = 4;

//! What holding \p id costs for a turn, where the fight does the harm
//! rather than the card. The numbers are health a turn where the curse deals
//! damage, and a stand-in of the same size where it does something else:
//! Doubt and Shame hand over a Weak and a Frail, Pain charges for every card
//! played, and the rest are simply a draw that does nothing.
//!
//! Battle::EndOfTurnCurses is where these actually happen; a curse added
//! there wants a line here.
int HarmOf(CardId id, int upgradeCount)
{
    switch (id)
    {
        case CardId::BURN:
            return upgradeCount > 0 ? 4 : 2;

        case CardId::DECAY:
            return 2;

        case CardId::REGRET:
            // A hand of cards, each one a point.
            return 4;

        case CardId::PAIN:
            // A point for every card played, and a turn is a few cards.
            return 3;

        case CardId::DOUBT:
        case CardId::SHAME:
            // A Weak or a Frail every turn, which is a quarter of a turn.
            return 2;

        case CardId::NORMALITY:
            // Three cards a turn and no more.
            return 2;

        case CardId::VOID:
            // An energy gone the moment it is drawn.
            return 2;

        case CardId::INJURY:
        case CardId::CLUMSY:
        case CardId::WRITHE:
        case CardId::PARASITE:
        case CardId::PRIDE:
        case CardId::ASCENDERS_BANE:
        case CardId::CURSE_OF_THE_BELL:
        case CardId::NECRONOMICURSE:
        case CardId::WOUND:
        case CardId::DAZED:
        case CardId::SLIMED:
            // Nothing of their own; a draw wasted is the whole of it.
            return 1;

        default:
            return 0;
    }
}

CardWorth WorthOf(const Card& card)
{
    // What a card that doubles or multiplies is called worth. Better than
    // zero and honest about being a stand-in.
    constexpr int MULTIPLIER = 5;


    // What a power that changes the rules is called worth. Corruption makes
    // every skill free for the rest of the fight; there is no honest number
    // for that, and one - which is what counting its single stack gave -
    // is the most dishonest one available.
    constexpr int RULE_WORTH = 3;

    CardWorth worth;

    // An unplayable card has a sentinel where its price would be. Written
    // out as it stands it reads as a card that hands energy back.
    worth.unplayable = card.IsPlayable() ? 0 : 1;
    worth.cost = std::max(0, card.GetCost());
    worth.rarity = std::max(0, static_cast<int>(card.GetRarity()) - 1);

    for (const auto& effect : card.GetEffects())
    {
        const int times = std::max(1, effect.times);

        // What one of it is worth. A figure read off the table has its rate
        // in the extra: Fiend Fire is written as nought damage from the
        // cards exhausted, seven each, and reading only the nought called
        // one of the strongest attacks in the game a card that does nothing.
        const int each = effect.valueSource != ValueSource::FIXED &&
                                 effect.value == 0
                             ? std::max(1, effect.extra)
                             : std::max(1, effect.value);
        const int value = each * times;

        if (effect.valueSource != ValueSource::FIXED)
        {
            worth.scales = 1;
        }

        switch (effect.type)
        {
            case EffectType::DEAL_DAMAGE:
                worth.damage += value;
                break;

            case EffectType::GAIN_BLOCK:
                worth.block += value;
                break;

            case EffectType::INCREASE_SELF_DAMAGE:
            case EffectType::INCREASE_CLAW_DAMAGE:
                worth.damage += value;
                break;

            case EffectType::INCREASE_SELF_BLOCK:
                worth.block += value;
                break;

            // Doubling what is already on the table.
            case EffectType::DOUBLE_BLOCK:
                worth.block += MULTIPLIER;
                break;

            case EffectType::DOUBLE_STRENGTH:
            case EffectType::MULTIPLY_TARGET_POWER:
                worth.power += MULTIPLIER;
                break;

            case EffectType::DOUBLE_ENERGY:
                worth.energy += MULTIPLIER;
                break;

            // The two that buy anything: a card drawn is any card, and an
            // energy is any card played. Kept apart from the rest, or the
            // strong ones read as the weak ones - Battle Trance draws three
            // and Flex gives two strength for a turn, and both came out at
            // power 4.
            case EffectType::GAIN_ENERGY:
            case EffectType::EXHAUST_FOR_ENERGY:
                worth.energy += value;
                break;

            case EffectType::DRAW_CARD:
            case EffectType::DRAW_UNTIL:
            case EffectType::DRAW_TO_HAND_FROM_TOP:
            case EffectType::RETURN_FROM_EXHAUST:
            case EffectType::RETURN_FROM_DISCARD:
            case EffectType::COPY_HAND_CARD:
                worth.draw += value;
                break;

            case EffectType::UPGRADE_HAND_CARD:
                // A value of nought means every card the climber holds, in
                // hand and in both piles, for the rest of the fight.
                // Counted as one thing handed over, Apotheosis - upgrade
                // the whole deck - read as worth the same as Armaments
                // sharpening a single card.
                if (effect.value == 0)
                {
                    worth.lasting += RULE_WORTH;
                }
                else
                {
                    worth.power += value;
                }

                break;

            case EffectType::APPLY_POWER:
            {
                // Who ends up holding it, and what holding it is worth.
                const bool onSelf =
                    effect.target == EffectTarget::SELF ||
                    (effect.target == EffectTarget::DEFAULT &&
                     card.GetTarget() == CardTarget::SELF);
                const PowerWorth holding = WorthOfPower(effect.power);

                if (holding == PowerWorth::HARM)
                {
                    // On somebody else it is worth having; on yourself it is
                    // what the card charges. Flex hands over two Strength
                    // and takes two back at the end of the turn, and both
                    // went to the power: it came out worth four, the biggest
                    // number on the table for a card that does nothing.
                    if (onSelf)
                    {
                        worth.cost += value;
                    }
                    else
                    {
                        worth.power += value;
                    }
                }
                else if (holding == PowerWorth::LASTING)
                {
                    worth.lasting += value;
                }
                else if (holding == PowerWorth::RULE)
                {
                    worth.lasting += RULE_WORTH;
                }
                else
                {
                    worth.power += value;
                }

                break;
            }

            case EffectType::HEAL:
            case EffectType::INCREASE_MAX_HEALTH:
            case EffectType::HEAL_PERCENT:
            case EffectType::PLAY_TOP_CARD:
            case EffectType::COPY_SELF_TO_DISCARD:
            case EffectType::DISCARD_TO_DRAW_TOP:
            case EffectType::HAND_TO_DRAW_TOP:
            case EffectType::REDUCE_SELF_COST:
            case EffectType::SET_HAND_COST:
            case EffectType::REMOVE_BLOCK:
            case EffectType::CHANNEL_ORB:
            case EffectType::EVOKE_ORB:
            case EffectType::EVOKE_ALL_ORBS:
            case EffectType::ADD_ORB_SLOTS:
            case EffectType::TRIGGER_DARK_ORBS:
            // Three held out and one taken is one card, not three. The
            // number on the card says how wide the choice is; being able to
            // pick is worth more than a roll, and how much more is left to
            // whoever reads this - what comes in is one card either way.
            case EffectType::OFFER_CARDS:
                ++worth.power;
                break;

            case EffectType::OBTAIN_POTION:
            case EffectType::SETUP_CARD:
            case EffectType::REMEMBER_CARD:
            case EffectType::TAKE_FROM_DRAW_BY_TYPE:
            case EffectType::ADD_RANDOM_ATTACK:
            case EffectType::ADD_RANDOM_SKILL:
            case EffectType::ADD_RANDOM_POWER:
            case EffectType::ADD_RANDOM_COMMON:
            case EffectType::ADD_RANDOM_CARD:

            case EffectType::RESHUFFLE_ALL:
            case EffectType::REMOVE_ALL_ORBS:
                worth.power += value;
                break;

            // What a card charges in health is its own number. Taken off
            // the power it hid the card - Bloodletting and Offering both
            // came out at exactly -1 - and rolled into the cost it hid the
            // one thing a fight has to know, which is what the card asks in
            // energy. Bloodletting and Offering are nought energy cards.
            case EffectType::LOSE_HEALTH:
                worth.health += value;
                break;

            // What it throws away of its own. Counted, not judged: see
            // CardWorth::exhausts.
            case EffectType::EXHAUST_HAND:
            case EffectType::DISCARD_HAND:
                worth.exhausts += A_HAND;
                break;

            case EffectType::EXHAUST_HAND_CARD:
            case EffectType::DISCARD_CARDS:
                worth.exhausts += value;
                break;

            case EffectType::ADD_CARD:
                // A card handed into a pile. A Burn or a Wound is a price;
                // a Shiv is not - Blade Dance hands over three of them and
                // came out asking four when its energy is one. So the kind
                // of card decides which way it counts.
                if (effect.cardId != CardId::INVALID &&
                    CardRegistry::Get(effect.cardId, 0).IsPlayable())
                {
                    worth.power += value;
                }
                else
                {
                    worth.cost += value;
                }

                break;

            case EffectType::INVALID:
                break;
        }
    }

    worth.harm = HarmOf(card.GetId(), card.GetUpgradeCount());

    // A card that exhausts itself is one more card gone from the fight.
    if (card.Has(CardFlag::EXHAUST))
    {
        ++worth.exhausts;
    }

    // And the three rules a card carries that no figure above it moves.
    // Sharpening is what these are here for: a Brutality is sharpened into
    // being innate, an Apparition out of being ethereal, and a Blind from one
    // thing to everything, and not one of the numbers above changes for any
    // of them. Without these the whole difference reads as nought.
    worth.innate = card.Has(CardFlag::INNATE) ? 1 : 0;
    worth.ethereal = card.Has(CardFlag::ETHEREAL) ? 1 : 0;
    worth.hitsAll = card.GetTarget() == CardTarget::ALL_ENEMIES ? 1 : 0;

    return worth;
}

//! How many sharpenings are worth telling apart. Past the second the only
//! card still changing is a Searing Blow, and one more of those is the same
//! shape of answer as the last.
constexpr int WORTH_UPGRADES = 15;
}  // namespace

const CardWorth& CardRegistry::Worth(CardId id, int upgradeCount)
{
    static std::map<std::pair<int, int>, CardWorth> known;
    static const CardWorth nothing;

    const int at = std::max(0, std::min(upgradeCount, WORTH_UPGRADES));
    const auto key = std::make_pair(static_cast<int>(id), at);
    const auto found = known.find(key);

    if (found != known.end())
    {
        return found->second;
    }

    const Card card = Get(id, at);

    if (card.GetId() == CardId::INVALID)
    {
        return nothing;
    }

    CardWorth worth = WorthOf(card);

    // A status keeps its name when it is sharpened, so the card cannot say
    // how many times it has been: a Burn that hits for four looks exactly
    // like one that hits for two. The count asked for is the one that knows.
    worth.harm = HarmOf(id, at);

    return known.emplace(key, worth).first->second;
}

bool CardRegistry::CanUpgrade(CardId id, int upgradeCount)
{
    const Card card = Get(id, std::max(0, upgradeCount));

    if (card.GetId() == CardId::INVALID ||
        card.GetCardType() == CardType::CURSE ||
        card.GetCardType() == CardType::STATUS)
    {
        return false;
    }

    const CardWorth& now = Worth(id, upgradeCount);
    const CardWorth& next = Worth(id, upgradeCount + 1);

    // Everything the table holds, not the four figures a sharpening most
    // often moves. A Limit Break is sharpened out of exhausting itself, a
    // Brutality into the opening hand, an Apparition out of burning itself,
    // a Blind from one thing to everything - and cost, damage, block and
    // power stand still for all of them. Asking only those four said there
    // was nothing to be had, which is also what the state then told a fire.
    return now.cost != next.cost || now.health != next.health ||
           now.damage != next.damage || now.block != next.block ||
           now.draw != next.draw || now.energy != next.energy ||
           now.power != next.power || now.lasting != next.lasting ||
           now.exhausts != next.exhausts || now.harm != next.harm ||
           now.unplayable != next.unplayable || now.scales != next.scales ||
           now.innate != next.innate || now.ethereal != next.ethereal ||
           now.hitsAll != next.hitsAll;
}

Card CardRegistry::Get(CardId id, int upgradeCount)
{
    Card card = Detail::MakeIroncladCard(id, upgradeCount);

    if (card.GetId() == CardId::INVALID)
    {
        card = Detail::MakeSilentCard(id, upgradeCount);
    }

    if (card.GetId() == CardId::INVALID)
    {
        card = Detail::MakeDefectCard(id, upgradeCount);
    }

    if (card.GetId() == CardId::INVALID)
    {
        card = Detail::MakeColorlessCard(id, upgradeCount);
    }

    if (card.GetId() == CardId::INVALID)
    {
        card = Detail::MakeStatusCard(id, upgradeCount);
    }

    if (card.GetId() == CardId::INVALID)
    {
        card = Detail::MakeCurseCard(id, upgradeCount);
    }

    // Statuses and curses have no upgraded form, so they keep their name.
    if (card.GetColor() != CardColor::STATUS &&
        card.GetColor() != CardColor::CURSE)
    {
        card.MarkUpgraded(upgradeCount);
    }

    return card;
}

const std::vector<CardId>& CardRegistry::GetPool(CardColor color)
{
    static const std::vector<CardId> empty;

    const auto& pools = AllPools();
    const auto iter = pools.find(color);

    return iter == pools.end() ? empty : iter->second;
}

std::vector<CardId> CardRegistry::GetPool(CardColor color, CardRarity rarity)
{
    std::vector<CardId> matching;

    for (const CardId id : GetPool(color))
    {
        if (Get(id).GetRarity() == rarity)
        {
            matching.emplace_back(id);
        }
    }

    return matching;
}

std::vector<CardId> CardRegistry::GetPoolByType(CardColor color,
                                                CardType type)
{
    std::vector<CardId> matching;

    for (const CardId id : GetPool(color))
    {
        const Card card = Get(id);
        const CardRarity rarity = card.GetRarity();

        // Basics are not in the reward pool, and neither are the cards other
        // cards make.
        if (card.GetCardType() != type || rarity == CardRarity::BASIC ||
            rarity == CardRarity::SPECIAL)
        {
            continue;
        }

        matching.emplace_back(id);
    }

    return matching;
}

std::vector<CardId> CardRegistry::GetPoolByRarity(CardColor color,
                                                  CardRarity rarity)
{
    return GetPool(color, rarity);
}

std::vector<CardId> CardRegistry::GetAttackPool(CardColor color)
{
    return GetPoolByType(color, CardType::ATTACK);
}

std::vector<Card> CardRegistry::MakeStarterDeck(CardColor color)
{
    std::vector<Card> deck;

    deck.reserve(10);

    if (color == CardColor::RED)
    {
        for (int i = 0; i < 5; ++i)
        {
            deck.emplace_back(Get(CardId::STRIKE_RED));
        }

        for (int i = 0; i < 4; ++i)
        {
            deck.emplace_back(Get(CardId::DEFEND_RED));
        }

        deck.emplace_back(Get(CardId::BASH));
    }
    else if (color == CardColor::BLUE)
    {
        for (int i = 0; i < 4; ++i)
        {
            deck.emplace_back(Get(CardId::STRIKE_BLUE));
        }

        for (int i = 0; i < 4; ++i)
        {
            deck.emplace_back(Get(CardId::DEFEND_BLUE));
        }

        deck.emplace_back(Get(CardId::ZAP));
        deck.emplace_back(Get(CardId::DUALCAST));
    }
    else if (color == CardColor::GREEN)
    {
        for (int i = 0; i < 5; ++i)
        {
            deck.emplace_back(Get(CardId::STRIKE_GREEN));
        }

        for (int i = 0; i < 5; ++i)
        {
            deck.emplace_back(Get(CardId::DEFEND_GREEN));
        }

        deck.emplace_back(Get(CardId::NEUTRALIZE));
        deck.emplace_back(Get(CardId::SURVIVOR));
    }

    return deck;
}
}  // namespace ConquerTheSpire
