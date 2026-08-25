/* Copyright (c) 2019 Chris Ohk */

/* We are making my contributions/submissions to this project solely in our
 * personal capacity and are not conveying any rights to any intellectual
 * property of any third parties. */

#ifndef CONQUER_THE_SPIRE_C_API_H
#define CONQUER_THE_SPIRE_C_API_H

#include <stddef.h>

#if defined(_WIN32)
#define CTS_API __declspec(dllexport)
#else
#define CTS_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* A climb, held on the other side of this wall. Everything here is plain C
 * so that a learner in another language can talk to it with nothing but a
 * shared library and a struct-free calling convention. */
typedef void* cts_env;

/* The characters, which are the card colours of the engine. */
#define CTS_IRONCLAD 1
#define CTS_SILENT 2
#define CTS_DEFECT 3

CTS_API cts_env cts_create(void);
CTS_API void cts_destroy(cts_env env);

/* Starts a climb. The same character and seed always give the same climb. */
CTS_API void cts_reset(cts_env env, int character, unsigned int seed);

/* How wide the state vector is, and how many moves the fixed head has.
 * Neither changes over a run, so both can be asked once. */
CTS_API size_t cts_observation_size(void);
CTS_API size_t cts_action_count(void);

/* Where each part of the state starts. \p out takes thirteen numbers, in the
 * order of the layout: phase, run, deck, relics, battle, powers, monsters,
 * hand, piles, total, monsterStride, handStride, pileStride. */
/* How many numbers cts_layout and cts_id_layout write, so that whatever
 * reads them cannot quietly fall out of step. */
CTS_API size_t cts_layout_slots(void);
CTS_API size_t cts_id_layout_slots(void);

CTS_API void cts_layout(size_t* out);

/* Writes the state into \p out, which must hold cts_observation_size()
 * floats. */
CTS_API void cts_observe(cts_env env, float* out);

/* How many ids the id vector holds. These say which card, relic, potion, room
 * and monster each slot is about: they are meant to be looked up or embedded,
 * not weighed. */
CTS_API size_t cts_id_count(void);

/* Where each part of the id vector starts. \p out takes twelve numbers: hand,
 * potions, relics, rewardKinds, rewardOptions, rewardOptionKinds, shopCards,
 * shopRelics, shopPotions, event, monsters, total. */
CTS_API void cts_id_layout(size_t* out);

/* Writes the ids into \p out, which must hold cts_id_count() ints. */
CTS_API void cts_observe_ids(cts_env env, int* out);

/* Writes one byte a move into \p out, which must hold cts_action_count()
 * bytes: one where the move is legal just now. */
CTS_API void cts_action_mask(cts_env env, unsigned char* out);

/* What kind of move each slot of the fixed head is, and what it works on.
 * \p kinds, \p a and \p b each take cts_action_count() ints and any of them
 * may be null. The kinds are numbered as ActionKind is. */
CTS_API void cts_action_table(int* kinds, int* a, int* b);

/* Takes the move at \p index. Returns what it was worth, and puts whether it
 * was legal at all and whether the run is over into \p taken and \p done,
 * either of which may be null. */
CTS_API float cts_step(cts_env env, size_t index, int* taken, int* done);

/* The handful of numbers worth reading without the whole vector. */
CTS_API int cts_phase(cts_env env);
CTS_API int cts_done(cts_env env);
CTS_API int cts_act(cts_env env);
CTS_API int cts_floor(cts_env env);
CTS_API int cts_total_floors(cts_env env);
CTS_API int cts_gold(cts_env env);
CTS_API int cts_health(cts_env env);
CTS_API int cts_max_health(cts_env env);
CTS_API int cts_deck_size(cts_env env);

/* Writes the climb out. Returns how many bytes it takes, and copies that
 * many into \p out when it is not null and \p size is big enough. A climb in
 * the middle of a fight cannot be written out and returns zero. */
CTS_API size_t cts_save(cts_env env, char* out, size_t size);

/* Reads a climb back. Returns one when it took. */
CTS_API int cts_load(cts_env env, const char* text);

/* ------------------------------------------------------- looking ahead */

/* What the rest of this turn costs if \p index is taken now. Plays a copy
 * of the fight out and throws it away, so nothing here changes the climb.
 *
 * Writes six numbers into \p out: health lost by the end of the turn,
 * health left, the monsters' health added up, how many still stand, whether
 * the fight ended, and whether it was won. Returns one when there was a
 * fight to look into and the move belonged to a turn, and zero otherwise -
 * in which case \p out is left alone.
 *
 * \p follow says what fills the rest of the turn: 0 ends it there, 1 keeps
 * playing whatever is playable. Neither is how a policy plays, so read a
 * cost from this as a comparison between moves, not a prediction. */
CTS_API size_t cts_peek_slots(void);
CTS_API int cts_peek(cts_env env, size_t index, int follow, int* out);

/* ------------------------------------------------------------- the record */

/* How many numbers a summary is, and how many lines the log of this climb
 * holds so far. */
CTS_API size_t cts_summary_slots(void);
CTS_API size_t cts_log_lines(cts_env env);

/* Reads the counts of the climb: floors, act, deepest act, fights won, elites
 * won, bosses won, cards taken, cards bought, cards removed, cards upgraded,
 * cards transformed, relics taken, relics bought, potions taken, potions
 * bought, potions drunk, gold earned, gold spent, rooms entered, rests, died,
 * won the spire. \p out takes cts_summary_slots() of them. */
CTS_API void cts_summary(cts_env env, int* out);

/* Reads the log itself: seven numbers a line, which are the entry, where it
 * came from, the id it was about, whatever else the line needed, the act, the
 * floor and the stage of the room. \p out takes seven times cts_log_lines()
 * ints. */
CTS_API void cts_log(cts_env env, int* out);

/* The names behind the ids, written into \p out and cut off at \p size.
 * Returns how many bytes the name takes, counting the one that ends it. */
CTS_API size_t cts_card_name(int id, char* out, size_t size);
CTS_API size_t cts_relic_name(int id, char* out, size_t size);
CTS_API size_t cts_potion_name(int id, char* out, size_t size);
CTS_API size_t cts_monster_name(int id, char* out, size_t size);
CTS_API size_t cts_event_name(int id, char* out, size_t size);

/* What a point of health taken off costs, against a floor being worth one. */
CTS_API void cts_set_health_weight(void* env, float weight);
CTS_API void cts_vec_set_health_weight(void* vec, float weight);

/* The name of one kind of move, or null when there is no such kind. */
CTS_API const char* cts_action_kind_name(int kind);

/* What a point of the health ceiling is worth, off or handed over. */
CTS_API void cts_set_max_health_weight(void* env, float weight);
CTS_API void cts_vec_set_max_health_weight(void* vec, float weight);

/* What a curse in the deck costs for every floor walked with it. */
CTS_API void cts_set_curse_penalty(void* env, float penalty);
CTS_API void cts_vec_set_curse_penalty(void* vec, float penalty);

/* Ends a climb once this many acts are cleared; 0 for the whole spire. */
CTS_API void cts_set_act_limit(void* env, int acts);
CTS_API void cts_vec_set_act_limit(void* vec, int acts);

/* What a kind of place on the map is called, numbered as MapNodeType is. */
CTS_API size_t cts_map_node_name(int type, char* out, size_t size);

/* What a kind of log line is called, and how many kinds there are. Anything
 * reading the log asks here rather than keeping a list of its own. */
CTS_API size_t cts_log_entry_count(void);
CTS_API size_t cts_log_entry_name(int entry, char* out, size_t size);

/* What kind of card and what rarity a card id is, numbered as CardType and
 * CardRarity are. This is what tells a curse from an attack on the other side
 * of the wall. */
CTS_API int cts_card_type(int id);

/* Which colour a card belongs to, numbered as CardColor is: which of the
 * four decks it comes out of, or whether it is a status or a curse. */
CTS_API int cts_card_color(int id);
CTS_API int cts_card_rarity(int id);

/* What an option of a room is called: the \p option th of the \p stage th
 * stage of the room \p id. */
CTS_API size_t cts_event_option_name(int id, int stage, int option, char* out,
                                     size_t size);

/* How many numbers a row of the table is, how many rows it holds, and the
 * rows themselves: the kind, the id, how often it was chosen, how many climbs
 * it was in, how many of those were won, how many died, and the floors of all
 * of them added up. The four totals are the climbs counted, the wins, the
 * deaths and the floors. */
CTS_API size_t cts_stat_slots(void);
CTS_API size_t cts_stat_rows(cts_env env);
CTS_API void cts_stats(cts_env env, int* out);
CTS_API void cts_stat_totals(cts_env env, int* out);
CTS_API void cts_stats_clear(cts_env env);

/* ------------------------------------------------------------------ rows */

/* A row of climbs stepped together, so that the wall is crossed once a tick
 * rather than once a move. Everything written to is laid out row by row:
 * climb zero first, then climb one, and so on. */
typedef void* cts_vec;

CTS_API cts_vec cts_vec_create(size_t count);
CTS_API void cts_vec_destroy(cts_vec vec);
CTS_API size_t cts_vec_count(cts_vec vec);

/* Starts every climb, the one at i seeded with seed + i. */
CTS_API void cts_vec_reset(cts_vec vec, int character, unsigned int seed);

/* Starts one of them over. */
CTS_API void cts_vec_reset_one(cts_vec vec, size_t index, int character,
                               unsigned int seed);

/* Whether a climb that ends starts another one on its own, which it does
 * unless this is told otherwise. */
CTS_API void cts_vec_set_auto_reset(cts_vec vec, int on);

/* Writes count rows of cts_observation_size() floats, of cts_id_count()
 * ints, and of cts_action_count() bytes. */
CTS_API void cts_vec_observe(cts_vec vec, float* out);
CTS_API void cts_vec_observe_ids(cts_vec vec, int* out);
CTS_API void cts_vec_action_mask(cts_vec vec, unsigned char* out);

/* Takes one move in every climb. \p actions holds one index a climb; the
 * rest take one value a climb and any of them may be null. \p returns and
 * \p lengths are only written where \p dones is set. */
CTS_API void cts_vec_step(cts_vec vec, const size_t* actions, float* rewards,
                          unsigned char* dones, unsigned char* taken,
                          float* returns, int* lengths);

/* Looks a fight ahead in every climb at once, and says which of the moves
 * offered comes out best.
 *
 * \p candidates holds \p width move indices a climb, in the order something
 * else preferred them; a value at or past cts_action_count() is a slot that
 * holds nothing. For each climb the fight is played out once per candidate
 * and the best is written to \p out - the one that wins, else the one that
 * costs the least health, else the one that leaves the monsters worst off.
 * A climb that is not in a fight, or whose candidates cannot be looked
 * into, gets its first candidate back unchanged.
 *
 * This is done here rather than a climb at a time from outside because the
 * crossing costs more than the work: a whole simulated fight is about five
 * microseconds. */
CTS_API void cts_vec_rank(cts_vec vec, const size_t* candidates, size_t width,
                          size_t* out);

/* A look at one of the climbs, for the numbers that are worth reading on
 * their own. */
CTS_API int cts_vec_phase(cts_vec vec, size_t index);
CTS_API int cts_vec_total_floors(cts_vec vec, size_t index);

/* Reads the counts of every climb of the row, one row of
 * cts_summary_slots() numbers each; and the counts of the climb that ended
 * last in each row, which is what a learner writes down when dones is set. */
CTS_API void cts_vec_summary(cts_vec vec, int* out);
CTS_API void cts_vec_last_summary(cts_vec vec, int* out);

/* The same table, over every climb the row has played. */
CTS_API size_t cts_vec_stat_rows(cts_vec vec);
CTS_API void cts_vec_stats(cts_vec vec, int* out);
CTS_API void cts_vec_stat_totals(cts_vec vec, int* out);
CTS_API void cts_vec_stats_clear(cts_vec vec);

/* Plays \p runs climbs right through with a die, counting them into the
 * row's table. */
CTS_API void cts_vec_roll_random(cts_vec vec, int character,
                                 unsigned int seed, size_t runs,
                                 float* returns, int* floors, int* steps);

/* Plays \p runs climbs right through with a die, all on this side of the
 * wall. Each of the three arrays takes \p runs values and may be null. */
CTS_API void cts_roll_random(int character, unsigned int seed, size_t runs,
                             float* returns, int* floors, int* steps);

#ifdef __cplusplus
}
#endif

#endif /* CONQUER_THE_SPIRE_C_API_H */
