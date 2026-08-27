#include "doctest.h"

#include <conquer-the-spire/Battle/Battle.hpp>
#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Monsters/EncounterLibrary.hpp>
#include <conquer-the-spire/Monsters/MonsterRoster.hpp>
#include <conquer-the-spire/Run/Run.hpp>

#include <cstddef>
#include <map>
#include <random>
#include <string>
#include <utility>
#include <vector>

using namespace ConquerTheSpire;

namespace
{
//! Builds one monster from the roster with a fixed generator.
Monster Make(MonsterId id, unsigned int seed = 5)
{
    std::mt19937 rng(seed);

    return MonsterRoster::Make(id, rng);
}

//! Builds a started battle against \p monsters with a deck of Strikes.
Battle BattleAgainst(std::vector<Monster> monsters, int strikes = 10,
                     int playerHealth = 80)
{
    Player player("Ironclad", playerHealth);
    player.SetColor(CardColor::RED);

    for (int i = 0; i < strikes; ++i)
    {
        player.AddCardToDeck(CardRegistry::Get(CardId::STRIKE_RED));
    }

    Battle battle(std::move(player), std::move(monsters), 31);
    battle.Start();

    return battle;
}

//! Counts how often each move comes up over many turns of one monster.
std::map<std::string, int> MoveTally(MonsterId id, int turns)
{
    std::map<std::string, int> tally;

    // A fresh battle each time keeps the player out of the way.
    for (unsigned int seed = 0; seed < 60; ++seed)
    {
        std::mt19937 rng(seed);
        Monster monster = MonsterRoster::Make(id, rng);
        monster.ChooseOpeningMove(rng);

        for (int turn = 0; turn < turns; ++turn)
        {
            ++tally[monster.GetCurrentMove().name];
            monster.AdvanceMove(rng);
        }
    }

    return tally;
}
}  // namespace

TEST_CASE("Every monster of the roster builds with health and moves")
{
    const std::vector<MonsterId>& all = MonsterRoster::GetAll();

    CHECK(all.size() == 67u);

    for (const MonsterId id : all)
    {
        const Monster monster = Make(id);

        CHECK(monster.GetMonsterId() == id);
        CHECK(monster.GetName().empty() == false);
        CHECK(monster.GetMaxHealth() > 0);
        CHECK(monster.GetHealth() == monster.GetMaxHealth());

        if (id != MonsterId::TRAINING_DUMMY)
        {
            CHECK(monster.GetMoves().empty() == false);
        }
    }

    // Three elites and three bosses for each act that has a roster.
    CHECK(MonsterRoster::GetPool(MonsterType::ELITE).size() == 11u);
    CHECK(MonsterRoster::GetPool(MonsterType::BOSS).size() == 11u);
}

TEST_CASE("Health is rolled from the range the monster has")
{
    int low = 999;
    int high = 0;

    for (unsigned int seed = 0; seed < 40; ++seed)
    {
        const int health = Make(MonsterId::JAW_WORM, seed).GetMaxHealth();

        low = health < low ? health : low;
        high = health > high ? health : high;
    }

    CHECK(low >= 40);
    CHECK(high <= 44);
    CHECK(low < high);
}

TEST_CASE("A Jaw Worm always opens with Chomp")
{
    for (unsigned int seed = 0; seed < 20; ++seed)
    {
        std::mt19937 rng(seed);
        Monster worm = MonsterRoster::Make(MonsterId::JAW_WORM, rng);
        worm.ChooseOpeningMove(rng);

        CHECK(worm.GetCurrentMove().name == "Chomp");
    }
}

TEST_CASE("A Jaw Worm keeps to its own repeat limits")
{
    for (unsigned int seed = 0; seed < 40; ++seed)
    {
        std::mt19937 rng(seed);
        Monster worm = MonsterRoster::Make(MonsterId::JAW_WORM, rng);
        worm.ChooseOpeningMove(rng);

        std::string last;
        int run = 0;

        for (int turn = 0; turn < 60; ++turn)
        {
            const std::string name = worm.GetCurrentMove().name;

            run = name == last ? run + 1 : 1;
            last = name;

            // Chomp and Bellow never come twice in a row, Thrash never three
            // times.
            if (name == "Chomp" || name == "Bellow")
            {
                CHECK(run <= 1);
            }
            else
            {
                CHECK(run <= 2);
            }

            worm.AdvanceMove(rng);
        }
    }
}

TEST_CASE("A Jaw Worm picks its moves by their weights")
{
    const std::map<std::string, int> tally =
        MoveTally(MonsterId::JAW_WORM, 40);

    int total = 0;

    for (const auto& entry : tally)
    {
        total += entry.second;
    }

    REQUIRE(total > 0);
    REQUIRE(tally.size() == 3u);

    // The weights are 25 Chomp, 30 Thrash and 45 Bellow, so Bellow comes up
    // most often and every move comes up.
    for (const auto& entry : tally)
    {
        CHECK(entry.second > 0);
    }

    CHECK(tally.at("Bellow") > tally.at("Chomp"));
}

TEST_CASE("A Cultist sets up its Ritual and then gains Strength every turn")
{
    Battle battle = BattleAgainst({ Make(MonsterId::CULTIST) });

    REQUIRE(battle.GetMonsters()[0].GetCurrentMove().name == "Incantation");

    // The Incantation itself hands over the Ritual, which pays out from then
    // on.
    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::RITUAL) == 3);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::STRENGTH) == 3);
    CHECK(battle.GetPlayer().GetHealth() == 80);

    // 6 from Dark Strike and 3 from the Ritual.
    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetHealth() == 71);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::STRENGTH) == 6);
}

TEST_CASE("A Louse curls up the first time it is hit")
{
    Battle battle = BattleAgainst({ Make(MonsterId::RED_LOUSE) });

    const int curl = battle.GetMonsters()[0].GetPower(PowerType::CURL_UP);

    REQUIRE(curl >= 3);
    REQUIRE(curl <= 7);

    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetMonsters()[0].GetPower(PowerType::CURL_UP) == 0);
    CHECK(battle.GetMonsters()[0].GetBlock() == curl);

    // Only the first hit.
    const int blockLeft = battle.GetMonsters()[0].GetBlock();

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetBlock() < blockLeft);
}

TEST_CASE("A Mad Gremlin grows angry every time it is hit")
{
    Battle battle = BattleAgainst({ Make(MonsterId::MAD_GREMLIN) });

    REQUIRE(battle.GetMonsters()[0].GetPower(PowerType::ANGRY) == 1);

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::STRENGTH) == 1);

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::STRENGTH) == 2);
}

TEST_CASE("A Gremlin Nob is enraged by every skill played")
{
    Player player("Ironclad", 80);
    player.AddCardToDeck(CardRegistry::Get(CardId::DEFEND_RED));
    player.AddCardToDeck(CardRegistry::Get(CardId::DEFEND_RED));

    Battle battle(std::move(player), { Make(MonsterId::GREMLIN_NOB) }, 31);
    battle.Start();

    REQUIRE(battle.GetMonsters()[0].GetCurrentMove().name == "Bellow");
    REQUIRE(battle.EndTurn() == true);
    REQUIRE(battle.GetMonsters()[0].GetPower(PowerType::ENRAGE) == 2);

    const std::size_t before =
        static_cast<std::size_t>(
            battle.GetMonsters()[0].GetPower(PowerType::STRENGTH));

    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetMonsters()[0].GetPower(PowerType::STRENGTH) ==
          static_cast<int>(before) + 2);
}

TEST_CASE("A Fungi Beast leaves its spores behind when it dies")
{
    Battle battle = BattleAgainst({ Make(MonsterId::FUNGI_BEAST) }, 10, 80);

    REQUIRE(battle.GetMonsters()[0].GetPower(PowerType::SPORE_CLOUD) == 2);

    // Beat it down over as many turns as it takes.
    while (!battle.IsDone())
    {
        const std::vector<std::size_t> playable =
            battle.GetPlayableCardIndices();

        if (playable.empty())
        {
            battle.EndTurn();
            continue;
        }

        battle.PlayCard(playable.front());
    }

    CHECK(battle.GetPhase() == BattlePhase::WON);
    CHECK(battle.GetPlayer().GetPower(PowerType::VULNERABLE) == 2);
}

TEST_CASE("A Red Slaver can tie up the attacks in hand")
{
    Battle battle = BattleAgainst({ Make(MonsterId::RED_SLAVER) });

    battle.GetPlayer().AddPower(PowerType::ENTANGLED, 1);

    CHECK(battle.CanPlay(0) == false);
    CHECK(battle.GetPlayableCardIndices().empty());

    REQUIRE(battle.EndTurn() == true);

    // Entangled only holds for the one turn.
    CHECK(battle.GetPlayer().GetPower(PowerType::ENTANGLED) == 0);
    CHECK(battle.CanPlay(0) == true);
}

TEST_CASE("A Red Slaver stabs oftener than it scrapes once it has entangled")
{
    const Monster slaver = Make(MonsterId::RED_SLAVER);
    int stab = 0;
    int scrape = 0;

    for (const MonsterMove& move : slaver.GetMoves())
    {
        // Only the pair that waits on the entangle: the walk before it is
        // gated on the turn, not on the weights.
        if (move.afterMove != "Entangle")
        {
            continue;
        }

        if (move.name == "Stab")
        {
            stab = move.weight;
        }
        else if (move.name == "Scrape")
        {
            scrape = move.weight;
        }
    }

    // Fifty-five to the stab, forty-five to the scrape. Turning the two round
    // makes the climber vulnerable oftener than the game does.
    CHECK(stab == 55);
    CHECK(scrape == 45);
}

TEST_CASE("A Looter walks out of the fight")
{
    Battle battle = BattleAgainst({ Make(MonsterId::LOOTER) });

    // Mug, mug, and then whichever way the coin went: lunge, smoke, away, or
    // smoke and straight away.
    for (int turn = 0; turn < 2; ++turn)
    {
        REQUIRE(battle.GetMonsters()[0].GetCurrentMove().name == "Mug");
        REQUIRE(battle.EndTurn() == true);
    }

    const std::string third = battle.GetMonsters()[0].GetCurrentMove().name;

    REQUIRE((third == "Lunge" || third == "Smoke Bomb"));
    REQUIRE(battle.EndTurn() == true);

    if (third == "Lunge")
    {
        REQUIRE(battle.GetMonsters()[0].GetCurrentMove().name == "Smoke Bomb");
        REQUIRE(battle.EndTurn() == true);
    }

    REQUIRE(battle.GetMonsters()[0].GetCurrentMove().name == "Escape");
    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetMonsters()[0].HasEscaped() == true);
    CHECK(battle.GetMonsters()[0].IsGone() == true);

    // Nothing is left to fight, so the battle is over.
    CHECK(battle.GetPhase() == BattlePhase::WON);
}

TEST_CASE("A thief tosses its coin on the third turn, not when it is made")
{
    for (const MonsterId id : { MonsterId::LOOTER, MonsterId::MUGGER })
    {
        std::map<std::string, int> third;
        const int rounds = 400;

        // The same thief every time - one seed for the making of it - and a
        // different generator driving the turns. If the coin were tossed when
        // the thief was made, every one of these would go the same way.
        for (int round = 0; round < rounds; ++round)
        {
            Monster thief = Make(id);
            std::mt19937 rng(static_cast<unsigned int>(round) + 1u);
            MoveContext context;

            // The move of turn one, chosen with nothing behind it.
            context.turn = 0;
            thief.ChooseOpeningMove(rng, context);

            const auto step = [&thief, &rng, &context](int turn) {
                // What a battle does at the end of a monster's turn: the move
                // is written down as made, and then the next one is chosen.
                // The gates that ask whether the lunge happened read that
                // count, so a bare walk that skips it sees no lunge ever.
                thief.CountMoveUsed();
                context.turn = turn - 1;
                thief.AdvanceMove(rng, context);
            };

            CHECK(thief.GetCurrentMove().name == "Mug");
            step(2);
            CHECK(thief.GetCurrentMove().name == "Mug");
            step(3);
            ++third[thief.GetCurrentMove().name];

            // And what follows the coin follows from the coin: a lunge is
            // paid for with an extra turn before the thief can go.
            const bool lunged = thief.GetCurrentMove().name == "Lunge";

            step(4);

            if (lunged)
            {
                CHECK(thief.GetCurrentMove().name == "Smoke Bomb");
                step(5);
            }

            CHECK(thief.GetCurrentMove().name == "Escape");
        }

        REQUIRE(third.count("Lunge") == 1u);
        REQUIRE(third.count("Smoke Bomb") == 1u);
        CHECK(third.size() == 2u);
        CHECK(third["Lunge"] + third["Smoke Bomb"] == rounds);

        // A coin, so near enough half each.
        const double share = 100.0 * third["Lunge"] / rounds;

        CHECK(share > 42.0);
        CHECK(share < 58.0);
    }
}

TEST_CASE("A Gremlin Wizard charges before it blasts")
{
    Battle battle = BattleAgainst({ Make(MonsterId::GREMLIN_WIZARD) }, 10, 80);

    const char* order[] = { "Charging",       "Charging", "Ultimate Blast",
                            "Charging",       "Charging", "Charging",
                            "Ultimate Blast", "Charging" };

    for (const char* name : order)
    {
        CHECK(battle.GetMonsters()[0].GetCurrentMove().name == name);
        REQUIRE(battle.EndTurn() == true);
    }
}

TEST_CASE("A Shield Gremlin covers its friend, and hits once it is alone")
{
    Battle battle = BattleAgainst(
        { Make(MonsterId::SHIELD_GREMLIN), Make(MonsterId::SNEAKY_GREMLIN) });

    REQUIRE(battle.GetMonsters()[0].GetCurrentMove().name == "Protect");
    REQUIRE(battle.EndTurn() == true);

    // The block went on the other gremlin.
    CHECK(battle.GetMonsters()[1].GetBlock() == 7);
    CHECK(battle.GetMonsters()[0].GetBlock() == 0);

    // Alone it stops shielding and starts hitting. Shielding itself instead
    // left the last gremlin standing as a wall that threatened nothing, and
    // a fight that threatens nothing can be waited out.
    Battle alone = BattleAgainst({ Make(MonsterId::SHIELD_GREMLIN) });

    REQUIRE(alone.GetMonsters()[0].GetCurrentMove().name == "Shield Bash");

    const int before = alone.GetPlayer().GetHealth();

    REQUIRE(alone.EndTurn() == true);

    CHECK(alone.GetMonsters()[0].GetBlock() == 0);
    CHECK(alone.GetPlayer().GetHealth() < before);
}

TEST_CASE("A Bronze Orb's beam props up the automaton and never another orb")
{
    // Three orbs standing with the thing they were spawned by, every one of
    // them beaming. The block is the automaton's, all of it: an orb propping
    // up an orb is block spent on a thing that is not the fight.
    for (unsigned int seed = 1; seed <= 40u; ++seed)
    {
        Battle battle = BattleAgainst({ Make(MonsterId::BRONZE_AUTOMATON),
                                        Make(MonsterId::BRONZE_ORB),
                                        Make(MonsterId::BRONZE_ORB),
                                        Make(MonsterId::BRONZE_ORB) },
                                      10, 400);

        for (std::size_t at = 1; at < battle.GetMonsters().size(); ++at)
        {
            REQUIRE(battle.GetMonsters()[at].ForceMove("Support Beam") ==
                    true);
        }

        REQUIRE(battle.EndTurn() == true);

        for (const Monster& one : battle.GetMonsters())
        {
            if (one.GetMonsterId() == MonsterId::BRONZE_ORB)
            {
                CHECK(one.GetBlock() == 0);
            }
        }

        // Three beams of twelve, and whatever the automaton put up itself.
        CHECK(battle.GetMonsters()[0].GetBlock() >= 36);
    }
}

TEST_CASE("A Sentry clogs the deck and the pair open the other way round")
{
    Battle battle = BattleAgainst({ Make(MonsterId::SENTRY) });

    REQUIRE(battle.GetMonsters()[0].GetPower(PowerType::ARTIFACT) == 1);
    REQUIRE(battle.GetMonsters()[0].GetCurrentMove().name == "Bolt");

    REQUIRE(battle.EndTurn() == true);

    int dazed = 0;

    for (const auto& card : battle.GetPlayer().GetDiscardPile())
    {
        if (card.GetId() == CardId::DAZED)
        {
            ++dazed;
        }
    }

    CHECK(dazed == 2);
    CHECK(battle.GetMonsters()[0].GetCurrentMove().name == "Beam");
}

TEST_CASE("Lagavulin sleeps behind its Metallicize until it is hit")
{
    Battle battle = BattleAgainst({ Make(MonsterId::LAGAVULIN) });

    REQUIRE(battle.GetMonsters()[0].GetPower(PowerType::ASLEEP) == 3);
    REQUIRE(battle.GetMonsters()[0].GetPower(PowerType::METALLICIZE) == 8);

    // While it sleeps it does nothing at all.
    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetHealth() == 80);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::ASLEEP) == 2);

    // A hit wakes it and takes its Metallicize away.
    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetMonsters()[0].GetPower(PowerType::ASLEEP) == 0);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::METALLICIZE) == 0);
    CHECK(battle.GetMonsters()[0].GetCurrentMove().name == "Stunned");

    // Stunned for a turn, then it swings.
    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetMonsters()[0].GetCurrentMove().name == "Attack");

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetHealth() == 62);
}

TEST_CASE("A large slime steps aside for two smaller ones")
{
    Battle battle = BattleAgainst({ Make(MonsterId::ACID_SLIME_L) }, 40);

    const int full = battle.GetMonsters()[0].GetMaxHealth();

    // Beat it down to half and the split is what it means to do next.
    while (battle.GetMonsters()[0].GetHealth() * 2 > full)
    {
        const std::vector<std::size_t> playable =
            battle.GetPlayableCardIndices();

        if (playable.empty())
        {
            REQUIRE(battle.EndTurn() == true);
            continue;
        }

        REQUIRE(battle.PlayCard(playable.front()) == true);
    }

    REQUIRE(battle.GetMonsters()[0].GetCurrentMove().name == "Split");

    const int left = battle.GetMonsters()[0].GetHealth();

    REQUIRE(battle.EndTurn() == true);

    // The big one is gone and two mediums stand in its place, each with what
    // it had left.
    REQUIRE(battle.GetMonsters().size() == 3u);
    CHECK(battle.GetMonsters()[0].IsGone() == true);
    CHECK(battle.GetMonsters()[1].GetName() == "Acid Slime (M)");
    CHECK(battle.GetMonsters()[2].GetName() == "Acid Slime (M)");
    CHECK(battle.GetMonsters()[1].GetHealth() == left);
    CHECK(battle.GetPhase() != BattlePhase::WON);
}

TEST_CASE("The Slime Boss splits into two large slimes")
{
    Battle battle = BattleAgainst({ Make(MonsterId::SLIME_BOSS) }, 60);

    while (battle.GetMonsters()[0].GetHealth() * 2 > 140)
    {
        const std::vector<std::size_t> playable =
            battle.GetPlayableCardIndices();

        if (playable.empty())
        {
            REQUIRE(battle.EndTurn() == true);
            continue;
        }

        REQUIRE(battle.PlayCard(playable.front()) == true);
    }

    REQUIRE(battle.GetMonsters()[0].GetCurrentMove().name == "Split");
    REQUIRE(battle.EndTurn() == true);

    REQUIRE(battle.GetMonsters().size() == 3u);
    CHECK(battle.GetMonsters()[1].GetName() == "Acid Slime (L)");
    CHECK(battle.GetMonsters()[2].GetName() == "Spike Slime (L)");
}

TEST_CASE("The Guardian pulls into its shell once it has taken enough")
{
    // A Bludgeon comes to 32, which is more than it holds out for.
    Player player("Ironclad", 200);
    player.SetColor(CardColor::RED);

    for (int i = 0; i < 6; ++i)
    {
        player.AddCardToDeck(CardRegistry::Get(CardId::BLUDGEON));
    }

    Battle battle(std::move(player), { Make(MonsterId::THE_GUARDIAN) }, 31);
    battle.Start();

    REQUIRE(battle.GetMonsters()[0].GetPower(PowerType::MODE_SHIFT) == 30);
    REQUIRE(battle.GetMonsters()[0].GetCurrentMove().name == "Charging Up");

    REQUIRE(battle.PlayCard(0) == true);

    CHECK(battle.GetMonsters()[0].GetPower(PowerType::MODE_SHIFT) == 0);
    CHECK(battle.GetMonsters()[0].GetCurrentMove().name == "Defensive Mode");

    REQUIRE(battle.EndTurn() == true);

    // In its shell it hurts whoever swings at it.
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::SHARP_HIDE) == 3);

    const int before = battle.GetPlayer().GetHealth();

    REQUIRE(battle.PlayCard(0) == true);
    CHECK(battle.GetPlayer().GetHealth() == before - 3);

    // Roll Attack, then Twin Slam puts the shell away and winds it up again.
    CHECK(battle.GetMonsters()[0].GetCurrentMove().name == "Roll Attack");

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetMonsters()[0].GetCurrentMove().name == "Twin Slam");

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::SHARP_HIDE) == 0);
    CHECK(battle.GetMonsters()[0].GetPower(PowerType::MODE_SHIFT) == 40);
}

TEST_CASE("Hexaghost opens with Activate and Divider")
{
    Battle battle = BattleAgainst({ Make(MonsterId::HEXAGHOST) }, 10, 84);

    const char* order[] = { "Activate", "Divider", "Sear",   "Tackle",
                            "Sear",     "Inflame", "Tackle", "Sear",
                            "Inferno",  "Sear" };

    CHECK(battle.GetMonsters()[0].GetCurrentMove().name == order[0]);

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetHealth() == 84);

    // Divider hits six times for a seventh of the health plus one: 84 / 12 is
    // 7, so 8 damage a hit.
    CHECK(battle.GetMonsters()[0].GetCurrentMove().name == "Divider");
    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetPlayer().GetHealth() == 36);

    for (std::size_t i = 2; i < 10; ++i)
    {
        CHECK(battle.GetMonsters()[0].GetCurrentMove().name == order[i]);

        if (battle.IsDone())
        {
            break;
        }

        battle.EndTurn();
    }
}

TEST_CASE("The act one encounters hold what they should")
{
    CHECK(EncounterLibrary::GetAct1Weak().size() == 4u);
    // Ten, not eleven: the large slime is one room where a large acid slime
    // and a large spike slime had been written as two, which handed that one
    // room twice the share the spire gives it.
    CHECK(EncounterLibrary::GetAct1Strong().size() == 10u);
    CHECK(EncounterLibrary::GetAct1Elites().size() == 3u);
    CHECK(EncounterLibrary::GetAct1Bosses().size() == 3u);

    std::mt19937 rng(4);

    for (const auto* pool :
         { &EncounterLibrary::GetAct1Weak(),
           &EncounterLibrary::GetAct1Strong(),
           &EncounterLibrary::GetAct1Elites(),
           &EncounterLibrary::GetAct1Bosses() })
    {
        for (const auto& encounter : *pool)
        {
            const std::vector<Monster> monsters =
                EncounterLibrary::Build(encounter, rng);

            CHECK(encounter.name.empty() == false);
            REQUIRE(monsters.empty() == false);

            for (const auto& monster : monsters)
            {
                CHECK(monster.GetMaxHealth() > 0);
            }
        }
    }
}

TEST_CASE("The opening fights are drawn from the weak groups")
{
    std::mt19937 rng(9);

    for (int fight = 0; fight < EncounterLibrary::WEAK_FIGHTS; ++fight)
    {
        const Encounter picked =
            EncounterLibrary::Pick(1, MapNodeType::MONSTER, fight, rng);

        bool fromWeak = false;

        for (const auto& weak : EncounterLibrary::GetAct1Weak())
        {
            if (weak.name == picked.name)
            {
                fromWeak = true;
            }
        }

        CHECK(fromWeak == true);
    }

    // After those it reaches for the harder ones.
    const Encounter later =
        EncounterLibrary::Pick(1, MapNodeType::MONSTER, 5, rng);
    bool fromStrong = false;

    for (const auto& strong : EncounterLibrary::GetAct1Strong())
    {
        if (strong.name == later.name)
        {
            fromStrong = true;
        }
    }

    CHECK(fromStrong == true);

    // An elite place holds an elite, and the boss holds a boss.
    CHECK(EncounterLibrary::Pick(1, MapNodeType::ELITE, 9, rng).type ==
          MonsterType::ELITE);
    CHECK(EncounterLibrary::Pick(1, MapNodeType::BOSS, 9, rng).type ==
          MonsterType::BOSS);
}

TEST_CASE("A run starts the fight that waits where it stands")
{
    Run run(CardColor::RED, 44);

    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);
    REQUIRE(run.GetCurrentNodeType() == MapNodeType::MONSTER);

    Battle battle = run.StartBattleHere();

    CHECK(run.GetFightCount() == 1);
    CHECK(run.GetCurrentEncounter().name.empty() == false);
    REQUIRE(battle.GetMonsters().empty() == false);
    CHECK(battle.GetMonsters()[0].GetMaxHealth() > 0);
    CHECK(battle.GetPlayer().GetHand().size() == 5u);
}
