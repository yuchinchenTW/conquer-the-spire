// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Rl/CApi.h>
#include <conquer-the-spire/Rl/SpireEnv.hpp>
#include <conquer-the-spire/Rl/VecSpireEnv.hpp>

#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Events/EventLibrary.hpp>
#include <conquer-the-spire/Monsters/MonsterRoster.hpp>
#include <conquer-the-spire/Potions/PotionRegistry.hpp>
#include <conquer-the-spire/Relics/RelicRegistry.hpp>

#include <cstring>
#include <random>
#include <tuple>

using ConquerTheSpire::CardColor;
using ConquerTheSpire::SpireEnv;
using ConquerTheSpire::StepResult;
using ConquerTheSpire::VecSpireEnv;

namespace
{
//! Turns the handle back into what it is.
SpireEnv* Of(cts_env env)
{
    return static_cast<SpireEnv*>(env);
}

VecSpireEnv* VecOf(cts_vec vec)
{
    return static_cast<VecSpireEnv*>(vec);
}

//! Copies \p name out, cut off at \p size, and says how long it was.
size_t Copied(const std::string& name, char* out, size_t size)
{
    if (out != nullptr && size > 0u)
    {
        const size_t room = size - 1u;
        const size_t kept = name.size() < room ? name.size() : room;

        std::memcpy(out, name.c_str(), kept);
        out[kept] = '\0';
    }

    return name.size() + 1u;
}

CardColor ColorOf(int character)
{
    switch (character)
    {
        case CTS_SILENT:
            return CardColor::GREEN;

        case CTS_DEFECT:
            return CardColor::BLUE;

        default:
            return CardColor::RED;
    }
}
}  // namespace

extern "C" {

cts_env cts_create(void)
{
    return new SpireEnv();
}

void cts_destroy(cts_env env)
{
    delete Of(env);
}

void cts_reset(cts_env env, int character, unsigned int seed)
{
    if (env != nullptr)
    {
        Of(env)->Reset(ColorOf(character), seed);
    }
}

size_t cts_observation_size(void)
{
    return SpireEnv::ObservationSize();
}

size_t cts_action_count(void)
{
    return SpireEnv::ActionCount();
}

size_t cts_layout_slots(void)
{
    return 23u;
}

size_t cts_id_layout_slots(void)
{
    return 13u;
}

void cts_layout(size_t* out)
{
    if (out == nullptr)
    {
        return;
    }

    const SpireEnv::Layout layout = SpireEnv::GetLayout();

    out[0] = layout.phase;
    out[1] = layout.run;
    out[2] = layout.deck;
    out[3] = layout.relics;
    out[4] = layout.battle;
    out[5] = layout.powers;
    out[6] = layout.monsters;
    out[7] = layout.hand;
    out[8] = layout.piles;
    out[9] = layout.total;
    out[10] = layout.monsterStride;
    out[11] = layout.handStride;
    out[12] = layout.pileStride;

    // The parts added after the fights: what is on offer and what lies
    // ahead. Anything reading these counts them against cts_layout_slots.
    out[13] = layout.rewards;
    out[14] = layout.shop;
    out[15] = layout.event;
    out[16] = layout.potions;
    out[17] = layout.moves;
    out[18] = layout.map;
    out[19] = layout.deckCards;
    out[20] = layout.rewardStride;
    out[21] = layout.eventStride;
    out[22] = layout.deckStride;
}

void cts_observe(cts_env env, float* out)
{
    if (env == nullptr || out == nullptr)
    {
        return;
    }

    const std::vector<float> state = Of(env)->Observe();

    std::memcpy(out, state.data(), state.size() * sizeof(float));
}

size_t cts_id_count(void)
{
    return SpireEnv::IdCount();
}

void cts_id_layout(size_t* out)
{
    if (out == nullptr)
    {
        return;
    }

    const SpireEnv::IdLayout layout = SpireEnv::GetIdLayout();

    out[0] = layout.hand;
    out[1] = layout.potions;
    out[2] = layout.relics;
    out[3] = layout.rewardKinds;
    out[4] = layout.rewardOptions;
    out[5] = layout.rewardOptionKinds;
    out[6] = layout.shopCards;
    out[7] = layout.shopRelics;
    out[8] = layout.shopPotions;
    out[9] = layout.event;
    out[10] = layout.monsters;
    out[11] = layout.deck;
    out[12] = layout.total;
}

void cts_observe_ids(cts_env env, int* out)
{
    if (env == nullptr || out == nullptr)
    {
        return;
    }

    const std::vector<int> ids = Of(env)->ObserveIds();

    std::memcpy(out, ids.data(), ids.size() * sizeof(int));
}

void cts_action_table(int* kinds, int* a, int* b)
{
    for (size_t index = 0; index < SpireEnv::ActionCount(); ++index)
    {
        const ConquerTheSpire::Action move =
            SpireEnv::ActionFromIndex(index);

        if (kinds != nullptr)
        {
            kinds[index] = static_cast<int>(move.kind);
        }

        if (a != nullptr)
        {
            a[index] = move.a;
        }

        if (b != nullptr)
        {
            b[index] = move.b;
        }
    }
}

void cts_action_mask(cts_env env, unsigned char* out)
{
    if (env == nullptr || out == nullptr)
    {
        return;
    }

    const std::vector<unsigned char> mask = Of(env)->ActionMask();

    std::memcpy(out, mask.data(), mask.size());
}

float cts_step(cts_env env, size_t index, int* taken, int* done)
{
    if (env == nullptr)
    {
        return 0.0f;
    }

    const StepResult result = Of(env)->StepIndex(index);

    if (taken != nullptr)
    {
        *taken = result.taken ? 1 : 0;
    }

    if (done != nullptr)
    {
        *done = result.done ? 1 : 0;
    }

    return result.reward;
}

int cts_phase(cts_env env)
{
    return env == nullptr ? 0 : static_cast<int>(Of(env)->GetPhase());
}

int cts_done(cts_env env)
{
    return env != nullptr && Of(env)->IsDone() ? 1 : 0;
}

int cts_act(cts_env env)
{
    return env == nullptr ? 0 : Of(env)->GetRun().GetAct();
}

int cts_floor(cts_env env)
{
    return env == nullptr ? 0 : Of(env)->GetRun().GetFloor();
}

int cts_total_floors(cts_env env)
{
    return env == nullptr ? 0 : Of(env)->GetTotalFloors();
}

int cts_gold(cts_env env)
{
    return env == nullptr ? 0 : Of(env)->GetRun().GetGold();
}

int cts_health(cts_env env)
{
    return env == nullptr ? 0 : Of(env)->GetRun().GetPlayer().GetHealth();
}

int cts_max_health(cts_env env)
{
    return env == nullptr ? 0 : Of(env)->GetRun().GetPlayer().GetMaxHealth();
}

int cts_deck_size(cts_env env)
{
    return env == nullptr
               ? 0
               : static_cast<int>(Of(env)->GetRun().GetDeck().size());
}

size_t cts_save(cts_env env, char* out, size_t size)
{
    if (env == nullptr)
    {
        return 0u;
    }

    const std::string text = Of(env)->Save();

    if (out != nullptr && size >= text.size() + 1u)
    {
        std::memcpy(out, text.c_str(), text.size() + 1u);
    }

    return text.empty() ? 0u : text.size() + 1u;
}

int cts_load(cts_env env, const char* text)
{
    if (env == nullptr || text == nullptr)
    {
        return 0;
    }

    return Of(env)->Load(std::string(text)) ? 1 : 0;
}

size_t cts_peek_slots(void)
{
    return 6u;
}

int cts_peek(cts_env env, size_t index, int follow, int* out)
{
    if (env == nullptr || out == nullptr)
    {
        return 0;
    }

    const SpireEnv::TurnCost cost = Of(env)->Peek(index, follow);

    if (!cost.looked)
    {
        return 0;
    }

    out[0] = cost.healthLost;
    out[1] = cost.healthLeft;
    out[2] = cost.monsterHealth;
    out[3] = cost.monstersLeft;
    out[4] = cost.over ? 1 : 0;
    out[5] = cost.won ? 1 : 0;

    return 1;
}

size_t cts_summary_slots(void)
{
    return ConquerTheSpire::RunLog::Summary::SLOTS;
}

size_t cts_log_lines(cts_env env)
{
    return env == nullptr
               ? 0u
               : Of(env)->GetRun().GetLog().GetLines().size();
}

void cts_summary(cts_env env, int* out)
{
    if (env != nullptr)
    {
        Of(env)->GetRun().GetLog().ReadSummary(out);
    }
}

void cts_log(cts_env env, int* out)
{
    if (env == nullptr || out == nullptr)
    {
        return;
    }

    const std::vector<ConquerTheSpire::LogLine>& lines =
        Of(env)->GetRun().GetLog().GetLines();

    for (size_t i = 0; i < lines.size(); ++i)
    {
        out[i * 7u + 0u] = static_cast<int>(lines[i].entry);
        out[i * 7u + 1u] = static_cast<int>(lines[i].source);
        out[i * 7u + 2u] = lines[i].id;
        out[i * 7u + 3u] = lines[i].extra;
        out[i * 7u + 4u] = lines[i].act;
        out[i * 7u + 5u] = lines[i].floor;
        out[i * 7u + 6u] = lines[i].stage;
    }
}

size_t cts_card_name(int id, char* out, size_t size)
{
    return Copied(
        ConquerTheSpire::CardRegistry::Get(
            static_cast<ConquerTheSpire::CardId>(id))
            .GetName(),
        out, size);
}

size_t cts_relic_name(int id, char* out, size_t size)
{
    return Copied(
        ConquerTheSpire::RelicRegistry::Get(
            static_cast<ConquerTheSpire::RelicId>(id))
            .GetName(),
        out, size);
}

size_t cts_potion_name(int id, char* out, size_t size)
{
    return Copied(
        ConquerTheSpire::PotionRegistry::Get(
            static_cast<ConquerTheSpire::PotionId>(id))
            .GetName(),
        out, size);
}

size_t cts_monster_name(int id, char* out, size_t size)
{
    std::mt19937 rng(1);

    return Copied(
        ConquerTheSpire::MonsterRoster::Make(
            static_cast<ConquerTheSpire::MonsterId>(id), rng)
            .GetName(),
        out, size);
}

size_t cts_event_name(int id, char* out, size_t size)
{
    return Copied(
        ConquerTheSpire::EventLibrary::Get(
            static_cast<ConquerTheSpire::EventId>(id))
            .GetName(),
        out, size);
}

void cts_set_health_weight(void* env, float weight)
{
    if (auto* one = static_cast<ConquerTheSpire::SpireEnv*>(env);
        one != nullptr)
    {
        one->SetHealthWeight(weight);
    }
}

void cts_vec_set_health_weight(void* vec, float weight)
{
    if (auto* row = static_cast<ConquerTheSpire::VecSpireEnv*>(vec);
        row != nullptr)
    {
        row->SetHealthWeight(weight);
    }
}

void cts_set_act_limit(void* env, int acts)
{
    if (auto* one = static_cast<ConquerTheSpire::SpireEnv*>(env);
        one != nullptr)
    {
        one->SetActLimit(acts);
    }
}

void cts_vec_set_act_limit(void* vec, int acts)
{
    if (auto* row = static_cast<ConquerTheSpire::VecSpireEnv*>(vec);
        row != nullptr)
    {
        row->SetActLimit(acts);
    }
}

size_t cts_map_node_name(int type, char* out, size_t size)
{
    return Copied(ConquerTheSpire::NameOf(
                      static_cast<ConquerTheSpire::MapNodeType>(type)),
                  out, size);
}

size_t cts_log_entry_count(void)
{
    return static_cast<size_t>(ConquerTheSpire::LogEntry::COUNT);
}

size_t cts_log_entry_name(int entry, char* out, size_t size)
{
    return Copied(
        ConquerTheSpire::NameOf(static_cast<ConquerTheSpire::LogEntry>(entry)),
        out, size);
}

int cts_card_type(int id)
{
    return static_cast<int>(
        ConquerTheSpire::CardRegistry::Get(
            static_cast<ConquerTheSpire::CardId>(id))
            .GetCardType());
}

int cts_card_rarity(int id)
{
    return static_cast<int>(
        ConquerTheSpire::CardRegistry::Get(
            static_cast<ConquerTheSpire::CardId>(id))
            .GetRarity());
}

size_t cts_event_option_name(int id, int stage, int option, char* out,
                             size_t size)
{
    ConquerTheSpire::Event room = ConquerTheSpire::EventLibrary::Get(
        static_cast<ConquerTheSpire::EventId>(id));

    if (stage > 0)
    {
        room.GoTo(stage);
    }

    const std::vector<ConquerTheSpire::EventOption>& options =
        room.GetOptions();
    const size_t which = static_cast<size_t>(option < 0 ? 0 : option);

    return Copied(which < options.size() ? options[which].label
                                         : std::string(),
                  out, size);
}

void cts_vec_summary(cts_vec vec, int* out)
{
    if (vec != nullptr)
    {
        VecOf(vec)->ReadSummaries(out);
    }
}

void cts_vec_last_summary(cts_vec vec, int* out)
{
    if (vec != nullptr)
    {
        VecOf(vec)->ReadLastSummaries(out);
    }
}

size_t cts_stat_slots(void)
{
    return ConquerTheSpire::RunStats::Row::SLOTS;
}

size_t cts_stat_rows(cts_env env)
{
    return env == nullptr ? 0u : Of(env)->GetStats().GetRowCount();
}

void cts_stats(cts_env env, int* out)
{
    if (env != nullptr)
    {
        Of(env)->GetStats().ReadRows(out);
    }
}

void cts_stat_totals(cts_env env, int* out)
{
    if (env != nullptr)
    {
        Of(env)->GetStats().ReadTotals(out);
    }
}

void cts_stats_clear(cts_env env)
{
    if (env != nullptr)
    {
        Of(env)->ClearStats();
    }
}

size_t cts_vec_stat_rows(cts_vec vec)
{
    return vec == nullptr ? 0u : VecOf(vec)->GetStats().GetRowCount();
}

void cts_vec_stats(cts_vec vec, int* out)
{
    if (vec != nullptr)
    {
        VecOf(vec)->GetStats().ReadRows(out);
    }
}

void cts_vec_stat_totals(cts_vec vec, int* out)
{
    if (vec != nullptr)
    {
        VecOf(vec)->GetStats().ReadTotals(out);
    }
}

void cts_vec_stats_clear(cts_vec vec)
{
    if (vec != nullptr)
    {
        VecOf(vec)->ClearStats();
    }
}

void cts_vec_roll_random(cts_vec vec, int character, unsigned int seed,
                         size_t runs, float* returns, int* floors,
                         int* steps)
{
    if (vec != nullptr)
    {
        VecOf(vec)->RollRandomHere(ColorOf(character), seed, runs, returns,
                                   floors, steps);
    }
}

cts_vec cts_vec_create(size_t count)
{
    return new VecSpireEnv(count);
}

void cts_vec_destroy(cts_vec vec)
{
    delete VecOf(vec);
}

size_t cts_vec_count(cts_vec vec)
{
    return vec == nullptr ? 0u : VecOf(vec)->GetCount();
}

void cts_vec_reset(cts_vec vec, int character, unsigned int seed)
{
    if (vec != nullptr)
    {
        VecOf(vec)->Reset(ColorOf(character), seed);
    }
}

void cts_vec_reset_one(cts_vec vec, size_t index, int character,
                       unsigned int seed)
{
    if (vec != nullptr)
    {
        VecOf(vec)->ResetOne(index, ColorOf(character), seed);
    }
}

void cts_vec_set_auto_reset(cts_vec vec, int on)
{
    if (vec != nullptr)
    {
        VecOf(vec)->SetAutoReset(on != 0);
    }
}

void cts_vec_observe(cts_vec vec, float* out)
{
    if (vec != nullptr)
    {
        VecOf(vec)->Observe(out);
    }
}

void cts_vec_observe_ids(cts_vec vec, int* out)
{
    if (vec != nullptr)
    {
        VecOf(vec)->ObserveIds(out);
    }
}

void cts_vec_action_mask(cts_vec vec, unsigned char* out)
{
    if (vec != nullptr)
    {
        VecOf(vec)->ActionMask(out);
    }
}

void cts_vec_step(cts_vec vec, const size_t* actions, float* rewards,
                  unsigned char* dones, unsigned char* taken, float* returns,
                  int* lengths)
{
    if (vec != nullptr)
    {
        VecOf(vec)->Step(actions, rewards, dones, taken, returns, lengths);
    }
}

void cts_vec_rank(cts_vec vec, const size_t* candidates, size_t width,
                  size_t* out)
{
    if (vec == nullptr || candidates == nullptr || out == nullptr ||
        width == 0u)
    {
        return;
    }

    VecSpireEnv* envs = VecOf(vec);
    const size_t none = SpireEnv::ActionCount();

    for (size_t row = 0; row < envs->GetCount(); ++row)
    {
        const size_t* offered = candidates + row * width;

        // Whatever preferred them goes first, so the head of the row is the
        // answer when there is nothing to look into.
        out[row] = offered[0];

        const SpireEnv& env = envs->At(row);
        bool found = false;
        std::tuple<int, int, int> best;

        for (size_t at = 0; at < width; ++at)
        {
            if (offered[at] >= none)
            {
                continue;
            }

            const SpireEnv::TurnCost cost =
                env.Peek(offered[at], SpireEnv::FOLLOW_TO_THE_END);

            if (!cost.looked)
            {
                continue;
            }

            // Won beats survived beats a fight the rule of thumb could not
            // finish; then least health gone; then the monsters worst off.
            const std::tuple<int, int, int> key(
                cost.won ? 0 : (cost.over ? 1 : 2), cost.healthLost,
                cost.monsterHealth);

            if (!found || key < best)
            {
                found = true;
                best = key;
                out[row] = offered[at];
            }
        }
    }
}

int cts_vec_phase(cts_vec vec, size_t index)
{
    return vec == nullptr
               ? 0
               : static_cast<int>(VecOf(vec)->At(index).GetPhase());
}

int cts_vec_total_floors(cts_vec vec, size_t index)
{
    return vec == nullptr ? 0 : VecOf(vec)->At(index).GetTotalFloors();
}

void cts_roll_random(int character, unsigned int seed, size_t runs,
                     float* returns, int* floors, int* steps)
{
    VecSpireEnv::RollRandom(ColorOf(character), seed, runs, returns, floors,
                            steps);
}
}
