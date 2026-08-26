// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Monsters/MonsterRoster.hpp>

#include <utility>

namespace ConquerTheSpire
{
namespace
{
using ME = MonsterEffect;
using MM = MonsterMove;

//! Rolls a number between \p low and \p high, both included.
int Roll(std::mt19937& rng, int low, int high)
{
    std::uniform_int_distribution<int> pick(low, high);

    return pick(rng);
}

//! Builds a monster that picks its moves by weight.
Monster Thinking(MonsterId id, const char* name, MonsterType type, int health,
                 std::vector<MonsterMove> moves)
{
    return Monster(id, name, type, health, std::move(moves));
}

//! Builds a monster that walks a fixed pattern. \p loopFrom is where the
//! pattern goes back to, for the ones whose opening never comes round again.
Monster Patterned(MonsterId id, const char* name, MonsterType type,
                  int health, std::vector<MonsterMove> pattern,
                  bool loop = true, std::size_t loopFrom = 0)
{
    Monster monster(name, health, std::move(pattern), loop, loopFrom);
    monster.SetIdentity(id, type);

    return monster;
}

constexpr MonsterId LAST_MONSTER_ID = MonsterId::TRAINING_DUMMY;
}  // namespace

Monster MonsterRoster::Make(MonsterId id, std::mt19937& rng,
                            int healthOverride)
{
    Monster monster;

    switch (id)
    {
        // ------------------------------------------------ Act 1, normal

        case MonsterId::CULTIST:
        {
            monster = Thinking(
                id, "Cultist", MonsterType::NORMAL, Roll(rng, 48, 54),
                { MM::Buff("Incantation", PowerType::RITUAL, 3).Opener(),
                  MM::Attack("Dark Strike", 6).Chance(100) });
            break;
        }

        case MonsterId::JAW_WORM:
        {
            monster = Thinking(
                id, "Jaw Worm", MonsterType::NORMAL, Roll(rng, 40, 44),
                { MM::Attack("Chomp", 11).Chance(25, 1).Opener(),
                  MM::Of("Bellow", Intent::BUFF,
                         { ME::Buff(PowerType::STRENGTH, 3), ME::Block(6) })
                      .Chance(45, 1),
                  MM::AttackAndDefend("Thrash", 7, 5).Chance(30, 2) });
            break;
        }

        case MonsterId::RED_LOUSE:
        {
            // A Louse settles on how hard it bites for the whole fight.
            const int bite = Roll(rng, 5, 7);

            monster = Thinking(id, "Red Louse", MonsterType::NORMAL,
                               Roll(rng, 10, 15),
                               { MM::Attack("Bite", bite).Chance(75, 2),
                                 MM::Buff("Grow", PowerType::STRENGTH, 3)
                                     .Chance(25, 2) });
            monster.AddPower(PowerType::CURL_UP, Roll(rng, 3, 7));
            break;
        }

        case MonsterId::GREEN_LOUSE:
        {
            const int bite = Roll(rng, 5, 7);

            monster = Thinking(id, "Green Louse", MonsterType::NORMAL,
                               Roll(rng, 11, 17),
                               { MM::Attack("Bite", bite).Chance(75, 2),
                                 MM::Debuff("Spit Web", PowerType::WEAK, 2)
                                     .Chance(25, 2) });
            monster.AddPower(PowerType::CURL_UP, Roll(rng, 3, 7));
            break;
        }

        case MonsterId::ACID_SLIME_S:
        {
            monster = Thinking(
                id, "Acid Slime (S)", MonsterType::NORMAL, Roll(rng, 8, 12),
                { MM::Debuff("Lick", PowerType::WEAK, 1).Chance(50, 1),
                  MM::Attack("Tackle", 3).Chance(50, 1) });
            break;
        }

        case MonsterId::ACID_SLIME_M:
        {
            monster = Thinking(
                id, "Acid Slime (M)", MonsterType::NORMAL, Roll(rng, 28, 32),
                { MM::Of("Corrosive Spit", Intent::ATTACK,
                         { ME::Damage(7), ME::AddCard(CardId::SLIMED) })
                      .Chance(30, 2),
                  MM::Attack("Tackle", 10).Chance(40, 2),
                  MM::Debuff("Lick", PowerType::WEAK, 1).Chance(30, 1) });
            break;
        }

        case MonsterId::ACID_SLIME_L:
        {
            monster = Thinking(
                id, "Acid Slime (L)", MonsterType::NORMAL, Roll(rng, 65, 69),
                { MM::Of("Corrosive Spit", Intent::ATTACK,
                         { ME::Damage(11), ME::AddCard(CardId::SLIMED, 2) })
                      .Chance(30, 2),
                  MM::Attack("Tackle", 16).Chance(40, 1),
                  MM::Debuff("Lick", PowerType::WEAK, 2).Chance(30, 2),
                  MM::Of("Split", Intent::SUMMON,
                         { ME::Split(MonsterId::ACID_SLIME_M,
                                     MonsterId::ACID_SLIME_M) }) });
            break;
        }

        case MonsterId::SPIKE_SLIME_S:
        {
            monster =
                Thinking(id, "Spike Slime (S)", MonsterType::NORMAL,
                         Roll(rng, 10, 14),
                         { MM::Attack("Tackle", 5).Chance(100) });
            break;
        }

        case MonsterId::SPIKE_SLIME_M:
        {
            monster = Thinking(
                id, "Spike Slime (M)", MonsterType::NORMAL, Roll(rng, 28, 32),
                { MM::Of("Flame Tackle", Intent::ATTACK,
                         { ME::Damage(8), ME::AddCard(CardId::SLIMED) })
                      .Chance(30, 2),
                  MM::Debuff("Lick", PowerType::FRAIL, 1).Chance(70, 2) });
            break;
        }

        case MonsterId::SPIKE_SLIME_L:
        {
            monster = Thinking(
                id, "Spike Slime (L)", MonsterType::NORMAL, Roll(rng, 64, 70),
                { MM::Of("Flame Tackle", Intent::ATTACK,
                         { ME::Damage(16), ME::AddCard(CardId::SLIMED, 2) })
                      .Chance(30, 2),
                  MM::Debuff("Lick", PowerType::FRAIL, 2).Chance(70, 2),
                  MM::Of("Split", Intent::SUMMON,
                         { ME::Split(MonsterId::SPIKE_SLIME_M,
                                     MonsterId::SPIKE_SLIME_M) }) });
            break;
        }

        case MonsterId::FUNGI_BEAST:
        {
            monster = Thinking(id, "Fungi Beast", MonsterType::NORMAL,
                               Roll(rng, 22, 28),
                               { MM::Attack("Bite", 6).Chance(60, 2),
                                 MM::Buff("Grow", PowerType::STRENGTH, 3)
                                     .Chance(40, 2) });
            monster.AddPower(PowerType::SPORE_CLOUD, 2);
            break;
        }

        case MonsterId::LOOTER:
        {
            // The real Looter tosses a coin on its third turn between Lunge
            // and Smoke Bomb; this one always lunges first.
            monster = Patterned(
                id, "Looter", MonsterType::NORMAL, Roll(rng, 44, 48),
                { MM::Attack("Mug", 10), MM::Attack("Mug", 10),
                  MM::Attack("Lunge", 12), MM::Defend("Smoke Bomb", 6),
                  MM::Of("Escape", Intent::ESCAPE, { ME::Escape() }) },
                false);
            break;
        }

        case MonsterId::BLUE_SLAVER:
        {
            monster = Thinking(
                id, "Blue Slaver", MonsterType::NORMAL, Roll(rng, 46, 50),
                { MM::Attack("Stab", 12).Chance(60),
                  MM::Of("Rake", Intent::ATTACK,
                         { ME::Damage(7), ME::Debuff(PowerType::WEAK, 1) })
                      .Chance(40, 1) });
            break;
        }

        case MonsterId::RED_SLAVER:
        {
            // The real Red Slaver only entangles once a fight; this one may
            // come back to it, though never twice in a row.
            monster = Thinking(
                id, "Red Slaver", MonsterType::NORMAL, Roll(rng, 46, 50),
                { MM::Attack("Stab", 13).Chance(55).Opener(),
                  MM::Of("Scrape", Intent::ATTACK,
                         { ME::Damage(8),
                           ME::Debuff(PowerType::VULNERABLE, 1) })
                      .Chance(20, 1),
                  MM::Debuff("Entangle", PowerType::ENTANGLED, 1)
                      .Chance(25, 1)
                      .NotFirst() });
            break;
        }

        case MonsterId::MAD_GREMLIN:
        {
            monster =
                Thinking(id, "Mad Gremlin", MonsterType::NORMAL,
                         Roll(rng, 20, 24),
                         { MM::Attack("Scratch", 4).Chance(100) });
            monster.AddPower(PowerType::ANGRY, 1);
            break;
        }

        case MonsterId::SNEAKY_GREMLIN:
        {
            monster =
                Thinking(id, "Sneaky Gremlin", MonsterType::NORMAL,
                         Roll(rng, 10, 14),
                         { MM::Attack("Puncture", 9).Chance(100) });
            break;
        }

        case MonsterId::FAT_GREMLIN:
        {
            monster = Thinking(
                id, "Fat Gremlin", MonsterType::NORMAL, Roll(rng, 13, 17),
                { MM::Of("Smash", Intent::ATTACK,
                         { ME::Damage(4), ME::Debuff(PowerType::WEAK, 1) })
                      .Chance(100) });
            break;
        }

        case MonsterId::SHIELD_GREMLIN:
        {
            // Protect covers somebody else, and itself once it is alone.
            monster = Thinking(
                id, "Shield Gremlin", MonsterType::NORMAL, Roll(rng, 12, 15),
                { MM::Of("Protect", Intent::DEFEND, { ME::BlockAlly(7) })
                      .Chance(100),
                  MM::Attack("Shield Bash", 6) });
            break;
        }

        case MonsterId::GREMLIN_WIZARD:
        {
            const MonsterMove charging =
                MM::Nothing("Charging", Intent::CHARGING);

            monster = Patterned(
                id, "Gremlin Wizard", MonsterType::NORMAL, Roll(rng, 21, 25),
                { charging, charging, MM::Attack("Ultimate Blast", 25),
                  charging, charging, charging,
                  MM::Attack("Ultimate Blast", 25) },
                true, 3);
            break;
        }

        // ------------------------------------------------- Act 1, elite

        case MonsterId::GREMLIN_NOB:
        {
            monster = Thinking(
                id, "Gremlin Nob", MonsterType::ELITE, Roll(rng, 82, 86),
                { MM::Buff("Bellow", PowerType::ENRAGE, 2).Opener(),
                  MM::Of("Skull Bash", Intent::ATTACK,
                         { ME::Damage(6),
                           ME::Debuff(PowerType::VULNERABLE, 2) })
                      .Chance(33),
                  MM::Attack("Bull Rush", 14).Chance(67, 2) });
            break;
        }

        case MonsterId::LAGAVULIN:
        {
            // It sleeps behind its Metallicize until it is hit, or until three
            // turns have gone by.
            monster = Patterned(id, "Lagavulin", MonsterType::ELITE,
                                Roll(rng, 109, 111),
                                { MM::Nothing("Stunned", Intent::STUN),
                                  MM::Attack("Attack", 18),
                                  MM::Attack("Attack", 18),
                                  MM::Of("Siphon Soul", Intent::DEBUFF,
                                         { ME::Debuff(PowerType::STRENGTH, -1),
                                           ME::Debuff(PowerType::DEXTERITY,
                                                      -1) }) },
                                true, 1);
            monster.AddPower(PowerType::METALLICIZE, 8);
            monster.AddPower(PowerType::ASLEEP, 3);
            monster.AddBlock(8);
            break;
        }

        case MonsterId::SENTRY:
        {
            monster = Patterned(
                id, "Sentry", MonsterType::ELITE, Roll(rng, 38, 42),
                { MM::Of("Bolt", Intent::DEBUFF,
                         { ME::AddCard(CardId::DAZED, 2) }),
                  MM::Attack("Beam", 9) });
            monster.AddPower(PowerType::ARTIFACT, 1);
            break;
        }

        // -------------------------------------------------- Act 1, boss

        case MonsterId::THE_GUARDIAN:
        {
            // Once enough has been dealt to it, Mode Shift turns it round; the
            // real fight raises the threshold by 10 every time, this one holds
            // at 40 after the first turn.
            monster = Patterned(
                id, "The Guardian", MonsterType::BOSS, 240,
                { MM::Defend("Charging Up", 9),
                  MM::Attack("Fierce Bash", 32),
                  MM::Of("Vent Steam", Intent::DEBUFF,
                         { ME::Debuff(PowerType::WEAK, 2),
                           ME::Debuff(PowerType::VULNERABLE, 2) }),
                  MM::Attack("Whirlwind", 5, 4),
                  MM::Of("Defensive Mode", Intent::BUFF,
                         { ME::Buff(PowerType::SHARP_HIDE, 3) }),
                  MM::Attack("Roll Attack", 9),
                  MM::Of("Twin Slam", Intent::ATTACK,
                         { ME::Damage(8, 2),
                           ME::Buff(PowerType::SHARP_HIDE, -3),
                           ME::Buff(PowerType::MODE_SHIFT, 40) }) });
            monster.AddPower(PowerType::MODE_SHIFT, 30);
            break;
        }

        case MonsterId::HEXAGHOST:
        {
            monster = Patterned(
                id, "Hexaghost", MonsterType::BOSS, 250,
                { MM::Nothing("Activate", Intent::CHARGING),
                  MM::Of("Divider", Intent::ATTACK,
                         { ME::DamageByPlayerHealth(12, 6) }),
                  MM::Of("Sear", Intent::ATTACK,
                         { ME::Damage(6), ME::AddCard(CardId::BURN) }),
                  MM::Attack("Tackle", 5, 2),
                  MM::Of("Sear", Intent::ATTACK,
                         { ME::Damage(6), ME::AddCard(CardId::BURN) }),
                  MM::Of("Inflame", Intent::BUFF,
                         { ME::Block(12),
                           ME::Buff(PowerType::STRENGTH, 2) }),
                  MM::Attack("Tackle", 5, 2),
                  MM::Of("Sear", Intent::ATTACK,
                         { ME::Damage(6), ME::AddCard(CardId::BURN) }),
                  MM::Of("Inferno", Intent::ATTACK,
                         { ME::Damage(2, 6),
                           ME::AddCard(CardId::BURN, 3, true) }) },
                true, 2);
            break;
        }

        case MonsterId::SLIME_BOSS:
        {
            monster = Patterned(
                id, "Slime Boss", MonsterType::BOSS, 140,
                { MM::Of("Goop Spray", Intent::DEBUFF,
                         { ME::AddCard(CardId::SLIMED, 3) }),
                  MM::Nothing("Preparing", Intent::CHARGING), MM::Attack("Slam", 35),
                  MM::Of("Split", Intent::SUMMON,
                         { ME::Split(MonsterId::ACID_SLIME_L,
                                     MonsterId::SPIKE_SLIME_L) }) },
                true, 0);
            break;
        }

        // ------------------------------------------------ Act 2, normal

        case MonsterId::SPHERIC_GUARDIAN:
        {
            // It sits behind a wall of block and never varies: activate,
            // wear the climber down, then slam and harden turn about.
            monster = Patterned(id, "Spheric Guardian", MonsterType::NORMAL,
                                20,
                                { MM::Defend("Activate", 25),
                                  MM::Of("Attack Debuff", Intent::ATTACK_DEBUFF,
                                         { ME::Damage(10),
                                           ME::Debuff(PowerType::FRAIL, 5) }),
                                  MM::Attack("Slam", 10, 2),
                                  MM::Of("Harden", Intent::ATTACK_DEFEND,
                                         { ME::Damage(10), ME::Block(15) }) },
                                true, 2);

            // The block is what this fight is: it comes with a barricade, so
            // what it puts up stays up. Clearing it at the top of every turn
            // the way every other monster's block is cleared made a wall into
            // a nuisance.
            monster.AddPower(PowerType::BARRICADE, 1);
            monster.AddPower(PowerType::ARTIFACT, 3);
            break;
        }

        case MonsterId::CHOSEN:
        {
            monster = Thinking(
                id, "Chosen", MonsterType::NORMAL, Roll(rng, 95, 99),
                // A poke, then the hex, and after that it turns about
                // between two pairs: on the odd turns a debilitate or a drain,
                // on the even ones a poke or a zap. It was drawing from all
                // four every turn, so the hex never landed on the second turn
                // and the turning about was not there at all.
                { MM::Attack("Poke", 5, 2).Chance(60).OnTurnsLike(2, 0)
                      .Opener(),
                  MM::Debuff("Hex", PowerType::HEX, 1).Chance(0).OnTurn(2),
                  MM::Of("Debilitate", Intent::ATTACK_DEBUFF,
                         { ME::Damage(10),
                           ME::Debuff(PowerType::VULNERABLE, 2) })
                      .Chance(50)
                      .OnTurnsLike(2, 1),
                  MM::Of("Drain", Intent::DEBUFF,
                         { ME::Debuff(PowerType::WEAK, 3),
                           ME::Buff(PowerType::STRENGTH, 3) })
                      .Chance(50)
                      .OnTurnsLike(2, 1),
                  MM::Attack("Zap", 18).Chance(40).OnTurnsLike(2, 0) });

            // It pokes, hexes, and only then starts choosing.
            monster.ForceMove("Poke");
            break;
        }

        case MonsterId::SHELLED_PARASITE:
        {
            monster = Thinking(
                id, "Shelled Parasite", MonsterType::NORMAL,
                Roll(rng, 68, 72),
                { MM::Attack("Double Strike", 6, 2).Chance(40, 2),
                  // It drinks what gets through: ten thrown, and whatever
                  // of it lands comes back as health.
                  MM::Of("Suck", Intent::ATTACK_BUFF, { ME::Drain(10) })
                      .Chance(40, 2),
                  MM::Of("Fell", Intent::ATTACK_DEBUFF,
                         { ME::Damage(18), ME::Debuff(PowerType::FRAIL, 2) })
                      .Chance(20, 1)
                      .NotFirst(),
                  MM::Nothing("Stunned", Intent::STUN) });
            monster.AddPower(PowerType::PLATED_ARMOR, 14);
            break;
        }

        case MonsterId::BYRD:
        {
            monster = Thinking(
                id, "Byrd", MonsterType::NORMAL, Roll(rng, 25, 31),
                { MM::Attack("Peck", 1, 5).Chance(50, 2),
                  MM::Buff("Caw", PowerType::STRENGTH, 1).Chance(30, 1),
                  MM::Attack("Swoop", 12).Chance(20, 1).NotFirst(),
                  MM::Nothing("Stunned", Intent::STUN),
                  MM::Attack("Headbutt", 3).Chance(0),
                  MM::Buff("Fly", PowerType::FLIGHT, 3).Chance(0) });
            monster.AddPower(PowerType::FLIGHT, 3);
            monster.SetFlightBase(3);
            break;
        }

        case MonsterId::MUGGER:
        {
            // The same walk as a Looter, for more: mug twice, lunge or throw
            // down smoke, then away with whatever it took.
            monster = Patterned(
                id, "Mugger", MonsterType::NORMAL, Roll(rng, 48, 52),
                { MM::Attack("Mug", 10), MM::Attack("Mug", 10),
                  MM::Attack("Lunge", 16), MM::Defend("Smoke Bomb", 11),
                  MM::Of("Escape", Intent::ESCAPE, { ME::Escape() }) },
                false);
            monster.AddPower(PowerType::THIEVERY, 15);
            break;
        }

        case MonsterId::CENTURION:
        {
            monster = Thinking(
                id, "Centurion", MonsterType::NORMAL, Roll(rng, 76, 80),
                { MM::Attack("Slash", 12).Chance(65, 2),
                  MM::Of("Protect", Intent::DEFEND, { ME::BlockAlly(15) })
                      .Chance(35, 2)
                      .WithAlly(),
                  MM::Attack("Fury", 6, 3).Chance(35, 2).Alone() });
            break;
        }

        case MonsterId::MYSTIC:
        {
            monster = Thinking(
                id, "Mystic", MonsterType::NORMAL, Roll(rng, 48, 56),
                { MM::Of("Attack-Debuff", Intent::ATTACK_DEBUFF,
                         { ME::Damage(8), ME::Debuff(PowerType::FRAIL, 2) })
                      .Chance(60, 2),
                  MM::Of("Buff", Intent::BUFF,
                         { ME::BuffAll(PowerType::STRENGTH, 2) })
                      .Chance(40, 2),
                  // It reaches for the herbs the moment anybody is properly
                  // hurt, twice over at most.
                  MM::Of("Heal", Intent::BUFF, { ME::HealAll(16) })
                      .Chance(0, 2)
                      .WhenAllyMissing(16) });
            break;
        }

        case MonsterId::SNAKE_PLANT:
        {
            monster = Thinking(
                id, "Snake Plant", MonsterType::NORMAL, Roll(rng, 75, 79),
                { MM::Attack("Chomp", 7, 3).Chance(65, 2),
                  MM::Of("Enfeebling Spores", Intent::DEBUFF,
                         { ME::Debuff(PowerType::FRAIL, 2),
                           ME::Debuff(PowerType::WEAK, 2) })
                      .Chance(35, 1) });
            monster.AddPower(PowerType::MALLEABLE, 3);
            monster.SetMalleableBase(3);
            break;
        }

        case MonsterId::SNECKO:
        {
            monster = Thinking(
                id, "Snecko", MonsterType::NORMAL, Roll(rng, 114, 120),
                { MM::Debuff("Perplexing Glare", PowerType::CONFUSED, 1)
                      .Opener(),
                  MM::Attack("Bite", 15).Chance(60, 2),
                  MM::Of("Tail Whip", Intent::ATTACK_DEBUFF,
                         { ME::Damage(8),
                           ME::Debuff(PowerType::VULNERABLE, 2) })
                      .Chance(40) });
            break;
        }

        // ------------------------------------------------- Act 2, elite

        case MonsterId::GREMLIN_LEADER:
        {
            monster = Thinking(
                id, "Gremlin Leader", MonsterType::ELITE,
                Roll(rng, 140, 148),
                { MM::Of("Rally", Intent::SUMMON,
                         { ME::Summon(MonsterId::MAD_GREMLIN, 2, 3) })
                      .Chance(75, 1),
                  MM::Attack("Stab", 6, 3).Chance(25, 1),
                  MM::Of("Encourage", Intent::BUFF,
                         { ME::BuffAll(PowerType::STRENGTH, 3),
                           ME::BlockAllies(6) })
                      .Chance(66, 1)
                      .WithAlly() });
            break;
        }

        case MonsterId::TASKMASTER:
        {
            monster = Thinking(
                id, "Taskmaster", MonsterType::ELITE, Roll(rng, 54, 60),
                { MM::Of("Scouring Whip", Intent::ATTACK_DEBUFF,
                         { ME::Damage(7), ME::AddCard(CardId::WOUND, 1) })
                      .Chance(100) });
            break;
        }

        case MonsterId::BOOK_OF_STABBING:
        {
            // Multi Stab is six a hit, twice over to begin with and one
            // more hit for every time it has already been thrown. A book left
            // standing gets worse and worse, which is the whole reason to kill
            // it quickly, and it was throwing the same two hits all fight.
            monster = Thinking(
                id, "Book of Stabbing", MonsterType::ELITE,
                Roll(rng, 160, 164),
                { MM::Attack("Multi Stab", 6, 2).Chance(85, 2).GrowsWithUse(),
                  MM::Attack("Big Stab", 21).Chance(15, 1) });
            monster.AddPower(PowerType::PAINFUL_STABS, 1);
            break;
        }

        // -------------------------------------------------- Act 2, boss

        case MonsterId::BRONZE_AUTOMATON:
        {
            monster = Patterned(
                id, "Bronze Automaton", MonsterType::BOSS, 300,
                { MM::Of("Spawn Orbs", Intent::SUMMON,
                         { ME::Summon(MonsterId::BRONZE_ORB, 2, 2) }),
                  MM::Attack("Flail", 7, 2),
                  MM::Buff("Boost", PowerType::STRENGTH, 3, 9),
                  MM::Attack("Flail", 7, 2),
                  MM::Buff("Boost", PowerType::STRENGTH, 3, 9),
                  MM::Attack("HYPER BEAM", 45), MM::Nothing("Stunned", Intent::STUN) },
                true, 1);
            monster.AddPower(PowerType::ARTIFACT, 3);
            break;
        }

        case MonsterId::BRONZE_ORB:
        {
            // Until Stasis has been used it takes three quarters of the
            // weighted draw; afterwards the orb falls back to the 70/30
            // support-or-beam mix. The repeat limit is the spire rule that
            // it cannot do the same thing three times in a row.
            monster = Thinking(
                id, "Bronze Orb", MonsterType::NORMAL, Roll(rng, 52, 58),
                { MM::Of("Stasis", Intent::DEBUFF,
                         { ME::Stasis() }).Chance(30, 2).InPhase(1),
                  MM::Of("Support Beam", Intent::DEFEND,
                         { ME::BlockAlly(12) }).Chance(7, 2),
                  MM::Attack("Beam", 8).Chance(3, 2) });
            break;
        }

        case MonsterId::THE_CHAMP:
        {
            monster = Thinking(
                id, "The Champ", MonsterType::BOSS, 420,
                // A move he may not repeat hands its share to one named
                // other rather than to all of them: the stance to the gloat,
                // the gloat to the slap, the slap to the slash, and the slash
                // back to the slap rather than round to the stance. So a
                // Champ who has just gloated faces a slap at forty and every
                // other share exactly where it was. And the stance comes
                // twice in a fight at most; after that he gloats instead.
                { MM::Attack("Heavy Slash", 16).Chance(45, 1)
                      .SpillsTo("Face Slap"),
                  MM::Of("Face Slap", Intent::ATTACK_DEBUFF,
                         { ME::Damage(12), ME::Debuff(PowerType::FRAIL, 2),
                           ME::Debuff(PowerType::VULNERABLE, 2) })
                      .Chance(25, 1)
                      .SpillsTo("Heavy Slash"),
                  MM::Buff("Defensive Stance", PowerType::METALLICIZE, 5, 15)
                      .Chance(15, 1)
                      .SpillsTo("Gloat")
                      .AtMost(2, "Gloat"),
                  MM::Buff("Gloat", PowerType::STRENGTH, 2).Chance(15, 1)
                      .SpillsTo("Face Slap"),
                  MM::Of("Taunt", Intent::DEBUFF,
                         { ME::Debuff(PowerType::WEAK, 2),
                           ME::Debuff(PowerType::VULNERABLE, 2) })
                      .Every(4)
                      .InPhase(1),
                  // Once it is losing it shakes off what has been put on it
                  // and starts executing.
                  MM::Of("Anger", Intent::BUFF,
                         { ME::ShakeOff(),
                           ME::Buff(PowerType::STRENGTH, 6) })
                      .Chance(0),
                  // Counted from the turn he turned: the execute lands the
                  // turn straight after the anger and every third turn from
                  // there. Counted against the turn the fight started it only
                  // landed straight after when the two happened to line up,
                  // which is one time in three.
                  MM::Attack("Execute", 10, 2)
                      .Every(3)
                      .SincePhase()
                      .InPhase(2) });
            break;
        }

        case MonsterId::THE_COLLECTOR:
        {
            // She opens by calling two heads and always turns to the
            // debuff on the fourth turn. What she does the rest of the time
            // depends on how many of them are standing: with both up she
            // cannot call more, so the calling is out of the draw altogether
            // and the fireball takes its share - which is why the fireball
            // is written twice, once for each share. The two are never both
            // on offer, so the rule about not throwing three in a row still
            // counts them as the one move it is. Leaving the calling in the
            // draw with both heads up had her spending near a quarter of
            // those turns summoning nothing at all.
            monster = Thinking(
                id, "The Collector", MonsterType::BOSS, 282,
                { MM::Of("Spawn", Intent::SUMMON,
                         { ME::Summon(MonsterId::TORCH_HEAD, 2, 2) })
                      .Chance(25)
                      .WhenAlliesUnder(2)
                      .Opener(),
                  MM::Attack("Fireball", 18).Chance(70, 2)
                      .WhenAlliesAtLeast(2),
                  MM::Attack("Fireball", 18).Chance(45, 2)
                      .WhenAlliesUnder(2),
                  MM::Of("Buff", Intent::BUFF,
                         { ME::BuffAll(PowerType::STRENGTH, 3),
                           ME::Block(15) })
                      .Chance(30, 1),
                  MM::Of("Mega Debuff", Intent::DEBUFF,
                         { ME::Debuff(PowerType::WEAK, 3),
                           ME::Debuff(PowerType::VULNERABLE, 3),
                           ME::Debuff(PowerType::FRAIL, 3) })
                      .OnTurn(4) });
            break;
        }

        case MonsterId::TORCH_HEAD:
        {
            monster = Thinking(id, "Torch Head", MonsterType::NORMAL,
                               Roll(rng, 38, 40),
                               { MM::Attack("Tackle", 7).Chance(100) });
            monster.AddPower(PowerType::MINION, 1);
            break;
        }

        // ------------------------------------------------ Act 3, normal

        case MonsterId::DARKLING:
        {
            monster = Thinking(
                id, "Darkling", MonsterType::NORMAL, Roll(rng, 48, 56),
                { MM::Attack("Nip", 9).Chance(30, 2),
                  MM::Attack("Chomp", 8, 2).Chance(40, 1).NotFirst(),
                  MM::Defend("Harden", 12).Chance(30, 1),
                  MM::Of("Reincarnate", Intent::SUMMON,
                         { ME::Revive(50) }) });
            monster.AddPower(PowerType::LIFE_LINK, 1);
            break;
        }

        case MonsterId::ORB_WALKER:
        {
            monster = Thinking(
                id, "Orb Walker", MonsterType::NORMAL, Roll(rng, 90, 96),
                { MM::Of("Laser", Intent::ATTACK_DEBUFF,
                         { ME::Damage(10), ME::AddCard(CardId::BURN, 2) })
                      .Chance(60, 2),
                  MM::Attack("Claw", 15).Chance(40, 2) });
            monster.AddPower(PowerType::RITUAL, 3);
            break;
        }

        case MonsterId::SPIKER:
        {
            monster = Thinking(
                id, "Spiker", MonsterType::NORMAL, Roll(rng, 42, 56),
                { MM::Attack("Cut", 7).Chance(50, 1),
                  MM::Buff("Spike", PowerType::THORNS, 2).Chance(50) });
            monster.AddPower(PowerType::THORNS, 3);
            break;
        }

        case MonsterId::REPULSOR:
        {
            monster = Thinking(
                id, "Repulsor", MonsterType::NORMAL, Roll(rng, 29, 35),
                { MM::Of("Repulse", Intent::DEBUFF,
                         { ME::AddCard(CardId::DAZED, 2) })
                      .Chance(80),
                  MM::Attack("Bash", 11).Chance(20, 1) });
            break;
        }

        case MonsterId::EXPLODER:
        {
            // Two swings and it goes off, taking itself with it.
            monster = Patterned(id, "Exploder", MonsterType::NORMAL, 30,
                                { MM::Attack("Slam", 9),
                                  MM::Attack("Slam", 9),
                                  MM::Of("Explode", Intent::ATTACK,
                                         { ME::SelfDestruct(30) }) },
                                false);
            break;
        }

        case MonsterId::THE_MAW:
        {
            monster = Thinking(
                id, "The Maw", MonsterType::NORMAL, 300,
                { MM::Of("Roar", Intent::DEBUFF,
                         { ME::Debuff(PowerType::WEAK, 3),
                           ME::Debuff(PowerType::FRAIL, 3) })
                      .Opener(),
                  MM::Attack("Slam", 25).Chance(50, 1),
                  MM::Attack("Nom Nom", 5, 3).Chance(50, 1),
                  MM::Buff("Drool", PowerType::STRENGTH, 3).Chance(30, 1) });
            break;
        }

        case MonsterId::SPIRE_GROWTH:
        {
            monster = Thinking(
                id, "Spire Growth", MonsterType::NORMAL, 170,
                { MM::Attack("Quick Tackle", 16).Chance(50, 2),
                  MM::Debuff("Constrict", PowerType::CONSTRICTED, 10)
                      .Chance(50, 1),
                  MM::Attack("Smash", 22).Chance(0) });
            break;
        }

        case MonsterId::TRANSIENT:
        {
            // It hits harder every turn and fades away after five of them.
            monster = Patterned(id, "Transient", MonsterType::NORMAL, 999,
                                { MM::Attack("Attack", 30),
                                  MM::Attack("Attack", 40),
                                  MM::Attack("Attack", 50),
                                  MM::Attack("Attack", 60),
                                  MM::Attack("Attack", 70) },
                                false);
            monster.AddPower(PowerType::FADING, 5);
            monster.AddPower(PowerType::SHIFTING, 1);
            break;
        }

        case MonsterId::WRITHING_MASS:
        {
            monster = Thinking(
                id, "Writhing Mass", MonsterType::NORMAL, 160,
                { MM::Attack("Multi Hit", 7, 3).Chance(30, 1),
                  MM::Of("Debuff Attack", Intent::ATTACK_DEBUFF,
                         { ME::Damage(10), ME::Debuff(PowerType::WEAK, 2),
                           ME::Debuff(PowerType::VULNERABLE, 2) })
                      .Chance(30, 1),
                  MM::Attack("Big Hit", 32).Chance(20, 1),
                  MM::Of("Block Attack", Intent::ATTACK_DEFEND,
                         { ME::Damage(15), ME::Block(16) })
                      .Chance(10, 1),
                  MM::Of("Implant", Intent::DEBUFF,
                         { ME::AddCard(CardId::PARASITE, 1) })
                      .Chance(10, 1)
                      .NotFirst() });
            monster.AddPower(PowerType::MALLEABLE, 4);
            monster.SetMalleableBase(4);
            monster.AddPower(PowerType::REACTIVE, 1);
            break;
        }

        // ------------------------------------------------- Act 3, elite

        case MonsterId::GIANT_HEAD:
        {
            // It counts the climber down for four turns and then swings for
            // everything.
            monster = Thinking(
                id, "Giant Head", MonsterType::ELITE, 500,
                { MM::Attack("Count", 13).Chance(50, 2).InPhase(1),
                  MM::Debuff("Glare", PowerType::WEAK, 1)
                      .Chance(50, 2)
                      .InPhase(1),
                  MM::Attack("It Is Time", 30).Chance(100).InPhase(2) });
            monster.AddPower(PowerType::SLOW, 1);
            break;
        }

        case MonsterId::NEMESIS:
        {
            monster = Thinking(
                id, "Nemesis", MonsterType::ELITE, 185,
                { MM::Attack("Tri Attack", 6, 3).Chance(35, 2),
                  MM::Of("Tri Burn", Intent::DEBUFF,
                         { ME::AddCard(CardId::BURN, 3) })
                      .Chance(35, 1),
                  MM::Attack("Scythe", 45).Chance(30, 1).NotFirst() });
            monster.AddPower(PowerType::INTANGIBLE_CYCLE, 1);
            monster.AddPower(PowerType::INTANGIBLE, 1);
            break;
        }

        case MonsterId::REPTOMANCER:
        {
            monster = Thinking(
                id, "Reptomancer", MonsterType::ELITE, Roll(rng, 180, 190),
                { MM::Of("Summon", Intent::SUMMON,
                         { ME::Summon(MonsterId::DAGGER, 1, 4) })
                      .Chance(33, 2)
                      .Opener(),
                  MM::Attack("Big Bite", 30).Chance(33, 1),
                  MM::Of("Snake Strike", Intent::ATTACK_DEBUFF,
                         { ME::Damage(13, 2), ME::Debuff(PowerType::WEAK, 1) })
                      .Chance(34, 1) });
            break;
        }

        case MonsterId::DAGGER:
        {
            monster = Patterned(
                id, "Dagger", MonsterType::NORMAL, Roll(rng, 20, 25),
                { MM::Of("Stab", Intent::ATTACK_DEBUFF,
                         { ME::Damage(9), ME::AddCard(CardId::WOUND, 1) }),
                  MM::Of("Explode", Intent::ATTACK,
                         { ME::SelfDestruct(25) }) },
                false);
            monster.AddPower(PowerType::MINION, 1);
            break;
        }

        // -------------------------------------------------- Act 3, boss

        case MonsterId::AWAKENED_ONE:
        {
            monster = Thinking(
                id, "Awakened One", MonsterType::BOSS, 300,
                { MM::Attack("Slash", 20).Chance(75, 2).InPhase(1),
                  MM::Attack("Soul Strike", 6, 4).Chance(25, 1).InPhase(1),
                  MM::Attack("Dark Echo", 40).Chance(0).InPhase(2),
                  MM::Attack("Tackle", 10, 3).Chance(50, 2).InPhase(2),
                  MM::Of("Sludge", Intent::ATTACK_DEBUFF,
                         { ME::Damage(18), ME::AddCard(CardId::VOID, 1) })
                      .Chance(50, 2)
                      .InPhase(2) });
            monster.AddPower(PowerType::REGENERATION, 10);
            monster.AddPower(PowerType::CURIOSITY, 1);
            break;
        }

        case MonsterId::TIME_EATER:
        {
            monster = Thinking(
                id, "Time Eater", MonsterType::BOSS, 456,
                { MM::Attack("Reverberate", 7, 3).Chance(45, 2),
                  MM::Of("Head Slam", Intent::ATTACK_DEBUFF,
                         { ME::Damage(26),
                           ME::Debuff(PowerType::DRAW_REDUCTION, 1) })
                      .Chance(35, 1),
                  MM::Of("Ripple", Intent::DEFEND,
                         { ME::Block(20),
                           ME::Debuff(PowerType::VULNERABLE, 1),
                           ME::Debuff(PowerType::WEAK, 1) })
                      .Chance(20, 1),
                  MM::Nothing("Haste", Intent::BUFF).Chance(0) });
            monster.AddPower(PowerType::TIME_WARP, 12);
            break;
        }

        case MonsterId::DONU:
        {
            monster = Patterned(
                id, "Donu", MonsterType::BOSS, 250,
                { MM::Of("Circle of Power", Intent::BUFF,
                         { ME::BuffAll(PowerType::STRENGTH, 3) }),
                  MM::Attack("Beam", 10, 2) });
            monster.AddPower(PowerType::ARTIFACT, 2);
            break;
        }

        case MonsterId::DECA:
        {
            monster = Patterned(
                id, "Deca", MonsterType::BOSS, 250,
                { MM::Of("Beam", Intent::ATTACK_DEBUFF,
                         { ME::Damage(10, 2), ME::AddCard(CardId::DAZED, 2) }),
                  MM::Of("Square of Protection", Intent::DEFEND,
                         { ME::Block(16), ME::BlockAllies(16) }) });
            monster.AddPower(PowerType::ARTIFACT, 2);
            break;
        }

        // ------------------------------------------------------- Act 4

        case MonsterId::SPIRE_SHIELD:
        {
            monster = Thinking(
                id, "Spire Shield", MonsterType::ELITE, 110,
                { MM::Of("Bash", Intent::ATTACK_DEBUFF,
                         { ME::Damage(12),
                           ME::Debuff(PowerType::STRENGTH, -1) })
                      .Chance(50, 1),
                  MM::Of("Fortify", Intent::DEFEND,
                         { ME::Block(30), ME::BlockAllies(30) })
                      .Chance(50, 1),
                  MM::Of("Smash", Intent::ATTACK_DEFEND,
                         { ME::Damage(34), ME::Block(34) })
                      .Every(3) });
            monster.AddPower(PowerType::ARTIFACT, 1);
            break;
        }

        case MonsterId::SPIRE_SPEAR:
        {
            monster = Thinking(
                id, "Spire Spear", MonsterType::ELITE, 160,
                { MM::Of("Burn Strike", Intent::ATTACK_DEBUFF,
                         { ME::Damage(5, 2), ME::AddCard(CardId::BURN, 2) })
                      .Chance(50, 1)
                      .Opener(),
                  MM::Of("Piercer", Intent::BUFF,
                         { ME::BuffAll(PowerType::STRENGTH, 2) })
                      .Chance(50, 1),
                  MM::Attack("Skewer", 10, 3).Every(3) });
            monster.AddPower(PowerType::ARTIFACT, 1);
            break;
        }

        case MonsterId::CORRUPT_HEART:
        {
            monster = Thinking(
                id, "Corrupt Heart", MonsterType::BOSS, 750,
                { MM::Of("Debilitate", Intent::DEBUFF,
                         { ME::Debuff(PowerType::VULNERABLE, 2),
                           ME::Debuff(PowerType::WEAK, 2),
                           ME::Debuff(PowerType::FRAIL, 2),
                           ME::AddCard(CardId::BURN, 1),
                           ME::AddCard(CardId::DAZED, 1),
                           ME::AddCard(CardId::SLIMED, 1),
                           ME::AddCard(CardId::VOID, 1),
                           ME::AddCard(CardId::WOUND, 1) })
                      .Opener(),
                  MM::Attack("Blood Shots", 2, 12).Chance(50, 1),
                  MM::Attack("Echo", 40).Chance(50, 1),
                  MM::Buff("Buff", PowerType::STRENGTH, 2).Every(3) });
            monster.AddPower(PowerType::BEAT_OF_DEATH, 1);
            monster.AddPower(PowerType::INVINCIBLE, 300);
            monster.SetDamageCapLeft(300);
            break;
        }

        // ------------------------------------------- The masked bandits

        case MonsterId::POINTY:
        {
            monster = Thinking(id, "Pointy", MonsterType::NORMAL, 30,
                               { MM::Attack("Pointy Special", 5, 2)
                                     .Chance(100) });
            break;
        }

        case MonsterId::ROMEO:
        {
            // It mocks first and then swings turn about.
            monster = Patterned(
                id, "Romeo", MonsterType::NORMAL, Roll(rng, 35, 39),
                { MM::Nothing("Mock", Intent::UNKNOWN),
                  MM::Of("Agonizing Slash", Intent::ATTACK_DEBUFF,
                         { ME::Damage(10), ME::Debuff(PowerType::WEAK, 2) }),
                  MM::Attack("Cross Slash", 15) },
                true, 1);
            break;
        }

        case MonsterId::BEAR:
        {
            monster = Patterned(
                id, "Bear", MonsterType::NORMAL, Roll(rng, 38, 42),
                { MM::Debuff("Bear Hug", PowerType::DEXTERITY, -2),
                  MM::Of("Lunge", Intent::ATTACK_DEFEND,
                         { ME::Damage(9), ME::Block(9) }),
                  MM::Attack("Maul", 18) },
                true, 1);
            break;
        }

        case MonsterId::TRAINING_DUMMY:
        {
            monster = Patterned(id, "Training Dummy", MonsterType::NORMAL,
                               50, {});
            break;
        }

        case MonsterId::INVALID:
            break;
    }

    if (healthOverride > 0)
    {
        monster.SetMaxHealth(healthOverride);
        monster.SetHealth(healthOverride);
    }

    return monster;
}

const std::vector<MonsterId>& MonsterRoster::GetAll()
{
    static const std::vector<MonsterId> all = [] {
        std::vector<MonsterId> built;
        std::mt19937 rng(1);

        for (int i = 1; i <= static_cast<int>(LAST_MONSTER_ID); ++i)
        {
            const MonsterId id = static_cast<MonsterId>(i);

            if (!MonsterRoster::Make(id, rng).GetName().empty())
            {
                built.emplace_back(id);
            }
        }

        return built;
    }();

    return all;
}

std::vector<MonsterId> MonsterRoster::GetPool(MonsterType type)
{
    std::vector<MonsterId> matching;
    std::mt19937 rng(1);

    for (const MonsterId id : GetAll())
    {
        if (Make(id, rng).GetMonsterType() == type)
        {
            matching.emplace_back(id);
        }
    }

    return matching;
}
}  // namespace ConquerTheSpire
