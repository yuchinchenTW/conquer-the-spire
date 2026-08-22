// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Map/MapGenerator.hpp>
#include <conquer-the-spire/Potions/PotionRegistry.hpp>
#include <conquer-the-spire/Relics/RelicRegistry.hpp>
#include <conquer-the-spire/Run/Run.hpp>

#include <algorithm>
#include <utility>

namespace ConquerTheSpire
{
namespace
{
//! Returns the health a character opens a run with.
int StartingHealth(CardColor character)
{
    switch (character)
    {
        case CardColor::RED:
            return 80;

        case CardColor::GREEN:
            return 70;

        case CardColor::BLUE:
            return 75;

        default:
            return 75;
    }
}

//! Returns the name a character goes by.
const char* CharacterName(CardColor character)
{
    switch (character)
    {
        case CardColor::RED:
            return "Ironclad";

        case CardColor::GREEN:
            return "Silent";

        case CardColor::BLUE:
            return "Defect";

        default:
            return "Climber";
    }
}
}  // namespace

Run::Run(CardColor character, unsigned int seed)
    : m_character(character),
      m_rng(seed),
      m_player(CharacterName(character), StartingHealth(character))
{
    m_map = MapGenerator::Generate(m_rng);
    m_rewardGenerator = RewardGenerator(character, m_rng);

    m_player.SetColor(character);

    for (auto& card : CardRegistry::MakeStarterDeck(character))
    {
        m_player.AddCardToDeck(std::move(card));
    }

    const RelicId starter = RelicRegistry::GetStarterRelic(character);

    if (starter != RelicId::INVALID)
    {
        AddRelic(starter);
    }
}

const RunLog& Run::GetLog() const
{
    return m_log;
}

void Run::Note(LogEntry entry, int id, int extra)
{
    m_log.Add(entry, m_source, id, extra, m_act, m_floor,
              m_event.GetStage());
}

LogSource Run::Blame(LogSource source)
{
    const LogSource was = m_source;

    m_source = source;

    return was;
}

CardColor Run::GetCharacter() const
{
    return m_character;
}

const Map& Run::GetMap() const
{
    return m_map;
}

Player& Run::GetPlayer()
{
    return m_player;
}

const Player& Run::GetPlayer() const
{
    return m_player;
}

int Run::GetFloor() const
{
    return m_floor;
}

int Run::GetColumn() const
{
    return m_column;
}

MapNodeType Run::GetCurrentNodeType() const
{
    if (m_floor <= 0)
    {
        return MapNodeType::EMPTY;
    }

    if (m_floor > Map::ROWS)
    {
        return MapNodeType::BOSS;
    }

    return m_map.GetNode(m_floor - 1, m_column).type;
}

int Run::GetGold() const
{
    return m_gold;
}

void Run::AddGold(int amount)
{
    // Ectoplasm means the climber has no use for gold any more.
    if (amount <= 0 || m_player.HasRelic(RelicId::ECTOPLASM))
    {
        return;
    }

    m_gold += amount;
    Note(LogEntry::GOLD_EARNED, 0, amount);

    // A bloody idol pays out in health for every coin.
    if (m_player.HasRelic(RelicId::BLOODY_IDOL))
    {
        Heal(5);
    }
}

void Run::Heal(int amount)
{
    // Whoever carries the bloom cannot be healed at all.
    if (amount > 0 && !m_player.HasRelic(RelicId::MARK_OF_THE_BLOOM))
    {
        m_player.Heal(amount);
    }
}

bool Run::SpendGold(int amount)
{
    if (amount < 0 || amount > m_gold)
    {
        return false;
    }

    m_gold -= amount;
    Note(LogEntry::GOLD_SPENT, 0, amount);

    // A maw bank shuts for good once its holder spends anything.
    m_mawBankOpen = false;

    return true;
}

std::vector<int> Run::GetAvailableColumns() const
{
    if (m_bossDone)
    {
        return {};
    }

    if (m_floor <= 0)
    {
        return m_map.GetStartingColumns();
    }

    if (m_floor > m_map.GetRows())
    {
        // Nowhere left to walk.
        return {};
    }

    return m_map.GetNode(m_floor - 1, m_column).nextColumns;
}

bool Run::Travel(int column, bool ignorePaths)
{
    const std::vector<int> allowed = GetAvailableColumns();
    const bool onPath =
        std::find(allowed.begin(), allowed.end(), column) != allowed.end();

    if (!onPath)
    {
        // Wing boots step over a path that is not there, three times.
        if (!ignorePaths || m_pathSkips <= 0 || m_floor >= m_map.GetRows() ||
            !Map::IsInside(m_floor, column) || m_bossDone)
        {
            return false;
        }

        --m_pathSkips;
    }

    // What else the map led to from here, before walking spends the choice.
    // Only what a path actually reached counts, wing boots aside.
    for (const int other : allowed)
    {
        if (other != column && Map::IsInside(m_floor, other))
        {
            Note(LogEntry::PATH_PASSED,
                 static_cast<int>(m_map.GetNode(m_floor, other).type));
        }
    }

    ++m_floor;
    m_column = column;

    Note(LogEntry::FLOOR_WALKED, static_cast<int>(GetCurrentNodeType()));
    OnFloorEntered();

    return true;
}

int Run::GetPathSkips() const
{
    return m_pathSkips;
}

void Run::OnFloorEntered()
{
    // A maw bank pays out for every floor climbed, until its holder spends
    // something.
    if (m_mawBankOpen && m_player.HasRelic(RelicId::MAW_BANK))
    {
        AddGold(12);
    }

    const MapNodeType here = GetCurrentNodeType();

    if (here == MapNodeType::MERCHANT &&
        m_player.HasRelic(RelicId::MEAL_TICKET))
    {
        Heal(15);
    }

    if (here == MapNodeType::EVENT)
    {
        ++m_questionRooms;
        ResolveUnknownRoom();
    }

    if (here == MapNodeType::REST)
    {
        // A tea set leaves the next fight with something in hand, and a
        // feather pays by the size of the deck.
        if (m_player.HasRelic(RelicId::ANCIENT_TEA_SET))
        {
            m_player.SetBonusEnergy(2);
        }

        if (m_player.HasRelic(RelicId::ETERNAL_FEATHER))
        {
            Heal(static_cast<int>(m_player.GetDeck().size()) / 5 * 3);
        }
    }
}

bool Run::IsAtBoss() const
{
    return m_floor > m_map.GetRows() && !m_bossDone;
}

bool Run::IsFinished() const
{
    return m_bossDone;
}

void Run::FinishBoss()
{
    if (m_floor > m_map.GetRows())
    {
        m_bossDone = true;

        // The last act has nothing above it: whoever puts its boss down is
        // done with the spire.
        if (m_act >= 4)
        {
            Note(LogEntry::SPIRE_DONE, m_act);
        }
    }
}

Battle Run::StartBattle(std::vector<Monster> monsters)
{
    // A preserved insect leaves an elite a quarter short of itself.
    if (m_player.HasRelic(RelicId::PRESERVED_INSECT))
    {
        for (auto& monster : monsters)
        {
            if (monster.GetMonsterType() == MonsterType::ELITE)
            {
                const int less = monster.GetMaxHealth() * 3 / 4;

                monster.SetMaxHealth(less);
                monster.SetHealth(less);
            }
        }
    }

    // Written down on the way in, not on the way out: a climb that dies here
    // never reaches FinishBattle(), so this is the only place that can say
    // what it was fighting when it went.
    //
    // The monster named is the one that makes the fight the kind of fight it
    // is, rather than whichever happens to stand first in the line: the
    // Awakened One is led in behind a pair of cultists, and a table of bosses
    // with a row called Cultist in it says nothing about which boss that was.
    MonsterId leading = m_encounter.monsters.empty()
                            ? MonsterId::INVALID
                            : m_encounter.monsters.front();

    for (const auto& monster : monsters)
    {
        if (monster.GetMonsterType() == m_encounter.type)
        {
            leading = monster.GetMonsterId();
            break;
        }
    }

    if (leading != MonsterId::INVALID)
    {
        Note(LogEntry::FIGHT_STARTED, static_cast<int>(leading),
             static_cast<int>(m_encounter.type));
    }

    // The battle works on a copy: what it changes comes back through
    // FinishBattle().
    Battle battle(m_player, std::move(monsters), m_rng());
    battle.Start();

    return battle;
}

Battle Run::StartBattleHere()
{
    m_encounter = EncounterLibrary::Pick(m_act, GetCurrentNodeType(),
                                         m_fights, m_rng);

    if (GetCurrentNodeType() == MapNodeType::MONSTER ||
        GetCurrentNodeType() == MapNodeType::ELITE)
    {
        ++m_fights;
    }

    return StartBattle(EncounterLibrary::Build(m_encounter, m_rng));
}

const Encounter& Run::GetCurrentEncounter() const
{
    return m_encounter;
}

int Run::GetFightCount() const
{
    return m_fights;
}

namespace
{
//! Rolls a number between \p low and \p high, both included.
int RollBetween(std::mt19937& rng, int low, int high)
{
    std::uniform_int_distribution<int> pick(low, high);

    return pick(rng);
}

//! Returns true when a roll out of a hundred comes in under \p chance.
bool Chance(std::mt19937& rng, int chance)
{
    return RollBetween(rng, 1, 100) <= chance;
}

//! The curses no shrine and no merchant can take out of a deck.
bool IsStuckCurse(CardId id)
{
    return id == CardId::ASCENDERS_BANE || id == CardId::CURSE_OF_THE_BELL ||
           id == CardId::NECRONOMICURSE;
}

//! What the elites a dead adventurer can wake up are.
const Encounter& AdventurerElite(std::mt19937& rng)
{
    static const std::vector<Encounter> elites = {
        { "Gremlin Nob", MonsterType::ELITE, { MonsterId::GREMLIN_NOB } },
        { "Sentries",
          MonsterType::ELITE,
          { MonsterId::SENTRY, MonsterId::SENTRY, MonsterId::SENTRY } },
        { "Lagavulin", MonsterType::ELITE, { MonsterId::LAGAVULIN } }
    };

    return elites[static_cast<std::size_t>(
        RollBetween(rng, 0, static_cast<int>(elites.size()) - 1))];
}
}  // namespace

const Event& Run::StartEvent()
{
    return OpenRoom(EventLibrary::Pick(m_rewardGenerator.GetAct(), m_player,
                                       m_seenEvents, m_rng));
}

const Event& Run::StartEvent(EventId id)
{
    return OpenRoom(EventLibrary::Get(id));
}

const Event& Run::OpenRoom(Event room)
{
    m_event = std::move(room);
    Note(LogEntry::ROOM_ENTERED, static_cast<int>(m_event.GetId()));

    if (m_event.GetKind() == EventKind::ONE_TIME)
    {
        m_seenEvents.emplace_back(m_event.GetId());
    }

    // What a dead adventurer has on him is laid out before the first search.
    if (m_event.GetId() == EventId::DEAD_ADVENTURER)
    {
        m_event.GetBag() = { 0, 1, 2 };
        std::shuffle(m_event.GetBag().begin(), m_event.GetBag().end(), m_rng);
    }

    return m_event;
}

const Event& Run::StartNeow()
{
    m_event = EventLibrary::MakeNeow(m_character, m_rng);

    return m_event;
}

const Event& Run::GetEvent() const
{
    return m_event;
}

bool Run::HasEvent() const
{
    if (m_event.IsDone() || m_event.GetOptions().empty())
    {
        return false;
    }

    // Having options is not the same as having one that can be taken. A room
    // whose every answer is behind a price it cannot pay is a room it cannot
    // answer, and standing in front of it is a state with no move in it.
    for (std::size_t i = 0; i < m_event.GetOptions().size(); ++i)
    {
        if (CanChooseEventOption(i))
        {
            return true;
        }
    }

    return false;
}

bool Run::OptionCurses(const EventOption& option)
{
    for (const auto& effect : option.effects)
    {
        if (effect.type == EventEffectType::GAIN_CURSE ||
            effect.type == EventEffectType::GAIN_RANDOM_CURSE)
        {
            return true;
        }
    }

    return false;
}

bool Run::CanChooseEventOption(std::size_t index) const
{
    const std::vector<EventOption>& options = m_event.GetOptions();

    if (index >= options.size())
    {
        return false;
    }

    const EventOption& option = options[index];
    const std::vector<Card>& deck = m_player.GetDeck();

    switch (option.requirement)
    {
        case EventRequirement::GOLD:
            return m_gold >= option.requirementValue;

        case EventRequirement::HAS_CURSE:
            for (const auto& card : deck)
            {
                if (card.GetCardType() == CardType::CURSE)
                {
                    return true;
                }
            }

            return false;

        case EventRequirement::HAS_POTION:
            return !m_player.GetPotions().empty();

        case EventRequirement::HAS_RELIC:
            return m_player.HasRelic(option.requirementRelic);

        case EventRequirement::HAS_REMOVABLE_CARD:
            for (std::size_t i = 0; i < deck.size(); ++i)
            {
                if (IsRemovable(i) &&
                    deck[i].GetRarity() != CardRarity::BASIC)
                {
                    return true;
                }
            }

            return false;

        case EventRequirement::HAS_UPGRADEABLE_CARD:
            for (std::size_t i = 0; i < deck.size(); ++i)
            {
                if (IsUpgradeable(i))
                {
                    return true;
                }
            }

            return false;

        case EventRequirement::HAS_HEAVY_ATTACK:
            // The statue wants breaking, which takes a swing of ten or more
            // in one hit.
            for (const auto& card : deck)
            {
                if (card.GetCardType() != CardType::ATTACK)
                {
                    continue;
                }

                for (const auto& effect : card.GetEffects())
                {
                    if (effect.type == EffectType::DEAL_DAMAGE &&
                        effect.value >= option.requirementValue)
                    {
                        return true;
                    }
                }
            }

            return false;

        default:
            return true;
    }
}

bool Run::ChooseEventOption(std::size_t index,
                            const std::vector<std::size_t>& picks)
{
    if (!HasEvent() || !CanChooseEventOption(index))
    {
        return false;
    }

    // A copy: what the option does can close the room and throw its options
    // away.
    const EventOption option = m_event.GetOptions()[index];

    if (option.goldCost > 0 && !SpendGold(option.goldCost))
    {
        return false;
    }

    m_eventEnds = false;
    m_eventOption = index;

    const LogSource was = Blame(LogSource::ROOM);

    Note(LogEntry::ROOM_ANSWERED, static_cast<int>(m_event.GetId()),
         static_cast<int>(index));

    if (OptionCurses(option))
    {
        Note(LogEntry::CURSE_CHOSEN, static_cast<int>(m_event.GetId()),
             static_cast<int>(index));
    }

    // And what it turned down to answer that way. Only what could have been
    // taken counts: an option with a price it cannot pay was never a choice.
    for (std::size_t other = 0; other < m_event.GetOptions().size(); ++other)
    {
        if (other == index || !CanChooseEventOption(other))
        {
            continue;
        }

        Note(LogEntry::ROOM_PASSED, static_cast<int>(m_event.GetId()),
             static_cast<int>(other));

        const EventOption& turnedDown = m_event.GetOptions()[other];

        if (!OptionCurses(turnedDown))
        {
            continue;
        }

        Note(LogEntry::CURSE_REFUSED, static_cast<int>(m_event.GetId()),
             static_cast<int>(other));

        // Against the curse itself as well, so that turning one down reads
        // the way leaving a card on a reward pile does. A curse the option
        // would have drawn at random is not named here: there is no one card
        // to hang it on. The pair of lines above counts it either way.
        for (const auto& effect : turnedDown.effects)
        {
            if (effect.type == EventEffectType::GAIN_CURSE &&
                effect.card != CardId::INVALID)
            {
                Note(LogEntry::CARD_PASSED, static_cast<int>(effect.card), 0);
            }
        }
    }

    std::size_t cursor = 0;

    for (const auto& effect : option.effects)
    {
        ResolveEventEffect(effect, picks, cursor);
    }

    m_event.CountTry();
    m_event.CountOption(index);
    m_event.GoTo(m_eventEnds ? -1 : option.nextStage);
    Blame(was);

    return true;
}

bool Run::HasPendingFight() const
{
    return m_pendingFight;
}

Battle Run::StartPendingBattle()
{
    m_pendingFight = false;
    m_encounter = m_pendingEncounter;

    return StartBattle(EncounterLibrary::Build(m_encounter, m_rng));
}

void Run::ResolveEventEffect(const EventEffect& effect,
                             const std::vector<std::size_t>& picks,
                             std::size_t& cursor)
{
    std::vector<Card>& deck = m_player.GetDeck();
    const int maxHealth = m_player.GetMaxHealth();

    switch (effect.type)
    {
        case EventEffectType::GAIN_GOLD:
            AddGold(RollBetween(m_rng, effect.amount,
                                std::max(effect.amount, effect.high)));
            break;

        case EventEffectType::LOSE_GOLD:
            m_gold = std::max(
                0, m_gold - RollBetween(m_rng, effect.amount,
                                        std::max(effect.amount, effect.high)));
            break;

        case EventEffectType::LOSE_ALL_GOLD:
            m_gold = 0;
            break;

        case EventEffectType::HEAL:
            m_player.Heal(effect.amount);
            break;

        case EventEffectType::HEAL_PERCENT:
            m_player.Heal(maxHealth * effect.percent / 100);
            break;

        case EventEffectType::HEAL_FULL:
            m_player.Heal(maxHealth);
            break;

        case EventEffectType::LOSE_HEALTH:
            m_player.LoseHealth(effect.amount);
            break;

        case EventEffectType::LOSE_HEALTH_PERCENT:
            m_player.LoseHealth(maxHealth * effect.percent / 100);
            break;

        case EventEffectType::LOSE_HEALTH_PERCENT_CURRENT:
            // Neow counts in tenths of what is left.
            m_player.LoseHealth(m_player.GetHealth() / 10 *
                                (effect.percent / 10));
            break;

        case EventEffectType::GAIN_MAX_HEALTH:
            m_player.IncreaseMaxHealth(effect.amount);
            break;

        case EventEffectType::LOSE_MAX_HEALTH:
        case EventEffectType::LOSE_MAX_HEALTH_PERCENT:
        {
            const int lost = effect.percent > 0
                                 ? maxHealth * effect.percent / 100
                                 : effect.amount;

            m_player.SetMaxHealth(std::max(1, maxHealth - lost));

            if (m_player.GetHealth() > m_player.GetMaxHealth())
            {
                m_player.SetHealth(m_player.GetMaxHealth());
            }

            break;
        }

        case EventEffectType::GAIN_RELIC:
            AddRelic(effect.relic);
            break;

        case EventEffectType::GAIN_RANDOM_RELIC:
        {
            const RelicTier tier = effect.tier == RelicTier::INVALID
                                       ? m_rewardGenerator.RollTier(m_rng)
                                       : effect.tier;
            const RelicId id =
                m_rewardGenerator.TakeRelic(tier, m_player, m_rng);

            if (id != RelicId::INVALID)
            {
                AddRelic(id);
            }

            break;
        }

        case EventEffectType::BOSS_RELIC_SWAP:
        {
            const RelicId starter =
                RelicRegistry::GetStarterRelic(m_character);
            std::vector<Relic>& relics = m_player.GetRelics();

            relics.erase(std::remove_if(relics.begin(), relics.end(),
                                        [starter](const Relic& relic) {
                                            return relic.GetId() == starter;
                                        }),
                         relics.end());

            const RelicId id = m_rewardGenerator.TakeRelic(RelicTier::BOSS,
                                                           m_player, m_rng);

            if (id != RelicId::INVALID)
            {
                AddRelic(id);
            }

            break;
        }

        case EventEffectType::GAIN_CURSE:
            // A curse that only comes half the time says so with a chance.
            if (effect.percent <= 0 || Chance(m_rng, effect.percent))
            {
                AddCardToDeck(CardRegistry::Get(effect.card));
            }

            break;

        case EventEffectType::GAIN_RANDOM_CURSE:
        {
            const std::vector<CardId>& curses =
                CardRegistry::GetPool(CardColor::CURSE);

            if (!curses.empty())
            {
                AddCardToDeck(CardRegistry::Get(curses[static_cast<std::size_t>(
                    RollBetween(m_rng, 0,
                                static_cast<int>(curses.size()) - 1))]));
            }

            break;
        }

        case EventEffectType::GAIN_CARD:
            for (int i = 0; i < std::max(1, effect.count); ++i)
            {
                AddCardToDeck(CardRegistry::Get(effect.card));
            }

            break;

        case EventEffectType::GAIN_RANDOM_CARDS:
            for (int i = 0; i < effect.count; ++i)
            {
                const CardId id =
                    RollEventCard(effect.color, effect.rarity);

                if (id != CardId::INVALID)
                {
                    AddCardToDeck(CardRegistry::Get(id));
                }
            }

            break;

        case EventEffectType::CARD_REWARD:
        {
            std::vector<CardId> choices;

            for (int i = 0; i < effect.count * 8 &&
                            static_cast<int>(choices.size()) < effect.count;
                 ++i)
            {
                const CardId id = RollEventCard(effect.color, effect.rarity);

                if (id != CardId::INVALID &&
                    std::find(choices.begin(), choices.end(), id) ==
                        choices.end())
                {
                    choices.emplace_back(id);
                }
            }

            if (!choices.empty())
            {
                m_rewards.emplace_back(
                    Reward::CardChoice(std::move(choices)));
            }

            break;
        }

        case EventEffectType::GAIN_POTIONS:
            for (int i = 0; i < effect.count; ++i)
            {
                AddPotion(m_rewardGenerator.TakePotion(m_rng));
            }

            break;

        case EventEffectType::REMOVE_CARDS:
        case EventEffectType::LOSE_CARD:
            for (int i = 0; i < effect.count; ++i)
            {
                const std::size_t slot = PickDeckSlot(picks, cursor, false);

                // A room cannot take out what a shop cannot either.
                if (IsRemovable(slot))
                {
                    RemoveCardFromDeck(slot);
                }
            }

            break;

        case EventEffectType::UPGRADE_CARDS:
            for (int i = 0; i < effect.count; ++i)
            {
                const std::size_t slot = PickDeckSlot(picks, cursor, true);

                if (slot < m_player.GetDeck().size())
                {
                    Smith(slot);
                }
            }

            break;

        case EventEffectType::UPGRADE_RANDOM_CARDS:
            for (int i = 0; i < effect.count; ++i)
            {
                std::vector<std::size_t> open;

                for (std::size_t slot = 0; slot < deck.size(); ++slot)
                {
                    if (IsUpgradeable(slot))
                    {
                        open.emplace_back(slot);
                    }
                }

                if (open.empty())
                {
                    break;
                }

                Smith(open[static_cast<std::size_t>(RollBetween(
                    m_rng, 0, static_cast<int>(open.size()) - 1))]);
            }

            break;

        case EventEffectType::TRANSFORM_CARDS:
            for (int i = 0; i < effect.count; ++i)
            {
                const std::size_t slot = PickDeckSlot(picks, cursor, false);

                if (slot >= m_player.GetDeck().size())
                {
                    break;
                }

                const CardId was = m_player.GetDeck()[slot].GetId();

                RemoveCardFromDeck(slot);

                CardId now = CardId::INVALID;

                for (int tries = 0; tries < 12; ++tries)
                {
                    now = RollEventCard(m_character, CardRarity::INVALID);

                    if (now != was)
                    {
                        break;
                    }
                }

                if (now != CardId::INVALID)
                {
                    AddCardToDeck(CardRegistry::Get(now));
                }
            }

            break;

        case EventEffectType::DUPLICATE_CARD:
        {
            const std::size_t slot = PickDeckSlot(picks, cursor, false);

            if (slot < m_player.GetDeck().size())
            {
                AddCardToDeck(m_player.GetDeck()[slot]);
            }

            break;
        }

        case EventEffectType::CLEANSE_CURSES:
        {
            std::vector<Card>& cards = m_player.GetDeck();

            cards.erase(std::remove_if(cards.begin(), cards.end(),
                                       [](const Card& card) {
                                           return card.GetCardType() ==
                                                      CardType::CURSE &&
                                                  !IsStuckCurse(card.GetId());
                                       }),
                        cards.end());

            break;
        }

        case EventEffectType::LOSE_POTION:
        {
            std::vector<Potion>& potions = m_player.GetPotions();

            if (!potions.empty())
            {
                potions.erase(potions.begin());
            }

            break;
        }

        case EventEffectType::BURN_OFFERING:
        {
            const std::size_t slot = PickDeckSlot(picks, cursor, false);

            if (slot >= m_player.GetDeck().size())
            {
                break;
            }

            const Card burned = m_player.GetDeck()[slot];

            RemoveCardFromDeck(slot);

            // The spirits pay by what was worth burning.
            if (burned.GetCardType() == CardType::CURSE)
            {
                AddRelic(RelicId::SPIRIT_POOP);
            }
            else if (burned.GetRarity() == CardRarity::COMMON)
            {
                m_player.Heal(5);
            }
            else if (burned.GetRarity() == CardRarity::UNCOMMON)
            {
                m_player.Heal(m_player.GetMaxHealth());
            }
            else if (burned.GetRarity() == CardRarity::RARE)
            {
                m_player.IncreaseMaxHealth(10);
                m_player.Heal(m_player.GetMaxHealth());
            }

            break;
        }

        case EventEffectType::FIGHT:
        {
            m_pendingEncounter =
                Encounter{ m_event.GetName(), MonsterType::NORMAL,
                           effect.monsters };
            m_pendingFight = true;
            m_pendingPrize = effect.prize;

            // A prize named by kind rather than by name is drawn now, so
            // that winning hands over something the run has not seen.
            if (effect.prizeTier != RelicTier::INVALID)
            {
                m_pendingPrize = m_rewardGenerator.TakeRelic(
                    effect.prizeTier, m_player, m_rng);
            }

            // A room with more to say after the fight stays open.
            m_eventEnds = m_event.GetOptions()[m_eventOption].nextStage < 0;
            break;
        }

        case EventEffectType::FIGHT_OLD_BOSS:
        {
            const std::vector<Encounter>& bosses =
                EncounterLibrary::GetAct1Bosses();

            m_pendingEncounter =
                bosses[static_cast<std::size_t>(RollBetween(
                    m_rng, 0, static_cast<int>(bosses.size()) - 1))];
            m_pendingFight = true;
            m_pendingPrize =
                m_rewardGenerator.TakeRelic(RelicTier::RARE, m_player, m_rng);
            m_eventEnds = true;
            break;
        }

        case EventEffectType::REMOVE_RANDOM_OF_TYPE:
        {
            std::vector<std::size_t> open;
            const std::vector<Card>& deck = m_player.GetDeck();

            for (std::size_t i = 0; i < deck.size(); ++i)
            {
                // What is in a bottle is kept out of the fall.
                const std::vector<CardId>& bottled =
                    m_player.GetBottledCards();
                const bool safe =
                    std::find(bottled.begin(), bottled.end(),
                              deck[i].GetId()) != bottled.end();

                if (deck[i].GetCardType() == effect.cardType &&
                    IsRemovable(i) && !safe)
                {
                    open.emplace_back(i);
                }
            }

            if (!open.empty())
            {
                RemoveCardFromDeck(open[static_cast<std::size_t>(RollBetween(
                    m_rng, 0, static_cast<int>(open.size()) - 1))]);
            }

            break;
        }

        case EventEffectType::UPGRADE_ALL:
        case EventEffectType::UPGRADE_ALL_BASIC:
        {
            const bool basicOnly =
                effect.type == EventEffectType::UPGRADE_ALL_BASIC;

            for (std::size_t i = 0; i < m_player.GetDeck().size(); ++i)
            {
                if (!IsUpgradeable(i))
                {
                    continue;
                }

                if (basicOnly &&
                    m_player.GetDeck()[i].GetRarity() != CardRarity::BASIC)
                {
                    continue;
                }

                Smith(i);
            }

            break;
        }

        case EventEffectType::REPLACE_ALL_OF_TYPE:
        {
            std::vector<Card>& deck = m_player.GetDeck();
            const std::size_t before = deck.size();

            // Only the plain ones go: a bite is traded for a strike.
            deck.erase(std::remove_if(deck.begin(), deck.end(),
                                      [&effect](const Card& card) {
                                          return card.GetCardType() ==
                                                     effect.cardType &&
                                                 card.GetRarity() ==
                                                     CardRarity::BASIC;
                                      }),
                       deck.end());

            if (before != deck.size() || effect.count > 0)
            {
                for (int i = 0; i < effect.count; ++i)
                {
                    AddCardToDeck(CardRegistry::Get(effect.card));
                }
            }

            break;
        }

        case EventEffectType::LOSE_RELIC:
        {
            std::vector<Relic>& carried = m_player.GetRelics();

            if (effect.relic != RelicId::INVALID)
            {
                carried.erase(
                    std::remove_if(carried.begin(), carried.end(),
                                   [&effect](const Relic& relic) {
                                       return relic.GetId() == effect.relic;
                                   }),
                    carried.end());
            }
            else if (!carried.empty())
            {
                // Whatever it wants, it takes one at random.
                const std::size_t which = static_cast<std::size_t>(
                    RollBetween(m_rng, 0,
                                static_cast<int>(carried.size()) - 1));

                carried.erase(carried.begin() +
                              static_cast<std::ptrdiff_t>(which));
            }

            break;
        }

        case EventEffectType::GAIN_ONE_OF_RELICS:
            if (!effect.relics.empty())
            {
                AddRelic(effect.relics[static_cast<std::size_t>(RollBetween(
                    m_rng, 0, static_cast<int>(effect.relics.size()) - 1))]);
            }

            break;

        case EventEffectType::SKULL_TOLL:
        {
            // A tenth of the whole, never less than six, and one more for
            // every time this question has been asked.
            const int toll = std::max(6, m_player.GetMaxHealth() / 10) +
                             m_event.GetOptionTries(m_eventOption);

            m_player.LoseHealth(toll);
            break;
        }

        case EventEffectType::WAGER:
            if (SpendGold(effect.amount) && Chance(m_rng, effect.percent))
            {
                AddGold(effect.high);
            }

            break;

        case EventEffectType::TO_THE_BOSS:
            // Straight up, whatever the map said.
            m_floor = m_map.GetRows() + 1;
            m_column = -1;
            break;

        case EventEffectType::SPIN_WHEEL:
        {
            // Six faces, none likelier than another. The page does not say
            // how much gold, so it pays what a golden shrine prays for.
            switch (RollBetween(m_rng, 1, 6))
            {
                case 1:
                    m_player.LoseHealth(maxHealth * 10 / 100);
                    break;

                case 2:
                    AddGold(100);
                    break;

                case 3:
                {
                    const RelicId id = m_rewardGenerator.TakeRelic(
                        m_rewardGenerator.RollTier(m_rng), m_player, m_rng);

                    if (id != RelicId::INVALID)
                    {
                        AddRelic(id);
                    }

                    break;
                }

                case 4:
                    m_player.Heal(maxHealth);
                    break;

                case 5:
                    AddCardToDeck(CardRegistry::Get(CardId::DECAY));
                    break;

                default:
                {
                    const std::size_t slot =
                        PickDeckSlot(picks, cursor, false);

                    if (slot < m_player.GetDeck().size())
                    {
                        RemoveCardFromDeck(slot);
                    }

                    break;
                }
            }

            break;
        }

        case EventEffectType::REACH_INTO_OOZE:
        {
            // Every reach costs a point more and pays out a tenth more
            // often.
            const int tries = m_event.GetTries();

            m_player.LoseHealth(3 + tries);

            if (Chance(m_rng, 25 + tries * 10))
            {
                const RelicId id = m_rewardGenerator.TakeRelic(
                    m_rewardGenerator.RollTier(m_rng), m_player, m_rng);

                if (id != RelicId::INVALID)
                {
                    AddRelic(id);
                }

                m_eventEnds = true;
            }

            break;
        }

        case EventEffectType::SEARCH_BODY:
        {
            const int tries = m_event.GetTries();

            // A quarter of the time at first, and a quarter more with every
            // search after that.
            if (Chance(m_rng, 25 + tries * 25))
            {
                m_pendingEncounter = AdventurerElite(m_rng);
                m_pendingFight = true;
                m_pendingPrize = RelicId::INVALID;
                m_eventEnds = true;

                break;
            }

            std::vector<int>& bag = m_event.GetBag();

            if (bag.empty())
            {
                m_eventEnds = true;

                break;
            }

            const int found = bag.back();
            bag.pop_back();

            if (found == 0)
            {
                AddGold(30);
            }
            else if (found == 1)
            {
                const RelicId id = m_rewardGenerator.TakeRelic(
                    m_rewardGenerator.RollTier(m_rng), m_player, m_rng);

                if (id != RelicId::INVALID)
                {
                    AddRelic(id);
                }
            }

            if (bag.empty())
            {
                // Three searches and the body has nothing left.
                m_eventEnds = true;
            }

            break;
        }

        case EventEffectType::TRADE_FACE:
        {
            // Two in five faces are kind, two in five are not, and the rest
            // belongs to a cultist.
            const int roll = RollBetween(m_rng, 1, 100);
            RelicId id = RelicId::CULTIST_HEADPIECE;

            if (roll <= 40)
            {
                id = Chance(m_rng, 50) ? RelicId::FACE_OF_CLERIC
                                       : RelicId::SSSERPENT_HEAD;
            }
            else if (roll <= 80)
            {
                id = Chance(m_rng, 50) ? RelicId::GREMLIN_VISAGE
                                       : RelicId::NLOTHS_HUNGRY_FACE;
            }

            AddRelic(id);

            break;
        }

        default:
            break;
    }
}

std::size_t Run::PickDeckSlot(const std::vector<std::size_t>& picks,
                              std::size_t& cursor, bool upgradeable)
{
    const std::vector<Card>& deck = m_player.GetDeck();

    // What the caller named, when it may be touched at all.
    if (cursor < picks.size())
    {
        const std::size_t slot = picks[cursor];

        ++cursor;

        if (slot < deck.size() &&
            (upgradeable ? IsUpgradeable(slot) : IsRemovable(slot)))
        {
            return slot;
        }

        return deck.size();
    }

    std::vector<std::size_t> open;

    for (std::size_t slot = 0; slot < deck.size(); ++slot)
    {
        if (upgradeable ? IsUpgradeable(slot) : IsRemovable(slot))
        {
            open.emplace_back(slot);
        }
    }

    if (open.empty())
    {
        return deck.size();
    }

    return open[static_cast<std::size_t>(
        RollBetween(m_rng, 0, static_cast<int>(open.size()) - 1))];
}

CardId Run::RollEventCard(CardColor color, CardRarity rarity)
{
    if (color == CardColor::INVALID)
    {
        color = m_character;
    }

    CardRarity wanted = rarity;

    if (wanted == CardRarity::INVALID)
    {
        // The weights a plain fight offers: mostly common, rarely rare.
        const int roll = RollBetween(m_rng, 1, 100);

        wanted = roll <= 60 ? CardRarity::COMMON
                            : (roll <= 97 ? CardRarity::UNCOMMON
                                          : CardRarity::RARE);
    }

    std::vector<CardId> pool = CardRegistry::GetPool(color, wanted);

    if (pool.empty())
    {
        pool = CardRegistry::GetPool(color, CardRarity::COMMON);
    }

    if (pool.empty())
    {
        return CardId::INVALID;
    }

    return pool[static_cast<std::size_t>(
        RollBetween(m_rng, 0, static_cast<int>(pool.size()) - 1))];
}

bool Run::IsRemovable(std::size_t index) const
{
    const std::vector<Card>& deck = m_player.GetDeck();

    return index < deck.size() && !IsStuckCurse(deck[index].GetId());
}

bool Run::IsUpgradeable(std::size_t index) const
{
    const std::vector<Card>& deck = m_player.GetDeck();

    if (index >= deck.size())
    {
        return false;
    }

    const Card& card = deck[index];

    if (card.GetCardType() == CardType::CURSE ||
        card.GetCardType() == CardType::STATUS)
    {
        return false;
    }

    // Only a Searing Blow takes more than one turn on the whetstone.
    return !card.IsUpgraded() || card.GetId() == CardId::SEARING_BLOW;
}

const Shop& Run::OpenShop()
{
    // Whoever never walked out of the last one walks out of it now.
    CloseShop();

    m_shop = Shop(m_character, m_player, m_rewardGenerator, m_removalPrice,
                  m_rng);
    m_shopOpen = true;

    return m_shop;
}

void Run::CloseShop()
{
    if (!m_shopOpen)
    {
        return;
    }

    m_shopOpen = false;

    const LogSource was = Blame(LogSource::SHOP);

    for (const auto& slot : m_shop.GetCards())
    {
        if (!slot.sold && slot.id != CardId::INVALID)
        {
            Note(LogEntry::CARD_PASSED, static_cast<int>(slot.id));
        }
    }

    for (const auto& slot : m_shop.GetRelics())
    {
        if (!slot.sold && slot.id != RelicId::INVALID)
        {
            Note(LogEntry::RELIC_PASSED, static_cast<int>(slot.id));
        }
    }

    for (const auto& slot : m_shop.GetPotions())
    {
        if (!slot.sold && slot.id != PotionId::INVALID)
        {
            Note(LogEntry::POTION_PASSED, static_cast<int>(slot.id));
        }
    }

    Blame(was);
}

const Shop& Run::GetShop() const
{
    return m_shop;
}

bool Run::BuyCard(std::size_t index)
{
    const std::vector<ShopCard>& cards = m_shop.GetCards();

    if (index >= cards.size() || cards[index].sold)
    {
        return false;
    }

    if (!SpendGold(cards[index].price))
    {
        return false;
    }

    const LogSource was = Blame(LogSource::SHOP);

    AddCardToDeck(CardRegistry::Get(cards[index].id));
    Blame(was);

    return m_shop.TakeCard(index, m_player, m_rng);
}

bool Run::BuyRelic(std::size_t index)
{
    const std::vector<ShopRelic>& relics = m_shop.GetRelics();

    if (index >= relics.size() || relics[index].sold)
    {
        return false;
    }

    if (!SpendGold(relics[index].price))
    {
        return false;
    }

    const LogSource was = Blame(LogSource::SHOP);

    AddRelic(relics[index].id);
    Blame(was);

    return m_shop.TakeRelic(index, m_player, m_rewardGenerator, m_rng);
}

bool Run::BuyPotion(std::size_t index)
{
    const std::vector<ShopPotion>& potions = m_shop.GetPotions();

    if (index >= potions.size() || potions[index].sold)
    {
        return false;
    }

    // A full belt leaves it on the shelf, and the gold in the purse.
    const LogSource was = Blame(LogSource::SHOP);
    const bool bought =
        m_gold >= potions[index].price && AddPotion(potions[index].id);

    Blame(was);

    if (!bought)
    {
        return false;
    }

    SpendGold(potions[index].price);

    return m_shop.TakePotion(index, m_player, m_rewardGenerator, m_rng);
}

bool Run::BuyCardRemoval(std::size_t deckIndex)
{
    // Some curses cannot be got rid of at any price, and a shelf is a price.
    if (m_shop.IsRemovalSpent() || !IsRemovable(deckIndex))
    {
        return false;
    }

    if (!SpendGold(m_shop.GetRemovalPrice()))
    {
        return false;
    }

    RemoveCardFromDeck(deckIndex);
    m_shop.SpendRemoval();
    m_removalPrice += Shop::REMOVAL_PRICE_STEP;

    return true;
}

int Run::GetCardRemovalPrice() const
{
    return m_removalPrice;
}

void Run::BeginAct(int act)
{
    m_act = act;
    Note(LogEntry::ACT_STARTED, act);
    m_rewardGenerator.BeginAct(act);
    m_map = act >= 4 ? MapGenerator::GenerateFinalAct()
                     : MapGenerator::Generate(m_rng);
    m_floor = 0;
    m_column = -1;
    m_fights = 0;
    m_bossDone = false;
    m_rewards.clear();

    // The odds of a question mark start each act over.
    m_monsterChance = MONSTER_CHANCE;
    m_shopChance = SHOP_CHANCE;
    m_treasureChance = TREASURE_CHANCE;
}

int Run::GetAct() const
{
    return m_act;
}

Run::UnknownOdds Run::GetUnknownOdds() const
{
    UnknownOdds odds;

    odds.monster = m_monsterChance;
    odds.shop = m_shopChance;
    odds.treasure = m_treasureChance;

    return odds;
}

void Run::ResolveUnknownRoom()
{
    const int row = m_floor - 1;

    if (!Map::IsInside(row, m_column))
    {
        return;
    }

    // A tiny chest turns every fourth question mark into a chest, whatever
    // the odds say.
    if (m_player.HasRelic(RelicId::TINY_CHEST) && m_questionRooms % 4 == 0)
    {
        m_map.SetType(row, m_column, MapNodeType::TREASURE);
        m_treasureChance = TREASURE_CHANCE;

        return;
    }

    const int roll = RollBetween(m_rng, 1, 100);

    // A juzu bracelet keeps the fights out of the question marks, and their
    // share of the odds keeps climbing unseen.
    const bool fights = !m_player.HasRelic(RelicId::JUZU_BRACELET);

    if (fights && roll <= m_monsterChance)
    {
        m_map.SetType(row, m_column, MapNodeType::MONSTER);
        m_monsterChance = MONSTER_CHANCE;
        m_shopChance += SHOP_CHANCE;
        m_treasureChance += TREASURE_CHANCE;

        return;
    }

    if (roll <= m_monsterChance + m_shopChance)
    {
        m_map.SetType(row, m_column, MapNodeType::MERCHANT);
        m_shopChance = SHOP_CHANCE;
        m_monsterChance += MONSTER_CHANCE;
        m_treasureChance += TREASURE_CHANCE;

        // The merchant is worth a meal ticket to whoever carries one.
        if (m_player.HasRelic(RelicId::MEAL_TICKET))
        {
            Heal(15);
        }

        return;
    }

    if (roll <= m_monsterChance + m_shopChance + m_treasureChance)
    {
        m_map.SetType(row, m_column, MapNodeType::TREASURE);
        m_treasureChance = TREASURE_CHANCE;
        m_monsterChance += MONSTER_CHANCE;
        m_shopChance += SHOP_CHANCE;

        return;
    }

    // A room with something to decide in it, and everything else a little
    // likelier next time.
    m_monsterChance += MONSTER_CHANCE;
    m_shopChance += SHOP_CHANCE;
    m_treasureChance += TREASURE_CHANCE;
}

bool Run::AdvanceAct()
{
    if (!m_bossDone)
    {
        return false;
    }

    // The last act is behind a door with three locks.
    if (m_act >= 3 && !HasAllKeys())
    {
        return false;
    }

    BeginAct(m_act + 1);

    return true;
}

bool Run::TakeKeyInsteadOf(std::size_t index, KeyType key)
{
    if (index >= m_rewards.size())
    {
        return false;
    }

    Reward& reward = m_rewards[index];

    if (reward.claimed || reward.kind != RewardKind::RELIC_CHOICE)
    {
        return false;
    }

    reward.claimed = true;
    TakeKey(key);

    return true;
}

void Run::RecallKey()
{
    TakeKey(KeyType::RUBY);
}

void Run::TakeKey(KeyType key)
{
    if (key != KeyType::INVALID && !HasKey(key))
    {
        m_keys.emplace_back(key);
    }
}

bool Run::HasKey(KeyType key) const
{
    return std::find(m_keys.begin(), m_keys.end(), key) != m_keys.end();
}

bool Run::HasAllKeys() const
{
    return HasKey(KeyType::RUBY) && HasKey(KeyType::EMERALD) &&
           HasKey(KeyType::SAPPHIRE);
}

ChestSize Run::OpenChest()
{
    const ChestSize size = RewardGenerator::RollChestSize(m_rng);

    const LogSource was = Blame(LogSource::CHEST);

    m_rewards = m_rewardGenerator.ForChest(size, m_player, m_rng);
    Blame(was);

    for (auto& reward : m_rewards)
    {
        reward.fromChest = true;
    }

    return size;
}

const std::vector<Reward>& Run::GetRewards() const
{
    return m_rewards;
}

bool Run::HasUnclaimedRewards() const
{
    for (const auto& reward : m_rewards)
    {
        if (!reward.claimed)
        {
            return true;
        }
    }

    return false;
}

bool Run::ClaimReward(std::size_t index, std::size_t option)
{
    if (index >= m_rewards.size())
    {
        return false;
    }

    Reward& reward = m_rewards[index];

    if (reward.claimed)
    {
        return false;
    }

    const LogSource was =
        Blame(reward.fromChest ? LogSource::CHEST : LogSource::REWARD);
    const bool paid = ClaimRewardNow(reward, option);

    Blame(was);

    // The rest of what it was offering was left where it was.
    if (paid)
    {
        NotePassedOver(reward, option);
    }

    return paid;
}

bool Run::ClaimRewardNow(Reward& reward, std::size_t option)
{
    switch (reward.kind)
    {
        case RewardKind::GOLD:
            AddGold(reward.amount);
            break;

        case RewardKind::CARD_CHOICE:
            if (option >= reward.cards.size())
            {
                return false;
            }

            AddCardToDeck(CardRegistry::Get(reward.cards[option]));
            break;

        case RewardKind::RELIC_CHOICE:
            if (option >= reward.relics.size())
            {
                return false;
            }

            AddRelic(reward.relics[option]);
            break;

        case RewardKind::POTION:
            if (!AddPotion(reward.potion))
            {
                // A full belt leaves it on the pile.
                return false;
            }
            break;

        case RewardKind::MAX_HEALTH:
            m_player.IncreaseMaxHealth(reward.amount);
            break;

        case RewardKind::CURSE:
            if (reward.cards.empty())
            {
                return false;
            }

            AddCardToDeck(CardRegistry::Get(reward.cards.front()));
            break;

        case RewardKind::INVALID:
            return false;
    }

    reward.claimed = true;

    return true;
}

bool Run::SkipReward(std::size_t index)
{
    if (index >= m_rewards.size())
    {
        return false;
    }

    Reward& reward = m_rewards[index];

    if (reward.claimed)
    {
        return false;
    }

    // A Singing Bowl turns a card down into a little more health.
    if (reward.kind == RewardKind::CARD_CHOICE &&
        m_player.HasRelic(RelicId::SINGING_BOWL))
    {
        m_player.IncreaseMaxHealth(2);
    }

    // Everything it was offering was left where it was.
    NotePassedOver(reward, reward.cards.size() + reward.relics.size() + 1u);

    reward.claimed = true;

    return true;
}

void Run::ClearRewards()
{
    // Whatever is left on the floor was looked at and left.
    for (const auto& reward : m_rewards)
    {
        if (!reward.claimed)
        {
            NotePassedOver(reward, reward.cards.size() +
                                       reward.relics.size() + 1u);
        }
    }

    m_rewards.clear();
}

void Run::NotePassedOver(const Reward& reward, std::size_t taken)
{
    const LogSource was =
        Blame(reward.fromChest ? LogSource::CHEST : LogSource::REWARD);

    for (std::size_t i = 0; i < reward.cards.size(); ++i)
    {
        if (i != taken)
        {
            Note(LogEntry::CARD_PASSED,
                 static_cast<int>(reward.cards[i]));
        }
    }

    for (std::size_t i = 0; i < reward.relics.size(); ++i)
    {
        if (i != taken)
        {
            Note(LogEntry::RELIC_PASSED,
                 static_cast<int>(reward.relics[i]));
        }
    }

    if (reward.potion != PotionId::INVALID && taken != 0u)
    {
        Note(LogEntry::POTION_PASSED, static_cast<int>(reward.potion));
    }

    Blame(was);
}

const RewardGenerator& Run::GetRewardGenerator() const
{
    return m_rewardGenerator;
}

void Run::FinishBattle(const Battle& battle)
{
    const Player& fought = battle.GetPlayer();

    m_player.SetHealth(fought.GetHealth());
    m_player.SetMaxHealth(fought.GetMaxHealth());
    m_player.GetPotions() = fought.GetPotions();

    if (m_player.GetHealth() <= 0)
    {
        Note(LogEntry::DIED, static_cast<int>(m_encounter.type));

        return;
    }

    // Whether the fight left the player under half is read before any of the
    // healing lands, so the order the relics were picked up in does not
    // change the outcome.
    const bool endedHurt =
        m_player.GetHealth() * 2 <= m_player.GetMaxHealth();

    // The relics that patch the player up once the fight is over.
    if (m_player.HasRelic(RelicId::BURNING_BLOOD))
    {
        m_player.Heal(6);
    }

    if (m_player.HasRelic(RelicId::BLACK_BLOOD))
    {
        m_player.Heal(12);
    }

    if (endedHurt && m_player.HasRelic(RelicId::MEAT_ON_THE_BONE))
    {
        m_player.Heal(12);
    }

    // What the thieves of the fight got away with is gone from the purse.
    if (const int stolen = battle.GetGoldStolen(); stolen > 0)
    {
        m_gold = std::max(0, m_gold - stolen);
    }

    // A fight that was won leaves something on the floor, and one walked
    // out of leaves nothing.
    if (battle.GetPhase() == BattlePhase::WON && !battle.WasEscaped())
    {
        Note(LogEntry::FIGHT_WON, static_cast<int>(m_encounter.type));

        if (m_pendingPrize != RelicId::INVALID)
        {
            AddRelic(m_pendingPrize);
            m_pendingPrize = RelicId::INVALID;
        }

            const LogSource was = Blame(LogSource::FIGHT);

        m_rewards = m_rewardGenerator.ForCombat(m_encounter.type, m_player,
                                                m_rng);
        Blame(was);
    }
}

bool Run::Rest()
{
    // A coffee dripper is no place to sleep.
    if (m_player.HasRelic(RelicId::COFFEE_DRIPPER))
    {
        return false;
    }

    int healed = m_player.GetMaxHealth() * REST_HEAL_PERCENT / 100;

    // A regal pillow is worth another fifteen.
    if (m_player.HasRelic(RelicId::REGAL_PILLOW))
    {
        healed += 15;
    }

    Heal(healed);
    Note(LogEntry::RESTED, healed);

    // A dream catcher leaves a pick on the pile for whoever slept.
    if (m_player.HasRelic(RelicId::DREAM_CATCHER))
    {
        std::vector<CardId> choices;

        for (int attempt = 0; attempt < 24 && choices.size() < 3u; ++attempt)
        {
            const CardId rolled =
                RollEventCard(m_character, CardRarity::INVALID);

            if (rolled != CardId::INVALID &&
                std::find(choices.begin(), choices.end(), rolled) ==
                    choices.end())
            {
                choices.emplace_back(rolled);
            }
        }

        if (!choices.empty())
        {
            m_rewards.emplace_back(Reward::CardChoice(std::move(choices)));
        }
    }

    return true;
}

bool Run::Toke(std::size_t index)
{
    if (!m_player.HasRelic(RelicId::PEACE_PIPE) || !IsRemovable(index))
    {
        return false;
    }

    return RemoveCardFromDeck(index);
}

RelicId Run::Dig()
{
    if (!m_player.HasRelic(RelicId::SHOVEL))
    {
        return RelicId::INVALID;
    }

    const RelicId found = m_rewardGenerator.TakeRelic(
        m_rewardGenerator.RollTier(m_rng), m_player, m_rng);

    if (found != RelicId::INVALID)
    {
        AddRelic(found);
    }

    return found;
}

bool Run::Lift()
{
    if (!m_player.HasRelic(RelicId::GIRYA) || m_lifts >= 3)
    {
        return false;
    }

    ++m_lifts;
    m_player.Lift(1);

    return true;
}

int Run::GetLifts() const
{
    return m_lifts;
}

bool Run::Smith(std::size_t index)
{
    std::vector<Card>& deck = m_player.GetDeck();

    // A fusion hammer leaves no hand free for the whetstone, and a card the
    // whetstone would leave as it was is not worth a fire. The rule for that
    // is IsUpgradeable, which is what the moves on offer are drawn from: the
    // two have to agree, or a fire is spent on nothing.
    if (m_player.HasRelic(RelicId::FUSION_HAMMER) || !IsUpgradeable(index))
    {
        return false;
    }

    const Card& card = deck[index];
    const CardId sharpened = card.GetId();

    deck[index] = CardRegistry::Get(card.GetId(), card.GetUpgradeCount() + 1);
    Note(LogEntry::CARD_UPGRADED, static_cast<int>(sharpened));

    return true;
}

const std::vector<Card>& Run::GetDeck() const
{
    return m_player.GetDeck();
}

void Run::AddCardToDeck(Card card)
{
    const CardType type = card.GetCardType();

    // An omamori turns the next couple of curses away at the door.
    if (type == CardType::CURSE && m_curseWards > 0)
    {
        --m_curseWards;

        return;
    }

    // The eggs sharpen whatever of their kind comes in.
    const bool egg =
        (type == CardType::ATTACK && m_player.HasRelic(RelicId::MOLTEN_EGG)) ||
        (type == CardType::SKILL && m_player.HasRelic(RelicId::TOXIC_EGG)) ||
        (type == CardType::POWER && m_player.HasRelic(RelicId::FROZEN_EGG));

    if (egg && !card.IsUpgraded())
    {
        card = CardRegistry::Get(card.GetId(), card.GetUpgradeCount() + 1);
    }

    const CardId taken = card.GetId();

    m_player.AddCardToDeck(std::move(card));
    Note(LogEntry::CARD_TAKEN, static_cast<int>(taken));

    // A ceramic fish pays for every card that comes in.
    if (m_player.HasRelic(RelicId::CERAMIC_FISH))
    {
        AddGold(9);
    }

    // A darkstone periapt grows on what a curse costs.
    if (type == CardType::CURSE &&
        m_player.HasRelic(RelicId::DARKSTONE_PERIAPT))
    {
        m_player.IncreaseMaxHealth(6);
    }
}

bool Run::RemoveCardFromDeck(std::size_t index)
{
    if (index < m_player.GetDeck().size())
    {
        Note(LogEntry::CARD_REMOVED,
             static_cast<int>(m_player.GetDeck()[index].GetId()));
    }

    std::vector<Card>& deck = m_player.GetDeck();

    if (index >= deck.size())
    {
        return false;
    }

    deck.erase(deck.begin() + static_cast<std::ptrdiff_t>(index));

    return true;
}

void Run::AddRelic(RelicId id)
{
    const int bonus = RelicRegistry::BonusMaxHealth(id);

    m_player.AddRelic(RelicRegistry::Get(id));
    Note(LogEntry::RELIC_TAKEN, static_cast<int>(id));

    if (bonus > 0)
    {
        m_player.IncreaseMaxHealth(bonus);
    }

    const LogSource was = Blame(LogSource::RELIC);

    OnRelicTaken(id);
    Blame(was);
}

void Run::OnRelicTaken(RelicId id)
{
    switch (id)
    {
        case RelicId::WHETSTONE:
            UpgradeRandomOfType(CardType::ATTACK, 2);
            break;

        case RelicId::WAR_PAINT:
            UpgradeRandomOfType(CardType::SKILL, 2);
            break;

        case RelicId::ASTROLABE:
            // Three cards turned into something else, and sharpened.
            for (int i = 0; i < 3; ++i)
            {
                const std::size_t slot = RandomDeckSlot();

                if (slot >= m_player.GetDeck().size())
                {
                    break;
                }

                TransformAt(slot, true);
            }

            break;

        case RelicId::DOLLYS_MIRROR:
        {
            const std::size_t slot = RandomDeckSlot();

            if (slot < m_player.GetDeck().size())
            {
                AddCardToDeck(m_player.GetDeck()[slot]);
            }

            break;
        }

        case RelicId::EMPTY_CAGE:
            for (int i = 0; i < 2; ++i)
            {
                const std::size_t slot = RandomDeckSlot();

                if (slot < m_player.GetDeck().size())
                {
                    RemoveCardFromDeck(slot);
                }
            }

            break;

        case RelicId::PANDORAS_BOX:
        {
            // Every strike and every defend becomes something else.
            std::vector<std::size_t> slots;
            const std::vector<Card>& deck = m_player.GetDeck();

            for (std::size_t i = 0; i < deck.size(); ++i)
            {
                if (deck[i].GetRarity() == CardRarity::BASIC &&
                    deck[i].GetCardType() != CardType::CURSE)
                {
                    slots.emplace_back(i);
                }
            }

            // Backwards, so that removing one does not move the next.
            for (std::size_t i = slots.size(); i > 0; --i)
            {
                TransformAt(slots[i - 1], false);
            }

            break;
        }

        case RelicId::CAULDRON:
            for (int i = 0; i < 5; ++i)
            {
                AddPotion(m_rewardGenerator.TakePotion(m_rng));
            }

            break;

        case RelicId::TINY_HOUSE:
        {
            AddPotion(m_rewardGenerator.TakePotion(m_rng));
            AddGold(50);
            m_player.IncreaseMaxHealth(5);

            const CardId gift = RollEventCard(m_character, CardRarity::INVALID);

            if (gift != CardId::INVALID)
            {
                AddCardToDeck(CardRegistry::Get(gift));
            }

            UpgradeRandomOfType(CardType::INVALID, 1);
            break;
        }

        case RelicId::OLD_COIN:
            AddGold(300);
            break;

        case RelicId::LEES_WAFFLE:
            m_player.IncreaseMaxHealth(7);
            Heal(m_player.GetMaxHealth());
            break;

        case RelicId::CALLING_BELL:
        {
            AddCardToDeck(CardRegistry::Get(CardId::CURSE_OF_THE_BELL));

            const RelicTier tiers[] = { RelicTier::COMMON,
                                        RelicTier::UNCOMMON,
                                        RelicTier::RARE };

            for (const RelicTier tier : tiers)
            {
                const RelicId found =
                    m_rewardGenerator.TakeRelic(tier, m_player, m_rng);

                if (found != RelicId::INVALID)
                {
                    AddRelic(found);
                }
            }

            break;
        }

        case RelicId::ORRERY:
            // Five picks left on the pile, one after another.
            for (int i = 0; i < 5; ++i)
            {
                std::vector<CardId> choices;

                for (int attempt = 0; attempt < 24 && choices.size() < 3u;
                     ++attempt)
                {
                    const CardId rolled =
                        RollEventCard(m_character, CardRarity::INVALID);

                    if (rolled != CardId::INVALID &&
                        std::find(choices.begin(), choices.end(), rolled) ==
                            choices.end())
                    {
                        choices.emplace_back(rolled);
                    }
                }

                if (!choices.empty())
                {
                    m_rewards.emplace_back(
                        Reward::CardChoice(std::move(choices)));
                }
            }

            break;

        case RelicId::POTION_BELT:
            // The belt itself is what holds the extra slots.
            break;

        case RelicId::OMAMORI:
            m_curseWards += 2;
            break;

        case RelicId::WING_BOOTS:
            m_pathSkips += 3;
            break;

        case RelicId::NECRONOMICON:
            AddCardToDeck(CardRegistry::Get(CardId::NECRONOMICURSE));
            break;

        case RelicId::BOTTLED_FLAME:
            BottleFirstOfType(CardType::ATTACK);
            break;

        case RelicId::BOTTLED_LIGHTNING:
            BottleFirstOfType(CardType::SKILL);
            break;

        case RelicId::BOTTLED_TORNADO:
            BottleFirstOfType(CardType::POWER);
            break;

        default:
            break;
    }
}

std::size_t Run::RandomDeckSlot()
{
    std::vector<std::size_t> open;
    const std::vector<Card>& deck = m_player.GetDeck();

    for (std::size_t i = 0; i < deck.size(); ++i)
    {
        if (IsRemovable(i))
        {
            open.emplace_back(i);
        }
    }

    if (open.empty())
    {
        return deck.size();
    }

    std::uniform_int_distribution<std::size_t> pick(0, open.size() - 1);

    return open[pick(m_rng)];
}

void Run::TransformAt(std::size_t index, bool upgrade)
{
    if (index >= m_player.GetDeck().size())
    {
        return;
    }

    const CardId was = m_player.GetDeck()[index].GetId();

    RemoveCardFromDeck(index);

    CardId now = CardId::INVALID;

    for (int tries = 0; tries < 12; ++tries)
    {
        now = RollEventCard(m_character, CardRarity::INVALID);

        if (now != was)
        {
            break;
        }
    }

    if (now == CardId::INVALID)
    {
        return;
    }

    AddCardToDeck(CardRegistry::Get(now, upgrade ? 1 : 0));
}

void Run::UpgradeRandomOfType(CardType type, int count)
{
    for (int i = 0; i < count; ++i)
    {
        std::vector<std::size_t> open;
        const std::vector<Card>& deck = m_player.GetDeck();

        for (std::size_t slot = 0; slot < deck.size(); ++slot)
        {
            if (IsUpgradeable(slot) &&
                (type == CardType::INVALID ||
                 deck[slot].GetCardType() == type))
            {
                open.emplace_back(slot);
            }
        }

        if (open.empty())
        {
            return;
        }

        std::uniform_int_distribution<std::size_t> pick(0, open.size() - 1);

        Smith(open[pick(m_rng)]);
    }
}

void Run::BottleFirstOfType(CardType type)
{
    const std::vector<Card>& deck = m_player.GetDeck();

    for (std::size_t i = 0; i < deck.size(); ++i)
    {
        if (deck[i].GetCardType() == type)
        {
            m_player.BottleCard(deck[i].GetId());

            return;
        }
    }
}

bool Run::BottleCard(std::size_t index)
{
    const std::vector<Card>& deck = m_player.GetDeck();

    if (index >= deck.size())
    {
        return false;
    }

    const CardType type = deck[index].GetCardType();
    const bool holds =
        (type == CardType::ATTACK &&
         m_player.HasRelic(RelicId::BOTTLED_FLAME)) ||
        (type == CardType::SKILL &&
         m_player.HasRelic(RelicId::BOTTLED_LIGHTNING)) ||
        (type == CardType::POWER &&
         m_player.HasRelic(RelicId::BOTTLED_TORNADO));

    if (!holds)
    {
        return false;
    }

    m_player.BottleCard(deck[index].GetId());

    return true;
}

bool Run::CanAddPotion() const
{
    return !m_player.HasRelic(RelicId::SOZU) &&
           static_cast<int>(m_player.GetPotions().size()) <
               m_player.GetPotionSlots();
}

bool Run::AddPotion(PotionId id)
{
    if (!m_player.AddPotion(PotionRegistry::Get(id)))
    {
        return false;
    }

    Note(LogEntry::POTION_TAKEN, static_cast<int>(id));

    return true;
}

bool Run::CanDrinkPotion(std::size_t index) const
{
    const std::vector<Potion>& held = m_player.GetPotions();

    return index < held.size() && held[index].IsUsableOutside();
}

bool Run::DrinkPotion(std::size_t index)
{
    if (!CanDrinkPotion(index))
    {
        return false;
    }

    std::vector<Potion>& held = m_player.GetPotions();
    const Potion potion = held[index];

    held.erase(held.begin() + static_cast<std::ptrdiff_t>(index));
    Note(LogEntry::POTION_DRUNK, static_cast<int>(potion.GetId()));

    // A toy ornithopter answers every potion, wherever it is drunk.
    if (m_player.HasRelic(RelicId::TOY_ORNITHOPTER))
    {
        Heal(5);
    }

    const int times = m_player.HasRelic(RelicId::SACRED_BARK) ? 2 : 1;

    for (int i = 0; i < times; ++i)
    {
        // A brew fills the belt; on the map it will pour anything at all.
        if (potion.GetId() == PotionId::ENTROPIC_BREW)
        {
            const std::vector<PotionId>& pool = PotionRegistry::GetAll();

            while (static_cast<int>(m_player.GetPotions().size()) <
                       m_player.GetPotionSlots() &&
                   !pool.empty())
            {
                // Stopping the moment one is turned away, because a belt
                // that did not take that one is not going to take the next
                // either: a sozu refuses all of them, and waiting for it to
                // fill is waiting for ever.
                if (!AddPotion(pool[static_cast<std::size_t>(RollBetween(
                        m_rng, 0, static_cast<int>(pool.size()) - 1))]))
                {
                    break;
                }
            }

            continue;
        }

        // The rest of what can be drunk out here is health, one way or
        // another.
        for (const auto& effect : potion.GetEffects())
        {
            switch (effect.type)
            {
                case EffectType::HEAL:
                    Heal(effect.value);
                    break;

                case EffectType::HEAL_PERCENT:
                    Heal(m_player.GetMaxHealth() * effect.value / 100);
                    break;

                case EffectType::INCREASE_MAX_HEALTH:
                    m_player.IncreaseMaxHealth(effect.value);
                    break;

                default:
                    break;
            }
        }
    }

    return true;
}

bool Run::DiscardPotion(std::size_t index)
{
    std::vector<Potion>& held = m_player.GetPotions();

    if (index >= held.size())
    {
        return false;
    }

    Note(LogEntry::POTION_THROWN, static_cast<int>(held[index].GetId()));
    held.erase(held.begin() + static_cast<std::ptrdiff_t>(index));

    return true;
}
}  // namespace ConquerTheSpire
