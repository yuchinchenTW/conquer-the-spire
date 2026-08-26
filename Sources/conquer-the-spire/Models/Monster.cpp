// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Models/Monster.hpp>

#include <utility>

namespace ConquerTheSpire
{
MonsterEffect MonsterEffect::Damage(int amount, int times)
{
    MonsterEffect effect;
    effect.type = MonsterEffectType::DAMAGE;
    effect.amount = amount;
    effect.times = times;

    return effect;
}

MonsterEffect MonsterEffect::DamageByPlayerHealth(int divisor, int times)
{
    MonsterEffect effect;
    effect.type = MonsterEffectType::DAMAGE_SCALED;
    effect.amount = divisor;
    effect.times = times;

    return effect;
}

MonsterEffect MonsterEffect::Block(int amount)
{
    MonsterEffect effect;
    effect.type = MonsterEffectType::BLOCK;
    effect.amount = amount;

    return effect;
}

MonsterEffect MonsterEffect::BlockAlly(int amount)
{
    MonsterEffect effect;
    effect.type = MonsterEffectType::BLOCK_ALLY;
    effect.amount = amount;

    return effect;
}

MonsterEffect MonsterEffect::Buff(PowerType power, int amount)
{
    MonsterEffect effect;
    effect.type = MonsterEffectType::APPLY_POWER;
    effect.power = power;
    effect.amount = amount;

    return effect;
}

MonsterEffect MonsterEffect::Debuff(PowerType power, int amount)
{
    MonsterEffect effect = Buff(power, amount);
    effect.toPlayer = true;

    return effect;
}

MonsterEffect MonsterEffect::AddCard(CardId id, int count, bool upgraded)
{
    MonsterEffect effect;
    effect.type = MonsterEffectType::ADD_CARD;
    effect.cardId = id;
    effect.amount = count;
    effect.upgradedCard = upgraded;

    return effect;
}

MonsterEffect MonsterEffect::BuffAll(PowerType power, int amount)
{
    MonsterEffect effect;
    effect.type = MonsterEffectType::BUFF_ALL;
    effect.power = power;
    effect.amount = amount;

    return effect;
}

MonsterEffect MonsterEffect::BlockAllies(int amount)
{
    MonsterEffect effect;
    effect.type = MonsterEffectType::BLOCK_ALLIES;
    effect.amount = amount;

    return effect;
}

MonsterEffect MonsterEffect::HealAll(int amount)
{
    MonsterEffect effect;
    effect.type = MonsterEffectType::HEAL_ALL;
    effect.amount = amount;

    return effect;
}

MonsterEffect MonsterEffect::Summon(MonsterId id, int count, int cap)
{
    MonsterEffect effect;
    effect.type = MonsterEffectType::SUMMON;
    effect.summon = id;
    effect.times = count;
    effect.cap = cap;

    return effect;
}

MonsterEffect MonsterEffect::Stasis()
{
    MonsterEffect effect;
    effect.type = MonsterEffectType::STASIS;

    return effect;
}

MonsterEffect MonsterEffect::Split(MonsterId first, MonsterId second)
{
    MonsterEffect effect;
    effect.type = MonsterEffectType::SPLIT;
    effect.splitFirst = first;
    effect.splitSecond = second;

    return effect;
}

MonsterEffect MonsterEffect::Revive(int percent)
{
    MonsterEffect effect;
    effect.type = MonsterEffectType::REVIVE;
    effect.amount = percent;

    return effect;
}

MonsterEffect MonsterEffect::SelfDestruct(int amount)
{
    MonsterEffect effect;
    effect.type = MonsterEffectType::SELF_DESTRUCT;
    effect.amount = amount;

    return effect;
}

MonsterEffect MonsterEffect::Escape()
{
    MonsterEffect effect;
    effect.type = MonsterEffectType::ESCAPE;

    return effect;
}

MonsterMove MonsterMove::Attack(std::string name, int damage, int times)
{
    MonsterMove move;
    move.name = std::move(name);
    move.intent = Intent::ATTACK;
    move.effects.emplace_back(MonsterEffect::Damage(damage, times));

    return move;
}

MonsterMove MonsterMove::Defend(std::string name, int block)
{
    MonsterMove move;
    move.name = std::move(name);
    move.intent = Intent::DEFEND;
    move.effects.emplace_back(MonsterEffect::Block(block));

    return move;
}

MonsterMove MonsterMove::AttackAndDefend(std::string name, int damage,
                                         int block)
{
    MonsterMove move;
    move.name = std::move(name);
    move.intent = Intent::ATTACK;
    move.effects.emplace_back(MonsterEffect::Damage(damage));
    move.effects.emplace_back(MonsterEffect::Block(block));

    return move;
}

MonsterMove MonsterMove::Buff(std::string name, PowerType power, int amount,
                              int block)
{
    MonsterMove move;
    move.name = std::move(name);
    move.intent = Intent::BUFF;
    move.effects.emplace_back(MonsterEffect::Buff(power, amount));

    if (block > 0)
    {
        move.effects.emplace_back(MonsterEffect::Block(block));
    }

    return move;
}

MonsterMove MonsterMove::Debuff(std::string name, PowerType power, int amount)
{
    MonsterMove move;
    move.name = std::move(name);
    move.intent = Intent::DEBUFF;
    move.effects.emplace_back(MonsterEffect::Debuff(power, amount));

    return move;
}

MonsterMove MonsterMove::Of(std::string name, Intent intent,
                            std::vector<MonsterEffect> effects)
{
    MonsterMove move;
    move.name = std::move(name);
    move.intent = intent;
    move.effects = std::move(effects);

    return move;
}

MonsterMove MonsterMove::Nothing(std::string name, Intent intent)
{
    MonsterMove move;
    move.name = std::move(name);
    move.intent = intent;

    return move;
}

MonsterMove& MonsterMove::Chance(int wanted, int limit)
{
    weight = wanted;
    maxInARow = limit;

    return *this;
}

MonsterMove& MonsterMove::Opener()
{
    opener = true;

    return *this;
}

MonsterMove& MonsterMove::NotFirst()
{
    notFirst = true;

    return *this;
}

MonsterMove& MonsterMove::Every(int turns)
{
    everyTurns = turns;

    return *this;
}

MonsterMove& MonsterMove::OnTurn(int turn)
{
    onTurn = turn;

    return *this;
}

MonsterMove& MonsterMove::InPhase(int wanted)
{
    phase = wanted;

    return *this;
}

MonsterMove& MonsterMove::Alone()
{
    alone = true;

    return *this;
}

MonsterMove& MonsterMove::WhenAlliesUnder(int many)
{
    alliesUnder = many;

    return *this;
}

MonsterMove& MonsterMove::WhenAlliesAtLeast(int many)
{
    alliesAtLeast = many;

    return *this;
}

MonsterMove& MonsterMove::WithAlly()
{
    withAlly = true;

    return *this;
}

MonsterMove& MonsterMove::WhenAllyMissing(int amount)
{
    allyMissing = amount;

    return *this;
}

Monster::Monster(std::string name, int maxHealth,
                 std::vector<MonsterMove> moveScript, bool loopMoves,
                 std::size_t loopFrom)
    : Creature(std::move(name), maxHealth),
      m_moves(std::move(moveScript)),
      m_scripted(true),
      m_loopMoves(loopMoves),
      m_loopFrom(loopFrom)
{
    // Do nothing
}

Monster::Monster(MonsterId id, std::string name, MonsterType type,
                 int maxHealth, std::vector<MonsterMove> moves)
    : Creature(std::move(name), maxHealth),
      m_id(id),
      m_type(type),
      m_moves(std::move(moves)),
      m_scripted(false)
{
    // Do nothing
}

void Monster::SetIdentity(MonsterId id, MonsterType type)
{
    m_id = id;
    m_type = type;
}

MonsterId Monster::GetMonsterId() const
{
    return m_id;
}

MonsterType Monster::GetMonsterType() const
{
    return m_type;
}

const MonsterMove& Monster::GetCurrentMove() const
{
    // A monster without a move does nothing on its turn.
    static const MonsterMove idle;

    if (m_moves.empty() || m_moveIndex >= m_moves.size())
    {
        return idle;
    }

    return m_moves[m_moveIndex];
}

Intent Monster::GetIntent() const
{
    return GetCurrentMove().intent;
}

const std::vector<MonsterMove>& Monster::GetMoves() const
{
    return m_moves;
}

void Monster::ChooseOpeningMove(std::mt19937& rng)
{
    ChooseOpeningMove(rng, MoveContext());
}

void Monster::ChooseOpeningMove(std::mt19937& rng, const MoveContext& context)
{
    if (m_scripted || m_moves.empty())
    {
        return;
    }

    // An opener beats the weights.
    for (std::size_t i = 0; i < m_moves.size(); ++i)
    {
        if (m_moves[i].opener)
        {
            m_moveIndex = i;
            m_sameMoveRun = 1;
            return;
        }
    }

    m_moveIndex = PickWeightedMove(rng, context);
    m_sameMoveRun = 1;
}

void Monster::AdvanceMove(std::mt19937& rng)
{
    AdvanceMove(rng, MoveContext());
}

void Monster::AdvanceMove(std::mt19937& rng, const MoveContext& context)
{
    ++m_movesMade;

    if (m_moves.empty())
    {
        return;
    }

    if (m_scripted)
    {
        if (m_loopMoves)
        {
            m_moveIndex = m_moveIndex + 1 < m_moves.size() ? m_moveIndex + 1
                                                           : m_loopFrom;
        }
        else if (m_moveIndex + 1 < m_moves.size())
        {
            ++m_moveIndex;
        }

        return;
    }

    const std::size_t previous = m_moveIndex;

    m_moveIndex = PickWeightedMove(rng, context);
    m_sameMoveRun = m_moveIndex == previous ? m_sameMoveRun + 1 : 1;
}

int Monster::GetPhase() const
{
    return m_phase;
}

void Monster::SetPhase(int phase)
{
    m_phase = phase;
}

int Monster::GetFlightBase() const
{
    return m_flightBase;
}

void Monster::SetFlightBase(int amount)
{
    m_flightBase = amount;
}

int Monster::GetMalleableBase() const
{
    return m_malleableBase;
}

void Monster::SetMalleableBase(int amount)
{
    m_malleableBase = amount;
}

bool Monster::IsRegrowing() const
{
    return m_regrowing;
}

void Monster::SetRegrowing(bool regrowing)
{
    m_regrowing = regrowing;
}

int Monster::GetDamageCapLeft() const
{
    return m_damageCapLeft;
}

void Monster::SetDamageCapLeft(int amount)
{
    m_damageCapLeft = amount;
}

int Monster::GetStolenGold() const
{
    return m_stolenGold;
}

void Monster::StealGold(int amount)
{
    if (amount > 0)
    {
        m_stolenGold += amount;
    }
}

bool Monster::HasStasisCard() const
{
    return m_stasisCard.GetId() != CardId::INVALID;
}

const Card& Monster::GetStasisCard() const
{
    return m_stasisCard;
}

void Monster::HoldStasisCard(Card card)
{
    m_stasisCard = std::move(card);
}

Card Monster::ReleaseStasisCard()
{
    Card card = std::move(m_stasisCard);
    m_stasisCard = Card();

    return card;
}

bool Monster::MoveAllowed(const MonsterMove& move,
                          const MoveContext& context) const
{
    if (move.phase > 0 && move.phase != context.phase)
    {
        return false;
    }

    if (move.alone && context.allies > 0)
    {
        return false;
    }

    if (move.withAlly && context.allies == 0)
    {
        return false;
    }

    if (move.alliesUnder > 0 && context.allies >= move.alliesUnder)
    {
        return false;
    }

    if (move.alliesAtLeast > 0 && context.allies < move.alliesAtLeast)
    {
        return false;
    }

    if (move.notFirst && m_movesMade == 0)
    {
        return false;
    }

    return true;
}

bool Monster::ForceMove(const std::string& name)
{
    for (std::size_t i = 0; i < m_moves.size(); ++i)
    {
        if (m_moves[i].name == name)
        {
            m_sameMoveRun = m_moveIndex == i ? m_sameMoveRun + 1 : 1;
            m_moveIndex = i;

            return true;
        }
    }

    return false;
}

bool Monster::HasEscaped() const
{
    return m_escaped;
}

void Monster::MarkEscaped()
{
    m_escaped = true;
}

bool Monster::IsGone() const
{
    return (IsDead() && !m_regrowing) || m_escaped;
}

std::size_t Monster::PickWeightedMove(std::mt19937& rng,
                                      const MoveContext& context) const
{
    // A move that belongs to a certain turn, or that comes round every so
    // many turns, is made whatever the weights say. So is the one a healer
    // waits for.
    for (std::size_t i = 0; i < m_moves.size(); ++i)
    {
        const MonsterMove& move = m_moves[i];

        if (!MoveAllowed(move, context))
        {
            continue;
        }

        const bool spent = move.maxInARow > 0 && i == m_moveIndex &&
                           m_sameMoveRun >= move.maxInARow;

        if (spent)
        {
            continue;
        }

        const int next = context.turn + 1;

        if (move.onTurn > 0 && move.onTurn == next)
        {
            return i;
        }

        if (move.everyTurns > 0 && next % move.everyTurns == 0)
        {
            return i;
        }

        if (move.allyMissing > 0 && context.allyMissing >= move.allyMissing)
        {
            return i;
        }
    }

    std::vector<std::size_t> allowed;
    int total = 0;

    for (std::size_t i = 0; i < m_moves.size(); ++i)
    {
        const MonsterMove& move = m_moves[i];

        if (move.weight <= 0 || !MoveAllowed(move, context))
        {
            continue;
        }

        // A move that has come up too often in a row steps aside.
        if (move.maxInARow > 0 && i == m_moveIndex &&
            m_sameMoveRun >= move.maxInARow)
        {
            continue;
        }

        allowed.emplace_back(i);
        total += move.weight;
    }

    if (allowed.empty() || total <= 0)
    {
        // Nothing is allowed, so the limits are ignored rather than the
        // monster standing there.
        return m_moveIndex;
    }

    std::uniform_int_distribution<int> roll(1, total);
    int score = roll(rng);

    for (const std::size_t index : allowed)
    {
        score -= m_moves[index].weight;

        if (score <= 0)
        {
            return index;
        }
    }

    return allowed.back();
}
}  // namespace ConquerTheSpire
