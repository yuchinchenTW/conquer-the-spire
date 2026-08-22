// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Relics/RelicRegistry.hpp>
#include <conquer-the-spire/Rl/SpireEnv.hpp>

#include <algorithm>
#include <sstream>

namespace ConquerTheSpire
{
namespace
{
//! The powers the state keeps a number for. Everything else is left out
//! rather than making the vector unreadable.
const PowerType WATCHED_POWERS[] = {
    PowerType::STRENGTH,    PowerType::DEXTERITY,  PowerType::VULNERABLE,
    PowerType::WEAK,        PowerType::FRAIL,      PowerType::POISON,
    PowerType::ARTIFACT,    PowerType::INTANGIBLE, PowerType::THORNS,
    PowerType::METALLICIZE, PowerType::BARRICADE,  PowerType::DEMON_FORM,
    PowerType::CONFUSED,    PowerType::HEX,        PowerType::NO_DRAW,
    PowerType::ENERGIZED
};

constexpr std::size_t WATCHED_COUNT =
    sizeof(WATCHED_POWERS) / sizeof(WATCHED_POWERS[0]);

//! How many kinds of thing each part of the state takes up.
constexpr std::size_t PHASE_SLOTS = 10;
constexpr std::size_t RUN_SLOTS = 14;
constexpr std::size_t DECK_SUMMARY_SLOTS = 7;
constexpr std::size_t BATTLE_SLOTS = 9;
constexpr std::size_t INTENT_SLOTS = 12;
constexpr std::size_t MONSTER_SLOTS = 5 + INTENT_SLOTS;

//! How many piles of cards the state counts up: the deck, the draw pile and
//! the discard pile.
constexpr std::size_t COUNTED_PILES = 3;

//! How many kinds of reward there are, plus what else a reward slot says: how
//! much it is worth, whether it has been taken, and how many choices it holds.
constexpr std::size_t REWARD_KINDS = 7;
constexpr std::size_t REWARD_EXTRAS = 3;

//! What the shelf says: a price and a sold flag for every slot of it, and the
//! two numbers the card removal has.
constexpr std::size_t SHOP_EXTRAS = 2;

//! What a room says besides which one it is: the stage it is at and how many
//! times it has been answered.
constexpr std::size_t EVENT_EXTRAS = 2;

//! What an option of a room does to the climber. Without these the only way
//! to tell the option that pays in a curse from the one that pays in gold is
//! to have walked into the room often enough to remember it.
enum class OptionSignal : std::size_t
{
    GOLD_UP = 0,
    GOLD_DOWN,
    HEAL,
    HURT,
    MAX_HEALTH_UP,
    MAX_HEALTH_DOWN,
    RELIC,
    POTION,
    CARD,
    CURSE,
    REMOVE_CARD,
    UPGRADE_CARD,
    CHANGE_CARD,
    CLEANSE,
    LOSE_THING,
    FIGHT,

    //! An outcome nobody can read: a wheel, a wager, a hand in the ooze.
    GAMBLE,
    COUNT
};

constexpr std::size_t OPTION_SIGNALS =
    static_cast<std::size_t>(OptionSignal::COUNT);

//! What each option of a room says: whether it can be taken, what it costs,
//! and what it does.
constexpr std::size_t EVENT_OPTION_SLOTS = 2 + OPTION_SIGNALS;

//! What a monster means to do: how hard, how many times, how much it blocks,
//! and whether anything nasty comes with it.
constexpr std::size_t MOVE_SLOTS = 4;

//! What kind of card a slot holds: attack, skill, power, status or curse.
//! Which of those it is decides most of what there is to do about it, and a
//! curse is not something to be worked out from the name of the card.
constexpr std::size_t CARD_KINDS = 5;

//! What each slot of the deck says besides which card it is: whether there
//! is a card there at all, which kind it is, whether it has been sharpened,
//! whether sharpening it again would change anything, how many of that same
//! card the deck holds, what it is worth now - cost, damage, block, power -
//! and what sharpening it would add to each of those.
constexpr std::size_t DECK_CARD_SLOTS = 12 + CARD_KINDS;

//! What a card in hand is worth, beside what it costs and whether it can be
//! played: damage, block and power.
constexpr std::size_t HAND_WORTH_SLOTS = 3;

//! What can stand on a place on the map, counting everything but the empty
//! places: a fight, an elite, a question mark, a fire, a shop, a chest and a
//! boss.
constexpr std::size_t MAP_KINDS = 7;

//! What each place says: whether a path leads there from here, and which of
//! the kinds waits on it.
constexpr std::size_t MAP_NODE_SLOTS = 1 + MAP_KINDS;

//! How many rows ahead the map is read out. Two is enough to walk around an
//! elite towards a fire, which is most of what the map is for.
constexpr std::size_t MAP_ROWS_AHEAD = 2;

//! The map ahead: the rows read out, what the rest of the act still holds,
//! how much of the act is left and whether the boss is the next thing on it.
constexpr std::size_t MAP_SLOTS =
    MAP_ROWS_AHEAD * SpireEnv::MAP_COLUMNS * MAP_NODE_SLOTS + MAP_KINDS + 2;

//! Squashes \p value into something around zero to one.
float Scaled(int value, int by);

//! Writes out what a card is worth: damage, block and whatever else it hands
//! over. Three numbers, in that order.
void PushWorth(std::vector<float>& out, const CardWorth& worth);

//! Writes out one row of the map: every column of it, whether \p reachable
//! holds that column, and what waits there.
void PushRow(std::vector<float>& out, const Map& map, int row,
             const std::vector<int>& reachable);

//! Writes out what taking \p option would do, a number to a signal. An
//! option that is not there says nothing at all.
void PushOption(std::vector<float>& out, const EventOption* option);

//! Reads what a move is worth: the damage of one hit, how many hits, the
//! block, and whether it puts something on the climber.
void PushOption(std::vector<float>& out, const EventOption* option)
{
    float signals[OPTION_SIGNALS] = { 0.0f };

    const auto note = [&signals](OptionSignal signal, float much) {
        const auto at = static_cast<std::size_t>(signal);

        // The strongest of the effects that speak to the same signal, so
        // that a long option does not read as a big one.
        signals[at] = std::max(signals[at], much);
    };

    if (option != nullptr)
    {
        for (const auto& effect : option->effects)
        {
            const float count = Scaled(std::max(1, effect.count), 3);
            const float share = Scaled(effect.percent, 100);

            switch (effect.type)
            {
                case EventEffectType::GAIN_GOLD:
                    note(OptionSignal::GOLD_UP,
                         Scaled(std::max(effect.amount, effect.high), 300));
                    break;

                case EventEffectType::LOSE_GOLD:
                    note(OptionSignal::GOLD_DOWN,
                         Scaled(std::max(effect.amount, effect.high), 300));
                    break;

                case EventEffectType::LOSE_ALL_GOLD:
                    note(OptionSignal::GOLD_DOWN, 1.0f);
                    break;

                case EventEffectType::HEAL:
                    note(OptionSignal::HEAL, Scaled(effect.amount, 50));
                    break;

                case EventEffectType::HEAL_PERCENT:
                    note(OptionSignal::HEAL, share);
                    break;

                case EventEffectType::HEAL_FULL:
                    note(OptionSignal::HEAL, 1.0f);
                    break;

                case EventEffectType::LOSE_HEALTH:
                    note(OptionSignal::HURT, Scaled(effect.amount, 50));
                    break;

                case EventEffectType::LOSE_HEALTH_PERCENT:
                case EventEffectType::LOSE_HEALTH_PERCENT_CURRENT:
                    note(OptionSignal::HURT, share);
                    break;

                case EventEffectType::GAIN_MAX_HEALTH:
                    note(OptionSignal::MAX_HEALTH_UP,
                         Scaled(effect.amount, 20));
                    break;

                case EventEffectType::LOSE_MAX_HEALTH:
                    note(OptionSignal::MAX_HEALTH_DOWN,
                         Scaled(effect.amount, 20));
                    break;

                case EventEffectType::LOSE_MAX_HEALTH_PERCENT:
                    note(OptionSignal::MAX_HEALTH_DOWN, share);
                    break;

                case EventEffectType::GAIN_RELIC:
                case EventEffectType::GAIN_RANDOM_RELIC:
                case EventEffectType::GAIN_ONE_OF_RELICS:
                case EventEffectType::BOSS_RELIC_SWAP:
                    note(OptionSignal::RELIC, 1.0f);
                    break;

                case EventEffectType::GAIN_POTIONS:
                    note(OptionSignal::POTION, count);
                    break;

                case EventEffectType::GAIN_CARD:
                case EventEffectType::GAIN_RANDOM_CARDS:
                case EventEffectType::CARD_REWARD:
                    note(OptionSignal::CARD, count);
                    break;

                case EventEffectType::GAIN_CURSE:
                case EventEffectType::GAIN_RANDOM_CURSE:
                    // A curse that only might come says how likely it is;
                    // one that always comes says how many.
                    note(OptionSignal::CURSE,
                         effect.percent > 0 ? share : count);
                    break;

                case EventEffectType::REMOVE_CARDS:
                case EventEffectType::REMOVE_RANDOM_OF_TYPE:
                case EventEffectType::LOSE_CARD:
                    note(OptionSignal::REMOVE_CARD, count);
                    break;

                case EventEffectType::UPGRADE_CARDS:
                case EventEffectType::UPGRADE_RANDOM_CARDS:
                    note(OptionSignal::UPGRADE_CARD, count);
                    break;

                case EventEffectType::UPGRADE_ALL:
                case EventEffectType::UPGRADE_ALL_BASIC:
                    note(OptionSignal::UPGRADE_CARD, 1.0f);
                    break;

                case EventEffectType::TRANSFORM_CARDS:
                case EventEffectType::DUPLICATE_CARD:
                case EventEffectType::REPLACE_ALL_OF_TYPE:
                    note(OptionSignal::CHANGE_CARD, count);
                    break;

                case EventEffectType::CLEANSE_CURSES:
                    note(OptionSignal::CLEANSE, 1.0f);
                    break;

                case EventEffectType::LOSE_POTION:
                case EventEffectType::LOSE_RELIC:
                    note(OptionSignal::LOSE_THING, 1.0f);
                    break;

                case EventEffectType::FIGHT:
                case EventEffectType::FIGHT_OLD_BOSS:
                case EventEffectType::TO_THE_BOSS:
                    note(OptionSignal::FIGHT, 1.0f);
                    break;

                case EventEffectType::SPIN_WHEEL:
                case EventEffectType::WAGER:
                case EventEffectType::REACH_INTO_OOZE:
                case EventEffectType::SEARCH_BODY:
                case EventEffectType::TRADE_FACE:
                case EventEffectType::SKULL_TOLL:
                case EventEffectType::BURN_OFFERING:
                    note(OptionSignal::GAMBLE, 1.0f);
                    break;

                default:
                    break;
            }
        }
    }

    out.insert(out.end(), signals, signals + OPTION_SIGNALS);
}

void PushMove(std::vector<float>& out, const MonsterMove& move, int strength)
{
    int damage = 0;
    int hits = 0;
    int block = 0;
    bool nasty = false;

    for (const auto& effect : move.effects)
    {
        switch (effect.type)
        {
            case MonsterEffectType::DAMAGE:
            case MonsterEffectType::DAMAGE_SCALED:
                damage = effect.amount + strength;
                hits = effect.times;
                break;

            case MonsterEffectType::BLOCK:
            case MonsterEffectType::BLOCK_ALLY:
            case MonsterEffectType::BLOCK_ALLIES:
                block += effect.amount;
                break;

            case MonsterEffectType::APPLY_POWER:
                nasty = nasty || effect.toPlayer;
                break;

            case MonsterEffectType::ADD_CARD:
                nasty = true;
                break;

            default:
                break;
        }
    }

    out.emplace_back(Scaled(damage, 50));
    out.emplace_back(Scaled(hits, 6));
    out.emplace_back(Scaled(block, 40));
    out.emplace_back(nasty ? 1.0f : 0.0f);
}

//! Every relic there is, so that the state says which ones are carried.
std::size_t RelicSlots()
{
    return RelicRegistry::GetAll().size();
}

//! Every card there is, so that a hand slot and a pile can say which.
std::size_t CardSlots()
{
    return CardRegistry::IdCount();
}

//! Which slot of a card vector the card \p id sits in.
std::size_t SlotOfCard(CardId id)
{
    const std::size_t slot = static_cast<std::size_t>(id);

    return slot < CardSlots() ? slot : CardSlots() - 1u;
}

//! Counts the cards of \p pile into \p out, which is one slot a card.
void PushPile(std::vector<float>& out, const std::vector<Card>& pile)
{
    const std::size_t start = out.size();

    out.insert(out.end(), CardSlots(), 0.0f);

    for (const auto& card : pile)
    {
        out[start + SlotOfCard(card.GetId())] += 1.0f / 3.0f;
    }
}

float Scaled(int value, int by)
{
    return by <= 0 ? 0.0f
                   : static_cast<float>(value) / static_cast<float>(by);
}

void PushWorth(std::vector<float>& out, const CardWorth& worth)
{
    out.emplace_back(Scaled(worth.damage, 20));
    out.emplace_back(Scaled(worth.block, 20));
    out.emplace_back(Scaled(worth.power, 10));
}

void PushRow(std::vector<float>& out, const Map& map, int row,
             const std::vector<int>& reachable)
{
    for (int column = 0; column < static_cast<int>(SpireEnv::MAP_COLUMNS);
         ++column)
    {
        const bool open =
            std::find(reachable.begin(), reachable.end(), column) !=
            reachable.end();

        out.emplace_back(open ? 1.0f : 0.0f);

        // What waits there, but only where a path actually leads: what is on
        // a place that cannot be walked to is nothing to plan around.
        const MapNodeType type =
            open && Map::IsInside(row, column) && row < map.GetRows()
                ? map.GetNode(row, column).type
                : MapNodeType::EMPTY;

        for (std::size_t kind = 0; kind < MAP_KINDS; ++kind)
        {
            // The kinds run from MONSTER, the one after EMPTY, upwards.
            out.emplace_back(static_cast<std::size_t>(type) == kind + 1u
                                 ? 1.0f
                                 : 0.0f);
        }
    }
}

void PushPowers(std::vector<float>& out, const Creature& creature)
{
    for (const PowerType power : WATCHED_POWERS)
    {
        out.emplace_back(Scaled(creature.GetPower(power), 10));
    }
}
}  // namespace

Action::Action(ActionKind kind, int a, int b) : kind(kind), a(a), b(b)
{
    // Nothing else to set up.
}

void SpireEnv::Reset(CardColor character, unsigned int seed)
{
    m_run = Run(character, seed);
    m_battle.reset();
    m_phase = EnvPhase::MAP;
    m_totalFloors = 0;
    m_bossFight = false;
    m_counted = false;
    m_moves = 0;
    m_healthBefore = m_run.GetPlayer().GetHealth();
}

EnvPhase SpireEnv::GetPhase() const
{
    return m_phase;
}

const Run& SpireEnv::GetRun() const
{
    return m_run;
}

Run& SpireEnv::GetRun()
{
    return m_run;
}

const Battle* SpireEnv::GetBattle() const
{
    return m_battle.get();
}

bool SpireEnv::IsDone() const
{
    return m_phase == EnvPhase::OVER;
}

int SpireEnv::GetTotalFloors() const
{
    return m_totalFloors;
}

const RunStats& SpireEnv::GetStats() const
{
    return m_stats;
}

void SpireEnv::ClearStats()
{
    m_stats.Clear();
}

std::vector<Action> SpireEnv::LegalActions() const
{
    std::vector<Action> moves;

    switch (m_phase)
    {
        case EnvPhase::MAP:
            for (const int column : m_run.GetAvailableColumns())
            {
                moves.emplace_back(Action(ActionKind::TRAVEL, column));
            }

            break;

        case EnvPhase::BATTLE:
        {
            if (m_battle == nullptr)
            {
                break;
            }

            const std::size_t living = std::min(
                m_battle->GetLivingMonsterIndices().size(),
                OBSERVED_MONSTERS);

            for (const std::size_t hand : m_battle->GetPlayableCardIndices())
            {
                if (hand >= HAND_SLOTS)
                {
                    continue;
                }

                if (living == 0u)
                {
                    moves.emplace_back(
                        Action(ActionKind::PLAY_CARD,
                               static_cast<int>(hand), 0));
                    continue;
                }

                for (std::size_t target = 0; target < living; ++target)
                {
                    moves.emplace_back(Action(ActionKind::PLAY_CARD,
                                              static_cast<int>(hand),
                                              static_cast<int>(target)));
                }
            }

            const std::size_t potions =
                m_battle->GetPlayer().GetPotions().size();

            for (std::size_t potion = 0;
                 potion < std::min(potions, POTION_SLOTS); ++potion)
            {
                const int index = static_cast<int>(potion);

                if (living == 0u)
                {
                    if (m_battle->CanUsePotion(potion, 0u))
                    {
                        moves.emplace_back(
                            Action(ActionKind::USE_POTION, index));
                    }
                }
                else
                {
                    for (std::size_t target = 0; target < living; ++target)
                    {
                        if (!m_battle->CanUsePotion(
                                potion, TargetOf(static_cast<int>(target))))
                        {
                            continue;
                        }

                        moves.emplace_back(
                            Action(ActionKind::USE_POTION, index,
                                   static_cast<int>(target)));
                    }
                }

                moves.emplace_back(
                    Action(ActionKind::DISCARD_POTION, index));
            }

            moves.emplace_back(Action(ActionKind::END_TURN));
            break;
        }

        case EnvPhase::REWARD:
            for (std::size_t i = 0; i < m_run.GetRewards().size(); ++i)
            {
                const Reward& reward = m_run.GetRewards()[i];

                if (reward.claimed)
                {
                    continue;
                }

                // The same goes for a potion on the pile: with a full belt
                // it stays there however often it is reached for.
                if (reward.kind == RewardKind::POTION &&
                    !m_run.CanAddPotion())
                {
                    moves.emplace_back(
                        Action(ActionKind::SKIP_REWARD, static_cast<int>(i)));

                    continue;
                }

                const std::size_t choices =
                    std::max(reward.cards.size(), reward.relics.size());

                if (choices <= 1u)
                {
                    moves.emplace_back(Action(ActionKind::CLAIM_REWARD,
                                              static_cast<int>(i)));
                }
                else
                {
                    for (std::size_t option = 0; option < choices; ++option)
                    {
                        moves.emplace_back(
                            Action(ActionKind::CLAIM_REWARD,
                                   static_cast<int>(i),
                                   static_cast<int>(option)));
                    }
                }

                moves.emplace_back(
                    Action(ActionKind::SKIP_REWARD, static_cast<int>(i)));
            }

            moves.emplace_back(Action(ActionKind::LEAVE_REWARDS));
            break;

        case EnvPhase::EVENT:
            for (std::size_t i = 0; i < m_run.GetEvent().GetOptions().size();
                 ++i)
            {
                if (!m_run.CanChooseEventOption(i))
                {
                    continue;
                }

                // A room that works on a card is offered one move for each
                // card it could work on, and one that leaves the choice to
                // chance.
                moves.emplace_back(
                    Action(ActionKind::CHOOSE_OPTION, static_cast<int>(i),
                           -1));

                for (std::size_t card = 0; card < m_run.GetDeck().size();
                     ++card)
                {
                    moves.emplace_back(
                        Action(ActionKind::CHOOSE_OPTION,
                               static_cast<int>(i), static_cast<int>(card)));
                }
            }

            break;

        case EnvPhase::SHOP:
        {
            const Shop& shop = m_run.GetShop();

            for (std::size_t i = 0; i < shop.GetCards().size(); ++i)
            {
                if (!shop.GetCards()[i].sold &&
                    shop.GetCards()[i].price <= m_run.GetGold())
                {
                    moves.emplace_back(
                        Action(ActionKind::BUY_CARD, static_cast<int>(i)));
                }
            }

            for (std::size_t i = 0; i < shop.GetRelics().size(); ++i)
            {
                if (!shop.GetRelics()[i].sold &&
                    shop.GetRelics()[i].price <= m_run.GetGold())
                {
                    moves.emplace_back(
                        Action(ActionKind::BUY_RELIC, static_cast<int>(i)));
                }
            }

            for (std::size_t i = 0; i < shop.GetPotions().size(); ++i)
            {
                // A belt with no room in it would leave the potion on the
                // shelf and the gold in the purse: an offer that cannot be
                // taken is not a move.
                if (!shop.GetPotions()[i].sold &&
                    shop.GetPotions()[i].price <= m_run.GetGold() &&
                    m_run.CanAddPotion())
                {
                    moves.emplace_back(
                        Action(ActionKind::BUY_POTION, static_cast<int>(i)));
                }
            }

            if (!shop.IsRemovalSpent() &&
                shop.GetRemovalPrice() <= m_run.GetGold())
            {
                for (std::size_t i = 0; i < m_run.GetDeck().size(); ++i)
                {
                    // A curse that will not come out stays where it is,
                    // whatever is paid for it.
                    if (m_run.IsRemovable(i))
                    {
                        moves.emplace_back(Action(ActionKind::BUY_REMOVAL,
                                                  static_cast<int>(i)));
                    }
                }
            }

            moves.emplace_back(Action(ActionKind::LEAVE_SHOP));
            break;
        }

        case EnvPhase::REST:
        {
            const Player& player = m_run.GetPlayer();

            if (!player.HasRelic(RelicId::COFFEE_DRIPPER))
            {
                moves.emplace_back(Action(ActionKind::REST));
            }

            if (!player.HasRelic(RelicId::FUSION_HAMMER))
            {
                for (std::size_t i = 0; i < m_run.GetDeck().size(); ++i)
                {
                    // Only the cards a whetstone would actually change.
                    if (m_run.IsUpgradeable(i))
                    {
                        moves.emplace_back(
                            Action(ActionKind::SMITH, static_cast<int>(i)));
                    }
                }
            }

            if (player.HasRelic(RelicId::PEACE_PIPE))
            {
                for (std::size_t i = 0; i < m_run.GetDeck().size(); ++i)
                {
                    if (m_run.IsRemovable(i))
                    {
                        moves.emplace_back(Action(ActionKind::TOKE,
                                                  static_cast<int>(i)));
                    }
                }
            }

            if (player.HasRelic(RelicId::SHOVEL))
            {
                moves.emplace_back(Action(ActionKind::DIG));
            }

            if (player.HasRelic(RelicId::GIRYA) && m_run.GetLifts() < 3)
            {
                moves.emplace_back(Action(ActionKind::LIFT));
            }

            moves.emplace_back(Action(ActionKind::LEAVE_REST));
            break;
        }

        case EnvPhase::BOSS:
            moves.emplace_back(Action(ActionKind::FIGHT_BOSS));
            break;

        case EnvPhase::ACT_DONE:
            moves.emplace_back(Action(ActionKind::NEXT_ACT));
            break;

        default:
            break;
    }

    // A potion that is as good on the map as in a fight can be drunk wherever
    // the climber is standing, and any of them can be thrown away to make
    // room. These come after the moves of the room itself, so that the first
    // legal move is still the one the room is about.
    if (m_phase != EnvPhase::BATTLE && m_phase != EnvPhase::OVER)
    {
        const std::size_t held = m_run.GetPlayer().GetPotions().size();

        for (std::size_t potion = 0; potion < std::min(held, POTION_SLOTS);
             ++potion)
        {
            const int index = static_cast<int>(potion);

            if (m_run.CanDrinkPotion(potion))
            {
                moves.emplace_back(Action(ActionKind::USE_POTION, index));
            }

            moves.emplace_back(Action(ActionKind::DISCARD_POTION, index));
        }
    }

    return moves;
}

std::size_t SpireEnv::TargetOf(int ordinal) const
{
    if (m_battle == nullptr || ordinal < 0)
    {
        return 0u;
    }

    const std::vector<std::size_t> living =
        m_battle->GetLivingMonsterIndices();
    const std::size_t which = static_cast<std::size_t>(ordinal);

    // Naming a monster that is not there aims at nothing in particular, and
    // the fight turns the move down.
    return which < living.size() ? living[which]
                                 : m_battle->GetMonsters().size();
}

void SpireEnv::EnterRoom()
{
    switch (m_run.GetCurrentNodeType())
    {
        case MapNodeType::MONSTER:
        case MapNodeType::ELITE:
            m_bossFight = false;
            m_battle.reset(new Battle(m_run.StartBattleHere()));
            m_phase = EnvPhase::BATTLE;
            break;

        case MapNodeType::EVENT:
            m_run.StartEvent();
            m_phase = m_run.HasEvent() ? EnvPhase::EVENT : EnvPhase::MAP;
            break;

        case MapNodeType::MERCHANT:
            m_run.OpenShop();
            m_phase = EnvPhase::SHOP;
            break;

        case MapNodeType::TREASURE:
            m_run.OpenChest();
            m_phase = m_run.HasUnclaimedRewards() ? EnvPhase::REWARD
                                                  : EnvPhase::MAP;
            break;

        case MapNodeType::REST:
            m_phase = EnvPhase::REST;
            break;

        case MapNodeType::BOSS:
            // The top of the act. Without this the climb stands in front of
            // the boss with nowhere to walk and nothing to do, which reads
            // from the outside exactly like a climb that has ended.
            m_phase = m_run.IsAtBoss() ? EnvPhase::BOSS : EnvPhase::MAP;
            break;

        default:
            m_phase = EnvPhase::MAP;
            break;
    }
}

void SpireEnv::Settle()
{
    if (m_run.GetPlayer().IsDead())
    {
        m_phase = EnvPhase::OVER;

        return;
    }

    // A room that turned into a fight goes there first.
    if (m_run.HasPendingFight())
    {
        m_bossFight = false;
        m_battle.reset(new Battle(m_run.StartPendingBattle()));
        m_phase = EnvPhase::BATTLE;

        return;
    }

    if (m_run.HasEvent())
    {
        m_phase = EnvPhase::EVENT;

        return;
    }

    if (m_run.HasUnclaimedRewards())
    {
        m_phase = EnvPhase::REWARD;

        return;
    }

    m_run.ClearRewards();

    // Asked before the run is called finished, because IsFinished() only
    // says the act's boss is down - which is the end of an act, not of the
    // climb. Asked the other way round it answered for every act, the climb
    // stopped at the top of the first one whatever it had been asked for,
    // and this branch never ran at all: no ACT_DONE, no NEXT_ACT, no second
    // act, and the hundred points for the spire never paid to anyone.
    if (m_bossFight)
    {
        m_bossFight = false;

        // As far as this climb was asked to go. The boss has already paid
        // out by the time this is reached, so the climb is over on a win.
        m_phase = m_actLimit > 0 && m_run.GetAct() >= m_actLimit
                      ? EnvPhase::OVER
                      : EnvPhase::ACT_DONE;

        return;
    }

    if (m_run.IsFinished())
    {
        m_phase = EnvPhase::OVER;

        return;
    }

    m_phase = m_run.IsAtBoss() ? EnvPhase::BOSS : EnvPhase::MAP;
}

void SpireEnv::Close()
{
    m_phase = EnvPhase::OVER;

    // A climb that has finished is counted once, whatever happens next.
    if (!m_counted)
    {
        m_counted = true;
        m_stats.Ingest(m_run.GetLog());
    }
}

StepResult SpireEnv::Step(const Action& action)
{
    StepResult result;

    if (m_phase == EnvPhase::OVER)
    {
        result.done = true;

        return result;
    }

    // Counted here rather than at the end, because a move the phase turns
    // down returns from the middle of the switch below and never reached a
    // counter kept down there. A climb that cannot be moved on from - one
    // standing in a state with nothing legal in it - would then never be
    // called off at all, and the batch it sits in would never finish.
    ++m_moves;

    if (m_moves >= MOVE_LIMIT)
    {
        Close();
        result.done = true;

        return result;
    }

    // And the state with nothing legal in it is ended here rather than three
    // thousand turned-down moves later. Nothing can be played out of it, so
    // there is nothing to wait for.
    if (LegalActions().empty())
    {
        Close();
        result.done = true;

        return result;
    }

    const int healthBefore = m_run.GetPlayer().GetHealth();
    const int floorBefore = m_run.GetFloor();

    switch (action.kind)
    {
        case ActionKind::TRAVEL:
            if (m_phase != EnvPhase::MAP || !m_run.Travel(action.a))
            {
                return result;
            }

            result.taken = true;
            ++m_totalFloors;
            EnterRoom();
            break;

        case ActionKind::PLAY_CARD:
        {
            if (m_phase != EnvPhase::BATTLE || m_battle == nullptr ||
                !m_battle->PlayCard(static_cast<std::size_t>(action.a),
                                    TargetOf(action.b)))
            {
                return result;
            }

            result.taken = true;
            break;
        }

        case ActionKind::USE_POTION:
        {
            // Out of a fight it is the run that drinks.
            if (m_phase != EnvPhase::BATTLE || m_battle == nullptr)
            {
                if (!m_run.DrinkPotion(static_cast<std::size_t>(action.a)))
                {
                    return result;
                }

                result.taken = true;
                break;
            }

            const std::vector<Potion>& held =
                m_battle->GetPlayer().GetPotions();
            const std::size_t which = static_cast<std::size_t>(action.a);
            const PotionId drunk = which < held.size()
                                       ? held[which].GetId()
                                       : PotionId::INVALID;

            if (!m_battle->UsePotion(which, TargetOf(action.b)))
            {
                return result;
            }

            m_run.Note(LogEntry::POTION_DRUNK, static_cast<int>(drunk));
            result.taken = true;
            break;
        }

        case ActionKind::DISCARD_POTION:
        {
            if (m_phase != EnvPhase::BATTLE || m_battle == nullptr)
            {
                if (!m_run.DiscardPotion(
                        static_cast<std::size_t>(action.a)))
                {
                    return result;
                }

                result.taken = true;
                break;
            }

            std::vector<Potion>& held = m_battle->GetPlayer().GetPotions();
            const std::size_t index = static_cast<std::size_t>(action.a);

            if (index >= held.size())
            {
                return result;
            }

            held.erase(held.begin() +
                       static_cast<std::ptrdiff_t>(index));
            result.taken = true;
            break;
        }

        case ActionKind::END_TURN:
            if (m_phase != EnvPhase::BATTLE || m_battle == nullptr ||
                !m_battle->EndTurn())
            {
                return result;
            }

            result.taken = true;
            break;

        case ActionKind::CLAIM_REWARD:
            if (m_phase != EnvPhase::REWARD ||
                !m_run.ClaimReward(static_cast<std::size_t>(action.a),
                                   static_cast<std::size_t>(
                                       action.b < 0 ? 0 : action.b)))
            {
                return result;
            }

            result.taken = true;
            break;

        case ActionKind::SKIP_REWARD:
            if (m_phase != EnvPhase::REWARD ||
                !m_run.SkipReward(static_cast<std::size_t>(action.a)))
            {
                return result;
            }

            result.taken = true;
            break;

        case ActionKind::LEAVE_REWARDS:
            if (m_phase != EnvPhase::REWARD)
            {
                return result;
            }

            m_run.ClearRewards();
            result.taken = true;
            break;

        case ActionKind::CHOOSE_OPTION:
        {
            if (m_phase != EnvPhase::EVENT)
            {
                return result;
            }

            std::vector<std::size_t> picks;

            if (action.b >= 0)
            {
                picks.emplace_back(static_cast<std::size_t>(action.b));
            }

            if (!m_run.ChooseEventOption(
                    static_cast<std::size_t>(action.a), picks))
            {
                return result;
            }

            result.taken = true;
            break;
        }

        case ActionKind::BUY_CARD:
            if (m_phase != EnvPhase::SHOP ||
                !m_run.BuyCard(static_cast<std::size_t>(action.a)))
            {
                return result;
            }

            result.taken = true;
            break;

        case ActionKind::BUY_RELIC:
            if (m_phase != EnvPhase::SHOP ||
                !m_run.BuyRelic(static_cast<std::size_t>(action.a)))
            {
                return result;
            }

            result.taken = true;
            break;

        case ActionKind::BUY_POTION:
            if (m_phase != EnvPhase::SHOP ||
                !m_run.BuyPotion(static_cast<std::size_t>(action.a)))
            {
                return result;
            }

            result.taken = true;
            break;

        case ActionKind::BUY_REMOVAL:
            if (m_phase != EnvPhase::SHOP ||
                !m_run.BuyCardRemoval(static_cast<std::size_t>(action.a)))
            {
                return result;
            }

            result.taken = true;
            break;

        case ActionKind::LEAVE_SHOP:
            if (m_phase != EnvPhase::SHOP)
            {
                return result;
            }

            m_run.CloseShop();
            result.taken = true;
            break;

        case ActionKind::REST:
            if (m_phase != EnvPhase::REST || !m_run.Rest())
            {
                return result;
            }

            result.taken = true;
            break;

        case ActionKind::SMITH:
            if (m_phase != EnvPhase::REST ||
                !m_run.Smith(static_cast<std::size_t>(action.a)))
            {
                return result;
            }

            result.taken = true;
            break;

        case ActionKind::TOKE:
            if (m_phase != EnvPhase::REST ||
                !m_run.Toke(static_cast<std::size_t>(action.a)))
            {
                return result;
            }

            result.taken = true;
            break;

        case ActionKind::DIG:
            if (m_phase != EnvPhase::REST ||
                m_run.Dig() == RelicId::INVALID)
            {
                return result;
            }

            result.taken = true;
            break;

        case ActionKind::LIFT:
            if (m_phase != EnvPhase::REST || !m_run.Lift())
            {
                return result;
            }

            result.taken = true;
            break;

        case ActionKind::LEAVE_REST:
            if (m_phase != EnvPhase::REST)
            {
                return result;
            }

            result.taken = true;
            break;

        case ActionKind::FIGHT_BOSS:
            if (m_phase != EnvPhase::BOSS)
            {
                return result;
            }

            m_bossFight = true;
            m_battle.reset(new Battle(m_run.StartBattleHere()));
            m_phase = EnvPhase::BATTLE;
            result.taken = true;
            break;

        case ActionKind::NEXT_ACT:
            if (m_phase != EnvPhase::ACT_DONE || !m_run.AdvanceAct())
            {
                // Nowhere left to go: the spire is done with.
                if (m_phase == EnvPhase::ACT_DONE)
                {
                    m_phase = EnvPhase::OVER;
                    result.taken = true;
                    result.reward += WIN_REWARD;
                    result.done = true;
                }

                return result;
            }

            result.taken = true;
            m_phase = EnvPhase::MAP;
            break;

        default:
            return result;
    }

    // A fight that has finished pays out and hands the run back.
    if (m_phase == EnvPhase::BATTLE && m_battle != nullptr &&
        m_battle->IsDone())
    {
        const bool won = m_battle->GetPhase() == BattlePhase::WON;
        const bool elite = m_battle->IsEliteFight();
        const bool boss = m_battle->IsBossFight();

        m_run.FinishBattle(*m_battle);
        m_battle.reset();

        if (won)
        {
            if (boss)
            {
                result.reward += BOSS_REWARD;
                m_run.FinishBoss();
            }
            else if (elite)
            {
                result.reward += ELITE_REWARD;
            }
        }

        Settle();
    }
    else if (m_phase != EnvPhase::BATTLE)
    {
        // Everywhere else, whatever was going on may have finished.
        switch (action.kind)
        {
            case ActionKind::CHOOSE_OPTION:
            case ActionKind::CLAIM_REWARD:
            case ActionKind::SKIP_REWARD:
            case ActionKind::LEAVE_REWARDS:
            case ActionKind::LEAVE_SHOP:
            case ActionKind::REST:
            case ActionKind::SMITH:
            case ActionKind::TOKE:
            case ActionKind::DIG:
            case ActionKind::LIFT:
            case ActionKind::LEAVE_REST:
                Settle();
                break;

            default:
                break;
        }
    }

    if (m_run.GetPlayer().IsDead())
    {
        result.reward += DEATH_REWARD;
        m_phase = EnvPhase::OVER;
    }

    result.reward += FLOOR_REWARD *
                     static_cast<float>(m_run.GetFloor() - floorBefore);
    result.reward -=
        m_healthWeight *
        static_cast<float>(std::max(0, healthBefore -
                                          m_run.GetPlayer().GetHealth()));
    // The move limit is counted at the top, where every way through Step()
    // passes it.
    result.done = m_phase == EnvPhase::OVER;
    m_healthBefore = m_run.GetPlayer().GetHealth();

    if (result.done)
    {
        Close();
    }

    return result;
}

SpireEnv::Layout SpireEnv::GetLayout()
{
    Layout layout;

    layout.phase = 0;
    layout.run = layout.phase + PHASE_SLOTS;
    layout.deck = layout.run + RUN_SLOTS;
    layout.relics = layout.deck + DECK_SUMMARY_SLOTS;
    layout.battle = layout.relics + RelicSlots();
    layout.powers = layout.battle + BATTLE_SLOTS;
    layout.monsters = layout.powers + WATCHED_COUNT;
    layout.monsterStride = MONSTER_SLOTS + WATCHED_COUNT;
    layout.hand = layout.monsters + OBSERVED_MONSTERS * layout.monsterStride;
    // A hand slot no longer spells out which card it holds one flag at a
    // time: the card is an id beside the state, which is a name the learner
    // can carry from one slot to the next. What it costs, and what it hands
    // over if played, are numbers of their own.
    layout.handStride = HAND_EXTRAS + HAND_WORTH_SLOTS;
    layout.piles = layout.hand + HAND_SLOTS * layout.handStride;
    layout.pileStride = CardSlots();

    layout.rewards = layout.piles + COUNTED_PILES * layout.pileStride;
    layout.rewardStride = REWARD_KINDS + REWARD_EXTRAS;

    layout.shop = layout.rewards + REWARD_SLOTS * layout.rewardStride;
    layout.event =
        layout.shop +
        (SHOP_CARD_SLOTS + SHOP_RELIC_SLOTS + SHOP_POTION_SLOTS) *
            SHOP_EXTRAS +
        SHOP_EXTRAS;
    layout.eventStride = EVENT_OPTION_SLOTS;

    layout.potions =
        layout.event + EVENT_EXTRAS + EVENT_OPTIONS * layout.eventStride;
    layout.moves = layout.potions + POTION_SLOTS;
    layout.moveStride = MOVE_SLOTS;
    layout.map = layout.moves + OBSERVED_MONSTERS * layout.moveStride;
    layout.deckCards = layout.map + MAP_SLOTS;
    layout.deckStride = DECK_CARD_SLOTS;
    layout.total = layout.deckCards + DECK_SLOTS * layout.deckStride;

    return layout;
}

SpireEnv::IdLayout SpireEnv::GetIdLayout()
{
    IdLayout layout;

    layout.hand = 0;
    layout.potions = layout.hand + HAND_SLOTS;
    layout.relics = layout.potions + POTION_SLOTS;
    layout.rewardKinds = layout.relics + OBSERVED_RELICS;
    layout.rewardOptions = layout.rewardKinds + REWARD_SLOTS;
    layout.rewardOptionKinds =
        layout.rewardOptions + REWARD_SLOTS * OBSERVED_OPTIONS;
    layout.shopCards =
        layout.rewardOptionKinds + REWARD_SLOTS * OBSERVED_OPTIONS;
    layout.shopRelics = layout.shopCards + SHOP_CARD_SLOTS;
    layout.shopPotions = layout.shopRelics + SHOP_RELIC_SLOTS;
    layout.event = layout.shopPotions + SHOP_POTION_SLOTS;
    layout.monsters = layout.event + 1u;
    layout.deck = layout.monsters + OBSERVED_MONSTERS;
    layout.total = layout.deck + DECK_SLOTS;

    return layout;
}

std::size_t SpireEnv::IdCount()
{
    return GetIdLayout().total;
}

std::vector<int> SpireEnv::ObserveIds() const
{
    const IdLayout layout = GetIdLayout();
    std::vector<int> out(layout.total, 0);

    const Player& player =
        m_battle == nullptr ? m_run.GetPlayer() : m_battle->GetPlayer();

    // What is in hand, and what is in the belt.
    if (m_battle != nullptr)
    {
        for (std::size_t i = 0;
             i < std::min(player.GetHand().size(), HAND_SLOTS); ++i)
        {
            out[layout.hand + i] =
                static_cast<int>(player.GetHand()[i].GetId());
        }
    }

    for (std::size_t i = 0;
         i < std::min(player.GetPotions().size(), POTION_SLOTS); ++i)
    {
        out[layout.potions + i] =
            static_cast<int>(player.GetPotions()[i].GetId());
    }

    for (std::size_t i = 0;
         i < std::min(player.GetRelics().size(), OBSERVED_RELICS); ++i)
    {
        out[layout.relics + i] =
            static_cast<int>(player.GetRelics()[i].GetId());
    }

    // What is on the reward pile, choice by choice.
    const std::vector<Reward>& rewards = m_run.GetRewards();

    for (std::size_t i = 0; i < std::min(rewards.size(), REWARD_SLOTS); ++i)
    {
        const Reward& reward = rewards[i];

        out[layout.rewardKinds + i] = static_cast<int>(reward.kind);

        for (std::size_t option = 0; option < OBSERVED_OPTIONS; ++option)
        {
            const std::size_t at = i * OBSERVED_OPTIONS + option;

            if (option < reward.cards.size())
            {
                out[layout.rewardOptions + at] =
                    static_cast<int>(reward.cards[option]);
                out[layout.rewardOptionKinds + at] = ITEM_CARD;
            }
            else if (option < reward.relics.size())
            {
                out[layout.rewardOptions + at] =
                    static_cast<int>(reward.relics[option]);
                out[layout.rewardOptionKinds + at] = ITEM_RELIC;
            }
            else if (option == 0u && reward.potion != PotionId::INVALID)
            {
                out[layout.rewardOptions + at] =
                    static_cast<int>(reward.potion);
                out[layout.rewardOptionKinds + at] = ITEM_POTION;
            }
        }
    }

    // What is on the shelf.
    const Shop& shop = m_run.GetShop();

    for (std::size_t i = 0;
         i < std::min(shop.GetCards().size(), SHOP_CARD_SLOTS); ++i)
    {
        if (!shop.GetCards()[i].sold)
        {
            out[layout.shopCards + i] =
                static_cast<int>(shop.GetCards()[i].id);
        }
    }

    for (std::size_t i = 0;
         i < std::min(shop.GetRelics().size(), SHOP_RELIC_SLOTS); ++i)
    {
        if (!shop.GetRelics()[i].sold)
        {
            out[layout.shopRelics + i] =
                static_cast<int>(shop.GetRelics()[i].id);
        }
    }

    for (std::size_t i = 0;
         i < std::min(shop.GetPotions().size(), SHOP_POTION_SLOTS); ++i)
    {
        if (!shop.GetPotions()[i].sold)
        {
            out[layout.shopPotions + i] =
                static_cast<int>(shop.GetPotions()[i].id);
        }
    }

    // Which room this is, and what is standing in the way.
    out[layout.event] = static_cast<int>(m_run.GetEvent().GetId());

    // And which card sits in each slot of the deck.
    const std::vector<Card>& deck = m_run.GetDeck();

    for (std::size_t slot = 0; slot < std::min(deck.size(), DECK_SLOTS);
         ++slot)
    {
        out[layout.deck + slot] = static_cast<int>(deck[slot].GetId());
    }

    if (m_battle != nullptr)
    {
        const std::vector<std::size_t> living =
            m_battle->GetLivingMonsterIndices();

        for (std::size_t i = 0; i < std::min(living.size(), OBSERVED_MONSTERS);
             ++i)
        {
            out[layout.monsters + i] = static_cast<int>(
                m_battle->GetMonsters()[living[i]].GetMonsterId());
        }
    }

    return out;
}

void SpireEnv::SetHealthWeight(float weight)
{
    m_healthWeight = weight >= 0.0f ? weight : 0.0f;
}

float SpireEnv::GetHealthWeight() const
{
    return m_healthWeight;
}

void SpireEnv::SetActLimit(int acts)
{
    m_actLimit = acts > 0 ? acts : 0;
}

int SpireEnv::GetActLimit() const
{
    return m_actLimit;
}

std::size_t SpireEnv::ObservationSize()
{
    return GetLayout().total;
}

std::vector<float> SpireEnv::Observe() const
{
    std::vector<float> out;

    out.reserve(ObservationSize());

    // Where the climber is standing.
    for (std::size_t i = 0; i < PHASE_SLOTS; ++i)
    {
        out.emplace_back(static_cast<std::size_t>(m_phase) == i ? 1.0f
                                                                : 0.0f);
    }

    const Player& player =
        m_battle == nullptr ? m_run.GetPlayer() : m_battle->GetPlayer();

    out.emplace_back(Scaled(m_run.GetAct(), 4));
    out.emplace_back(Scaled(m_run.GetFloor(), Map::ROWS));
    out.emplace_back(Scaled(m_run.GetColumn() + 1, Map::COLUMNS));
    out.emplace_back(Scaled(m_run.GetGold(), 999));
    out.emplace_back(Scaled(player.GetHealth(), player.GetMaxHealth()));
    out.emplace_back(Scaled(player.GetMaxHealth(), 200));
    out.emplace_back(Scaled(static_cast<int>(player.GetPotions().size()),
                            player.GetPotionSlots()));
    out.emplace_back(Scaled(m_run.GetPathSkips(), 3));
    out.emplace_back(Scaled(m_run.GetLifts(), 3));
    out.emplace_back(m_run.HasKey(KeyType::RUBY) ? 1.0f : 0.0f);
    out.emplace_back(m_run.HasKey(KeyType::EMERALD) ? 1.0f : 0.0f);
    out.emplace_back(m_run.HasKey(KeyType::SAPPHIRE) ? 1.0f : 0.0f);
    out.emplace_back(Scaled(m_run.GetFightCount(), Map::ROWS));
    out.emplace_back(Scaled(m_totalFloors, 60));

    // What the deck is made of.
    int upgraded = 0;
    int attacks = 0;
    int skills = 0;
    int powers = 0;
    int statuses = 0;
    int curses = 0;

    for (const auto& card : m_run.GetDeck())
    {
        upgraded += card.IsUpgraded() ? 1 : 0;

        switch (card.GetCardType())
        {
            case CardType::ATTACK:
                ++attacks;
                break;

            case CardType::SKILL:
                ++skills;
                break;

            case CardType::POWER:
                ++powers;
                break;

            case CardType::STATUS:
                ++statuses;
                break;

            case CardType::CURSE:
                ++curses;
                break;

            default:
                break;
        }
    }

    out.emplace_back(Scaled(static_cast<int>(m_run.GetDeck().size()), 50));
    out.emplace_back(Scaled(upgraded, 50));
    out.emplace_back(Scaled(attacks, 20));
    out.emplace_back(Scaled(skills, 20));
    out.emplace_back(Scaled(powers, 20));
    out.emplace_back(Scaled(statuses, 20));
    out.emplace_back(Scaled(curses, 20));

    // Which relics are being carried.
    for (const RelicId id : RelicRegistry::GetAll())
    {
        out.emplace_back(player.HasRelic(id) ? 1.0f : 0.0f);
    }

    // The fight, if there is one.
    const bool fighting = m_battle != nullptr;

    out.emplace_back(fighting ? 1.0f : 0.0f);
    out.emplace_back(fighting ? Scaled(m_battle->GetTurn(), 20) : 0.0f);
    out.emplace_back(fighting ? Scaled(player.GetEnergy(), 10) : 0.0f);
    out.emplace_back(fighting ? Scaled(player.GetBlock(), 60) : 0.0f);
    out.emplace_back(
        fighting ? Scaled(static_cast<int>(player.GetHand().size()), 10)
                 : 0.0f);
    out.emplace_back(
        fighting ? Scaled(static_cast<int>(player.GetDrawPile().size()), 50)
                 : 0.0f);
    out.emplace_back(
        fighting
            ? Scaled(static_cast<int>(player.GetDiscardPile().size()), 50)
            : 0.0f);
    out.emplace_back(
        fighting
            ? Scaled(static_cast<int>(player.GetExhaustPile().size()), 50)
            : 0.0f);
    out.emplace_back(fighting && m_battle->AreIntentsVisible() ? 1.0f
                                                              : 0.0f);

    PushPowers(out, player);

    // And what is standing across from the climber.
    const std::vector<std::size_t> living =
        fighting ? m_battle->GetLivingMonsterIndices()
                 : std::vector<std::size_t>();

    for (std::size_t i = 0; i < OBSERVED_MONSTERS; ++i)
    {
        if (i >= living.size())
        {
            out.insert(out.end(), MONSTER_SLOTS + WATCHED_COUNT, 0.0f);
            continue;
        }

        const Monster& monster = m_battle->GetMonsters()[living[i]];

        out.emplace_back(1.0f);
        out.emplace_back(
            Scaled(monster.GetHealth(), monster.GetMaxHealth()));
        out.emplace_back(Scaled(monster.GetMaxHealth(), 500));
        out.emplace_back(Scaled(monster.GetBlock(), 60));
        out.emplace_back(
            monster.GetMonsterType() == MonsterType::BOSS ? 1.0f : 0.0f);

        const std::size_t intent =
            m_battle->AreIntentsVisible()
                ? static_cast<std::size_t>(monster.GetIntent())
                : 0u;

        for (std::size_t slot = 0; slot < INTENT_SLOTS; ++slot)
        {
            out.emplace_back(slot == intent ? 1.0f : 0.0f);
        }

        PushPowers(out, monster);
    }

    // What is in hand, card by card, with what it costs and whether it can
    // be played at all right now.
    for (std::size_t slot = 0; slot < HAND_SLOTS; ++slot)
    {
        const bool held = fighting && slot < player.GetHand().size();

        if (!held)
        {
            out.insert(out.end(), HAND_EXTRAS + HAND_WORTH_SLOTS, 0.0f);
            continue;
        }

        const Card& card = player.GetHand()[slot];

        out.emplace_back(Scaled(m_battle->GetEffectiveCost(card), 6));
        out.emplace_back(card.IsUpgraded() ? 1.0f : 0.0f);
        out.emplace_back(m_battle->CanPlay(slot) ? 1.0f : 0.0f);

        PushWorth(out, CardRegistry::Worth(card.GetId(),
                                           card.GetUpgradeCount()));
    }

    // And what the piles hold, counted up card by card.
    PushPile(out, m_run.GetDeck());
    PushPile(out, fighting ? player.GetDrawPile() : std::vector<Card>());
    PushPile(out, fighting ? player.GetDiscardPile() : std::vector<Card>());

    // What is on the reward pile: what kind each one is, what it is worth,
    // whether it has been taken, and how many ways it can be taken.
    const std::vector<Reward>& rewards = m_run.GetRewards();

    for (std::size_t i = 0; i < REWARD_SLOTS; ++i)
    {
        if (i >= rewards.size())
        {
            out.insert(out.end(), REWARD_KINDS + REWARD_EXTRAS, 0.0f);
            continue;
        }

        const Reward& reward = rewards[i];

        for (std::size_t kind = 0; kind < REWARD_KINDS; ++kind)
        {
            out.emplace_back(
                static_cast<std::size_t>(reward.kind) == kind ? 1.0f : 0.0f);
        }

        out.emplace_back(Scaled(reward.amount, 300));
        out.emplace_back(reward.claimed ? 1.0f : 0.0f);
        out.emplace_back(Scaled(
            static_cast<int>(std::max(reward.cards.size(),
                                      reward.relics.size())),
            static_cast<int>(OBSERVED_OPTIONS)));
    }

    // What the shelf is asking.
    const Shop& shop = m_run.GetShop();

    for (std::size_t i = 0; i < SHOP_CARD_SLOTS; ++i)
    {
        const bool there = i < shop.GetCards().size();

        out.emplace_back(there ? Scaled(shop.GetCards()[i].price, 200)
                               : 0.0f);
        out.emplace_back(there && !shop.GetCards()[i].sold ? 1.0f : 0.0f);
    }

    for (std::size_t i = 0; i < SHOP_RELIC_SLOTS; ++i)
    {
        const bool there = i < shop.GetRelics().size();

        out.emplace_back(there ? Scaled(shop.GetRelics()[i].price, 350)
                               : 0.0f);
        out.emplace_back(there && !shop.GetRelics()[i].sold ? 1.0f : 0.0f);
    }

    for (std::size_t i = 0; i < SHOP_POTION_SLOTS; ++i)
    {
        const bool there = i < shop.GetPotions().size();

        out.emplace_back(there ? Scaled(shop.GetPotions()[i].price, 120)
                               : 0.0f);
        out.emplace_back(there && !shop.GetPotions()[i].sold ? 1.0f : 0.0f);
    }

    out.emplace_back(Scaled(shop.GetRemovalPrice(), 200));
    out.emplace_back(shop.IsRemovalSpent() ? 0.0f : 1.0f);

    // What the room is offering.
    const Event& room = m_run.GetEvent();

    out.emplace_back(Scaled(room.GetStage(), 4));
    out.emplace_back(Scaled(room.GetTries(), 8));

    for (std::size_t i = 0; i < EVENT_OPTIONS; ++i)
    {
        const bool there = i < room.GetOptions().size();

        out.emplace_back(there && m_run.CanChooseEventOption(i) ? 1.0f
                                                                : 0.0f);
        out.emplace_back(there ? Scaled(room.GetOptions()[i].goldCost, 100)
                               : 0.0f);

        PushOption(out, there ? &room.GetOptions()[i] : nullptr);
    }

    // What is in the belt.
    for (std::size_t i = 0; i < POTION_SLOTS; ++i)
    {
        out.emplace_back(i < player.GetPotions().size() ? 1.0f : 0.0f);
    }

    // And what each monster means to do, when it can be seen at all.
    for (std::size_t i = 0; i < OBSERVED_MONSTERS; ++i)
    {
        if (i >= living.size() || !m_battle->AreIntentsVisible())
        {
            out.insert(out.end(), MOVE_SLOTS, 0.0f);
            continue;
        }

        const Monster& monster = m_battle->GetMonsters()[living[i]];

        PushMove(out, monster.GetCurrentMove(),
                 monster.GetPower(PowerType::STRENGTH));
    }

    // The map ahead. Without this a step on the map can only be chosen by
    // which column it is, which is no way to walk around an elite.
    const Map& map = m_run.GetMap();
    const int row = m_run.GetFloor();
    const std::vector<int> next = m_run.GetAvailableColumns();

    PushRow(out, map, row, next);

    // And where those places lead in turn.
    std::vector<int> after;

    if (row < map.GetRows())
    {
        for (const int column : next)
        {
            if (!Map::IsInside(row, column))
            {
                continue;
            }

            for (const int beyond : map.GetNode(row, column).nextColumns)
            {
                if (std::find(after.begin(), after.end(), beyond) ==
                    after.end())
                {
                    after.emplace_back(beyond);
                }
            }
        }
    }

    PushRow(out, map, row + 1, after);

    // What the rest of the act still holds, which is what tells a climb that
    // has spent its fires from one that has not.
    int left[MAP_KINDS] = { 0 };

    for (int ahead = row; ahead < map.GetRows(); ++ahead)
    {
        for (int column = 0; column < Map::COLUMNS; ++column)
        {
            const MapNode& node = map.GetNode(ahead, column);
            const auto kind = static_cast<std::size_t>(node.type);

            if (node.Exists() && kind >= 1u && kind <= MAP_KINDS)
            {
                ++left[kind - 1u];
            }
        }
    }

    for (std::size_t kind = 0; kind < MAP_KINDS; ++kind)
    {
        out.emplace_back(Scaled(left[kind], Map::ROWS));
    }

    out.emplace_back(Scaled(map.GetRows() - row, Map::ROWS));
    out.emplace_back(row >= map.GetRows() ? 1.0f : 0.0f);

    // And the deck, slot by slot, since that is how it is spoken about: the
    // move that sharpens a card names the slot it sits in.
    const std::vector<Card>& deck = m_run.GetDeck();

    // How many of each card the deck holds. The fifth Strike is worth less
    // than the first, and tearing one out is worth more: without this the
    // only place that is written down is a row of two hundred and eighty-six
    // numbers somewhere else in the state.
    std::vector<int> copies(CardSlots(), 0);

    for (const auto& card : deck)
    {
        ++copies[SlotOfCard(card.GetId())];
    }

    for (std::size_t slot = 0; slot < DECK_SLOTS; ++slot)
    {
        const bool there = slot < deck.size();

        if (!there)
        {
            out.insert(out.end(), DECK_CARD_SLOTS, 0.0f);
            continue;
        }

        const Card& card = deck[slot];
        const int sharpenings = card.GetUpgradeCount();

        // The same rule the moves on offer are drawn from.
        const bool again = m_run.IsUpgradeable(slot);
        const CardWorth& now = CardRegistry::Worth(card.GetId(), sharpenings);

        out.emplace_back(1.0f);

        // Attack, skill, power, status, curse - in the order of CardType,
        // whose first value is the invalid one.
        for (std::size_t kind = 0; kind < CARD_KINDS; ++kind)
        {
            out.emplace_back(
                static_cast<std::size_t>(card.GetCardType()) == kind + 1u
                    ? 1.0f
                    : 0.0f);
        }

        out.emplace_back(card.IsUpgraded() ? 1.0f : 0.0f);
        out.emplace_back(again ? 1.0f : 0.0f);
        out.emplace_back(Scaled(copies[SlotOfCard(card.GetId())], 5));
        out.emplace_back(Scaled(now.cost, 3));

        PushWorth(out, now);

        // What a whetstone would buy: the difference, which is the whole
        // question at a fire. Nothing to be had, nothing said.
        const CardWorth& next =
            CardRegistry::Worth(card.GetId(), sharpenings + 1);

        out.emplace_back(again ? Scaled(now.cost - next.cost, 3) : 0.0f);
        out.emplace_back(again ? Scaled(next.damage - now.damage, 10) : 0.0f);
        out.emplace_back(again ? Scaled(next.block - now.block, 10) : 0.0f);
        out.emplace_back(again ? Scaled(next.power - now.power, 5) : 0.0f);
    }

    return out;
}

std::size_t SpireEnv::ActionCount()
{
    return 1u                                          // end the turn
           + HAND_SLOTS * OBSERVED_MONSTERS            // play a card
           + POTION_SLOTS * OBSERVED_MONSTERS          // drink a potion
           + POTION_SLOTS                              // throw one away
           + MAP_COLUMNS                               // walk
           + REWARD_SLOTS * REWARD_OPTIONS             // take a reward
           + REWARD_SLOTS                              // turn one down
           + 1u                                        // walk off the pile
           + EVENT_OPTIONS * (DECK_SLOTS + 1u)         // answer a room
           + SHOP_CARD_SLOTS + SHOP_RELIC_SLOTS + SHOP_POTION_SLOTS
           + DECK_SLOTS                                // pay to lose a card
           + 1u                                        // leave the shop
           + 1u                                        // rest
           + DECK_SLOTS                                // smith
           + DECK_SLOTS                                // toke
           + 1u + 1u + 1u                              // dig, lift, leave
           + 1u                                        // the boss
           + 1u;                                       // the next act
}

Action SpireEnv::ActionFromIndex(std::size_t index)
{
    std::size_t at = 0;

    if (index == at)
    {
        return Action(ActionKind::END_TURN);
    }

    ++at;

    if (index < at + HAND_SLOTS * OBSERVED_MONSTERS)
    {
        const std::size_t which = index - at;

        return Action(ActionKind::PLAY_CARD,
                      static_cast<int>(which / OBSERVED_MONSTERS),
                      static_cast<int>(which % OBSERVED_MONSTERS));
    }

    at += HAND_SLOTS * OBSERVED_MONSTERS;

    if (index < at + POTION_SLOTS * OBSERVED_MONSTERS)
    {
        const std::size_t which = index - at;

        return Action(ActionKind::USE_POTION,
                      static_cast<int>(which / OBSERVED_MONSTERS),
                      static_cast<int>(which % OBSERVED_MONSTERS));
    }

    at += POTION_SLOTS * OBSERVED_MONSTERS;

    if (index < at + POTION_SLOTS)
    {
        return Action(ActionKind::DISCARD_POTION,
                      static_cast<int>(index - at));
    }

    at += POTION_SLOTS;

    if (index < at + MAP_COLUMNS)
    {
        return Action(ActionKind::TRAVEL, static_cast<int>(index - at));
    }

    at += MAP_COLUMNS;

    if (index < at + REWARD_SLOTS * REWARD_OPTIONS)
    {
        const std::size_t which = index - at;

        return Action(ActionKind::CLAIM_REWARD,
                      static_cast<int>(which / REWARD_OPTIONS),
                      static_cast<int>(which % REWARD_OPTIONS));
    }

    at += REWARD_SLOTS * REWARD_OPTIONS;

    if (index < at + REWARD_SLOTS)
    {
        return Action(ActionKind::SKIP_REWARD, static_cast<int>(index - at));
    }

    at += REWARD_SLOTS;

    if (index == at)
    {
        return Action(ActionKind::LEAVE_REWARDS);
    }

    ++at;

    if (index < at + EVENT_OPTIONS * (DECK_SLOTS + 1u))
    {
        const std::size_t which = index - at;
        const std::size_t option = which / (DECK_SLOTS + 1u);
        const std::size_t pick = which % (DECK_SLOTS + 1u);

        // The first slot of each option is the one that leaves the card to
        // chance.
        return Action(ActionKind::CHOOSE_OPTION, static_cast<int>(option),
                      pick == 0u ? -1 : static_cast<int>(pick - 1u));
    }

    at += EVENT_OPTIONS * (DECK_SLOTS + 1u);

    if (index < at + SHOP_CARD_SLOTS)
    {
        return Action(ActionKind::BUY_CARD, static_cast<int>(index - at));
    }

    at += SHOP_CARD_SLOTS;

    if (index < at + SHOP_RELIC_SLOTS)
    {
        return Action(ActionKind::BUY_RELIC, static_cast<int>(index - at));
    }

    at += SHOP_RELIC_SLOTS;

    if (index < at + SHOP_POTION_SLOTS)
    {
        return Action(ActionKind::BUY_POTION, static_cast<int>(index - at));
    }

    at += SHOP_POTION_SLOTS;

    if (index < at + DECK_SLOTS)
    {
        return Action(ActionKind::BUY_REMOVAL, static_cast<int>(index - at));
    }

    at += DECK_SLOTS;

    if (index == at)
    {
        return Action(ActionKind::LEAVE_SHOP);
    }

    ++at;

    if (index == at)
    {
        return Action(ActionKind::REST);
    }

    ++at;

    if (index < at + DECK_SLOTS)
    {
        return Action(ActionKind::SMITH, static_cast<int>(index - at));
    }

    at += DECK_SLOTS;

    if (index < at + DECK_SLOTS)
    {
        return Action(ActionKind::TOKE, static_cast<int>(index - at));
    }

    at += DECK_SLOTS;

    if (index == at)
    {
        return Action(ActionKind::DIG);
    }

    ++at;

    if (index == at)
    {
        return Action(ActionKind::LIFT);
    }

    ++at;

    if (index == at)
    {
        return Action(ActionKind::LEAVE_REST);
    }

    ++at;

    if (index == at)
    {
        return Action(ActionKind::FIGHT_BOSS);
    }

    ++at;

    if (index == at)
    {
        return Action(ActionKind::NEXT_ACT);
    }

    return Action();
}

std::size_t SpireEnv::IndexOfAction(const Action& action)
{
    for (std::size_t index = 0; index < ActionCount(); ++index)
    {
        const Action other = ActionFromIndex(index);

        if (other.kind == action.kind && other.a == action.a &&
            other.b == action.b)
        {
            return index;
        }
    }

    return ActionCount();
}

std::vector<unsigned char> SpireEnv::ActionMask() const
{
    std::vector<unsigned char> mask(ActionCount(), 0u);

    for (const Action& move : LegalActions())
    {
        const std::size_t index = IndexOfAction(move);

        if (index < mask.size())
        {
            mask[index] = 1u;
        }
    }

    return mask;
}

StepResult SpireEnv::StepIndex(std::size_t index)
{
    return Step(ActionFromIndex(index));
}

std::string SpireEnv::Save() const
{
    // A fight is not written out, so a save is taken between rooms.
    if (m_battle != nullptr)
    {
        return std::string();
    }

    std::ostringstream out;

    out << "env " << static_cast<int>(m_phase) << ' ' << m_totalFloors << ' '
        << (m_bossFight ? 1 : 0) << '\n'
        << m_run.Serialize();

    return out.str();
}

bool SpireEnv::Load(const std::string& text)
{
    std::istringstream in(text);
    std::string tag;
    int phase = 0;
    int boss = 0;

    in >> tag >> phase >> m_totalFloors >> boss;

    if (!in || tag != "env")
    {
        return false;
    }

    std::string rest;
    std::string line;

    std::getline(in, line);

    while (std::getline(in, line))
    {
        rest += line;
        rest += '\n';
    }

    if (!m_run.Load(rest))
    {
        return false;
    }

    m_battle.reset();
    m_phase = static_cast<EnvPhase>(phase);
    m_bossFight = boss != 0;
    m_healthBefore = m_run.GetPlayer().GetHealth();

    return true;
}
}  // namespace ConquerTheSpire
