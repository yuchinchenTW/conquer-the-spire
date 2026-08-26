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

MonsterEffect MonsterEffect::ShakeOff()
{
    MonsterEffect effect;
    effect.type = MonsterEffectType::SHAKE_OFF;

    return effect;
}

MonsterEffect MonsterEffect::Drain(int amount)
{
    MonsterEffect effect;
    effect.type = MonsterEffectType::DRAIN;
    effect.amount = amount;

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

MonsterMove& MonsterMove::SincePhase()
{
    sincePhase = true;

    return *this;
}

MonsterMove& MonsterMove::GrowsWithUse()
{
    growsWithUse = true;

    // The hits live on the blow rather than on the move, so the number it
    // starts with is read off the first blow that has one.
    for (const MonsterEffect& effect : effects)
    {
        if (effect.type == MonsterEffectType::DAMAGE)
        {
            baseTimes = effect.times;
            break;
        }
    }

    return *this;
}

MonsterMove& MonsterMove::OnTurnsLike(int every, int remainder)
{
    turnEvery = every;
    turnLike = remainder;

    return *this;
}

MonsterMove& MonsterMove::BeforeMove(const std::string& other)
{
    beforeMove = other;

    return *this;
}

MonsterMove& MonsterMove::AfterMove(const std::string& other)
{
    afterMove = other;

    return *this;
}

MonsterMove& MonsterMove::SpillsTo(const std::string& other)
{
    spillsTo = other;

    return *this;
}

MonsterMove& MonsterMove::AtMost(int many, const std::string& other)
{
    atMost = many;
    insteadAfter = other;

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
            RefreshGrowingMove();
            return;
        }
    }

    m_moveIndex = PickWeightedMove(rng, context);
    m_sameMoveRun = 1;
    RefreshGrowingMove();
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

    // What it has been told to do comes before what it would choose.
    if (!m_queued.empty())
    {
        const std::string next = m_queued.front();

        m_queued.erase(m_queued.begin());

        if (ForceMove(next))
        {
            return;
        }
    }

    m_moveIndex = PickWeightedMove(rng, context);
    m_sameMoveRun = SameMoveAs(previous) ? m_sameMoveRun + 1 : 1;
    RefreshGrowingMove();
}

int Monster::GetPhase() const
{
    return m_phase;
}

void Monster::SetPhase(int phase)
{
    m_phase = phase;
    m_phaseTurn = m_movesMade;
}

int Monster::GetPhaseTurn() const
{
    return m_phaseTurn;
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

void Monster::QueueMoves(std::vector<std::string> names)
{
    m_queued = std::move(names);
}

void Monster::RefreshGrowingMove()
{
    if (m_moveIndex >= m_moves.size())
    {
        return;
    }

    MonsterMove& move = m_moves[m_moveIndex];

    if (!move.growsWithUse)
    {
        return;
    }

    // Two to start with, and one more for every one already made. The count
    // is put up as the move begins to resolve, so what it holds here is how
    // many came before this one.
    for (MonsterEffect& effect : move.effects)
    {
        if (effect.type == MonsterEffectType::DAMAGE)
        {
            effect.times = move.baseTimes + move.used;
            break;
        }
    }
}

bool Monster::SameMoveAs(std::size_t other) const
{
    // By name, because a move can be written twice - once for the share it
    // has in one company and once for the share it has in another. A
    // Collector's fireball is two lines, and when a torch head falls between
    // one turn and the next the share moves from one line to the other. Asking
    // which line it came from let her throw three fireballs running, which she
    // may not do; asking what the move is called does not.
    return other < m_moves.size() && m_moveIndex < m_moves.size() &&
           m_moves[other].name == m_moves[m_moveIndex].name;
}

bool Monster::MoveDrawable(std::size_t at, const MoveContext& context) const
{
    if (at >= m_moves.size())
    {
        return false;
    }

    const MonsterMove& move = m_moves[at];

    if (move.weight <= 0 || !MoveAllowed(move, context))
    {
        return false;
    }

    if (move.maxInARow > 0 && SameMoveAs(at) &&
        m_sameMoveRun >= move.maxInARow)
    {
        return false;
    }

    return move.atMost <= 0 || move.used < move.atMost;
}

std::size_t Monster::HeirOfMove(std::size_t at, const MoveContext& context)
    const
{
    // A share handed on can land somewhere that cannot take it either - a
    // Champ who has taken his stance twice gloats instead, and if he gloated
    // last turn he cannot gloat now, so the share goes on again to the slap.
    // Following it one step and stopping was letting a gloat come twice
    // running, which is the one thing the whole rule is about.
    for (std::size_t hops = 0; hops <= m_moves.size(); ++hops)
    {
        if (at >= m_moves.size())
        {
            return m_moves.size();
        }

        if (MoveDrawable(at, context))
        {
            return at;
        }

        const MonsterMove& move = m_moves[at];
        const bool finished = move.atMost > 0 && move.used >= move.atMost;
        const std::string& next =
            finished ? move.insteadAfter : move.spillsTo;

        if (next.empty())
        {
            return m_moves.size();
        }

        at = IndexOfMove(next);
    }

    // Round in a circle: nobody gets it rather than somebody getting it
    // twice.
    return m_moves.size();
}

int Monster::UsesOfMove(const std::string& name) const
{
    int made = 0;

    for (const MonsterMove& move : m_moves)
    {
        made += move.name == name ? move.used : 0;
    }

    return made;
}

std::size_t Monster::IndexOfMove(const std::string& name) const
{
    for (std::size_t i = 0; i < m_moves.size(); ++i)
    {
        if (m_moves[i].name == name)
        {
            return i;
        }
    }

    return m_moves.size();
}

void Monster::CountMoveUsed()
{
    if (m_moveIndex < m_moves.size())
    {
        ++m_moves[m_moveIndex].used;
    }
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

    if (move.turnEvery > 0 &&
        (context.turn + 1) % move.turnEvery != move.turnLike)
    {
        return false;
    }

    if (!move.beforeMove.empty() && UsesOfMove(move.beforeMove) > 0)
    {
        return false;
    }

    if (!move.afterMove.empty() && UsesOfMove(move.afterMove) == 0)
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
            m_sameMoveRun = SameMoveAs(i) ? m_sameMoveRun + 1 : 1;
            m_moveIndex = i;

            RefreshGrowingMove();

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

        const bool spent = move.maxInARow > 0 && SameMoveAs(i) &&
                           m_sameMoveRun >= move.maxInARow;

        if (spent || (move.atMost > 0 && move.used >= move.atMost))
        {
            continue;
        }

        const int next = context.turn + 1;

        if (move.onTurn > 0 && move.onTurn == next)
        {
            return i;
        }

        if (move.everyTurns > 0)
        {
            // Counted from the turn the monster turned, when it says so: a
            // Champ executes the turn after he stops fighting fair and every
            // third turn from there. Counted against the turn the fight
            // started otherwise, which is what a Taunt every fourth turn
            // means.
            if (move.sincePhase)
            {
                // The first one lands on the first turn after the phase
                // changed, and every so many turns from there. A Champ
                // executes the turn straight after he stops fighting fair,
                // then twice at random, then executes again.
                const int since = m_movesMade - m_phaseTurn;

                if (since >= 1 && (since - 1) % move.everyTurns == 0)
                {
                    return i;
                }
            }
            else if (next % move.everyTurns == 0)
            {
                return i;
            }
        }

        if (move.allyMissing > 0 && context.allyMissing >= move.allyMissing)
        {
            return i;
        }
    }

    std::vector<std::size_t> allowed;
    std::vector<int> shares(m_moves.size(), 0);
    int total = 0;

    for (std::size_t i = 0; i < m_moves.size(); ++i)
    {
        const MonsterMove& move = m_moves[i];

        if (move.weight <= 0 || !MoveAllowed(move, context))
        {
            continue;
        }

        // A move steps aside for having come up too often in a row, or for
        // having had all the turns it gets in one fight, and where it says so
        // its share goes to one named other rather than being shared out
        // among all of them. The two are not the same monster: a Champ who
        // has just gloated faces a face slap at forty and everything else
        // exactly where it was, and one who has taken his stance twice gloats
        // in its place at thirty.
        if (!MoveDrawable(i, context))
        {
            const std::size_t heir = HeirOfMove(i, context);

            if (heir < m_moves.size())
            {
                shares[heir] += move.weight;
            }

            continue;
        }

        shares[i] += move.weight;
    }

    for (std::size_t i = 0; i < m_moves.size(); ++i)
    {
        if (shares[i] > 0 && MoveDrawable(i, context))
        {
            allowed.emplace_back(i);
            total += shares[i];
        }
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
        // The share, not the weight written on the card: a move that took in
        // a blocked move's share is drawn for the two of them together.
        score -= shares[index];

        if (score <= 0)
        {
            return index;
        }
    }

    return allowed.back();
}
}  // namespace ConquerTheSpire
