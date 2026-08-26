// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Battle/Battle.hpp>
#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Monsters/MonsterRoster.hpp>
#include <conquer-the-spire/Potions/PotionRegistry.hpp>

#include <algorithm>
#include <utility>

namespace ConquerTheSpire
{
namespace
{
//! Returns true when applying \p amount of \p power counts as a debuff, which
//! is what Artifact eats.
bool IsDebuff(PowerType power, int amount)
{
    switch (power)
    {
        case PowerType::VULNERABLE:
        case PowerType::WEAK:
        case PowerType::FRAIL:
        case PowerType::POISON:
        case PowerType::NO_DRAW:
        case PowerType::STRENGTH_DOWN:
        case PowerType::DEXTERITY_DOWN:
            return amount > 0;

        case PowerType::STRENGTH:
        case PowerType::DEXTERITY:
            return amount < 0;

        default:
            return false;
    }
}

//! Returns true when \p card is one of the cards \p filter allows.
bool PassesFilter(const Card& card, CardFilter filter)
{
    switch (filter)
    {
        case CardFilter::NON_ATTACK:
            return card.GetCardType() != CardType::ATTACK;

        case CardFilter::ATTACK_OR_POWER:
            return card.GetCardType() == CardType::ATTACK ||
                   card.GetCardType() == CardType::POWER;

        case CardFilter::ATTACK_ONLY:
            return card.GetCardType() == CardType::ATTACK;

        case CardFilter::SKILL_ONLY:
            return card.GetCardType() == CardType::SKILL;

        case CardFilter::POWER_ONLY:
            return card.GetCardType() == CardType::POWER;

        case CardFilter::ANY:
            break;
    }

    return true;
}

//! Returns true when \p name contains "Strike", which is how Perfected Strike
//! decides what to count.
bool IsStrikeCard(const Card& card)
{
    return card.GetName().find("Strike") != std::string::npos;
}

int RarityRank(CardRarity rarity)
{
    switch (rarity)
    {
        case CardRarity::RARE:
            return 4;

        case CardRarity::UNCOMMON:
            return 3;

        case CardRarity::COMMON:
            return 2;

        case CardRarity::BASIC:
            return 1;

        case CardRarity::SPECIAL:
        case CardRarity::INVALID:
            return 0;
    }

    return 0;
}
}  // namespace

Battle::Battle(Player player, std::vector<Monster> monsters, unsigned int seed)
    : m_player(std::move(player)),
      m_monsters(std::move(monsters)),
      m_rng(seed)
{
    // Do nothing
}

void Battle::Start()
{
    if (m_phase != BattlePhase::NOT_STARTED)
    {
        return;
    }

    // Room for the monsters a split may add later, so that nothing holds a
    // pointer into a vector that has moved.
    m_monsters.reserve(m_monsters.size() + 8);

    for (auto& monster : m_monsters)
    {
        monster.ChooseOpeningMove(m_rng, ReadMoveContext(monster));
    }

    m_player.InitializePiles(m_rng);
    m_player.DrawInnateCards();

    // Whatever is in a bottle is in hand from the first turn.
    for (const CardId id : m_player.GetBottledCards())
    {
        std::vector<Card>& pile = m_player.GetDrawPile();

        for (std::size_t i = 0; i < pile.size(); ++i)
        {
            if (pile[i].GetId() == id)
            {
                Card bottled = pile[i];

                pile.erase(pile.begin() + static_cast<std::ptrdiff_t>(i));
                m_player.AddCardToPile(std::move(bottled), CardPile::HAND,
                                       m_rng);
                break;
            }
        }
    }

    m_turn = 1;
    m_lastShuffleCount = m_player.GetShuffleCount();

    for (auto& relic : m_player.GetRelics())
    {
        relic.ResetCounter();
    }

    // A snecko eye draws two more every turn and confuses every card drawn
    // for them - the price of the two, and the whole of what makes it a boss
    // relic rather than a gift. The drawing was here already; the confusion
    // was not, so the relic was worth taking every time.
    if (m_player.HasRelic(RelicId::SNECKO_EYE))
    {
        m_player.AddPower(PowerType::CONFUSED, 1);
    }

    // What lifting at rest sites has put on is there from the start.
    if (const int lifted = m_player.GetLiftedStrength(); lifted > 0)
    {
        m_player.AddPower(PowerType::STRENGTH, lifted);
    }

    // A doll answers for every curse the deck carries.
    if (m_player.HasRelic(RelicId::DU_VU_DOLL))
    {
        int curses = 0;

        for (const auto& card : m_player.GetDeck())
        {
            if (card.GetCardType() == CardType::CURSE)
            {
                ++curses;
            }
        }

        if (curses > 0)
        {
            m_player.AddPower(PowerType::STRENGTH, curses);
        }
    }

    // A mark of pain buys its energy with wounds.
    if (m_player.HasRelic(RelicId::MARK_OF_PAIN))
    {
        for (int i = 0; i < 2; ++i)
        {
            m_player.AddCardToPile(CardRegistry::Get(CardId::WOUND),
                                   CardPile::DRAW_SHUFFLED, m_rng);
        }
    }

    // A pantograph is worth something only where it is needed.
    if (m_player.HasRelic(RelicId::PANTOGRAPH) && IsBossFight())
    {
        HealPlayer(25);
    }

    BeginPlayerTurn();
}

bool Battle::ChoiceTakesMany(const std::vector<CardEffect>& effects)
{
    for (const auto& effect : effects)
    {
        if (effect.manyCards)
        {
            return true;
        }
    }

    return false;
}

bool Battle::ChoiceTakesMany(const Card& card)
{
    return ChoiceTakesMany(card.GetEffects());
}

bool Battle::ChoiceTakesMany(const Potion& potion)
{
    return ChoiceTakesMany(potion.GetEffects());
}

bool Battle::PlayCard(std::size_t handIndex, std::size_t monsterIndex,
                      const std::vector<std::size_t>& choices)
{
    // The list is kept here rather than threaded through every step of the
    // resolving, because one step in all of it wants more than the first of
    // them.
    m_choices = choices;

    const bool played = PlayCard(handIndex, monsterIndex,
                                choices.empty() ? 0u : choices.front());

    m_choices.clear();

    return played;
}

bool Battle::PlayCard(std::size_t handIndex, std::size_t monsterIndex,
                      std::size_t choiceIndex)
{
    if (!CanPlay(handIndex, monsterIndex))
    {
        return false;
    }

    std::vector<Card>& hand = m_player.GetHand();

    // Copy the card out before it leaves the hand: its effects are read while
    // the piles are being shuffled around.
    Card card = hand[handIndex];

    // Counted here, where a card is actually played, rather than worked out
    // from what a card says it does. A Clash is nought energy for fourteen
    // damage on paper and unplayable two turns in three in fact, and nothing
    // about the first number says so.
    ++m_playedCounts[card.GetId()];

    // An X cost spends everything that is left.
    int energySpent = card.GetCost() == Card::COST_X
                          ? m_player.GetEnergy()
                          : GetEffectiveCost(card);

    if (card.GetCost() == Card::COST_X &&
        m_player.HasRelic(RelicId::CHEMICAL_X))
    {
        // Chemical X counts two more without asking for them.
        m_player.UseEnergy(energySpent);
        energySpent += 2;

        hand.erase(hand.begin() + static_cast<std::ptrdiff_t>(handIndex));
        ResolvePlayedCard(std::move(card), monsterIndex, choiceIndex,
                          energySpent);
        UpdatePhase();

        return true;
    }

    m_player.UseEnergy(energySpent);
    hand.erase(hand.begin() + static_cast<std::ptrdiff_t>(handIndex));

    ResolvePlayedCard(std::move(card), monsterIndex, choiceIndex, energySpent);
    ApplyPendingSpawns();
    UpdatePhase();

    // Something that eats time takes the rest of the turn with it.
    if (m_turnCutShort)
    {
        m_turnCutShort = false;

        if (m_phase == BattlePhase::PLAYER_TURN)
        {
            EndTurn();
        }
    }

    return true;
}

bool Battle::EndTurn()
{
    if (m_phase != BattlePhase::PLAYER_TURN)
    {
        return false;
    }

    // A card still in hand when the turn is handed over is a card that could
    // not be used this turn - which is the only honest denominator for how
    // much use a card is. Counted per turn rather than per fight: a Clash
    // played once in a fight it sat out seven turns of is not a card that
    // works, and counting the fight would have said it was.
    for (const auto& held : m_player.GetHand())
    {
        ++m_strandedCounts[held.GetId()];
    }

    EndPlayerTurn();

    if (IsDone())
    {
        return true;
    }

    RunMonsterTurn();

    if (IsDone())
    {
        return true;
    }

    ++m_turn;

    BeginPlayerTurn();

    return true;
}

BattlePhase Battle::GetPhase() const
{
    return m_phase;
}

bool Battle::IsDone() const
{
    return m_phase == BattlePhase::WON || m_phase == BattlePhase::LOST;
}

int Battle::GetTurn() const
{
    return m_turn;
}

Player& Battle::GetPlayer()
{
    return m_player;
}

const Player& Battle::GetPlayer() const
{
    return m_player;
}

bool Battle::AreIntentsVisible() const
{
    return !m_player.HasRelic(RelicId::RUNIC_DOME);
}

bool Battle::IsDrawPileOrdered() const
{
    return m_player.HasRelic(RelicId::FROZEN_EYE);
}

bool Battle::Gamble(int count)
{
    if (m_gambleSpent || m_turn != 1 || count <= 0 ||
        !m_player.HasRelic(RelicId::GAMBLING_CHIP) ||
        m_phase != BattlePhase::PLAYER_TURN)
    {
        return false;
    }

    m_gambleSpent = true;

    const int thrown =
        std::min(count, static_cast<int>(m_player.GetHand().size()));

    for (int i = 0; i < thrown; ++i)
    {
        DiscardCards(1, false, 0);
    }

    DrawCards(thrown);

    return true;
}

bool Battle::IsBossFight() const
{
    for (const auto& monster : m_monsters)
    {
        if (monster.GetMonsterType() == MonsterType::BOSS)
        {
            return true;
        }
    }

    return false;
}

bool Battle::IsEliteFight() const
{
    for (const auto& monster : m_monsters)
    {
        if (monster.GetMonsterType() == MonsterType::ELITE)
        {
            return true;
        }
    }

    return false;
}

void Battle::HealPlayer(int amount)
{
    // Whoever carries the bloom cannot be healed at all.
    if (amount > 0 && !m_player.HasRelic(RelicId::MARK_OF_THE_BLOOM))
    {
        m_player.Heal(amount);
    }
}

void Battle::FillPotionBelt(bool includeJuice)
{
    std::vector<Potion>& held = m_player.GetPotions();
    const std::size_t slots =
        static_cast<std::size_t>(m_player.GetPotionSlots());

    for (int guard = 0; held.size() < slots && guard < 40; ++guard)
    {
        const std::vector<PotionId> pool =
            PotionRegistry::GetAll(m_player.GetColor());

        if (pool.empty())
        {
            return;
        }

        std::uniform_int_distribution<std::size_t> pick(0, pool.size() - 1);
        const PotionId poured = pool[pick(m_rng)];

        // A fight is no place to squeeze fruit.
        if (!includeJuice && poured == PotionId::FRUIT_JUICE)
        {
            continue;
        }

        m_player.AddPotion(PotionRegistry::Get(poured));
    }
}

bool Battle::WasEscaped() const
{
    return m_escaped;
}

namespace
{
//! Which pile an effect picks a card out of, and nothing for the effects that
//! pick nothing. An effect that takes one at random, or takes the lot, is
//! choosing nothing: there is no question to put to anybody.
ChoiceSource SourceOfChoice(const CardEffect& effect)
{
    switch (effect.type)
    {
        case EffectType::COPY_HAND_CARD:
        case EffectType::HAND_TO_DRAW_TOP:
        case EffectType::SETUP_CARD:
        case EffectType::REMEMBER_CARD:
        case EffectType::EXHAUST_FOR_ENERGY:
            return ChoiceSource::HAND;

        case EffectType::DISCARD_CARDS:
        case EffectType::EXHAUST_HAND_CARD:
            return effect.randomPick ? ChoiceSource::NONE
                                     : ChoiceSource::HAND;

        // A nought here means every card in every pile, which is an
        // Apotheosis rather than a question.
        case EffectType::UPGRADE_HAND_CARD:
            return effect.value == 0 || effect.randomPick
                       ? ChoiceSource::NONE
                       : ChoiceSource::HAND;

        case EffectType::DISCARD_TO_DRAW_TOP:
            return ChoiceSource::DISCARD;

        // And an extra of one here means every card of the pile that costs
        // nothing, which is not a question either.
        case EffectType::RETURN_FROM_DISCARD:
            return effect.extra == 1 ? ChoiceSource::NONE
                                     : ChoiceSource::DISCARD;

        case EffectType::RETURN_FROM_EXHAUST:
            return ChoiceSource::EXHAUST;

        // Nothing the climber already has: a handful rolled up to be picked
        // from, which is what a Discovery does.
        case EffectType::OFFER_CARDS:
            return ChoiceSource::OFFERED;

        default:
            return ChoiceSource::NONE;
    }
}
}  // namespace

bool Battle::NeedsCardChoice(const Card& card)
{
    return ChoiceSourceOf(card) != ChoiceSource::NONE;
}

bool Battle::NeedsCardChoice(const Potion& potion)
{
    return ChoiceSourceOf(potion) != ChoiceSource::NONE;
}

ChoiceSource Battle::ChoiceSourceOf(const std::vector<CardEffect>& effects)
{
    for (const auto& effect : effects)
    {
        const ChoiceSource source = SourceOfChoice(effect);

        if (source != ChoiceSource::NONE)
        {
            return source;
        }
    }

    return ChoiceSource::NONE;
}

ChoiceSource Battle::ChoiceSourceOf(const Card& card)
{
    return ChoiceSourceOf(card.GetEffects());
}

ChoiceSource Battle::ChoiceSourceOf(const Potion& potion)
{
    return ChoiceSourceOf(potion.GetEffects());
}

void Battle::RollOffer(const Potion& potion)
{
    // A potion has no colour of its own, so it borrows the same stand-in it
    // borrows to resolve: the pool is the climber's unless an effect names
    // another, which is what the colourless potion does.
    const Card stand(CardId::INVALID, "Trinket", m_player.GetColor(),
                     CardType::SKILL, CardRarity::SPECIAL,
                     potion.GetTarget(), 0, {});

    RollOffer(stand, potion.GetEffects());
}

void Battle::RollOffer(const Card& card)
{
    RollOffer(card, card.GetEffects());
}

void Battle::RollOffer(const Card& card,
                       const std::vector<CardEffect>& effects)
{
    m_offered.clear();

    for (const auto& effect : effects)
    {
        if (effect.type != EffectType::OFFER_CARDS)
        {
            continue;
        }

        const std::vector<CardId>& whole =
            CardRegistry::GetPool(PoolColor(effect, card));
        std::vector<CardId> pool;

        // Only the kind asked for, which is how an attack potion differs from
        // a skill potion. No kind asked for means any of them.
        for (const CardId id : whole)
        {
            if (PassesFilter(CardRegistry::Get(id), effect.filter))
            {
                pool.emplace_back(id);
            }
        }

        if (pool.empty())
        {
            return;
        }

        // As many different cards as are asked for, or as many as the pool
        // holds when it is smaller than that.
        std::uniform_int_distribution<std::size_t> pick(0, pool.size() - 1);

        for (int tries = 0;
             tries < effect.value * 20 &&
             static_cast<int>(m_offered.size()) < effect.value &&
             m_offered.size() < pool.size();
             ++tries)
        {
            const CardId rolled = pool[pick(m_rng)];

            if (std::find(m_offered.begin(), m_offered.end(), rolled) ==
                m_offered.end())
            {
                m_offered.emplace_back(rolled);
            }
        }

        return;
    }
}

const std::vector<CardId>& Battle::GetOffered() const
{
    return m_offered;
}

std::size_t Battle::ChoiceCount(const Card& card) const
{
    const ChoiceSource source = ChoiceSourceOf(card);
    const std::size_t much = ChoiceCount(source);

    // A card doing the asking is still in the hand while it asks, and gone
    // from it by the time its own effects run, so it is not one of the cards
    // it may pick. A potion was never in the hand at all.
    return source == ChoiceSource::HAND && much > 0u ? much - 1u : much;
}

std::size_t Battle::ChoiceCount(const Potion& potion) const
{
    return ChoiceCount(ChoiceSourceOf(potion));
}

std::size_t Battle::ChoiceCount(ChoiceSource source) const
{
    switch (source)
    {
        case ChoiceSource::OFFERED:
            return m_offered.size();

        case ChoiceSource::HAND:
            return m_player.GetHand().size();

        case ChoiceSource::DISCARD:
            return m_player.GetDiscardPile().size();

        case ChoiceSource::EXHAUST:
            return m_player.GetExhaustPile().size();

        default:
            return 0u;
    }
}

int Battle::GetGoldFound() const
{
    return m_goldFound;
}

int Battle::GetGoldStolen() const
{
    int stolen = 0;

    for (const auto& monster : m_monsters)
    {
        // Killing a thief gets the gold back.
        if (!monster.IsDead())
        {
            stolen += monster.GetStolenGold();
        }
    }

    return stolen;
}

std::vector<Monster>& Battle::GetMonsters()
{
    return m_monsters;
}

const std::vector<Monster>& Battle::GetMonsters() const
{
    return m_monsters;
}

int Battle::GetEffectiveCost(const Card& card) const
{
    if (card.HasCostThisTurn())
    {
        return card.GetCostThisTurn();
    }

    // Bullet Time drops every cost for the turn.
    if (m_player.GetPower(PowerType::FREE_CARDS) > 0)
    {
        return 0;
    }

    // Corruption makes every skill free, at the price of exhausting it.
    if (card.GetCardType() == CardType::SKILL &&
        m_player.GetPower(PowerType::CORRUPTION) > 0)
    {
        return 0;
    }

    if (card.GetCost() == Card::COST_X)
    {
        return m_player.GetEnergy();
    }

    int cost = card.GetCost() - card.GetCostReduction();

    switch (card.GetCostModifier())
    {
        case CostModifier::HEALTH_LOST_THIS_BATTLE:
            cost -= m_healthLossCount;
            break;

        case CostModifier::HEALTH_LOST_RAISES_COST:
            cost += m_healthLossCount;
            break;

        case CostModifier::CARDS_DISCARDED_THIS_TURN:
            cost -= m_cardsDiscardedThisTurn;
            break;

        case CostModifier::POWERS_PLAYED_THIS_BATTLE:
            cost -= m_powersPlayedThisBattle;
            break;

        case CostModifier::NONE:
            break;
    }

    return cost < 0 ? 0 : cost;
}

bool Battle::CanPlay(std::size_t handIndex, std::size_t monsterIndex) const
{
    if (m_phase != BattlePhase::PLAYER_TURN)
    {
        return false;
    }

    const std::vector<Card>& hand = m_player.GetHand();

    if (handIndex >= hand.size())
    {
        return false;
    }

    const Card& card = hand[handIndex];

    const bool playableByRelic =
        (card.GetCardType() == CardType::CURSE &&
         m_player.HasRelic(RelicId::BLUE_CANDLE)) ||
        (card.GetCardType() == CardType::STATUS &&
         m_player.HasRelic(RelicId::MEDICAL_KIT));

    if ((!card.IsPlayable() && !playableByRelic) ||
        GetEffectiveCost(card) > m_player.GetEnergy())
    {
        return false;
    }

    // Entangled keeps every attack in hand.
    if (card.GetCardType() == CardType::ATTACK &&
        m_player.GetPower(PowerType::ENTANGLED) > 0)
    {
        return false;
    }

    // Velvet Choker only allows six cards a turn.
    if (m_cardsPlayedThisTurn >= 6 &&
        m_player.HasRelic(RelicId::VELVET_CHOKER))
    {
        return false;
    }

    if (card.GetTarget() == CardTarget::SINGLE_ENEMY &&
        (monsterIndex >= m_monsters.size() ||
         m_monsters[monsterIndex].IsGone()))
    {
        return false;
    }

    if (card.GetPlayCondition() == PlayCondition::HAND_ALL_ATTACKS)
    {
        for (const auto& other : hand)
        {
            if (other.GetCardType() != CardType::ATTACK)
            {
                return false;
            }
        }
    }

    if (card.GetPlayCondition() == PlayCondition::DRAW_PILE_EMPTY &&
        !m_player.GetDrawPile().empty())
    {
        return false;
    }

    // Normality allows three cards a turn and no more.
    if (m_cardsPlayedThisTurn >= 3)
    {
        for (const auto& other : hand)
        {
            if (other.GetId() == CardId::NORMALITY)
            {
                return false;
            }
        }
    }

    return true;
}

std::vector<std::size_t> Battle::GetLivingMonsterIndices() const
{
    std::vector<std::size_t> indices;

    for (std::size_t i = 0; i < m_monsters.size(); ++i)
    {
        if (!m_monsters[i].IsGone())
        {
            indices.emplace_back(i);
        }
    }

    return indices;
}

const std::map<CardId, int>& Battle::GetPlayedCounts() const
{
    return m_playedCounts;
}

const std::map<CardId, int>& Battle::GetStrandedCounts() const
{
    return m_strandedCounts;
}

std::vector<std::size_t> Battle::GetPlayableCardIndices() const
{
    std::vector<std::size_t> indices;

    if (m_phase != BattlePhase::PLAYER_TURN)
    {
        return indices;
    }

    const std::vector<std::size_t> living = GetLivingMonsterIndices();
    const std::size_t firstTarget = living.empty() ? 0 : living.front();

    for (std::size_t i = 0; i < m_player.GetHand().size(); ++i)
    {
        if (CanPlay(i, firstTarget))
        {
            indices.emplace_back(i);
        }
    }

    return indices;
}

int Battle::GetHealthLossCount() const
{
    return m_healthLossCount;
}

int Battle::GetCardsPlayedThisTurn() const
{
    return m_cardsPlayedThisTurn;
}

int Battle::GetCardsDiscardedThisTurn() const
{
    return m_cardsDiscardedThisTurn;
}

void Battle::BeginPlayerTurn()
{
    m_phase = BattlePhase::PLAYER_TURN;
    m_cardsPlayedThisTurn = 0;
    m_cardsDiscardedThisTurn = 0;
    m_attacksPlayedThisTurn = 0;

    // Flame Barrier and Intangible both have to survive the monster turn they
    // were raised against, so they are dropped here rather than when the
    // player turn ended.
    m_player.RemovePower(PowerType::FLAME_BARRIER);

    if (m_turn > 1 && m_player.GetPower(PowerType::INTANGIBLE) > 0)
    {
        m_player.AddPower(PowerType::INTANGIBLE, -1);
    }

    for (auto& monster : m_monsters)
    {
        if (monster.IsGone())
        {
            continue;
        }

        // What can only be brought so far down in a turn gets its footing
        // back.
        if (const int invincible = monster.GetPower(PowerType::INVINCIBLE);
            invincible > 0)
        {
            monster.SetDamageCapLeft(invincible);
        }

        // What is still in the air goes back to full, and armour that
        // answers hits forgets how many it has answered.
        if (monster.GetPower(PowerType::FLIGHT) > 0 &&
            monster.GetFlightBase() > 0)
        {
            monster.RemovePower(PowerType::FLIGHT);
            monster.AddPower(PowerType::FLIGHT, monster.GetFlightBase());
        }

        if (monster.GetPower(PowerType::MALLEABLE) > 0 &&
            monster.GetMalleableBase() > 0)
        {
            monster.RemovePower(PowerType::MALLEABLE);
            monster.AddPower(PowerType::MALLEABLE,
                             monster.GetMalleableBase());
        }
    }

    // A confused climber never knows what the cards in hand will cost.
    if (m_player.GetPower(PowerType::CONFUSED) > 0)
    {
        for (auto& card : m_player.GetHand())
        {
            if (card.GetCost() >= 0)
            {
                std::uniform_int_distribution<int> roll(0, 3);
                card.SetCostThisTurn(roll(m_rng));
            }
        }
    }

    // Barricade is the whole point of Barricade: the block stays. Blur holds
    // it for a single turn.
    const int blur = m_player.GetPower(PowerType::BLUR);

    if (m_player.GetPower(PowerType::BARRICADE) == 0 && blur == 0)
    {
        if (m_player.HasRelic(RelicId::CALIPERS))
        {
            // Calipers only lets 15 of it go.
            const int kept = m_player.GetBlock() - 15;

            m_player.ClearBlock();
            m_player.AddBlock(kept);
        }
        else
        {
            m_player.ClearBlock();
        }
    }

    if (blur > 0)
    {
        m_player.AddPower(PowerType::BLUR, -1);
    }

    // Phantasmal Killer was played last turn for this one.
    if (m_player.GetPower(PowerType::PHANTASMAL) > 0)
    {
        m_player.AddPower(PowerType::PHANTASMAL, -1);
        m_player.AddPower(PowerType::DOUBLE_DAMAGE, 1);
    }

    TickPoison(m_player);

    if (m_player.IsDead())
    {
        UpdatePhase();
        return;
    }

    if (const int demonForm = m_player.GetPower(PowerType::DEMON_FORM);
        demonForm > 0)
    {
        ApplyPowerTo(m_player, PowerType::STRENGTH, demonForm);
    }

    int energy = m_player.GetMaxEnergy() +
                 m_player.GetPower(PowerType::BERSERK) +
                 m_player.GetPower(PowerType::ENERGIZED);

    for (const auto& relic : m_player.GetRelics())
    {
        if (RelicRegistry::GivesExtraEnergy(relic.GetId()))
        {
            ++energy;
        }
    }

    // A slaver's collar is only worth carrying to the harder fights.
    if (m_player.HasRelic(RelicId::SLAVERS_COLLAR) &&
        (IsBossFight() || IsEliteFight()))
    {
        ++energy;
    }

    // And a tea set is worth something for the first turn only.
    if (m_turn == 1)
    {
        energy += m_player.GetBonusEnergy();
        m_player.SetBonusEnergy(0);
    }

    // Art of War pays out for a turn without an attack.
    if (m_player.HasRelic(RelicId::ART_OF_WAR) && m_turn > 1 &&
        !m_playedAttackLastTurn)
    {
        ++energy;
    }

    if (m_player.HasRelic(RelicId::ICE_CREAM))
    {
        // Ice Cream keeps whatever was left over.
        energy += m_player.GetEnergy();
    }

    m_player.SetEnergy(energy);

    // The relics that open a battle land inside the first turn, after the
    // block has been cleared, so that the block Anchor hands over survives.
    if (m_turn == 1)
    {
        FireRelics(RelicHook::BATTLE_START);
    }
    m_player.RemovePower(PowerType::ENERGIZED);

    if (const int planned = m_player.GetPower(PowerType::NEXT_TURN_BLOCK);
        planned > 0)
    {
        GainBlock(m_player.CalculateBlockGain(planned));
        m_player.RemovePower(PowerType::NEXT_TURN_BLOCK);
    }

    if (const int fumes = m_player.GetPower(PowerType::NOXIOUS_FUMES);
        fumes > 0)
    {
        for (auto& monster : m_monsters)
        {
            if (!monster.IsDead())
            {
                ApplyPowerTo(monster, PowerType::POISON, fumes);
            }
        }
    }

    const int brutality = m_player.GetPower(PowerType::BRUTALITY);

    if (brutality > 0)
    {
        // A card did this, even though a turn starting is what set it off,
        // so a Rupture answers for it. The wiki names Combust and Brutality
        // in the same breath: both trigger it every turn.
        PlayerLoseHealth(brutality, true);

        if (m_player.IsDead())
        {
            UpdatePhase();
            return;
        }
    }

    // Innate cards are already in hand and count towards the opening draw.
    int toDraw = m_player.GetCardsPerTurn();

    if (m_turn == 1)
    {
        for (const auto& held : m_player.GetHand())
        {
            if (held.Has(CardFlag::INNATE))
            {
                --toDraw;
            }
        }
    }

    if (m_player.HasRelic(RelicId::SNECKO_EYE))
    {
        toDraw += 2;
    }

    // A head slam leaves a climber drawing short for a turn.
    if (const int fewer = m_player.GetPower(PowerType::DRAW_REDUCTION);
        fewer > 0)
    {
        toDraw = std::max(0, toDraw - fewer);
        m_player.RemovePower(PowerType::DRAW_REDUCTION);
    }

    DrawCards(toDraw);
    DrawCards(brutality);

    if (const int extra = m_player.GetPower(PowerType::DRAW_NEXT_TURN);
        extra > 0)
    {
        DrawCards(extra);
        m_player.RemovePower(PowerType::DRAW_NEXT_TURN);
    }

    // Infinite Blades hands over a Shiv, Tools of the Trade trades a card.
    for (int i = 0; i < m_player.GetPower(PowerType::INFINITE_BLADES); ++i)
    {
        m_player.AddCardToPile(CardRegistry::Get(CardId::SHIV), CardPile::HAND,
                               m_rng);
    }

    if (const int tools = m_player.GetPower(PowerType::TOOLS_OF_THE_TRADE);
        tools > 0)
    {
        DrawCards(tools);
        DiscardCards(tools, false, 0);
    }

    for (int i = 0; i < m_player.GetPower(PowerType::CREATIVE_AI); ++i)
    {
        const std::vector<CardId> powers =
            CardRegistry::GetPoolByType(CardColor::BLUE, CardType::POWER);

        if (!powers.empty())
        {
            std::uniform_int_distribution<std::size_t> pick(
                0, powers.size() - 1);
            m_player.AddCardToPile(CardRegistry::Get(powers[pick(m_rng)]),
                                   CardPile::HAND, m_rng);
        }
    }

    for (int i = 0; i < m_player.GetPower(PowerType::HELLO_WORLD); ++i)
    {
        const std::vector<CardId> commons =
            CardRegistry::GetPool(CardColor::BLUE, CardRarity::COMMON);

        if (!commons.empty())
        {
            std::uniform_int_distribution<std::size_t> pick(
                0, commons.size() - 1);
            m_player.AddCardToPile(CardRegistry::Get(commons[pick(m_rng)]),
                                   CardPile::HAND, m_rng);
        }
    }

    for (int i = 0; i < m_player.GetPower(PowerType::MAGNETISM); ++i)
    {
        const std::vector<CardId>& colorless =
            CardRegistry::GetPool(CardColor::COLORLESS);

        if (!colorless.empty())
        {
            std::uniform_int_distribution<std::size_t> pick(
                0, colorless.size() - 1);
            m_player.AddCardToPile(CardRegistry::Get(colorless[pick(m_rng)]),
                                   CardPile::HAND, m_rng);
        }
    }

    for (int i = 0; i < m_player.GetPower(PowerType::MAYHEM); ++i)
    {
        PlayTopCardOfDrawPile();
    }

    TriggerPlasmaPassives();

    // Loop sets the front orb off again before anything else happens.
    if (!m_player.GetOrbs().empty())
    {
        for (int i = 0; i < m_player.GetPower(PowerType::LOOP); ++i)
        {
            TriggerOrbPassive(m_player.GetOrbs().front());
        }
    }

    if (const int learning = m_player.GetPower(PowerType::MACHINE_LEARNING);
        learning > 0)
    {
        DrawCards(learning);
    }

    if (const int bias = m_player.GetPower(PowerType::BIASED_COGNITION);
        bias > 0)
    {
        m_player.AddPower(PowerType::FOCUS, -bias);
    }

    // Nightmare hands over the copies it promised last turn.
    for (int i = 0; i < m_rememberedCopies; ++i)
    {
        m_player.AddCardToPile(CardRegistry::Get(m_rememberedCard),
                               CardPile::HAND, m_rng);
    }

    m_rememberedCopies = 0;
    m_rememberedCard = CardId::INVALID;

    m_kiteSpentThisTurn = false;
    m_pelletsSpentThisTurn = false;
    m_playedAttackThisTurn = false;
    m_playedSkillThisTurn = false;
    m_playedPowerThisTurn = false;

    for (auto& relic : m_player.GetRelics())
    {
        if (relic.CountsPerTurn())
        {
            relic.ResetCounter();
        }
    }

    FireRelics(RelicHook::TURN_START);

    UpdatePhase();
}

void Battle::EndPlayerTurn()
{
    ResolvePrideInHand();
    ResolveEndOfTurnHandCards();
    ExhaustEtherealCards();
    RetainPlannedCards();

    m_player.DiscardHand();

    // The retain mark only lasted for this discard.
    for (auto& held : m_player.GetHand())
    {
        held.RemoveFlag(CardFlag::RETAIN);
    }

    m_player.RemovePower(PowerType::RETAIN_HAND);

    if (const int bomb = m_player.GetPower(PowerType::THE_BOMB); bomb > 0)
    {
        m_player.AddPower(PowerType::THE_BOMB, -1);

        if (m_player.GetPower(PowerType::THE_BOMB) == 0)
        {
            DamageAllEnemies(m_bombDamage);
        }
    }

    if (m_player.GetPower(PowerType::NO_BLOCK) > 0)
    {
        m_player.AddPower(PowerType::NO_BLOCK, -1);
    }

    // Being constricted costs the same every turn it lasts.
    if (const int constricted = m_player.GetPower(PowerType::CONSTRICTED);
        constricted > 0)
    {
        PlayerLoseHealth(constricted, false);
    }

    // A codex leaves something in the draw pile at the end of every turn.
    if (m_player.HasRelic(RelicId::NILRYS_CODEX))
    {
        const std::vector<CardId>& pool =
            CardRegistry::GetPool(CardColor::COLORLESS);

        if (!pool.empty())
        {
            std::uniform_int_distribution<std::size_t> pick(
                0, pool.size() - 1);

            m_player.AddCardToPile(CardRegistry::Get(pool[pick(m_rng)]),
                                   CardPile::DRAW_TOP, m_rng);
        }
    }

    m_necronomiconSpent = false;

    TriggerOrbPassives();

    if (const int metallicize = m_player.GetPower(PowerType::METALLICIZE);
        metallicize > 0)
    {
        GainBlock(m_player.CalculateBlockGain(metallicize));
    }

    if (const int combust = m_player.GetPower(PowerType::COMBUST); combust > 0)
    {
        // One health for every copy played, which is not the same number as
        // the damage: a Combust and a sharpened one together deal twelve and
        // cost two, and twelve says nothing about two. So the copies are
        // counted beside the damage, the way the game itself keeps them.
        //
        // A card did this, so a Rupture answers for it.
        const int copies =
            std::max(1, m_player.GetPower(PowerType::COMBUST_COPIES));

        PlayerLoseHealth(copies, true);
        DamageAllEnemies(combust);
    }

    if (const int regen = m_player.GetPower(PowerType::REGENERATION);
        regen > 0)
    {
        HealPlayer(regen);
        m_player.AddPower(PowerType::REGENERATION, -1);
    }

    if (const int wraith = m_player.GetPower(PowerType::WRAITH_FORM);
        wraith > 0)
    {
        m_player.AddPower(PowerType::DEXTERITY, -wraith);
    }

    for (auto& monster : m_monsters)
    {
        monster.RemovePower(PowerType::CHOKED);
    }

    // Powers that only last the turn.
    m_player.RemovePower(PowerType::RAGE);
    m_player.RemovePower(PowerType::ENTANGLED);
    m_player.RemovePower(PowerType::NO_DRAW);
    m_player.RemovePower(PowerType::DOUBLE_TAP);
    m_player.RemovePower(PowerType::DOUBLE_DAMAGE);
    m_player.RemovePower(PowerType::FREE_CARDS);
    m_player.RemovePower(PowerType::BURST);
    m_player.RemovePower(PowerType::DUPLICATION);

    if (m_player.HasRelic(RelicId::POCKETWATCH) && m_cardsPlayedThisTurn <= 3)
    {
        m_player.AddPower(PowerType::DRAW_NEXT_TURN, 3);
    }

    m_playedAttackLastTurn = m_playedAttackThisTurn;

    FireRelics(RelicHook::TURN_END);

    DecayTimedPowers(m_player);
    ClearTurnCosts();
    UpdatePhase();
}

void Battle::RunMonsterTurn()
{
    m_phase = BattlePhase::MONSTER_TURN;

    for (auto& monster : m_monsters)
    {
        if (!monster.IsGone())
        {
            monster.ClearBlock();
            TickPoison(monster);

            // Poison can finish a monster off before anything moves.
            if (monster.IsDead())
            {
                OnMonsterDied(monster);
            }
        }
    }

    for (std::size_t i = 0; i < m_monsters.size(); ++i)
    {
        Monster& monster = m_monsters[i];

        if (monster.IsGone())
        {
            continue;
        }

        // A sleeping monster does nothing but count the turns.
        if (monster.GetPower(PowerType::ASLEEP) > 0)
        {
            monster.AddPower(PowerType::ASLEEP, -1);
            DecayTimedPowers(monster);
            continue;
        }

        ResolveMonsterMove(monster);

        if (!monster.IsGone())
        {
            const int metallicize = monster.GetPower(PowerType::METALLICIZE);
            const int plated = monster.GetPower(PowerType::PLATED_ARMOR);

            if (metallicize > 0)
            {
                monster.AddBlock(metallicize);
            }

            if (plated > 0)
            {
                monster.AddBlock(plated);
            }

            if (const int regen = monster.GetPower(PowerType::REGENERATION);
                regen > 0)
            {
                monster.Heal(regen);
            }

            // What it gave up when it was hit, it takes back.
            if (const int shifted =
                    monster.GetPower(PowerType::SHIFTING_LOSS);
                shifted > 0)
            {
                monster.AddPower(PowerType::STRENGTH, shifted);
                monster.RemovePower(PowerType::SHIFTING_LOSS);
            }

            // Something that slips out of reach does so turn about.
            if (monster.GetPower(PowerType::INTANGIBLE_CYCLE) > 0 &&
                m_turn % 2 == 1)
            {
                monster.AddPower(PowerType::INTANGIBLE, 1);
            }

            // And what is fading fades.
            if (monster.GetPower(PowerType::FADING) > 0)
            {
                monster.AddPower(PowerType::FADING, -1);

                if (monster.GetPower(PowerType::FADING) == 0)
                {
                    monster.SetHealth(0);
                    OnMonsterDied(monster);
                }
            }
        }

        DecayTimedPowers(monster);

        if (m_player.IsDead())
        {
            break;
        }
    }

    ApplyPendingSpawns();
    UpdatePhase();
}

void Battle::UpdatePhase()
{
    // A fairy in a bottle drinks itself, before anything else has a say.
    if (m_player.IsDead() && !m_fairySpent)
    {
        std::vector<Potion>& held = m_player.GetPotions();

        for (std::size_t i = 0; i < held.size(); ++i)
        {
            if (held[i].GetId() != PotionId::FAIRY_IN_A_BOTTLE)
            {
                continue;
            }

            // A sacred bark pours a double of this as well.
            const int share =
                m_player.HasRelic(RelicId::SACRED_BARK) ? 60 : 30;

            m_fairySpent = true;
            held.erase(held.begin() + static_cast<std::ptrdiff_t>(i));
            m_player.SetHealth(
                std::max(1, m_player.GetMaxHealth() * share / 100));
            break;
        }
    }

    if (m_player.IsDead() && !m_lizardTailSpent &&
        m_player.HasRelic(RelicId::LIZARD_TAIL))
    {
        // Lizard Tail picks the player back up, once.
        m_lizardTailSpent = true;
        m_player.Heal(m_player.GetMaxHealth() / 2 - m_player.GetHealth());
    }

    if (m_player.IsDead())
    {
        m_phase = BattlePhase::LOST;
        return;
    }

    const bool allMonstersDead =
        std::all_of(m_monsters.begin(), m_monsters.end(),
                    [](const Monster& monster) { return monster.IsGone(); });

    if (allMonstersDead)
    {
        m_phase = BattlePhase::WON;
    }
}

void Battle::ResolvePlayedCard(Card card, std::size_t monsterIndex,
                               std::size_t choiceIndex, int energySpent)
{
    ++m_cardsPlayedThisTurn;

    if (card.GetCardType() == CardType::ATTACK)
    {
        ++m_attacksPlayedThisTurn;
    }

    m_cardsExhaustedThisPlay = 0;
    m_cardsDiscardedThisPlay = 0;
    m_unblockedDamageThisPlay = 0;
    m_killedTargetThisPlay = false;

    // Read the reacting powers now: a card that grants one of them, such as
    // After Image or Choke, must not set it off on itself.
    const PlayTriggers before = ReadPlayTriggers();
    const int doubleTapHeld = m_player.GetPower(PowerType::DOUBLE_TAP);
    const int burstHeld = m_player.GetPower(PowerType::BURST);
    const int echoFormHeld = m_player.GetPower(PowerType::ECHO_FORM);
    const int duplicationHeld = m_player.GetPower(PowerType::DUPLICATION);

    ResolveCardEffects(card, ResolveTarget(card, monsterIndex), choiceIndex,
                       energySpent);

    // Double Tap makes the attack resolve a second time, for free, and Burst
    // does the same for a skill.
    const bool echoed = echoFormHeld > 0 && m_cardsPlayedThisTurn == 1;

    // A necronomicon reads the first heavy attack of the turn out twice.
    const bool necronomicon =
        !m_necronomiconSpent && card.GetCardType() == CardType::ATTACK &&
        card.GetCost() >= 2 && m_player.HasRelic(RelicId::NECRONOMICON);

    if (necronomicon)
    {
        m_necronomiconSpent = true;
    }

    const bool twice =
        (card.GetCardType() == CardType::ATTACK && doubleTapHeld > 0) ||
        (card.GetCardType() == CardType::SKILL && burstHeld > 0) || echoed ||
        duplicationHeld > 0 || necronomicon;

    if (twice)
    {
        if (duplicationHeld > 0)
        {
            m_player.AddPower(PowerType::DUPLICATION, -1);
        }
        else if (necronomicon)
        {
            // The book asks for nothing else.
        }
        else if (!echoed)
        {
            if (card.GetCardType() == CardType::ATTACK)
            {
                m_player.AddPower(PowerType::DOUBLE_TAP, -1);
            }
            else
            {
                m_player.AddPower(PowerType::BURST, -1);
            }
        }

        m_cardsExhaustedThisPlay = 0;
        m_unblockedDamageThisPlay = 0;

        ResolveCardEffects(card, ResolveTarget(card, monsterIndex), choiceIndex,
                           energySpent);
    }

    OnCardPlayed(card, before);

    if (card.GetCardType() == CardType::CURSE &&
        m_player.HasRelic(RelicId::BLUE_CANDLE))
    {
        PlayerLoseHealth(1, true);
    }

    const bool exhaust =
        card.Has(CardFlag::EXHAUST) ||
        card.GetCardType() == CardType::CURSE ||
        card.GetCardType() == CardType::STATUS ||
        (card.GetCardType() == CardType::SKILL &&
         m_player.GetPower(PowerType::CORRUPTION) > 0);

    if (exhaust)
    {
        OnCardExhausted(std::move(card));
    }
    else
    {
        m_player.GetDiscardPile().emplace_back(std::move(card));
    }
}

void Battle::ResolveCardEffects(Card& card, Monster* target,
                                std::size_t choiceIndex, int energySpent)
{
    for (const auto& effect : card.GetEffects())
    {
        ResolveEffect(effect, card, target, choiceIndex, energySpent);
    }
}

void Battle::ResolveEffect(const CardEffect& effect, Card& card,
                           Monster* target, std::size_t choiceIndex,
                           int energySpent)
{
    if (!ConditionHolds(effect, target))
    {
        return;
    }

    switch (effect.type)
    {
        case EffectType::DEAL_DAMAGE:
        {
            int times = effect.times;
            int amount = ResolveValue(effect, card, energySpent);

            if (effect.times == CardEffect::TIMES_X)
            {
                times = energySpent;
            }

            // Damage that counts something lands as one hit per unit: per card
            // exhausted, per attack played, per skill held.
            const int units = CountUnits(effect.valueSource);

            if (units >= 0 && !effect.singleHit)
            {
                times = units;
                amount = effect.extra;
            }

            amount += card.GetBonusDamage();

            // A Shiv hits harder once Accuracy is up.
            if (card.GetId() == CardId::SHIV)
            {
                amount += m_player.GetPower(PowerType::ACCURACY);
            }

            if (card.GetCardType() == CardType::ATTACK)
            {
                if (!m_akabekoSpent && m_player.HasRelic(RelicId::AKABEKO))
                {
                    m_akabekoSpent = true;
                    amount += 8;
                }

                if (m_player.HasRelic(RelicId::STRIKE_DUMMY) &&
                    IsStrikeCard(card))
                {
                    amount += 3;
                }
            }

            for (int i = 0; i < times; ++i)
            {
                for (Monster* monster : GetEffectTargets(effect, card, target))
                {
                    // Whether this blow was the one that finished it off, for
                    // the cards that are paid for a killing.
                    const bool standing = !monster->IsDead();

                    DealDamageToMonster(*monster, amount,
                                        card.GetCardType() ==
                                            CardType::ATTACK);

                    if (effect.goldIfFatal > 0 && standing &&
                        monster->IsDead() &&
                        monster->GetPower(PowerType::MINION) == 0)
                    {
                        m_goldFound += effect.goldIfFatal;
                    }
                }
            }
            break;
        }

        case EffectType::GAIN_BLOCK:
            GainBlock(m_player.CalculateBlockGain(
                ResolveValue(effect, card, energySpent) +
                card.GetBonusBlock()));
            break;

        case EffectType::APPLY_POWER:
        {
            const int amount = ResolveValue(effect, card, energySpent);

            if (effect.power == PowerType::THE_BOMB)
            {
                // The blast keeps the size it was played at.
                m_bombDamage = effect.extra;
            }
            const int times = effect.times == CardEffect::TIMES_X
                                  ? energySpent
                                  : effect.times;

            for (int i = 0; i < times; ++i)
            {
                if (effect.target == EffectTarget::SELF)
                {
                    ApplyPowerTo(m_player, effect.power, amount);
                }
                else
                {
                    for (Monster* monster :
                         GetEffectTargets(effect, card, target))
                    {
                        ApplyPowerTo(*monster, effect.power, amount);
                    }
                }
            }
            break;
        }

        case EffectType::GAIN_ENERGY:
            m_player.GainEnergy(ResolveValue(effect, card, energySpent));
            break;

        case EffectType::DRAW_CARD:
            DrawCards(ResolveValue(effect, card, energySpent));
            break;

        case EffectType::LOSE_HEALTH:
            PlayerLoseHealth(effect.value, true);
            break;

        case EffectType::HEAL:
            HealPlayer(ResolveValue(effect, card, energySpent));
            break;

        case EffectType::HEAL_PERCENT:
            HealPlayer(m_player.GetMaxHealth() * effect.value / 100);
            break;

        case EffectType::INCREASE_MAX_HEALTH:
            m_player.IncreaseMaxHealth(effect.value);
            break;

        case EffectType::ADD_CARD:
            for (int i = 0; i < ResolveValue(effect, card, energySpent); ++i)
            {
                m_player.AddCardToPile(
                    CardRegistry::Get(effect.cardId,
                                      effect.upgradedCard ? 1 : 0),
                    effect.pile, m_rng);
            }
            break;

        case EffectType::COPY_SELF_TO_DISCARD:
            m_player.GetDiscardPile().emplace_back(
                CardRegistry::Get(card.GetId(), card.GetUpgradeCount()));
            break;

        case EffectType::COPY_HAND_CARD:
        {
            std::vector<Card>& hand = m_player.GetHand();

            if (choiceIndex >= hand.size() ||
                !PassesFilter(hand[choiceIndex], effect.filter))
            {
                break;
            }

            const Card copied = hand[choiceIndex];

            for (int i = 0; i < effect.value; ++i)
            {
                m_player.AddCardToPile(copied, CardPile::HAND, m_rng);
            }
            break;
        }

        case EffectType::ADD_RANDOM_ATTACK:
        {
            const std::vector<CardId> pool =
                CardRegistry::GetAttackPool(PoolColor(effect, card));

            if (pool.empty())
            {
                break;
            }

            std::uniform_int_distribution<std::size_t> pick(0,
                                                            pool.size() - 1);

            for (int i = 0; i < effect.value; ++i)
            {
                Card attack = CardRegistry::Get(pool[pick(m_rng)]);
                attack.SetCostThisTurn(0);

                m_player.AddCardToPile(std::move(attack), effect.pile, m_rng);
            }
            break;
        }

        case EffectType::UPGRADE_HAND_CARD:
        {
            std::vector<Card>& hand = m_player.GetHand();

            if (effect.value == 0)
            {
                std::vector<Card>* piles[] = { &hand,
                                               &m_player.GetDrawPile(),
                                               &m_player.GetDiscardPile() };
                const std::size_t reach = effect.extra == 1 ? 3u : 1u;

                for (std::size_t p = 0; p < reach; ++p)
                {
                    for (auto& held : *piles[p])
                    {
                        held = CardRegistry::Get(held.GetId(),
                                                 held.GetUpgradeCount() + 1);
                    }
                }
            }
            else if (!hand.empty())
            {
                std::size_t index = choiceIndex;

                if (effect.randomPick)
                {
                    std::uniform_int_distribution<std::size_t> pick(
                        0, hand.size() - 1);
                    index = pick(m_rng);
                }

                if (index < hand.size())
                {
                    hand[index] = CardRegistry::Get(
                        hand[index].GetId(),
                        hand[index].GetUpgradeCount() + 1);
                }
            }
            break;
        }

        case EffectType::EXHAUST_HAND_CARD:
            // As many as the climber named, which is an Elixir.
            if (effect.manyCards)
            {
                ThrowAwayNamed(true);
                break;
            }

            for (int i = 0; i < effect.value; ++i)
            {
                std::vector<Card>& hand = m_player.GetHand();

                if (hand.empty())
                {
                    break;
                }

                std::size_t index = choiceIndex;

                if (effect.randomPick)
                {
                    std::uniform_int_distribution<std::size_t> pick(
                        0, hand.size() - 1);
                    index = pick(m_rng);
                }

                if (index >= hand.size())
                {
                    break;
                }

                Card exhausted = hand[index];
                hand.erase(hand.begin() + static_cast<std::ptrdiff_t>(index));

                OnCardExhausted(std::move(exhausted));
            }
            break;

        case EffectType::EXHAUST_HAND:
        {
            std::vector<Card> kept;
            std::vector<Card> exhausting;

            for (auto& held : m_player.GetHand())
            {
                if (PassesFilter(held, effect.filter))
                {
                    exhausting.emplace_back(std::move(held));
                }
                else
                {
                    kept.emplace_back(std::move(held));
                }
            }

            m_player.GetHand() = std::move(kept);

            for (auto& exhausted : exhausting)
            {
                OnCardExhausted(std::move(exhausted));
            }
            break;
        }

        case EffectType::RETURN_FROM_EXHAUST:
        {
            std::vector<Card>& exhaustPile = m_player.GetExhaustPile();

            if (choiceIndex >= exhaustPile.size())
            {
                break;
            }

            Card returning = exhaustPile[choiceIndex];
            exhaustPile.erase(exhaustPile.begin() +
                              static_cast<std::ptrdiff_t>(choiceIndex));

            m_player.AddCardToPile(std::move(returning), CardPile::HAND, m_rng);
            break;
        }

        case EffectType::DISCARD_TO_DRAW_TOP:
        {
            std::vector<Card>& discardPile = m_player.GetDiscardPile();

            if (choiceIndex >= discardPile.size())
            {
                break;
            }

            Card moving = discardPile[choiceIndex];
            discardPile.erase(discardPile.begin() +
                              static_cast<std::ptrdiff_t>(choiceIndex));

            m_player.AddCardToPile(std::move(moving), CardPile::DRAW_TOP,
                                   m_rng);
            break;
        }

        case EffectType::HAND_TO_DRAW_TOP:
        {
            std::vector<Card>& hand = m_player.GetHand();

            if (choiceIndex >= hand.size())
            {
                break;
            }

            Card moving = hand[choiceIndex];
            hand.erase(hand.begin() +
                       static_cast<std::ptrdiff_t>(choiceIndex));

            m_player.AddCardToPile(std::move(moving), CardPile::DRAW_TOP,
                                   m_rng);
            break;
        }

        case EffectType::PLAY_TOP_CARD:
            PlayTopCardOfDrawPile();
            break;

        case EffectType::DOUBLE_BLOCK:
            GainBlock(m_player.GetBlock());
            break;

        case EffectType::DOUBLE_STRENGTH:
            ApplyPowerTo(m_player, PowerType::STRENGTH,
                         m_player.GetPower(PowerType::STRENGTH));
            break;

        case EffectType::INCREASE_SELF_DAMAGE:
            card.AddBonusDamage(effect.value);
            break;

        case EffectType::DISCARD_CARDS:
            // As many as the climber named, which is a Gambler's Brew, or a
            // fixed number off the hand, which is everything else.
            if (effect.manyCards)
            {
                ThrowAwayNamed(false);
                break;
            }

            DiscardCards(effect.value, effect.randomPick, choiceIndex);
            break;

        case EffectType::DISCARD_HAND:
            DiscardWholeHand(effect.filter);
            break;

        case EffectType::MULTIPLY_TARGET_POWER:
            for (Monster* monster : GetEffectTargets(effect, card, target))
            {
                const int held = monster->GetPower(effect.power);

                if (held > 0)
                {
                    monster->AddPower(effect.power,
                                      held * (effect.value - 1));
                }
            }
            break;

        case EffectType::DRAW_UNTIL:
            while (static_cast<int>(m_player.GetHand().size()) < effect.value)
            {
                const std::size_t before = m_player.GetHand().size();

                DrawCards(1);

                if (m_player.GetHand().size() == before)
                {
                    break;
                }
            }
            break;

        case EffectType::ADD_RANDOM_SKILL:
        {
            const std::vector<CardId> pool =
                CardRegistry::GetPoolByType(PoolColor(effect, card),
                                            CardType::SKILL);

            if (pool.empty())
            {
                break;
            }

            std::uniform_int_distribution<std::size_t> pick(0,
                                                            pool.size() - 1);

            for (int i = 0; i < effect.value; ++i)
            {
                Card skill = CardRegistry::Get(pool[pick(m_rng)]);
                skill.SetCostThisTurn(0);

                m_player.AddCardToPile(std::move(skill), effect.pile, m_rng);
            }
            break;
        }

        case EffectType::OFFER_CARDS:
        {
            // Whichever of the handful was picked, into the hand at nought
            // for the turn. The handful was rolled up when the card was
            // played, so that it could be looked at before being picked from.
            if (choiceIndex < m_offered.size())
            {
                Card taken = CardRegistry::Get(m_offered[choiceIndex]);

                taken.SetCostThisTurn(0);
                m_player.AddCardToPile(std::move(taken), effect.pile, m_rng);
            }

            // Not cleared: a Sacred Bark pours the potion twice, and what the
            // wiki says that gives is two copies of the one card that was
            // picked rather than two pickings. The handful is thrown away
            // when the next one is rolled up.
            break;
        }

        case EffectType::SETUP_CARD:
        {
            std::vector<Card>& hand = m_player.GetHand();
            std::vector<std::size_t> picked;

            if (effect.manyCards)
            {
                for (const std::size_t at : m_choices)
                {
                    if (at < hand.size() &&
                        std::find(picked.begin(), picked.end(), at) ==
                            picked.end())
                    {
                        picked.emplace_back(at);
                    }
                }
            }
            else if (choiceIndex < hand.size())
            {
                picked.emplace_back(choiceIndex);
            }

            if (picked.empty())
            {
                break;
            }

            // Read out in the order they were named, because that is the
            // order they end up in. Then taken out of the hand from the back
            // forwards, so that taking one out does not move the next along.
            std::vector<Card> moving;

            for (const std::size_t at : picked)
            {
                moving.emplace_back(hand[at]);
                moving.back().SetCostThisTurn(0);
            }

            std::vector<std::size_t> backwards = picked;

            std::sort(backwards.begin(), backwards.end(),
                      std::greater<std::size_t>());

            for (const std::size_t at : backwards)
            {
                hand.erase(hand.begin() + static_cast<std::ptrdiff_t>(at));
            }

            for (auto& card : moving)
            {
                m_player.AddCardToPile(std::move(card), effect.pile, m_rng);
            }

            break;
        }

        case EffectType::REMEMBER_CARD:
        {
            const std::vector<Card>& hand = m_player.GetHand();

            if (choiceIndex < hand.size())
            {
                m_rememberedCard = hand[choiceIndex].GetId();
                m_rememberedCopies = effect.value;
            }
            break;
        }

        case EffectType::CHANNEL_ORB:
        {
            const int count = ResolveValue(effect, card, energySpent);

            for (int i = 0; i < count; ++i)
            {
                ChannelOrb(effect.orb);
            }
            break;
        }

        case EffectType::EVOKE_ORB:
        {
            const std::vector<Orb>& orbs = m_player.GetOrbs();
            const OrbType front =
                orbs.empty() ? OrbType::INVALID : orbs.front().type;

            EvokeFrontOrb(ResolveValue(effect, card, energySpent));

            if (effect.extra == 1 && front != OrbType::INVALID)
            {
                // Recursion puts the orb it just spent back into orbit.
                ChannelOrb(front);
            }
            break;
        }

        case EffectType::REMOVE_ALL_ORBS:
            m_player.GetOrbs().clear();
            break;

        case EffectType::TRIGGER_DARK_ORBS:
            for (auto& orb : m_player.GetOrbs())
            {
                if (orb.type == OrbType::DARK)
                {
                    TriggerOrbPassive(orb);
                }
            }
            break;

        case EffectType::OBTAIN_POTION:
        {
            const std::vector<PotionId> pool =
                PotionRegistry::GetAll(m_player.GetColor());

            if (pool.empty())
            {
                break;
            }

            std::uniform_int_distribution<std::size_t> pick(0,
                                                            pool.size() - 1);
            m_player.AddPotion(PotionRegistry::Get(pool[pick(m_rng)]));
            break;
        }

        case EffectType::EVOKE_ALL_ORBS:
            EvokeAllOrbs();
            break;

        case EffectType::ADD_ORB_SLOTS:
            m_player.AddOrbSlots(effect.value);
            break;

        case EffectType::ADD_RANDOM_POWER:
        {
            const std::vector<CardId> pool =
                CardRegistry::GetPoolByType(PoolColor(effect, card),
                                            CardType::POWER);

            if (pool.empty())
            {
                break;
            }

            std::uniform_int_distribution<std::size_t> pick(0,
                                                            pool.size() - 1);
            Card made = CardRegistry::Get(pool[pick(m_rng)]);
            made.SetCostThisTurn(0);

            m_player.AddCardToPile(std::move(made), CardPile::HAND, m_rng);
            break;
        }

        case EffectType::ADD_RANDOM_COMMON:
        {
            const std::vector<CardId> pool = CardRegistry::GetPoolByRarity(
                PoolColor(effect, card), CardRarity::COMMON);

            if (pool.empty())
            {
                break;
            }

            std::uniform_int_distribution<std::size_t> pick(0,
                                                            pool.size() - 1);

            for (int i = 0; i < effect.value; ++i)
            {
                m_player.AddCardToPile(CardRegistry::Get(pool[pick(m_rng)]),
                                       CardPile::HAND, m_rng);
            }
            break;
        }

        case EffectType::DOUBLE_ENERGY:
            m_player.GainEnergy(m_player.GetEnergy());
            break;

        case EffectType::REMOVE_BLOCK:
            for (Monster* monster : GetEffectTargets(effect, card, target))
            {
                monster->ClearBlock();
            }
            break;

        case EffectType::RETURN_FROM_DISCARD:
        {
            std::vector<Card>& discardPile = m_player.GetDiscardPile();

            if (effect.extra == 1)
            {
                // All for One takes back everything that costs nothing.
                std::vector<Card> left;

                for (auto& held : discardPile)
                {
                    if (GetEffectiveCost(held) == 0)
                    {
                        m_player.AddCardToPile(std::move(held), CardPile::HAND,
                                               m_rng);
                    }
                    else
                    {
                        left.emplace_back(std::move(held));
                    }
                }

                m_player.GetDiscardPile() = std::move(left);
                break;
            }

            if (choiceIndex >= discardPile.size())
            {
                break;
            }

            Card returning = discardPile[choiceIndex];
            discardPile.erase(discardPile.begin() +
                              static_cast<std::ptrdiff_t>(choiceIndex));

            m_player.AddCardToPile(std::move(returning), CardPile::HAND, m_rng);
            break;
        }

        case EffectType::DRAW_TO_HAND_FROM_TOP:
            // Seek picks from the draw pile; the top of it stands in for the
            // choice a player would make.
            for (int i = 0; i < effect.value; ++i)
            {
                std::vector<Card>& drawPile = m_player.GetDrawPile();

                if (drawPile.empty())
                {
                    break;
                }

                Card taken = drawPile.back();
                drawPile.pop_back();

                m_player.AddCardToPile(std::move(taken), CardPile::HAND,
                                       m_rng);
            }
            break;

        case EffectType::RESHUFFLE_ALL:
        {
            std::vector<Card>& drawPile = m_player.GetDrawPile();

            if (effect.extra != 1)
            {
                for (auto& held : m_player.GetHand())
                {
                    drawPile.emplace_back(std::move(held));
                }

                m_player.GetHand().clear();
            }

            for (auto& held : m_player.GetDiscardPile())
            {
                drawPile.emplace_back(std::move(held));
            }

            m_player.GetDiscardPile().clear();

            std::shuffle(drawPile.begin(), drawPile.end(), m_rng);
            DrawCards(effect.value);
            break;
        }

        case EffectType::EXHAUST_FOR_ENERGY:
        {
            std::vector<Card>& hand = m_player.GetHand();

            if (choiceIndex >= hand.size())
            {
                break;
            }

            Card exhausted = hand[choiceIndex];
            const int refund = GetEffectiveCost(exhausted);

            hand.erase(hand.begin() +
                       static_cast<std::ptrdiff_t>(choiceIndex));

            m_player.GainEnergy(refund);
            OnCardExhausted(std::move(exhausted));
            break;
        }

        case EffectType::INCREASE_SELF_BLOCK:
            card.AddBonusBlock(effect.value);
            break;

        case EffectType::INCREASE_CLAW_DAMAGE:
        {
            // Every Claw gets sharper, this one included.
            card.AddBonusDamage(effect.value);

            std::vector<Card>* piles[] = { &m_player.GetHand(),
                                           &m_player.GetDrawPile(),
                                           &m_player.GetDiscardPile(),
                                           &m_player.GetExhaustPile() };

            for (std::vector<Card>* pile : piles)
            {
                for (auto& held : *pile)
                {
                    if (held.GetId() == CardId::CLAW)
                    {
                        held.AddBonusDamage(effect.value);
                    }
                }
            }
            break;
        }

        case EffectType::REDUCE_SELF_COST:
            card.AddCostReduction(effect.value);
            break;

        case EffectType::ADD_RANDOM_CARD:
        {
            const std::vector<CardId>& pool =
                CardRegistry::GetPool(PoolColor(effect, card));

            if (pool.empty())
            {
                break;
            }

            std::uniform_int_distribution<std::size_t> pick(0,
                                                            pool.size() - 1);
            const int count = ResolveValue(effect, card, energySpent);

            for (int i = 0; i < count; ++i)
            {
                Card made = CardRegistry::Get(pool[pick(m_rng)],
                                              effect.upgradedCard ? 1 : 0);
                made.SetCostThisTurn(0);

                m_player.AddCardToPile(std::move(made), effect.pile, m_rng);
            }
            break;
        }

        case EffectType::RANDOMISE_HAND_COST:
        {
            // Nought to three, the same roll a confused climber gets on every
            // card drawn. A card that cannot be played at all keeps its
            // sentinel: there is no price to roll for it.
            std::uniform_int_distribution<int> roll(0, 3);

            for (auto& held : m_player.GetHand())
            {
                if (held.GetCost() >= 0)
                {
                    held.SetCostThisTurn(roll(m_rng));
                }
            }

            break;
        }

        case EffectType::SET_HAND_COST:
        {
            std::vector<Card>& hand = m_player.GetHand();

            if (hand.empty())
            {
                break;
            }

            if (effect.randomPick)
            {
                std::uniform_int_distribution<std::size_t> pick(
                    0, hand.size() - 1);
                hand[pick(m_rng)].SetCostThisTurn(effect.value);
                break;
            }

            for (auto& held : hand)
            {
                if (!effect.wholeBattle)
                {
                    held.SetCostThisTurn(effect.value);
                    continue;
                }

                // For the rest of the fight rather than the turn, which is a
                // cost this copy of the card carries about with it. Only the
                // cards that cost more than the floor are touched: a card
                // already at one or nought is left alone rather than being
                // put up to one.
                const int spare = held.GetCost() - effect.value;

                if (spare > 0)
                {
                    held.AddCostReduction(spare);
                }
            }
            break;
        }

        case EffectType::TAKE_FROM_DRAW_BY_TYPE:
        {
            std::vector<Card>& drawPile = m_player.GetDrawPile();
            int taken = 0;

            for (std::size_t i = drawPile.size(); i > 0 && taken < effect.value;
                 --i)
            {
                const std::size_t index = i - 1;

                if (!PassesFilter(drawPile[index], effect.filter))
                {
                    continue;
                }

                Card wanted = drawPile[index];
                drawPile.erase(drawPile.begin() +
                               static_cast<std::ptrdiff_t>(index));

                m_player.AddCardToPile(std::move(wanted), CardPile::HAND,
                                       m_rng);
                ++taken;
            }
            break;
        }

        case EffectType::INVALID:
            break;
    }
}

int Battle::ResolveValue(const CardEffect& effect, const Card& card,
                         int energySpent) const
{
    const int units = CountUnits(effect.valueSource);

    if (units >= 0)
    {
        return effect.extra * units;
    }

    switch (effect.valueSource)
    {
        case ValueSource::CURRENT_BLOCK:
            return m_player.GetBlock();

        case ValueSource::STRENGTH_MULTIPLE:
            // Strength already counts once through the damage calculation.
            return effect.value +
                   m_player.GetPower(PowerType::STRENGTH) * (effect.extra - 1);

        case ValueSource::STRIKE_COUNT:
            return effect.value + effect.extra * CountStrikeCards(card);

        case ValueSource::UNBLOCKED_DAMAGE:
            return m_unblockedDamageThisPlay;

        case ValueSource::ENERGY_SPENT:
            // The extra field scales the X, so that -1 turns it into a loss.
            return (effect.value + energySpent) *
                   (effect.extra == 0 ? 1 : effect.extra);

        case ValueSource::DISCARD_PILE_SIZE:
            return effect.value +
                   static_cast<int>(m_player.GetDiscardPile().size());

        case ValueSource::DRAW_PILE_SIZE:
            // Aggregate counts a group of cards at a time.
            return static_cast<int>(m_player.GetDrawPile().size()) /
                   (effect.extra == 0 ? 1 : effect.extra);

        case ValueSource::CARDS_EXHAUSTED:
        case ValueSource::CARDS_DISCARDED:
        case ValueSource::ATTACKS_PLAYED:
        case ValueSource::SKILLS_IN_HAND:
        case ValueSource::ORB_COUNT:
        case ValueSource::ORB_TYPES:
        case ValueSource::FROST_CHANNELED:
        case ValueSource::LIGHTNING_CHANNELED:
        case ValueSource::ENEMY_COUNT:
        case ValueSource::FIXED:
            break;
    }

    return effect.value;
}

int Battle::CountUnits(ValueSource source) const
{
    switch (source)
    {
        case ValueSource::CARDS_EXHAUSTED:
            return m_cardsExhaustedThisPlay;

        case ValueSource::CARDS_DISCARDED:
            return m_cardsDiscardedThisPlay;

        case ValueSource::ATTACKS_PLAYED:
            return m_attacksPlayedThisTurn;

        case ValueSource::SKILLS_IN_HAND:
        {
            int skills = 0;

            for (const auto& held : m_player.GetHand())
            {
                if (held.GetCardType() == CardType::SKILL)
                {
                    ++skills;
                }
            }

            return skills;
        }

        case ValueSource::ORB_COUNT:
            return static_cast<int>(m_player.GetOrbs().size());

        case ValueSource::ORB_TYPES:
        {
            bool seen[5] = { false, false, false, false, false };
            int kinds = 0;

            for (const auto& orb : m_player.GetOrbs())
            {
                const std::size_t index = static_cast<std::size_t>(orb.type);

                if (index < 5 && !seen[index])
                {
                    seen[index] = true;
                    ++kinds;
                }
            }

            return kinds;
        }

        case ValueSource::FROST_CHANNELED:
            return m_frostChanneled;

        case ValueSource::LIGHTNING_CHANNELED:
            return m_lightningChanneled;

        case ValueSource::ENEMY_COUNT:
            return static_cast<int>(GetLivingMonsterIndices().size());

        default:
            break;
    }

    // Not a per unit source.
    return -1;
}

bool Battle::ConditionHolds(const CardEffect& effect,
                            const Monster* target) const
{
    switch (effect.condition)
    {
        case EffectCondition::TARGET_VULNERABLE:
            return target != nullptr &&
                   target->GetPower(PowerType::VULNERABLE) > 0;

        case EffectCondition::TARGET_ATTACKING:
            return target != nullptr && target->GetIntent() == Intent::ATTACK;

        case EffectCondition::KILLED_TARGET:
            return m_killedTargetThisPlay;

        case EffectCondition::TARGET_POISONED:
            return target != nullptr &&
                   target->GetPower(PowerType::POISON) > 0;

        case EffectCondition::TARGET_WEAK:
            return target != nullptr && target->GetPower(PowerType::WEAK) > 0;

        case EffectCondition::DISCARDED_THIS_TURN:
            return m_cardsDiscardedThisTurn > 0;

        case EffectCondition::DREW_SKILL:
            return m_lastDrawnType == CardType::SKILL;

        case EffectCondition::PLAYER_HAS_NO_BLOCK:
            return m_player.GetBlock() == 0;

        case EffectCondition::FEW_CARDS_PLAYED:
            return m_cardsPlayedThisTurn <= effect.extra;

        case EffectCondition::NO_ATTACKS_IN_HAND:
            for (const auto& held : m_player.GetHand())
            {
                if (held.GetCardType() == CardType::ATTACK)
                {
                    return false;
                }
            }

            return true;

        case EffectCondition::NONE:
            break;
    }

    return true;
}

std::vector<Monster*> Battle::GetEffectTargets(const CardEffect& effect,
                                               const Card& card,
                                               Monster* target)
{
    EffectTarget wanted = effect.target;

    if (wanted == EffectTarget::DEFAULT)
    {
        switch (card.GetTarget())
        {
            case CardTarget::ALL_ENEMIES:
                wanted = EffectTarget::ALL_ENEMIES;
                break;

            case CardTarget::RANDOM_ENEMY:
                wanted = EffectTarget::RANDOM_ENEMY;
                break;

            case CardTarget::SINGLE_ENEMY:
                wanted = EffectTarget::SINGLE_ENEMY;
                break;

            case CardTarget::SELF:
            case CardTarget::NONE:
            case CardTarget::INVALID:
                wanted = EffectTarget::SELF;
                break;
        }
    }

    std::vector<Monster*> targets;

    switch (wanted)
    {
        case EffectTarget::SINGLE_ENEMY:
            if (target != nullptr && !target->IsDead())
            {
                targets.emplace_back(target);
            }
            break;

        case EffectTarget::ALL_ENEMIES:
            for (auto& monster : m_monsters)
            {
                if (!monster.IsDead())
                {
                    targets.emplace_back(&monster);
                }
            }
            break;

        case EffectTarget::RANDOM_ENEMY:
        {
            const std::vector<std::size_t> living = GetLivingMonsterIndices();

            if (!living.empty())
            {
                std::uniform_int_distribution<std::size_t> pick(
                    0, living.size() - 1);
                targets.emplace_back(&m_monsters[living[pick(m_rng)]]);
            }
            break;
        }

        case EffectTarget::SELF:
        case EffectTarget::DEFAULT:
            break;
    }

    return targets;
}

Monster* Battle::ResolveTarget(const Card& card, std::size_t monsterIndex)
{
    if (card.GetTarget() != CardTarget::SINGLE_ENEMY)
    {
        return nullptr;
    }

    if (monsterIndex >= m_monsters.size())
    {
        return nullptr;
    }

    return &m_monsters[monsterIndex];
}

Monster* Battle::FirstLivingMonster()
{
    for (auto& monster : m_monsters)
    {
        if (!monster.IsDead())
        {
            return &monster;
        }
    }

    return nullptr;
}

Battle::PlayTriggers Battle::ReadPlayTriggers() const
{
    PlayTriggers triggers;
    triggers.afterImage = m_player.GetPower(PowerType::AFTER_IMAGE);
    triggers.thousandCuts = m_player.GetPower(PowerType::THOUSAND_CUTS);
    triggers.storm = m_player.GetPower(PowerType::STORM);
    triggers.heatsinks = m_player.GetPower(PowerType::HEATSINKS);

    triggers.choked.reserve(m_monsters.size());

    for (const auto& monster : m_monsters)
    {
        triggers.choked.emplace_back(monster.GetPower(PowerType::CHOKED));
    }

    return triggers;
}

void Battle::OnCardPlayed(const Card& card, const PlayTriggers& before)
{
    // Rage lands after the card resolves, so a Body Slam played first does not
    // get to use the block.
    if (card.GetCardType() == CardType::ATTACK)
    {
        if (const int rage = m_player.GetPower(PowerType::RAGE); rage > 0)
        {
            GainBlock(m_player.CalculateBlockGain(rage));
        }
    }

    // Whatever the card was, some things answer every one of them.
    for (auto& monster : m_monsters)
    {
        if (monster.IsGone())
        {
            continue;
        }

        // Play too many cards in front of something that eats time and the
        // turn is over, and it is the stronger for it.
        if (const int warp = monster.GetPower(PowerType::TIME_WARP);
            warp > 0 && m_cardsPlayedThisTurn >= warp)
        {
            monster.AddPower(PowerType::STRENGTH, 2);
            m_cardsPlayedThisTurn = 0;
            m_turnCutShort = true;
        }

        if (const int beat = monster.GetPower(PowerType::BEAT_OF_DEATH);
            beat > 0)
        {
            PlayerLoseHealth(beat, false);
        }

        if (card.GetCardType() == CardType::POWER)
        {
            if (const int curiosity =
                    monster.GetPower(PowerType::CURIOSITY);
                curiosity > 0)
            {
                monster.AddPower(PowerType::STRENGTH, curiosity);
            }
        }
    }

    if (card.GetCardType() != CardType::ATTACK)
    {
        if (const int hex = m_player.GetPower(PowerType::HEX); hex > 0)
        {
            for (int i = 0; i < hex; ++i)
            {
                m_player.AddCardToPile(CardRegistry::Get(CardId::DAZED),
                                       CardPile::DRAW_SHUFFLED, m_rng);
            }
        }
    }

    if (card.GetCardType() == CardType::POWER)
    {
        ++m_powersPlayedThisBattle;

        for (int i = 0; i < before.storm; ++i)
        {
            ChannelOrb(OrbType::LIGHTNING);
        }

        if (before.heatsinks > 0)
        {
            DrawCards(before.heatsinks);
        }
    }

    if (const int panache = m_player.GetPower(PowerType::PANACHE);
        panache > 0)
    {
        ++m_panacheCounter;

        if (m_panacheCounter >= 5)
        {
            m_panacheCounter = 0;
            DamageAllEnemies(panache);
        }
    }

    if (before.afterImage > 0)
    {
        GainBlock(m_player.CalculateBlockGain(before.afterImage));
    }

    if (before.thousandCuts > 0)
    {
        DamageAllEnemies(before.thousandCuts);
    }

    // A choked monster pays for every card the player throws down.
    for (std::size_t i = 0; i < m_monsters.size(); ++i)
    {
        if (i >= before.choked.size() || before.choked[i] <= 0 ||
            m_monsters[i].IsDead())
        {
            continue;
        }

        m_monsters[i].LoseHealth(before.choked[i]);

        if (m_monsters[i].IsDead())
        {
            OnMonsterDied(m_monsters[i]);
        }
    }

    switch (card.GetCardType())
    {
        case CardType::ATTACK:
            m_playedAttackThisTurn = true;
            FireRelics(RelicHook::ATTACK_PLAYED);

            for (auto& monster : m_monsters)
            {
                if (monster.IsGone())
                {
                    continue;
                }

                // A Guardian in its shell hurts whoever swings at it.
                if (const int hide = monster.GetPower(PowerType::SHARP_HIDE);
                    hide > 0)
                {
                    PlayerLoseHealth(hide, false);
                }
            }
            break;

        case CardType::SKILL:
            m_playedSkillThisTurn = true;
            FireRelics(RelicHook::SKILL_PLAYED);

            for (auto& monster : m_monsters)
            {
                if (monster.IsGone())
                {
                    continue;
                }

                if (const int enrage = monster.GetPower(PowerType::ENRAGE);
                    enrage > 0)
                {
                    monster.AddPower(PowerType::STRENGTH, enrage);
                }
            }
            break;

        case CardType::POWER:
            m_playedPowerThisTurn = true;
            FireRelics(RelicHook::POWER_PLAYED);
            break;

        default:
            break;
    }

    FireRelics(RelicHook::CARD_PLAYED);

    // Orange Pellets clears the debuffs once all three kinds have been played.
    if (!m_pelletsSpentThisTurn && m_playedAttackThisTurn &&
        m_playedSkillThisTurn && m_playedPowerThisTurn &&
        m_player.HasRelic(RelicId::ORANGE_PELLETS))
    {
        m_pelletsSpentThisTurn = true;

        m_player.RemovePower(PowerType::VULNERABLE);
        m_player.RemovePower(PowerType::WEAK);
        m_player.RemovePower(PowerType::FRAIL);
    }

    // Unceasing Top hands over a card whenever the hand runs out.
    if (m_player.GetHand().empty() &&
        m_player.HasRelic(RelicId::UNCEASING_TOP))
    {
        DrawCards(1);
    }

    // Every Pain held in hand costs a health per card played.
    int pain = 0;

    for (const auto& held : m_player.GetHand())
    {
        if (held.GetId() == CardId::PAIN)
        {
            ++pain;
        }
    }

    PlayerLoseHealth(pain, true);
}

void Battle::OnCardDrawn(CardId id, CardType type, CardFlag flags)
{
    m_lastDrawnType = type;

    // Endless Agony makes a copy of itself every time it is drawn.
    if (id == CardId::ENDLESS_AGONY)
    {
        m_player.AddCardToPile(CardRegistry::Get(CardId::ENDLESS_AGONY),
                               CardPile::HAND, m_rng);
    }

    if (HasFlag(flags, CardFlag::LOSE_ENERGY_ON_DRAW))
    {
        m_player.UseEnergy(1);
    }

    const bool isStatus = type == CardType::STATUS;
    const bool isCurse = type == CardType::CURSE;

    if (isStatus)
    {
        if (const int evolve = m_player.GetPower(PowerType::EVOLVE); evolve > 0)
        {
            DrawCards(evolve);
        }
    }

    if (isStatus || isCurse)
    {
        if (const int breathing = m_player.GetPower(PowerType::FIRE_BREATHING);
            breathing > 0)
        {
            DamageAllEnemies(breathing);
        }
    }
}

void Battle::OnCardExhausted(Card card)
{
    ++m_cardsExhaustedThisPlay;

    // A strange spoon lets half of what should burn go back in the pile.
    if (m_player.HasRelic(RelicId::STRANGE_SPOON) &&
        card.GetId() != CardId::NECRONOMICURSE)
    {
        std::uniform_int_distribution<int> coin(1, 2);

        if (coin(m_rng) == 1)
        {
            m_player.AddCardToPile(std::move(card), CardPile::DISCARD, m_rng);

            return;
        }
    }

    if (card.Has(CardFlag::SENTINEL))
    {
        m_player.GainEnergy(card.IsUpgraded() ? 3 : 2);
    }

    if (card.GetId() == CardId::NECRONOMICURSE)
    {
        // It cannot be got rid of: it walks straight back into the hand.
        m_player.AddCardToPile(std::move(card), CardPile::HAND, m_rng);
    }
    else
    {
        m_player.GetExhaustPile().emplace_back(std::move(card));
    }

    FireRelics(RelicHook::CARD_EXHAUSTED);

    if (const int feelNoPain = m_player.GetPower(PowerType::FEEL_NO_PAIN);
        feelNoPain > 0)
    {
        GainBlock(m_player.CalculateBlockGain(feelNoPain));
    }

    if (const int darkEmbrace = m_player.GetPower(PowerType::DARK_EMBRACE);
        darkEmbrace > 0)
    {
        DrawCards(darkEmbrace);
    }
}

void Battle::DrawCards(int count)
{
    for (int i = 0; i < count; ++i)
    {
        if (m_player.GetPower(PowerType::NO_DRAW) > 0)
        {
            return;
        }

        if (!m_player.DrawOne(m_rng))
        {
            return;
        }

        if (m_player.GetShuffleCount() != m_lastShuffleCount)
        {
            m_lastShuffleCount = m_player.GetShuffleCount();
            FireRelics(RelicHook::SHUFFLED);
        }

        // Read what the card is before reacting, because reacting can draw
        // more cards and move the hand around.
        const CardId id = m_player.GetHand().back().GetId();
        const CardType type = m_player.GetHand().back().GetCardType();
        const CardFlag flags = m_player.GetHand().back().GetFlags();

        if (m_player.GetPower(PowerType::CONFUSED) > 0 &&
            m_player.GetHand().back().GetCost() >= 0)
        {
            std::uniform_int_distribution<int> roll(0, 3);
            m_player.GetHand().back().SetCostThisTurn(roll(m_rng));
        }

        OnCardDrawn(id, type, flags);
    }
}

int Battle::ThrowAwayNamed(bool exhaust)
{
    std::vector<Card>& hand = m_player.GetHand();
    std::vector<std::size_t> picked;

    for (const std::size_t at : m_choices)
    {
        if (at < hand.size() &&
            std::find(picked.begin(), picked.end(), at) == picked.end())
        {
            picked.emplace_back(at);
        }
    }

    if (picked.empty())
    {
        return 0;
    }

    std::vector<Card> going;

    for (const std::size_t at : picked)
    {
        going.emplace_back(hand[at]);
    }

    std::vector<std::size_t> backwards = picked;

    std::sort(backwards.begin(), backwards.end(),
              std::greater<std::size_t>());

    for (const std::size_t at : backwards)
    {
        hand.erase(hand.begin() + static_cast<std::ptrdiff_t>(at));
    }

    for (auto& card : going)
    {
        if (exhaust)
        {
            OnCardExhausted(std::move(card));
        }
        else
        {
            OnCardDiscarded(std::move(card));
        }
    }

    return static_cast<int>(going.size());
}

void Battle::DiscardCards(int count, bool random, std::size_t choiceIndex)
{
    for (int i = 0; i < count; ++i)
    {
        std::vector<Card>& hand = m_player.GetHand();

        if (hand.empty())
        {
            return;
        }

        std::size_t index = choiceIndex < hand.size() ? choiceIndex : 0;

        if (random)
        {
            std::uniform_int_distribution<std::size_t> pick(0,
                                                            hand.size() - 1);
            index = pick(m_rng);
        }

        Card discarded = hand[index];
        hand.erase(hand.begin() + static_cast<std::ptrdiff_t>(index));

        OnCardDiscarded(std::move(discarded));

        // A chosen index only points at one card, so the rest come off the
        // front of what is left.
        choiceIndex = 0;
    }
}

void Battle::DiscardWholeHand(CardFilter filter)
{
    std::vector<Card> kept;
    std::vector<Card> discarding;

    for (auto& held : m_player.GetHand())
    {
        if (PassesFilter(held, filter))
        {
            discarding.emplace_back(std::move(held));
        }
        else
        {
            kept.emplace_back(std::move(held));
        }
    }

    m_player.GetHand() = std::move(kept);

    for (auto& discarded : discarding)
    {
        OnCardDiscarded(std::move(discarded));
    }
}

void Battle::OnCardDiscarded(Card card)
{
    ++m_cardsDiscardedThisTurn;
    ++m_cardsDiscardedThisPlay;

    if (!m_kiteSpentThisTurn && m_player.HasRelic(RelicId::HOVERING_KITE))
    {
        m_kiteSpentThisTurn = true;
        m_player.GainEnergy(1);
    }

    const CardId id = card.GetId();
    const bool upgraded = card.IsUpgraded();

    m_player.GetDiscardPile().emplace_back(std::move(card));

    // A couple of cards only do anything when they are thrown away.
    switch (id)
    {
        case CardId::REFLEX:
            DrawCards(upgraded ? 3 : 2);
            break;

        case CardId::TACTICIAN:
            m_player.GainEnergy(upgraded ? 2 : 1);
            break;

        default:
            break;
    }
}

void Battle::OnMonsterDied(Monster& monster)
{
    // One of a linked pack goes down but not out, so long as another of them
    // is still standing.
    if (monster.GetPower(PowerType::LIFE_LINK) > 0 && !monster.IsRegrowing())
    {
        for (const auto& other : m_monsters)
        {
            if (&other != &monster && !other.IsGone() &&
                other.GetPower(PowerType::LIFE_LINK) > 0)
            {
                monster.SetRegrowing(true);
                monster.ForceMove("Reincarnate");

                return;
            }
        }
    }

    // What was only sleeping the first death off comes back whole.
    if (monster.GetMonsterId() == MonsterId::AWAKENED_ONE &&
        monster.GetPhase() == 1)
    {
        monster.SetPhase(2);
        monster.SetHealth(monster.GetMaxHealth());
        monster.RemovePower(PowerType::CURIOSITY);
        monster.RemovePower(PowerType::REGENERATION);
        monster.ForceMove("Dark Echo");

        return;
    }

    if (monster.HasStasisCard())
    {
        m_player.AddCardToPile(monster.ReleaseStasisCard(), CardPile::HAND,
                               m_rng);
    }

    FireRelics(RelicHook::ENEMY_KILLED);

    if (const int spores = monster.GetPower(PowerType::SPORE_CLOUD);
        spores > 0)
    {
        monster.RemovePower(PowerType::SPORE_CLOUD);
        ApplyPowerTo(m_player, PowerType::VULNERABLE, spores);
    }

    const int explosion = monster.GetPower(PowerType::CORPSE_EXPLOSION);

    if (explosion <= 0)
    {
        return;
    }

    // Take the power off first: the blast must not set itself off again.
    monster.RemovePower(PowerType::CORPSE_EXPLOSION);

    const int damage = monster.GetMaxHealth();

    for (auto& other : m_monsters)
    {
        if (!other.IsDead())
        {
            DealFlatDamage(other, damage);
        }
    }
}

void Battle::RetainPlannedCards()
{
    int keep = m_player.GetPower(PowerType::WELL_LAID_PLANS);

    if (m_player.GetPower(PowerType::RETAIN_HAND) > 0 ||
        m_player.HasRelic(RelicId::RUNIC_PYRAMID))
    {
        keep = static_cast<int>(m_player.GetHand().size());
    }

    if (keep <= 0)
    {
        return;
    }

    std::vector<Card>& hand = m_player.GetHand();
    const std::size_t held =
        std::min(static_cast<std::size_t>(keep), hand.size());

    // The first cards in hand are the ones kept.
    for (std::size_t i = 0; i < held; ++i)
    {
        hand[i].AddFlag(CardFlag::RETAIN);
    }
}

void Battle::PlayTopCardOfDrawPile()
{
    std::vector<Card>& drawPile = m_player.GetDrawPile();

    if (drawPile.empty() || m_playDepth >= MAX_PLAY_DEPTH)
    {
        return;
    }

    Card top = drawPile.back();
    drawPile.pop_back();

    if (top.IsPlayable())
    {
        // Keep the counters of the card that started this.
        const int exhausted = m_cardsExhaustedThisPlay;
        const int unblocked = m_unblockedDamageThisPlay;

        ++m_playDepth;
        ResolveCardEffects(top, FirstLivingMonster(), 0, 0);
        --m_playDepth;

        m_cardsExhaustedThisPlay = exhausted;
        m_unblockedDamageThisPlay = unblocked;
    }

    OnCardExhausted(std::move(top));
}

void Battle::FireRelics(RelicHook hook)
{
    // The relics are walked by index: an effect can hand over a card, and the
    // list itself never grows during a battle.
    for (std::size_t i = 0; i < m_player.GetRelics().size(); ++i)
    {
        const RelicId id = m_player.GetRelics()[i].GetId();
        const std::vector<RelicTrigger> triggers =
            m_player.GetRelics()[i].GetTriggers();

        for (const auto& trigger : triggers)
        {
            if (trigger.hook != hook)
            {
                continue;
            }

            if (trigger.onTurn > 0 && trigger.onTurn != m_turn)
            {
                continue;
            }

            if (trigger.every > 1)
            {
                bool ready = false;

                for (auto& relic : m_player.GetRelics())
                {
                    if (relic.GetId() == id)
                    {
                        ready = relic.CountUp(trigger.every);
                        break;
                    }
                }

                if (!ready)
                {
                    continue;
                }
            }

            ResolveRelicEffects(trigger.effects);
        }
    }
}

void Battle::ResolveEffectsWithoutCard(const std::vector<CardEffect>& effects,
                                      std::size_t choiceIndex,
                                      CardTarget target, CardColor color,
                                      Monster* aimedAt)
{
    // Neither a relic nor a potion has a card behind it, so they borrow a
    // stand-in that carries the target and the pool to draw from.
    Card stand(CardId::INVALID, "Trinket", color, CardType::SKILL,
               CardRarity::SPECIAL, target, 0, {});

    for (const auto& effect : effects)
    {
        ResolveEffect(effect, stand, aimedAt, choiceIndex, 0);
    }
}

void Battle::ResolveRelicEffects(const std::vector<CardEffect>& effects)
{
    // A relic aims at the player, and the cards one hands over come from the
    // character's own pool unless the effect says otherwise.
    // No answer to pass along: nothing a relic does asks the climber which
    // card it should work on.
    ResolveEffectsWithoutCard(effects, 0u, CardTarget::SELF,
                              m_player.GetColor(), FirstLivingMonster());
}

CardColor Battle::PoolColor(const CardEffect& effect, const Card& card) const
{
    return effect.colorOverride == CardColor::INVALID ? card.GetColor()
                                                      : effect.colorOverride;
}

bool Battle::CanUsePotion(std::size_t index, std::size_t monsterIndex) const
{
    if (m_phase != BattlePhase::PLAYER_TURN)
    {
        return false;
    }

    const std::vector<Potion>& potions = m_player.GetPotions();

    if (index >= potions.size() || !potions[index].IsUsableInBattle())
    {
        return false;
    }

    // A smoke bomb is no way out of a fight with a boss in it.
    if (potions[index].GetId() == PotionId::SMOKE_BOMB && IsBossFight())
    {
        return false;
    }

    // Something aimed at one monster needs that monster to be there.
    if (potions[index].GetTarget() == CardTarget::SINGLE_ENEMY &&
        (monsterIndex >= m_monsters.size() ||
         m_monsters[monsterIndex].IsDead()))
    {
        return false;
    }

    return true;
}

bool Battle::UsePotion(std::size_t index, std::size_t monsterIndex,
                       const std::vector<std::size_t>& choices)
{
    // Kept where the one step that works on more than one of them can reach
    // it, the same way a played card does.
    m_choices = choices;

    const bool drank = UsePotion(index, monsterIndex,
                                 choices.empty() ? 0u : choices.front());

    m_choices.clear();

    return drank;
}

bool Battle::UsePotion(std::size_t index, std::size_t monsterIndex,
                       std::size_t choiceIndex)
{
    if (!CanUsePotion(index, monsterIndex))
    {
        return false;
    }

    std::vector<Potion>& potions = m_player.GetPotions();
    const Potion potion = potions[index];
    Monster* target = potion.GetTarget() == CardTarget::SINGLE_ENEMY
                          ? &m_monsters[monsterIndex]
                          : FirstLivingMonster();

    const PotionId drunk = potion.GetId();

    potions.erase(potions.begin() + static_cast<std::ptrdiff_t>(index));

    if (m_player.HasRelic(RelicId::TOY_ORNITHOPTER))
    {
        m_player.Heal(5);
    }

    // A smoke bomb is a way out rather than a way through: the monsters are
    // left standing and the floor is left bare.
    if (drunk == PotionId::SMOKE_BOMB)
    {
        m_escaped = true;

        for (auto& monster : m_monsters)
        {
            if (!monster.IsGone())
            {
                monster.MarkEscaped();
            }
        }

        UpdatePhase();

        return true;
    }

    // A brew fills whatever the belt has room for. In a fight it will not
    // pour a fruit juice.
    if (drunk == PotionId::ENTROPIC_BREW)
    {
        FillPotionBelt(false);
        UpdatePhase();

        return true;
    }

    // Sacred Bark pours a double.
    const int times = m_player.HasRelic(RelicId::SACRED_BARK) &&
                              PotionRegistry::IsDoubledBySacredBark(drunk)
                          ? 2
                          : 1;

    for (int i = 0; i < times; ++i)
    {
        ResolveEffectsWithoutCard(potion.GetEffects(), choiceIndex,
                                  potion.GetTarget(), m_player.GetColor(),
                                  target);
    }

    UpdatePhase();

    return true;
}

void Battle::ChannelOrb(OrbType type)
{
    if (m_player.GetOrbSlots() <= 0)
    {
        return;
    }

    OrbType wanted = type;

    if (wanted == OrbType::INVALID)
    {
        // Chaos throws whatever comes to hand.
        const OrbType kinds[] = { OrbType::LIGHTNING, OrbType::FROST,
                                  OrbType::DARK, OrbType::PLASMA };
        std::uniform_int_distribution<std::size_t> pick(0, 3);
        wanted = kinds[pick(m_rng)];
    }

    // A full orbit sends the oldest orb off to make room.
    if (m_player.GetOrbs().size() >=
        static_cast<std::size_t>(m_player.GetOrbSlots()))
    {
        EvokeFrontOrb(1);
    }

    m_player.GetOrbs().emplace_back(Orb(wanted));

    if (wanted == OrbType::FROST)
    {
        ++m_frostChanneled;
    }
    else if (wanted == OrbType::LIGHTNING)
    {
        ++m_lightningChanneled;
    }
}

void Battle::EvokeFrontOrb(int times)
{
    std::vector<Orb>& orbs = m_player.GetOrbs();

    if (orbs.empty() || times <= 0)
    {
        return;
    }

    const Orb orb = orbs.front();
    orbs.erase(orbs.begin());

    for (int i = 0; i < times; ++i)
    {
        ApplyOrbEvoke(orb);
    }
}

void Battle::EvokeAllOrbs()
{
    while (!m_player.GetOrbs().empty())
    {
        EvokeFrontOrb(1);
    }
}

int Battle::OrbPower(OrbType type, bool evoking) const
{
    const int focus = m_player.GetPower(PowerType::FOCUS);
    int amount = 0;

    switch (type)
    {
        case OrbType::LIGHTNING:
            amount = (evoking ? 8 : 3) + focus;
            break;

        case OrbType::FROST:
            amount = (evoking ? 5 : 2) + focus;
            break;

        case OrbType::DARK:
            // A Dark orb has no passive number of its own: it stores what it
            // has built up and unloads that.
            amount = evoking ? 0 : 6 + focus;
            break;

        case OrbType::PLASMA:
            // Focus leaves Plasma alone.
            return evoking ? 2 : 1;

        case OrbType::INVALID:
            return 0;
    }

    return amount < 0 ? 0 : amount;
}

void Battle::ApplyOrbEvoke(const Orb& orb)
{
    switch (orb.type)
    {
        case OrbType::LIGHTNING:
            DealOrbDamageToTarget(OrbPower(OrbType::LIGHTNING, true));
            break;

        case OrbType::FROST:
            GainBlock(m_player.CalculateBlockGain(
                OrbPower(OrbType::FROST, true)));
            break;

        case OrbType::DARK:
            if (Monster* weakest = LowestHealthMonster(); weakest != nullptr)
            {
                DealOrbDamage(*weakest, orb.amount);
            }
            break;

        case OrbType::PLASMA:
            m_player.GainEnergy(OrbPower(OrbType::PLASMA, true));
            break;

        case OrbType::INVALID:
            break;
    }
}

void Battle::TriggerOrbPassive(Orb& orb)
{
    switch (orb.type)
    {
        case OrbType::LIGHTNING:
            DealOrbDamageToTarget(OrbPower(OrbType::LIGHTNING, false));
            break;

        case OrbType::FROST:
            GainBlock(m_player.CalculateBlockGain(
                OrbPower(OrbType::FROST, false)));
            break;

        case OrbType::DARK:
            orb.amount += OrbPower(OrbType::DARK, false);
            break;

        case OrbType::PLASMA:
        case OrbType::INVALID:
            // Plasma pays out at the start of the turn instead.
            break;
    }
}

void Battle::TriggerOrbPassives()
{
    for (auto& orb : m_player.GetOrbs())
    {
        TriggerOrbPassive(orb);
    }
}

void Battle::TriggerPlasmaPassives()
{
    for (const auto& orb : m_player.GetOrbs())
    {
        if (orb.type == OrbType::PLASMA)
        {
            m_player.GainEnergy(OrbPower(OrbType::PLASMA, false));
        }
    }
}

void Battle::DealOrbDamage(Monster& monster, int amount)
{
    if (amount <= 0 || monster.IsDead())
    {
        return;
    }

    int outgoing = amount;

    // Lock-On is the orb version of Vulnerable.
    if (monster.GetPower(PowerType::LOCK_ON) > 0)
    {
        outgoing += outgoing / 2;
    }

    outgoing = monster.CalculateDamageTaken(outgoing);

    if (monster.GetPower(PowerType::INTANGIBLE) > 0 && outgoing > 1)
    {
        outgoing = 1;
    }

    monster.TakeDamage(outgoing);

    if (monster.IsDead())
    {
        OnMonsterDied(monster);
    }
}

void Battle::DealOrbDamageToTarget(int amount)
{
    // Electro spreads what a Lightning orb does over everything.
    if (m_player.GetPower(PowerType::ELECTRO) > 0)
    {
        for (auto& monster : m_monsters)
        {
            if (!monster.IsDead())
            {
                DealOrbDamage(monster, amount);
            }
        }

        return;
    }

    const std::vector<std::size_t> living = GetLivingMonsterIndices();

    if (living.empty())
    {
        return;
    }

    std::uniform_int_distribution<std::size_t> pick(0, living.size() - 1);

    DealOrbDamage(m_monsters[living[pick(m_rng)]], amount);
}

Monster* Battle::LowestHealthMonster()
{
    Monster* weakest = nullptr;

    for (auto& monster : m_monsters)
    {
        if (monster.IsDead())
        {
            continue;
        }

        if (weakest == nullptr || monster.GetHealth() < weakest->GetHealth())
        {
            weakest = &monster;
        }
    }

    return weakest;
}

void Battle::GainBlock(int amount)
{
    if (amount <= 0 || m_player.GetPower(PowerType::NO_BLOCK) > 0)
    {
        return;
    }

    m_player.AddBlock(amount);

    if (const int juggernaut = m_player.GetPower(PowerType::JUGGERNAUT);
        juggernaut > 0)
    {
        DamageRandomEnemy(juggernaut);
    }
}

void Battle::DealDamageToMonster(Monster& monster, int base, bool fromAttack)
{
    if (monster.IsDead())
    {
        return;
    }

    int outgoing = m_player.CalculateDamageDealt(base);

    if (fromAttack && m_player.GetPower(PowerType::DOUBLE_DAMAGE) > 0)
    {
        outgoing *= 2;
    }

    if (fromAttack && outgoing > 0 && outgoing < 5 &&
        m_player.HasRelic(RelicId::THE_BOOT))
    {
        outgoing = 5;
    }

    outgoing = monster.CalculateDamageTaken(outgoing);

    // Something in the air takes half of what an attack would do.
    if (fromAttack && monster.GetPower(PowerType::FLIGHT) > 0)
    {
        outgoing /= 2;
    }

    // Something slow takes a tenth more for every card played this turn.
    if (fromAttack && monster.GetPower(PowerType::SLOW) > 0)
    {
        outgoing += outgoing * m_cardsPlayedThisTurn / 10;
    }

    // And something invincible can only be brought so far down in a turn.
    if (monster.GetPower(PowerType::INVINCIBLE) > 0)
    {
        outgoing = std::min(outgoing, monster.GetDamageCapLeft());
        monster.SetDamageCapLeft(monster.GetDamageCapLeft() - outgoing);
    }

    if (monster.GetPower(PowerType::INTANGIBLE) > 0 && outgoing > 1)
    {
        outgoing = 1;
    }

    const int blockBefore = monster.GetBlock();
    const int lost = monster.TakeDamage(outgoing);
    m_unblockedDamageThisPlay += lost;

    if (blockBefore > 0 && monster.GetBlock() == 0 &&
        m_player.HasRelic(RelicId::HAND_DRILL))
    {
        ApplyPowerTo(monster, PowerType::VULNERABLE, 2);
    }

    if (fromAttack && lost > 0)
    {
        if (const int envenom = m_player.GetPower(PowerType::ENVENOM);
            envenom > 0)
        {
            ApplyPowerTo(monster, PowerType::POISON, envenom);
        }
    }

    if (monster.IsDead())
    {
        m_killedTargetThisPlay = true;
        OnMonsterDied(monster);
    }

    if (fromAttack)
    {
        if (const int thorns = monster.GetPower(PowerType::THORNS); thorns > 0)
        {
            PlayerLoseHealth(thorns, false);
        }

        // Curl Up answers the first hit and then is spent.
        if (const int curl = monster.GetPower(PowerType::CURL_UP); curl > 0)
        {
            monster.RemovePower(PowerType::CURL_UP);
            monster.AddBlock(curl);
        }

        if (const int angry = monster.GetPower(PowerType::ANGRY); angry > 0)
        {
            monster.AddPower(PowerType::STRENGTH, angry);
        }

        // What shifts gives up as much strength as the hit took off it.
        if (lost > 0 && monster.GetPower(PowerType::SHIFTING) > 0)
        {
            monster.AddPower(PowerType::STRENGTH, -lost);
            monster.AddPower(PowerType::SHIFTING_LOSS, lost);
        }

        // Something reactive thinks again about what it was going to do.
        if (monster.GetPower(PowerType::REACTIVE) > 0 && !monster.IsGone())
        {
            monster.AdvanceMove(m_rng, ReadMoveContext(monster));
        }

        // Malleable armour answers every hit, and by more each time.
        if (const int malleable = monster.GetPower(PowerType::MALLEABLE);
            malleable > 0)
        {
            monster.AddBlock(malleable);
            monster.AddPower(PowerType::MALLEABLE, 1);
        }

        // Enough separate hits in one turn bring a flier down.
        if (monster.GetPower(PowerType::FLIGHT) > 0)
        {
            monster.AddPower(PowerType::FLIGHT, -1);

            if (monster.GetPower(PowerType::FLIGHT) == 0)
            {
                monster.ForceMove("Stunned");
            }
        }

        // Plated armour is worn away by whatever gets through it.
        if (lost > 0 && monster.GetPower(PowerType::PLATED_ARMOR) > 0)
        {
            monster.AddPower(PowerType::PLATED_ARMOR, -1);
        }

        // A sleeping monster wakes when it is hit.
        if (monster.GetPower(PowerType::ASLEEP) > 0)
        {
            monster.RemovePower(PowerType::ASLEEP);
            monster.RemovePower(PowerType::METALLICIZE);
            monster.ForceMove("Stunned");
        }
    }

    if (lost > 0 && monster.GetPower(PowerType::MODE_SHIFT) > 0)
    {
        // The Guardian only holds out for so much.
        monster.AddPower(PowerType::MODE_SHIFT, -lost);

        if (monster.GetPower(PowerType::MODE_SHIFT) == 0)
        {
            monster.ForceMove("Defensive Mode");
        }
    }

    CheckMonsterRules(monster);
}

void Battle::DealDamageToPlayer(int base, Monster& source)
{
    int outgoing = source.CalculateDamageDealt(base);
    outgoing = m_player.CalculateDamageTaken(outgoing);

    if (m_player.GetPower(PowerType::INTANGIBLE) > 0 && outgoing > 1)
    {
        outgoing = 1;
    }

    if (outgoing > 1 && outgoing <= 5 && m_player.HasRelic(RelicId::TORII))
    {
        outgoing = 1;
    }

    // Buffer eats the hit outright.
    if (outgoing > 0 && m_player.GetPower(PowerType::BUFFER) > 0)
    {
        m_player.AddPower(PowerType::BUFFER, -1);
        outgoing = 0;
    }

    const int landed = m_player.TakeDamage(outgoing);

    // A thief helps itself while it is at it.
    if (landed > 0)
    {
        if (const int thievery = source.GetPower(PowerType::THIEVERY);
            thievery > 0)
        {
            source.StealGold(thievery);
        }

        // Painful stabs leave something behind in the discard pile.
        if (const int stabs = source.GetPower(PowerType::PAINFUL_STABS);
            stabs > 0)
        {
            for (int i = 0; i < stabs; ++i)
            {
                m_player.AddCardToPile(CardRegistry::Get(CardId::WOUND),
                                       CardPile::DISCARD, m_rng);
            }
        }
    }

    if (landed > 0)
    {
        ++m_healthLossCount;

        for (int i = 0;
             i < m_player.GetPower(PowerType::STATIC_DISCHARGE); ++i)
        {
            ChannelOrb(OrbType::LIGHTNING);
        }

        FireRelics(RelicHook::HEALTH_LOST);

        if (!m_puzzleSpent && m_player.HasRelic(RelicId::CENTENNIAL_PUZZLE))
        {
            m_puzzleSpent = true;
            DrawCards(3);
        }
    }

    // Whatever hits the player gets hit back.
    const int barrier = m_player.GetPower(PowerType::FLAME_BARRIER);
    const int thorns = m_player.GetPower(PowerType::THORNS);

    if (barrier > 0)
    {
        DealFlatDamage(source, barrier);
    }

    if (thorns > 0)
    {
        DealFlatDamage(source, thorns);
    }
}

void Battle::DealFlatDamage(Creature& creature, int amount)
{
    if (amount <= 0 || creature.IsDead())
    {
        return;
    }

    int taken = amount;

    if (creature.GetPower(PowerType::INTANGIBLE) > 0 && taken > 1)
    {
        taken = 1;
    }

    creature.TakeDamage(taken);
}

void Battle::DamageAllEnemies(int amount)
{
    for (auto& monster : m_monsters)
    {
        if (!monster.IsDead())
        {
            DealDamageToMonster(monster, amount, false);
        }
    }
}

void Battle::DamageRandomEnemy(int amount)
{
    const std::vector<std::size_t> living = GetLivingMonsterIndices();

    if (living.empty())
    {
        return;
    }

    std::uniform_int_distribution<std::size_t> pick(0, living.size() - 1);

    DealDamageToMonster(m_monsters[living[pick(m_rng)]], amount, false);
}

void Battle::PlayerLoseHealth(int amount, bool fromCard)
{
    if (amount <= 0)
    {
        return;
    }

    int lost = amount;

    if (m_player.HasRelic(RelicId::TUNGSTEN_ROD))
    {
        // Tungsten Rod takes the edge off.
        --lost;

        if (lost <= 0)
        {
            return;
        }
    }

    m_player.LoseHealth(lost);
    ++m_healthLossCount;

    FireRelics(RelicHook::HEALTH_LOST);

    if (!m_puzzleSpent && m_player.HasRelic(RelicId::CENTENNIAL_PUZZLE))
    {
        m_puzzleSpent = true;
        DrawCards(3);
    }

    if (fromCard)
    {
        if (const int rupture = m_player.GetPower(PowerType::RUPTURE);
            rupture > 0)
        {
            ApplyPowerTo(m_player, PowerType::STRENGTH, rupture);
        }
    }
}

Card Battle::TakeStasisCardFrom(std::vector<Card>& pile)
{
    if (pile.empty())
    {
        return Card();
    }

    int best = -1;

    for (const Card& card : pile)
    {
        best = std::max(best, RarityRank(card.GetRarity()));
    }

    std::vector<std::size_t> choices;

    for (std::size_t i = 0; i < pile.size(); ++i)
    {
        if (RarityRank(pile[i].GetRarity()) == best)
        {
            choices.emplace_back(i);
        }
    }

    if (choices.empty())
    {
        return Card();
    }

    std::uniform_int_distribution<std::size_t> pick(0, choices.size() - 1);
    const std::size_t index = choices[pick(m_rng)];
    Card card = std::move(pile[index]);

    pile.erase(pile.begin() + static_cast<std::ptrdiff_t>(index));

    return card;
}

void Battle::PutCardInStasis(Monster& monster)
{
    if (monster.HasStasisCard())
    {
        return;
    }

    Card card = TakeStasisCardFrom(m_player.GetDrawPile());

    if (card.GetId() == CardId::INVALID)
    {
        card = TakeStasisCardFrom(m_player.GetDiscardPile());
    }

    if (card.GetId() != CardId::INVALID)
    {
        monster.HoldStasisCard(std::move(card));
    }
}

void Battle::ApplyPowerTo(Creature& creature, PowerType power, int amount)
{
    if (amount == 0 || power == PowerType::INVALID)
    {
        return;
    }

    if (&creature == &m_player)
    {
        if (power == PowerType::WEAK && m_player.HasRelic(RelicId::GINGER))
        {
            return;
        }

        if (power == PowerType::FRAIL && m_player.HasRelic(RelicId::TURNIP))
        {
            return;
        }
    }

    if (IsDebuff(power, amount) &&
        creature.GetPower(PowerType::ARTIFACT) > 0)
    {
        creature.AddPower(PowerType::ARTIFACT, -1);
        return;
    }

    creature.AddPower(power, amount);

    if (power == PowerType::VULNERABLE && amount > 0 &&
        &creature != &m_player && m_player.HasRelic(RelicId::CHAMPION_BELT))
    {
        // The belt puts a Weak on top of every Vulnerable.
        creature.AddPower(PowerType::WEAK, 1);
    }

    // Sadistic Nature answers every debuff the player lands.
    if (const int sadistic = m_player.GetPower(PowerType::SADISTIC);
        sadistic > 0 && IsDebuff(power, amount) && &creature != &m_player)
    {
        DealFlatDamage(creature, sadistic);
    }
}

void Battle::TickPoison(Creature& creature)
{
    const int poison = creature.GetPower(PowerType::POISON);

    if (poison <= 0)
    {
        return;
    }

    creature.LoseHealth(poison);
    creature.AddPower(PowerType::POISON, -1);
}

void Battle::DecayTimedPowers(Creature& creature)
{
    // Strength and Dexterity moved for a turn go back at the turn end.
    if (const int strengthDown = creature.GetPower(PowerType::STRENGTH_DOWN);
        strengthDown > 0)
    {
        creature.AddPower(PowerType::STRENGTH, -strengthDown);
        creature.RemovePower(PowerType::STRENGTH_DOWN);
    }

    if (const int strengthUp = creature.GetPower(PowerType::STRENGTH_UP);
        strengthUp > 0)
    {
        creature.AddPower(PowerType::STRENGTH, strengthUp);
        creature.RemovePower(PowerType::STRENGTH_UP);
    }

    if (const int dexterityDown = creature.GetPower(PowerType::DEXTERITY_DOWN);
        dexterityDown > 0)
    {
        creature.AddPower(PowerType::DEXTERITY, -dexterityDown);
        creature.RemovePower(PowerType::DEXTERITY_DOWN);
    }

    // Timed debuffs tick down at the end of their owner's turn, so a debuff
    // the player applies lasts through the monster's next turn.
    const PowerType decaying[] = { PowerType::VULNERABLE, PowerType::WEAK,
                                   PowerType::FRAIL };

    for (const PowerType type : decaying)
    {
        if (creature.GetPower(type) > 0)
        {
            creature.AddPower(type, -1);
        }
    }

    // A monster loses its Intangible when its own turn ends. The player's is
    // worn off at the start of the next player turn instead, so that a single
    // stack still covers the monster turn it was raised against.
    if (&creature != &m_player && creature.GetPower(PowerType::INTANGIBLE) > 0)
    {
        creature.AddPower(PowerType::INTANGIBLE, -1);
    }
}

MoveContext Battle::ReadMoveContext(const Monster& monster) const
{
    MoveContext context;

    context.turn = m_turn;
    context.phase = monster.GetPhase();

    for (const auto& other : m_monsters)
    {
        if (other.IsGone())
        {
            continue;
        }

        if (&other != &monster)
        {
            ++context.allies;
        }

        const int missing = other.GetMaxHealth() - other.GetHealth();

        context.allyMissing = std::max(context.allyMissing, missing);
    }

    // The ones called for and not yet standing count too. The cap on calling
    // for more counts them, so a monster deciding whether to call for more
    // has to count them the same way - otherwise it looks around, sees an
    // empty floor, calls again, and finds the floor full when the turn comes.
    for (const auto& spawn : m_pendingSpawns)
    {
        if (spawn.GetMonsterId() != monster.GetMonsterId())
        {
            ++context.allies;
        }
    }

    return context;
}

void Battle::ResolveMonsterMove(Monster& monster)
{
    // Copy the move: the monster picks its next one at the end of this.
    const MonsterMove move = monster.GetCurrentMove();

    for (const auto& effect : move.effects)
    {
        ResolveMonsterEffect(effect, monster);

        if (m_player.IsDead() || monster.IsGone())
        {
            break;
        }
    }

    // Ritual is what a Cultist spends its first turn setting up.
    if (const int ritual = monster.GetPower(PowerType::RITUAL); ritual > 0)
    {
        monster.AddPower(PowerType::STRENGTH, ritual);
    }

    monster.AdvanceMove(m_rng, ReadMoveContext(monster));
    CheckMonsterRules(monster);
}

void Battle::ResolveMonsterEffect(const MonsterEffect& effect,
                                  Monster& monster)
{
    switch (effect.type)
    {
        case MonsterEffectType::DAMAGE:
            for (int i = 0; i < effect.times; ++i)
            {
                DealDamageToPlayer(effect.amount, monster);

                if (m_player.IsDead() || monster.IsGone())
                {
                    return;
                }
            }
            break;

        case MonsterEffectType::DAMAGE_SCALED:
        {
            const int divisor = effect.amount <= 0 ? 1 : effect.amount;
            const int perHit = m_player.GetHealth() / divisor + 1;

            for (int i = 0; i < effect.times; ++i)
            {
                DealDamageToPlayer(perHit, monster);

                if (m_player.IsDead() || monster.IsGone())
                {
                    return;
                }
            }
            break;
        }

        case MonsterEffectType::BLOCK:
            monster.AddBlock(monster.CalculateBlockGain(effect.amount));
            break;

        case MonsterEffectType::BLOCK_ALLY:
        {
            // A Shield Gremlin covers somebody else, and only itself when it
            // is the last one standing.
            Monster* ally = nullptr;

            for (auto& other : m_monsters)
            {
                if (&other != &monster && !other.IsGone())
                {
                    ally = &other;
                    break;
                }
            }

            Monster& covered = ally == nullptr ? monster : *ally;
            covered.AddBlock(covered.CalculateBlockGain(effect.amount));
            break;
        }

        case MonsterEffectType::APPLY_POWER:
            if (effect.toPlayer)
            {
                ApplyPowerTo(m_player, effect.power, effect.amount);
            }
            else
            {
                ApplyPowerTo(monster, effect.power, effect.amount);
            }
            break;

        case MonsterEffectType::ADD_CARD:
            for (int i = 0; i < effect.amount; ++i)
            {
                m_player.AddCardToPile(
                    CardRegistry::Get(effect.cardId,
                                      effect.upgradedCard ? 1 : 0),
                    CardPile::DISCARD, m_rng);
            }
            break;

        case MonsterEffectType::BUFF_ALL:
            for (auto& other : m_monsters)
            {
                if (!other.IsGone())
                {
                    ApplyPowerTo(other, effect.power, effect.amount);
                }
            }
            break;

        case MonsterEffectType::BLOCK_ALLIES:
            for (auto& other : m_monsters)
            {
                if (&other != &monster && !other.IsGone())
                {
                    other.AddBlock(other.CalculateBlockGain(effect.amount));
                }
            }
            break;

        case MonsterEffectType::HEAL_ALL:
            for (auto& other : m_monsters)
            {
                if (!other.IsGone())
                {
                    other.Heal(effect.amount);
                }
            }
            break;

        case MonsterEffectType::SUMMON:
        {
            // A leader only shouts for so many at a time.
            int about = 0;

            for (const auto& other : m_monsters)
            {
                if (!other.IsGone() &&
                    other.GetMonsterId() == effect.summon)
                {
                    ++about;
                }
            }

            for (const auto& spawn : m_pendingSpawns)
            {
                if (spawn.GetMonsterId() == effect.summon)
                {
                    ++about;
                }
            }

            for (int i = 0; i < effect.times; ++i)
            {
                if (effect.cap > 0 && about >= effect.cap)
                {
                    break;
                }

                Monster called = MonsterRoster::Make(effect.summon, m_rng);
                called.AddPower(PowerType::MINION, 1);
                m_pendingSpawns.emplace_back(std::move(called));
                ++about;
            }

            break;
        }

        case MonsterEffectType::SPLIT:
        {
            // The two that step in carry what this one had left.
            const int health = monster.GetHealth();

            m_pendingSpawns.emplace_back(
                MonsterRoster::Make(effect.splitFirst, m_rng, health));
            m_pendingSpawns.emplace_back(
                MonsterRoster::Make(effect.splitSecond, m_rng, health));

            monster.SetHealth(0);
            break;
        }

        case MonsterEffectType::REVIVE:
            monster.SetRegrowing(false);
            monster.SetHealth(
                std::max(1, monster.GetMaxHealth() * effect.amount / 100));
            break;

        case MonsterEffectType::SELF_DESTRUCT:
            DealDamageToPlayer(effect.amount, monster);
            monster.SetHealth(0);
            OnMonsterDied(monster);
            break;

        case MonsterEffectType::ESCAPE:
            monster.MarkEscaped();
            break;

        case MonsterEffectType::STASIS:
            PutCardInStasis(monster);
            monster.SetPhase(2);
            break;

        case MonsterEffectType::NOTHING:
            break;
    }
}

void Battle::CheckMonsterRules(Monster& monster)
{
    if (monster.IsGone())
    {
        return;
    }

    // A champion who is losing stops fighting fair.
    if (monster.GetPhase() == 1 &&
        monster.GetMonsterId() == MonsterId::THE_CHAMP &&
        monster.GetHealth() * 2 <= monster.GetMaxHealth())
    {
        monster.SetPhase(2);
        monster.ForceMove("Anger");
    }

    // A parasite whose shell is gone is left standing there, once.
    if (monster.GetMonsterId() == MonsterId::SHELLED_PARASITE &&
        monster.GetPhase() == 1 &&
        monster.GetPower(PowerType::PLATED_ARMOR) == 0)
    {
        monster.SetPhase(2);
        monster.ForceMove("Stunned");
    }

    // Something that eats time makes itself whole again when it is losing,
    // and only the once.
    if (monster.GetMonsterId() == MonsterId::TIME_EATER &&
        monster.GetPhase() == 1 &&
        monster.GetHealth() * 2 <= monster.GetMaxHealth())
    {
        monster.SetPhase(2);
        monster.ForceMove("Haste");
    }

    // A slime that has been halved steps aside for two smaller ones.
    if (monster.GetHealth() * 2 <= monster.GetMaxHealth())
    {
        for (const auto& move : monster.GetMoves())
        {
            if (move.name == "Split")
            {
                monster.ForceMove("Split");
                break;
            }
        }
    }


}

void Battle::ApplyPendingSpawns()
{
    if (m_pendingSpawns.empty())
    {
        return;
    }

    for (auto& spawn : m_pendingSpawns)
    {
        spawn.ChooseOpeningMove(m_rng);
        m_monsters.emplace_back(std::move(spawn));
    }

    m_pendingSpawns.clear();
}

void Battle::ResolvePrideInHand()
{
    // What holding a Pride costs: a copy of it on top of the draw pile, one
    // for each still in hand as the turn ends. Playing it exhausts it, so a
    // Pride that was played is not in hand to be counted - which is the whole
    // of how a climber gets rid of one.
    //
    // On top, which is the back: the pile is drawn from the back, so the copy
    // comes straight back into the next hand.
    int owed = 0;

    for (const Card& held : m_player.GetHand())
    {
        owed += held.GetId() == CardId::PRIDE ? 1 : 0;
    }

    for (int i = 0; i < owed; ++i)
    {
        m_player.AddCardToPile(CardRegistry::Get(CardId::PRIDE),
                               CardPile::DRAW_TOP, m_rng);
    }
}

void Battle::ResolveEndOfTurnHandCards()
{
    // Read the hand first: a card that hurts can kill, and Doubt and Shame
    // change the powers while we walk.
    std::vector<CardId> held;
    std::vector<bool> upgraded;
    const std::size_t handSize = m_player.GetHand().size();

    for (const auto& card : m_player.GetHand())
    {
        held.emplace_back(card.GetId());
        upgraded.emplace_back(card.IsUpgraded());
    }

    for (std::size_t i = 0; i < held.size(); ++i)
    {
        switch (held[i])
        {
            case CardId::BURN:
                // Burn is soaked by block but ignores Strength and Vulnerable.
                m_player.TakeDamage(upgraded[i] ? 4 : 2);
                break;

            case CardId::DECAY:
                m_player.TakeDamage(2);
                break;

            case CardId::DOUBT:
                ApplyPowerTo(m_player, PowerType::WEAK, 1);
                break;

            case CardId::SHAME:
                ApplyPowerTo(m_player, PowerType::FRAIL, 1);
                break;

            case CardId::REGRET:
                // A curse is a card, and a Rupture answers for what it costs.
                PlayerLoseHealth(static_cast<int>(handSize), true);
                break;

            default:
                break;
        }
    }
}

void Battle::ExhaustEtherealCards()
{
    std::vector<Card> kept;
    std::vector<Card> exhausting;

    for (auto& card : m_player.GetHand())
    {
        if (card.Has(CardFlag::ETHEREAL))
        {
            exhausting.emplace_back(std::move(card));
        }
        else
        {
            kept.emplace_back(std::move(card));
        }
    }

    m_player.GetHand() = std::move(kept);

    for (auto& card : exhausting)
    {
        OnCardExhausted(std::move(card));
    }
}

void Battle::ClearTurnCosts()
{
    std::vector<Card>* piles[] = { &m_player.GetHand(),
                                   &m_player.GetDrawPile(),
                                   &m_player.GetDiscardPile(),
                                   &m_player.GetExhaustPile() };

    for (std::vector<Card>* pile : piles)
    {
        for (auto& card : *pile)
        {
            card.ClearCostThisTurn();
        }
    }
}

int Battle::CountStrikeCards(const Card& played) const
{
    int count = IsStrikeCard(played) ? 1 : 0;

    const std::vector<Card>* piles[] = { &m_player.GetHand(),
                                         &m_player.GetDrawPile(),
                                         &m_player.GetDiscardPile(),
                                         &m_player.GetExhaustPile() };

    for (const std::vector<Card>* pile : piles)
    {
        for (const auto& card : *pile)
        {
            if (IsStrikeCard(card))
            {
                ++count;
            }
        }
    }

    return count;
}
}  // namespace ConquerTheSpire
