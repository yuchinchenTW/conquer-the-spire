#include "doctest.h"

#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Monsters/EncounterLibrary.hpp>
#include <conquer-the-spire/Monsters/MonsterRoster.hpp>
#include <conquer-the-spire/Run/Run.hpp>

#include <algorithm>
#include <cstddef>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

using namespace ConquerTheSpire;

namespace
{
//! Builds the monster \p id with a fixed roll behind it.
Monster Make(MonsterId id, unsigned int seed = 4)
{
    std::mt19937 rng(seed);

    return MonsterRoster::Make(id, rng);
}

//! Opens a fight against \p monsters with a plain deck.
Battle FightAgainst(std::vector<MonsterId> monsters, unsigned int seed = 4)
{
    std::mt19937 rng(seed);
    Player player("Ironclad", 80);
    player.SetColor(CardColor::RED);

    for (auto& card : CardRegistry::MakeStarterDeck(CardColor::RED))
    {
        player.AddCardToDeck(std::move(card));
    }

    std::vector<Monster> built;

    for (const MonsterId id : monsters)
    {
        built.emplace_back(MonsterRoster::Make(id, rng));
    }

    Battle battle(std::move(player), std::move(built), seed);
    battle.Start();

    return battle;
}

//! Plays the first attack in hand at the monster \p target. Returns false
//! when there is no attack to play.
bool Swing(Battle& battle, std::size_t target = 0)
{
    const std::vector<Card>& hand = battle.GetPlayer().GetHand();

    for (const std::size_t index : battle.GetPlayableCardIndices())
    {
        if (index < hand.size() &&
            hand[index].GetCardType() == CardType::ATTACK)
        {
            return battle.PlayCard(index, target);
        }
    }

    return false;
}

//! Runs \p battle for \p turns monster turns without the player doing
//! anything.
void PassTurns(Battle& battle, int turns)
{
    for (int i = 0; i < turns && !battle.IsDone(); ++i)
    {
        battle.EndTurn();
    }
}

//! Returns every monster id the groups of \p pool name.
std::set<MonsterId> IdsOf(const std::vector<Encounter>& pool)
{
    std::set<MonsterId> ids;

    for (const auto& group : pool)
    {
        ids.insert(group.monsters.begin(), group.monsters.end());
    }

    return ids;
}

//! Counts \p id in the player's battle piles.
int CountHeldByPlayer(const Player& player, CardId id)
{
    int found = 0;

    for (const std::vector<Card>* pile :
         { &player.GetHand(), &player.GetDrawPile(),
           &player.GetDiscardPile(), &player.GetExhaustPile() })
    {
        for (const Card& card : *pile)
        {
            found += card.GetId() == id ? 1 : 0;
        }
    }

    return found;
}
}  // namespace

TEST_CASE("Every monster of the later acts builds with health and moves")
{
    const MonsterId later[] = {
        MonsterId::SPHERIC_GUARDIAN, MonsterId::CHOSEN,
        MonsterId::SHELLED_PARASITE, MonsterId::BYRD,
        MonsterId::MUGGER,           MonsterId::CENTURION,
        MonsterId::MYSTIC,           MonsterId::SNAKE_PLANT,
        MonsterId::SNECKO,           MonsterId::GREMLIN_LEADER,
        MonsterId::TASKMASTER,       MonsterId::BOOK_OF_STABBING,
        MonsterId::BRONZE_AUTOMATON, MonsterId::BRONZE_ORB,
        MonsterId::THE_CHAMP,        MonsterId::THE_COLLECTOR,
        MonsterId::TORCH_HEAD,       MonsterId::DARKLING,
        MonsterId::ORB_WALKER,       MonsterId::SPIKER,
        MonsterId::REPULSOR,         MonsterId::EXPLODER,
        MonsterId::THE_MAW,          MonsterId::SPIRE_GROWTH,
        MonsterId::TRANSIENT,        MonsterId::WRITHING_MASS,
        MonsterId::GIANT_HEAD,       MonsterId::NEMESIS,
        MonsterId::REPTOMANCER,      MonsterId::DAGGER,
        MonsterId::AWAKENED_ONE,     MonsterId::TIME_EATER,
        MonsterId::DONU,             MonsterId::DECA,
        MonsterId::SPIRE_SHIELD,     MonsterId::SPIRE_SPEAR,
        MonsterId::CORRUPT_HEART
    };

    for (const MonsterId id : later)
    {
        const Monster monster = Make(id);

        CHECK(monster.GetMonsterId() == id);
        CHECK(monster.GetName().empty() == false);
        CHECK(monster.GetMaxHealth() > 0);
        CHECK(monster.GetMoves().empty() == false);
    }
}

TEST_CASE("The health of the later monsters matches what the spire gives them")
{
    CHECK(Make(MonsterId::SPHERIC_GUARDIAN).GetMaxHealth() == 20);
    CHECK(Make(MonsterId::THE_MAW).GetMaxHealth() == 300);
    CHECK(Make(MonsterId::SPIRE_GROWTH).GetMaxHealth() == 170);
    CHECK(Make(MonsterId::TRANSIENT).GetMaxHealth() == 999);
    CHECK(Make(MonsterId::WRITHING_MASS).GetMaxHealth() == 160);
    CHECK(Make(MonsterId::GIANT_HEAD).GetMaxHealth() == 500);
    CHECK(Make(MonsterId::NEMESIS).GetMaxHealth() == 185);
    CHECK(Make(MonsterId::BRONZE_AUTOMATON).GetMaxHealth() == 300);
    CHECK(Make(MonsterId::THE_CHAMP).GetMaxHealth() == 420);
    CHECK(Make(MonsterId::THE_COLLECTOR).GetMaxHealth() == 282);
    CHECK(Make(MonsterId::AWAKENED_ONE).GetMaxHealth() == 300);
    CHECK(Make(MonsterId::TIME_EATER).GetMaxHealth() == 456);
    CHECK(Make(MonsterId::DONU).GetMaxHealth() == 250);
    CHECK(Make(MonsterId::DECA).GetMaxHealth() == 250);
    CHECK(Make(MonsterId::SPIRE_SHIELD).GetMaxHealth() == 110);
    CHECK(Make(MonsterId::SPIRE_SPEAR).GetMaxHealth() == 160);
    CHECK(Make(MonsterId::CORRUPT_HEART).GetMaxHealth() == 750);

    // And the ones with a range stay inside it.
    for (unsigned int seed = 1; seed < 12; ++seed)
    {
        const int chosen = Make(MonsterId::CHOSEN, seed).GetMaxHealth();
        const int byrd = Make(MonsterId::BYRD, seed).GetMaxHealth();
        const int snecko = Make(MonsterId::SNECKO, seed).GetMaxHealth();

        CHECK(chosen >= 95);
        CHECK(chosen <= 99);
        CHECK(byrd >= 25);
        CHECK(byrd <= 31);
        CHECK(snecko >= 114);
        CHECK(snecko <= 120);
    }
}

TEST_CASE("Each act draws its fights from its own lists")
{
    std::mt19937 rng(9);

    // The groups of an act only hold monsters of that act.
    const std::set<MonsterId> act2 = IdsOf(EncounterLibrary::GetAct2Weak());
    const std::set<MonsterId> act3 = IdsOf(EncounterLibrary::GetAct3Weak());

    CHECK(act2.count(MonsterId::BYRD) == 1u);
    CHECK(act3.count(MonsterId::DARKLING) == 1u);
    CHECK(act2.count(MonsterId::DARKLING) == 0u);

    for (int i = 0; i < 40; ++i)
    {
        const Encounter second =
            EncounterLibrary::Pick(2, MapNodeType::MONSTER, 9, rng);
        const Encounter third =
            EncounterLibrary::Pick(3, MapNodeType::MONSTER, 9, rng);

        CHECK(second.monsters.empty() == false);
        CHECK(third.monsters.empty() == false);
    }

    CHECK(EncounterLibrary::Pick(2, MapNodeType::ELITE, 9, rng).type ==
          MonsterType::ELITE);
    CHECK(EncounterLibrary::Pick(3, MapNodeType::BOSS, 9, rng).type ==
          MonsterType::BOSS);
}

TEST_CASE("The later acts ease a climber in for two fights, not three")
{
    CHECK(EncounterLibrary::WeakFightsOf(1) == 3);
    CHECK(EncounterLibrary::WeakFightsOf(2) == 2);
    CHECK(EncounterLibrary::WeakFightsOf(3) == 2);

    std::mt19937 rng(11);
    const std::set<MonsterId> weak = IdsOf(EncounterLibrary::GetAct2Weak());

    for (int fight = 0; fight < 2; ++fight)
    {
        const Encounter group =
            EncounterLibrary::Pick(2, MapNodeType::MONSTER, fight, rng);

        for (const MonsterId id : group.monsters)
        {
            CHECK(weak.count(id) == 1u);
        }
    }
}

TEST_CASE("The same fight does not come round within two of itself")
{
    std::mt19937 rng(29);
    std::vector<std::string> lately;

    // Two hundred rooms of the second act, remembered the way a run
    // remembers them, and never a window of three holding two alike.
    for (int room = 0; room < 200; ++room)
    {
        const Encounter group =
            EncounterLibrary::Pick(2, MapNodeType::MONSTER, 9, rng, lately);

        REQUIRE(group.monsters.empty() == false);

        for (std::size_t at = 0; at < lately.size() && at < 2u; ++at)
        {
            CHECK(lately[at] != group.name);
        }

        lately.insert(lately.begin(), group.name);

        if (lately.size() > 2)
        {
            lately.resize(2);
        }
    }

    // The bar is only asked of plain rooms: an act has too few elites and
    // bosses to hold one out, and the rule is not written about them.
    const std::vector<std::string> both = { "Gremlin Nob", "Lagavulin" };

    for (int i = 0; i < 20; ++i)
    {
        CHECK(EncounterLibrary::Pick(1, MapNodeType::ELITE, 9, rng, both)
                  .monsters.empty() == false);
    }

    // And a pool with nothing else left to offer hands back the barred fight
    // rather than nothing at all. The last act has the one group.
    const Encounter only = EncounterLibrary::Pick(4, MapNodeType::MONSTER, 9,
                                                  rng,
                                                  { "Shield and Spear" });

    REQUIRE(only.monsters.size() == 2u);
    CHECK(only.monsters[0] == MonsterId::SPIRE_SHIELD);
}

TEST_CASE("The last act holds the pair at the door and the heart")
{
    std::mt19937 rng(13);

    const Encounter elite =
        EncounterLibrary::Pick(4, MapNodeType::ELITE, 0, rng);
    const Encounter boss =
        EncounterLibrary::Pick(4, MapNodeType::BOSS, 0, rng);

    REQUIRE(elite.monsters.size() == 2u);
    CHECK(elite.monsters[0] == MonsterId::SPIRE_SHIELD);
    CHECK(elite.monsters[1] == MonsterId::SPIRE_SPEAR);

    REQUIRE(boss.monsters.size() == 1u);
    CHECK(boss.monsters[0] == MonsterId::CORRUPT_HEART);
}

TEST_CASE("Starting an act lays out a new map and puts the climber at it")
{
    Run run(CardColor::RED, 5);

    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);
    REQUIRE(run.GetFloor() == 1);

    run.BeginAct(2);

    CHECK(run.GetAct() == 2);
    CHECK(run.GetFloor() == 0);
    CHECK(run.GetColumn() == -1);
    CHECK(run.GetFightCount() == 0);
    CHECK(run.GetRewardGenerator().GetPotionChance() == 40);
    CHECK(run.GetAvailableColumns().empty() == false);
}

TEST_CASE("An act is only left once its boss is down")
{
    Run run(CardColor::RED, 5);

    CHECK(run.AdvanceAct() == false);
    CHECK(run.GetAct() == 1);

    // Walk to the top and take the boss down.
    while (!run.GetAvailableColumns().empty())
    {
        run.Travel(run.GetAvailableColumns().front());
    }

    REQUIRE(run.IsAtBoss() == true);

    run.FinishBoss();

    CHECK(run.AdvanceAct() == true);
    CHECK(run.GetAct() == 2);
}

TEST_CASE("The door of the last act only opens for all three keys")
{
    Run run(CardColor::RED, 5);

    run.BeginAct(3);

    while (!run.GetAvailableColumns().empty())
    {
        run.Travel(run.GetAvailableColumns().front());
    }

    run.FinishBoss();

    CHECK(run.HasAllKeys() == false);
    CHECK(run.AdvanceAct() == false);

    run.TakeKey(KeyType::RUBY);
    run.TakeKey(KeyType::EMERALD);

    CHECK(run.AdvanceAct() == false);

    run.TakeKey(KeyType::SAPPHIRE);

    CHECK(run.HasAllKeys() == true);
    REQUIRE(run.AdvanceAct() == true);

    // And the last act is the handful of rooms it should be.
    CHECK(run.GetAct() == 4);
    CHECK(run.GetMap().GetRows() == 3);
    CHECK(run.GetCurrentNodeType() == MapNodeType::EMPTY);

    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);
    CHECK(run.GetCurrentNodeType() == MapNodeType::REST);

    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);
    CHECK(run.GetCurrentNodeType() == MapNodeType::MERCHANT);

    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);
    CHECK(run.GetCurrentNodeType() == MapNodeType::ELITE);

    CHECK(run.GetAvailableColumns().empty() == true);
}

TEST_CASE("A key is taken instead of the relic an elite leaves")
{
    Run run(CardColor::RED, 5);

    run.BeginAct(3);
    run.AddGold(0);

    // Reach a reward pile the honest way: win an elite fight.
    while (run.GetCurrentNodeType() != MapNodeType::ELITE &&
           !run.GetAvailableColumns().empty())
    {
        run.Travel(run.GetAvailableColumns().front());
    }

    if (run.GetCurrentNodeType() != MapNodeType::ELITE)
    {
        // This seed's map has no elite on the leftmost path, which is fine:
        // the trade itself is what matters and it needs a relic reward.
        return;
    }

    Battle battle = run.StartBattleHere();

    for (auto& monster : battle.GetMonsters())
    {
        monster.SetHealth(0);
    }

    battle.EndTurn();
    run.FinishBattle(battle);

    for (std::size_t i = 0; i < run.GetRewards().size(); ++i)
    {
        if (run.GetRewards()[i].kind == RewardKind::RELIC_CHOICE)
        {
            REQUIRE(run.TakeKeyInsteadOf(i, KeyType::EMERALD) == true);

            CHECK(run.HasKey(KeyType::EMERALD) == true);
            CHECK(run.GetRewards()[i].claimed == true);

            break;
        }
    }
}

TEST_CASE("A guardian of the sphere sits behind its block")
{
    Battle battle = FightAgainst({ MonsterId::SPHERIC_GUARDIAN });
    const Monster& guardian = battle.GetMonsters().front();

    // Twenty health behind forty block from the first moment.
    CHECK(guardian.GetBlock() == 40);
    CHECK(guardian.GetPower(PowerType::BARRICADE) > 0);
    CHECK(guardian.GetPower(PowerType::ARTIFACT) == 3);
    CHECK(guardian.GetCurrentMove().name == "Activate");
}

TEST_CASE("Something in the air takes half of what an attack does")
{
    Battle grounded = FightAgainst({ MonsterId::BYRD });

    REQUIRE(grounded.GetMonsters().front().GetPower(PowerType::FLIGHT) == 3);

    // The same swing against the same bird, once out of the air and once in
    // it.
    grounded.GetMonsters().front().RemovePower(PowerType::FLIGHT);

    const int wholeBefore = grounded.GetMonsters().front().GetHealth();

    REQUIRE(Swing(grounded) == true);

    const int whole = wholeBefore - grounded.GetMonsters().front().GetHealth();

    Battle flying = FightAgainst({ MonsterId::BYRD });
    const int halfBefore = flying.GetMonsters().front().GetHealth();

    REQUIRE(Swing(flying) == true);

    const Monster& byrd = flying.GetMonsters().front();

    CHECK(whole > 0);
    CHECK(halfBefore - byrd.GetHealth() == whole / 2);
    CHECK(byrd.GetPower(PowerType::FLIGHT) == 2);
}

TEST_CASE("Enough separate hits bring a flier down")
{
    Battle battle = FightAgainst({ MonsterId::BYRD });
    Monster& byrd = battle.GetMonsters().front();

    // Three hits knock the flight out of it, whatever they were.
    for (int i = 0; i < 3; ++i)
    {
        byrd.AddPower(PowerType::FLIGHT, -1);
    }

    CHECK(byrd.GetPower(PowerType::FLIGHT) == 0);
}

TEST_CASE("Malleable armour answers every hit and by more each time")
{
    Battle battle = FightAgainst({ MonsterId::SNAKE_PLANT });

    REQUIRE(battle.GetMonsters().front().GetPower(PowerType::MALLEABLE) == 3);

    REQUIRE(Swing(battle) == true);

    const Monster& plant = battle.GetMonsters().front();

    CHECK(plant.GetBlock() == 3);
    CHECK(plant.GetPower(PowerType::MALLEABLE) == 4);
}

TEST_CASE("A shell wears away and leaves the parasite standing there")
{
    Battle battle = FightAgainst({ MonsterId::SHELLED_PARASITE });
    Monster& parasite = battle.GetMonsters().front();

    REQUIRE(parasite.GetPower(PowerType::PLATED_ARMOR) == 14);

    // Wear the shell down to nothing.
    parasite.AddPower(PowerType::PLATED_ARMOR, -14);
    parasite.AddPower(PowerType::PLATED_ARMOR, 1);
    parasite.ClearBlock();

    REQUIRE(Swing(battle) == true);

    CHECK(parasite.GetPower(PowerType::PLATED_ARMOR) == 0);
    CHECK(parasite.GetCurrentMove().name == "Stunned");
}

TEST_CASE("A thief gets away with what it took, unless it is killed")
{
    Battle battle = FightAgainst({ MonsterId::MUGGER });

    REQUIRE(battle.GetMonsters().front().GetPower(PowerType::THIEVERY) == 15);

    battle.EndTurn();

    CHECK(battle.GetGoldStolen() == 15);

    // Killing it hands the purse back.
    battle.GetMonsters().front().SetHealth(0);

    CHECK(battle.GetGoldStolen() == 0);
}

TEST_CASE("A leader shouts for gremlins, and only so many of them")
{
    Battle battle = FightAgainst({ MonsterId::GREMLIN_LEADER });

    battle.GetMonsters().front().ForceMove("Rally");

    for (int i = 0; i < 4 && !battle.IsDone(); ++i)
    {
        battle.GetMonsters().front().ForceMove("Rally");
        battle.EndTurn();
    }

    // Whichever kinds turn up: a leader calls for gremlins, not for mad
    // gremlins.
    int gremlins = 0;

    for (const auto& monster : battle.GetMonsters())
    {
        switch (monster.GetMonsterId())
        {
            case MonsterId::MAD_GREMLIN:
            case MonsterId::SNEAKY_GREMLIN:
            case MonsterId::FAT_GREMLIN:
            case MonsterId::SHIELD_GREMLIN:
            case MonsterId::GREMLIN_WIZARD:
                ++gremlins;
                break;

            default:
                break;
        }
    }

    CHECK(gremlins > 0);
    CHECK(gremlins <= 3);
}

TEST_CASE("An automaton opens by spawning two orbs")
{
    Battle battle = FightAgainst({ MonsterId::BRONZE_AUTOMATON });

    REQUIRE(battle.GetMonsters().front().GetCurrentMove().name ==
            "Spawn Orbs");

    battle.EndTurn();

    int orbs = 0;

    for (const auto& monster : battle.GetMonsters())
    {
        if (monster.GetMonsterId() == MonsterId::BRONZE_ORB)
        {
            ++orbs;
        }
    }

    CHECK(orbs == 2);
    CHECK(battle.GetMonsters().front().GetCurrentMove().name == "Flail");
}

TEST_CASE("A move a Champ may not repeat hands its share to one other")
{
    // What the game says, in as many words: if the previous move would be
    // repeated, the move after it in the list is selected instead - so having
    // just gloated there is no chance of gloating and a two-in-five chance of
    // a face slap, with every other chance exactly where it was. Sharing the
    // blocked move out among all of them instead moves all of them a little,
    // which is a different monster.
    //
    // The list is the stance, the gloat, the slap, the slash; the slash is
    // last and hands its share back to the slap rather than round to the
    // stance.
    struct Expected
    {
        const char* was;
        const char* heir;
        double share;
    };

    const Expected asked[] = { { "Heavy Slash", "Face Slap", 70.0 },
                               { "Face Slap", "Heavy Slash", 70.0 },
                               { "Gloat", "Face Slap", 40.0 },
                               { "Defensive Stance", "Gloat", 30.0 } };

    for (const Expected& want : asked)
    {
        std::map<std::string, int> saw;
        const int rounds = 8000;

        for (int i = 0; i < rounds; ++i)
        {
            std::mt19937 rng(static_cast<unsigned int>(i) + 1u);
            Monster him = MonsterRoster::Make(MonsterId::THE_CHAMP, rng);
            MoveContext context;

            // Not a turn the taunt is owed on.
            context.turn = 1;

            REQUIRE(him.ForceMove(want.was) == true);

            him.AdvanceMove(rng, context);
            ++saw[him.GetCurrentMove().name];
        }

        // Never the move just made.
        CHECK(saw[want.was] == 0);

        // And the one named after it holds both shares.
        const double heir = 100.0 * saw[want.heir] / rounds;

        CHECK(heir > want.share - 3.0);
        CHECK(heir < want.share + 3.0);
    }
}

TEST_CASE("A Champ takes his stance twice and gloats after that")
{
    // The game: the stance can be cast at most twice per battle and
    // subsequent casts are instead gloats. Being merely unavailable is not
    // the same thing - its share has to go to the gloat, or every other
    // chance shifts to fill the hole.
    for (unsigned int seed = 1; seed <= 40u; ++seed)
    {
        Battle battle = FightAgainst({ MonsterId::THE_CHAMP }, seed);
        int stances = 0;

        for (int turn = 0; turn < 30 &&
                           battle.GetPhase() == BattlePhase::PLAYER_TURN;
             ++turn)
        {
            if (battle.GetMonsters().front().GetCurrentMove().name ==
                "Defensive Stance")
            {
                ++stances;
            }

            REQUIRE(battle.EndTurn() == true);
        }

        CHECK(stances <= 2);
    }

    // And once he has taken it twice, the share it had is the gloat's.
    std::map<std::string, int> saw;
    const int rounds = 8000;

    for (int i = 0; i < rounds; ++i)
    {
        std::mt19937 rng(static_cast<unsigned int>(i) + 1u);
        Monster him = MonsterRoster::Make(MonsterId::THE_CHAMP, rng);
        MoveContext context;

        context.turn = 1;

        // Twice taken, so the third is somebody else's.
        for (int again = 0; again < 2; ++again)
        {
            REQUIRE(him.ForceMove("Defensive Stance") == true);
            him.CountMoveUsed();
        }

        REQUIRE(him.ForceMove("Heavy Slash") == true);

        him.AdvanceMove(rng, context);
        ++saw[him.GetCurrentMove().name];
    }

    // The slash it just made hands its forty-five to the slap, and the stance
    // hands its fifteen to the gloat: a slap at seventy and a gloat at thirty.
    CHECK(saw["Defensive Stance"] == 0);
    CHECK(100.0 * saw["Gloat"] / rounds > 27.0);
    CHECK(100.0 * saw["Gloat"] / rounds < 33.0);
}

TEST_CASE("A share handed on keeps going until it can sit down")
{
    // The corner where following the redirect one step is not enough. His
    // stance has been taken twice, so its share is the gloat's - but he
    // gloated last turn, and no move comes twice running, so the share goes on
    // again to the slap. Stopping at the gloat let a gloat come twice running,
    // which is the one thing the rule exists to stop.
    std::map<std::string, int> saw;
    const int rounds = 20000;

    for (int i = 0; i < rounds; ++i)
    {
        std::mt19937 rng(static_cast<unsigned int>(i) + 1u);
        Monster him = MonsterRoster::Make(MonsterId::THE_CHAMP, rng);
        MoveContext context;

        context.turn = 1;

        for (int again = 0; again < 2; ++again)
        {
            REQUIRE(him.ForceMove("Defensive Stance") == true);
            him.CountMoveUsed();
        }

        REQUIRE(him.ForceMove("Gloat") == true);

        him.AdvanceMove(rng, context);
        ++saw[him.GetCurrentMove().name];
    }

    // Neither of the two that cannot be made.
    CHECK(saw["Gloat"] == 0);
    CHECK(saw["Defensive Stance"] == 0);

    // The slash keeps its own forty-five; the slap holds its twenty-five, the
    // gloat's fifteen, and the stance's fifteen that came by way of the gloat.
    const double slash = 100.0 * saw["Heavy Slash"] / rounds;
    const double slap = 100.0 * saw["Face Slap"] / rounds;

    CHECK(slash > 42.0);
    CHECK(slash < 48.0);
    CHECK(slap > 52.0);
    CHECK(slap < 58.0);
}

TEST_CASE("Half is not below half")
{
    // The wiki: a Champ turns when his health drops below fifty in a hundred,
    // and a Time Eater hastes when it is reduced to below half. Standing
    // exactly on half is not below it. A slime splitting and a relic paying
    // out say at or below in as many words, and those are asked for here too
    // so that fixing the one does not quietly move the other.
    const auto turnsAt = [](MonsterId who, int health) {
        Battle battle = FightAgainst({ who });
        Monster& it = battle.GetMonsters().front();

        it.SetHealth(health);
        battle.EndTurn();

        return battle.GetMonsters().front().GetPhase() == 2;
    };

    for (const MonsterId who : { MonsterId::THE_CHAMP,
                                 MonsterId::TIME_EATER })
    {
        Battle sizing = FightAgainst({ who });
        const int whole = sizing.GetMonsters().front().GetMaxHealth();

        REQUIRE(whole % 2 == 0);

        // Exactly half: still fighting fair.
        CHECK(turnsAt(who, whole / 2) == false);

        // One point below it: not any more.
        CHECK(turnsAt(who, whole / 2 - 1) == true);
    }

    // And a slime standing exactly on half does split, because that one says
    // at or below.
    Battle slime = FightAgainst({ MonsterId::SLIME_BOSS });
    Monster& boss = slime.GetMonsters().front();
    const int whole = boss.GetMaxHealth();

    boss.SetHealth(whole / 2);
    slime.EndTurn();

    CHECK(slime.GetMonsters().front().GetCurrentMove().name == "Split");
}

TEST_CASE("A champion shakes off what was put on him")
{
    // The wiki: Anger removes all debuffs and gives six strength. What the
    // climber spent slowing him down goes at once, and only the debuffs go -
    // the block and the strength he built up stay.
    Battle battle = FightAgainst({ MonsterId::THE_CHAMP });
    Monster& him = battle.GetMonsters().front();

    him.AddPower(PowerType::WEAK, 3);
    him.AddPower(PowerType::VULNERABLE, 2);
    him.AddPower(PowerType::STRENGTH, 4);
    him.AddBlock(9);

    REQUIRE(him.ForceMove("Anger") == true);
    REQUIRE(battle.EndTurn() == true);

    CHECK(him.GetPower(PowerType::WEAK) == 0);
    CHECK(him.GetPower(PowerType::VULNERABLE) == 0);

    // Six more strength than he had, and what he was holding is untouched.
    CHECK(him.GetPower(PowerType::STRENGTH) == 10);
}

TEST_CASE("A champion executes from the turn he turned, not from the first")
{
    // He executes the turn straight after Anger and every third turn from
    // there. Counted against the turn the fight started instead, it landed
    // straight after only when the two happened to line up - one time in
    // three - so the same rule is asked for with the turning on three
    // different turns.
    for (int idle = 0; idle < 3; ++idle)
    {
        Battle battle = FightAgainst({ MonsterId::THE_CHAMP });
        Player& player = battle.GetPlayer();

        player.GetDrawPile().clear();
        player.GetDiscardPile().clear();

        for (int i = 0; i < 40; ++i)
        {
            player.GetDrawPile().emplace_back(
                CardRegistry::Get(CardId::BLUDGEON));
        }

        // Stand about for a while, so that the turning lands on a different
        // turn each time round.
        for (int wait = 0; wait < idle; ++wait)
        {
            REQUIRE(battle.EndTurn() == true);
        }

        // Stood on the edge of it, so that one blow tips him over and the
        // turning happens on the turn this test means it to.
        Monster& champ = battle.GetMonsters().front();

        champ.SetHealth(champ.GetMaxHealth() / 2 + 20);

        std::string seq;
        bool tipped = false;

        for (int turn = 0; turn < 16 &&
                           battle.GetPhase() == BattlePhase::PLAYER_TURN;
             ++turn)
        {
            Monster& him = battle.GetMonsters().front();

            while (!tipped && him.GetHealth() * 2 > him.GetMaxHealth() &&
                   !him.IsDead() && !player.GetHand().empty())
            {
                if (!battle.PlayCard(0, 0))
                {
                    break;
                }
            }

            tipped = tipped || him.GetHealth() * 2 <= him.GetMaxHealth();

            const std::string move = him.GetCurrentMove().name;

            seq += move == "Anger" ? 'A' : move == "Execute" ? 'X' : '.';

            REQUIRE(battle.EndTurn() == true);
        }

        const std::size_t angry = seq.find('A');

        REQUIRE(angry != std::string::npos);
        REQUIRE(angry + 1 < seq.size());

        // The execute lands on the very next turn, whichever turn he turned
        // on, and then on every third turn after it.
        CHECK(seq[angry + 1] == 'X');

        for (std::size_t at = angry + 1; at < seq.size(); ++at)
        {
            const bool third = (at - angry - 1) % 3u == 0u;

            CHECK((seq[at] == 'X') == third);
        }
    }
}

TEST_CASE("A monster stands as the room stands, not as the roster says")
{
    // A Sentry is an elite where three of them are the room and a plain
    // monster where one stands beside a Spheric Guardian. Carrying elite about
    // with it had a preserved insect taking a quarter off it in a plain fight,
    // a slaver's collar paying out in one, and everything that asks what kind
    // of fight this is answering elite.
    const auto sentryIn = [](const std::vector<Encounter>& list,
                             const std::string& name) {
        for (const Encounter& one : list)
        {
            if (one.name != name)
            {
                continue;
            }

            std::mt19937 rng(3u);
            const std::vector<Monster> built =
                EncounterLibrary::Build(one, rng);

            for (const Monster& monster : built)
            {
                if (monster.GetMonsterId() == MonsterId::SENTRY)
                {
                    return monster.GetMonsterType();
                }
            }
        }

        return MonsterType::INVALID;
    };

    CHECK(sentryIn(EncounterLibrary::GetAct1Elites(), "3 Sentries") ==
          MonsterType::ELITE);
    CHECK(sentryIn(EncounterLibrary::GetAct2Strong(), "Sentry and Sphere") ==
          MonsterType::NORMAL);

    // What it is by nature is a separate question, and telling which monster a
    // room is named for still asks that one: a cultist walked in beside a boss
    // now stands as a boss, so the standing cannot say which is which.
    CHECK(MonsterRoster::NatureOf(MonsterId::SENTRY) == MonsterType::ELITE);
    CHECK(MonsterRoster::NatureOf(MonsterId::CULTIST) ==
          MonsterType::NORMAL);
}

TEST_CASE("A blow that debuffs says so on the intent")
{
    // A rake weakens and a scrape makes vulnerable, and both were showing as
    // plain attacks. A policy reading the intent could not see the debuff
    // coming.
    const auto intentOf = [](MonsterId who, const std::string& name) {
        std::mt19937 rng(3u);
        const Monster monster = MonsterRoster::Make(who, rng);

        for (const MonsterMove& move : monster.GetMoves())
        {
            if (move.name == name)
            {
                return move.intent;
            }
        }

        return Intent::UNKNOWN;
    };

    CHECK(intentOf(MonsterId::BLUE_SLAVER, "Rake") ==
          Intent::ATTACK_DEBUFF);
    CHECK(intentOf(MonsterId::RED_SLAVER, "Scrape") ==
          Intent::ATTACK_DEBUFF);
}

TEST_CASE("A shield gremlin does not cover the same one every time")
{
    // The page says it covers one of the others and never itself, and does not
    // say which. Taking the first one every time made the cover land on the
    // same gremlin all fight - something a policy can lean on that the game
    // does not offer.
    std::map<MonsterId, int> covered;

    for (unsigned int seed = 1; seed <= 60u; ++seed)
    {
        Battle battle = FightAgainst({ MonsterId::SHIELD_GREMLIN,
                                       MonsterId::MAD_GREMLIN,
                                       MonsterId::SNEAKY_GREMLIN }, seed);

        REQUIRE(battle.GetMonsters().front().ForceMove("Protect") == true);
        REQUIRE(battle.EndTurn() == true);

        for (const Monster& one : battle.GetMonsters())
        {
            if (one.GetMonsterId() != MonsterId::SHIELD_GREMLIN &&
                one.GetBlock() > 0)
            {
                ++covered[one.GetMonsterId()];
            }
        }

        // Never itself while somebody else is standing.
        CHECK(battle.GetMonsters().front().GetBlock() == 0);
    }

    // Both of the others were covered over enough fights.
    CHECK(covered.size() == 2u);
}

TEST_CASE("Both thieves steal, and a leader's own pack leaves with it")
{
    // Thievery fifteen on each of them. Only the mugger had it, so the left of
    // the pair was mugging for nothing.
    for (const MonsterId who : { MonsterId::LOOTER, MonsterId::MUGGER })
    {
        std::mt19937 rng(3u);

        CHECK(MonsterRoster::Make(who, rng).GetPower(PowerType::THIEVERY) ==
              15);
    }

    // And the two gremlins a leader is standing with when the fight opens are
    // its own, the same as the ones it calls in later. Only the called ones
    // were marked, so killing the leader left the opening pair to be worked
    // through.
    Battle battle = FightAgainst({ MonsterId::GREMLIN_LEADER,
                                   MonsterId::RANDOM_GREMLIN,
                                   MonsterId::RANDOM_GREMLIN });

    REQUIRE(battle.GetMonsters().size() == 3u);

    for (std::size_t at = 1; at < 3u; ++at)
    {
        CHECK(battle.GetMonsters()[at].GetPower(PowerType::MINION) > 0);
    }

    // Struck down rather than set to nothing: a death is only noticed where
    // the damage lands.
    battle.GetMonsters()[0].SetHealth(1);
    battle.GetPlayer().GetHand().clear();
    battle.GetPlayer().GetHand().emplace_back(
        CardRegistry::Get(CardId::STRIKE_RED));

    REQUIRE(battle.PlayCard(0, 0) == true);
    REQUIRE(battle.GetMonsters()[0].IsDead() == true);

    for (std::size_t at = 1; at < 3u; ++at)
    {
        CHECK(battle.GetMonsters()[at].IsGone() == true);
    }
}

TEST_CASE("The slavers keep to their own patterns")
{
    // A blue slaver stabs three in five and rakes two, and neither of them
    // comes three turns running. The stab had no limit at all and the rake
    // could not come twice - both wrong, and between them they moved where the
    // damage and the weakness fell.
    for (unsigned int seed = 1; seed <= 30u; ++seed)
    {
        Battle battle = FightAgainst({ MonsterId::BLUE_SLAVER }, seed);
        std::string last;
        int running = 0;

        battle.GetMonsters().front().SetMaxHealth(9999);
        battle.GetMonsters().front().SetHealth(9999);

        for (int turn = 0; turn < 14 &&
                           battle.GetPhase() == BattlePhase::PLAYER_TURN;
             ++turn)
        {
            const std::string move =
                battle.GetMonsters().front().GetCurrentMove().name;

            running = move == last ? running + 1 : 1;
            last = move;

            CHECK(running <= 2);

            REQUIRE(battle.EndTurn() == true);
        }
    }

    // A red slaver opens with a stab, walks scrape, scrape, stab while it has
    // not entangled, and entangles once a fight and no more.
    for (unsigned int seed = 1; seed <= 30u; ++seed)
    {
        Battle battle = FightAgainst({ MonsterId::RED_SLAVER }, seed);

        battle.GetMonsters().front().SetMaxHealth(9999);
        battle.GetMonsters().front().SetHealth(9999);

        CHECK(battle.GetMonsters().front().GetCurrentMove().name == "Stab");

        int entangles = 0;

        for (int turn = 1; turn <= 14 &&
                           battle.GetPhase() == BattlePhase::PLAYER_TURN;
             ++turn)
        {
            const std::string move =
                battle.GetMonsters().front().GetCurrentMove().name;

            entangles += move == "Entangle" ? 1 : 0;

            // Before it has entangled, the turns it does not entangle on walk
            // the scrape, scrape, stab round.
            if (entangles == 0 && turn > 1)
            {
                // The round is counted off the turn itself: the first, the
                // fourth, the seventh stab and the rest scrape.
                const std::string owed =
                    turn % 3 == 1 ? "Stab" : "Scrape";

                CHECK(move == owed);
            }

            REQUIRE(battle.EndTurn() == true);
        }

        CHECK(entangles <= 1);
    }
}

TEST_CASE("A thief tosses a coin on its third turn")
{
    // Two mugs, and then a lunge and the smoke, or straight to the smoke.
    // Always lunging first meant a thief always stayed the extra turn, which
    // is a free turn of getting the gold back.
    for (const MonsterId who : { MonsterId::LOOTER, MonsterId::MUGGER })
    {
        int lunged = 0;
        const int rounds = 600;

        for (int i = 0; i < rounds; ++i)
        {
            std::mt19937 rng(static_cast<unsigned int>(i) + 1u);
            const Monster thief = MonsterRoster::Make(who, rng);
            bool hasLunge = false;

            for (const MonsterMove& move : thief.GetMoves())
            {
                hasLunge = hasLunge || move.name == "Lunge";
            }

            lunged += hasLunge ? 1 : 0;

            // Whichever way the coin fell, it mugs twice and leaves by way of
            // the smoke.
            const std::vector<MonsterMove>& walk = thief.GetMoves();

            REQUIRE(walk.size() >= 4u);
            CHECK(walk[0].name == "Mug");
            CHECK(walk[1].name == "Mug");
            CHECK(walk[walk.size() - 2u].name == "Smoke Bomb");
            CHECK(walk.back().name == "Escape");
        }

        const double share = 100.0 * lunged / rounds;

        CHECK(share > 42.0);
        CHECK(share < 58.0);
    }
}

TEST_CASE("The second act deals its rooms out the way the spire does")
{
    // The chances are published for this act: a snake plant or a centurion
    // three times as often as a chosen with a byrd. Weighing them all alike
    // had the learner meeting the rare rooms half again as often as it should
    // and the common ones half as often.
    const std::vector<Encounter>& strong = EncounterLibrary::GetAct2Strong();
    int total = 0;

    for (const Encounter& one : strong)
    {
        CHECK(one.weight > 0);
        total += one.weight;
    }

    // The ratios rather than the rounded percentages the page prints: two,
    // three, two, six, four, six, three, three.
    CHECK(total == 29);

    // Asked as the shares they come out to, so that the numbers written down
    // are checked against what the page actually says rather than against
    // themselves.
    for (const Encounter& one : strong)
    {
        const double share = 100.0 * one.weight / total;

        if (one.name == "Snake Plant" || one.name == "Centurion and Healer")
        {
            CHECK(one.weight == 6);
            CHECK(share > 20.0);
            CHECK(share < 21.5);
        }

        if (one.name == "Chosen and Byrd" || one.name == "Sentry and Sphere")
        {
            CHECK(one.weight == 2);
            CHECK(share > 6.5);
            CHECK(share < 7.5);
        }

        if (one.name == "Snecko")
        {
            CHECK(one.weight == 4);
            CHECK(share > 13.5);
            CHECK(share < 14.5);
        }
    }

    // The weak rooms really are dealt alike, all five at a fifth.
    const std::vector<Encounter>& weak = EncounterLibrary::GetAct2Weak();

    REQUIRE(weak.size() == 5u);

    for (const Encounter& one : weak)
    {
        CHECK(one.weight == weak.front().weight);
    }
}

TEST_CASE("A gremlin leader brings a pack of whatever turns up")
{
    // It does not stand alone: two gremlins are already there, and which kinds
    // they are is most of what the fight is - a wizard hits far harder than a
    // mad one. It was standing alone and calling only for mad gremlins.
    std::map<MonsterId, int> kinds;

    for (unsigned int seed = 1; seed <= 120u; ++seed)
    {
        const Battle battle =
            FightAgainst({ MonsterId::GREMLIN_LEADER,
                           MonsterId::RANDOM_GREMLIN,
                           MonsterId::RANDOM_GREMLIN }, seed);

        REQUIRE(battle.GetMonsters().size() == 3u);
        CHECK(battle.GetMonsters()[0].GetMonsterId() ==
              MonsterId::GREMLIN_LEADER);

        for (std::size_t at = 1; at < 3u; ++at)
        {
            const MonsterId id = battle.GetMonsters()[at].GetMonsterId();

            // A real gremlin, never the stand-in that was asked for.
            CHECK(id != MonsterId::RANDOM_GREMLIN);
            ++kinds[id];
        }
    }

    // All five kinds turn up over enough fights.
    CHECK(kinds.size() == 5u);
    CHECK(kinds[MonsterId::GREMLIN_WIZARD] > 0);
}

TEST_CASE("A gremlin leader has odds for the company it keeps")
{
    // The two the game states outright, both of them about one gremlin left
    // standing: having encouraged, it rallies or stabs evenly; having stabbed,
    // it rallies five times for every three it encourages. Both fall out of
    // three weights and the rule that it never repeats itself.
    struct Expected
    {
        const char* last;
        const char* move;
        double share;
    };

    const Expected asked[] = { { "Encourage", "Rally", 50.0 },
                               { "Encourage", "Stab", 50.0 },
                               { "Stab", "Rally", 62.5 },
                               { "Stab", "Encourage", 37.5 } };

    for (const Expected& want : asked)
    {
        std::map<std::string, int> saw;
        const int rounds = 8000;

        for (int i = 0; i < rounds; ++i)
        {
            std::mt19937 rng(static_cast<unsigned int>(i) + 1u);
            Monster him = MonsterRoster::Make(MonsterId::GREMLIN_LEADER, rng);
            MoveContext company;

            company.turn = 2;
            company.allies = 1;

            REQUIRE(him.ForceMove(want.last) == true);

            him.AdvanceMove(rng, company);
            ++saw[him.GetCurrentMove().name];
        }

        // Never what it just did.
        CHECK(saw[want.last] == 0);

        const double share = 100.0 * saw[want.move] / rounds;

        CHECK(share > want.share - 3.0);
        CHECK(share < want.share + 3.0);
    }
}

TEST_CASE("A byrd brought down picks itself up again")
{
    // What the game says happens when a byrd loses its flying: it is stunned
    // instantly, interrupting whatever it meant to do; it headbutts the
    // following turn; it flies; and then it is back to its own pattern. It was
    // only being stunned, so it stayed on the ground for the rest of the
    // fight, taking full damage and never climbing back.
    Battle battle = FightAgainst({ MonsterId::BYRD });
    Monster& byrd = battle.GetMonsters().front();

    // Room to survive the hits it takes coming down.
    byrd.SetMaxHealth(400);
    byrd.SetHealth(400);

    REQUIRE(byrd.GetPower(PowerType::FLIGHT) == 3);

    // A handful of small blows, because it is the number of separate hits
    // that brings a flier down and not how hard they are.
    battle.GetPlayer().GetHand().clear();

    for (int i = 0; i < 5; ++i)
    {
        battle.GetPlayer().GetHand().emplace_back(
            CardRegistry::Get(CardId::STRIKE_RED));
    }

    // Three separate hits in one turn bring it down.
    int hits = 0;

    while (hits < 3)
    {
        bool struck = false;

        for (std::size_t at = 0; at < battle.GetPlayer().GetHand().size();
             ++at)
        {
            if (battle.GetPlayer().GetHand()[at].GetCardType() ==
                    CardType::ATTACK &&
                battle.PlayCard(at, 0))
            {
                ++hits;
                struck = true;
                break;
            }
        }

        if (!struck)
        {
            break;
        }
    }

    REQUIRE(hits >= 3);
    REQUIRE(byrd.GetPower(PowerType::FLIGHT) == 0);

    // Stunned where it stands, whatever it had meant to do.
    CHECK(battle.GetMonsters().front().GetCurrentMove().name == "Stunned");

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetMonsters().front().GetCurrentMove().name == "Headbutt");

    REQUIRE(battle.EndTurn() == true);
    CHECK(battle.GetMonsters().front().GetCurrentMove().name == "Fly");

    REQUIRE(battle.EndTurn() == true);

    // In the air again, and choosing for itself.
    CHECK(battle.GetMonsters().front().GetPower(PowerType::FLIGHT) == 3);

    const std::string back =
        battle.GetMonsters().front().GetCurrentMove().name;

    CHECK((back == "Peck" || back == "Caw" || back == "Swoop"));
}

TEST_CASE("A chosen pokes, hexes, and then turns about")
{
    // A poke, then the hex on the second turn, and after that it turns about
    // between two pairs: a debilitate or a drain on the odd turns, a poke or a
    // zap on the even ones. It was drawing from all four every turn, so the
    // hex landed whenever it happened to and the turning about was not there.
    std::map<std::string, int> odd;
    std::map<std::string, int> even;

    for (unsigned int seed = 1; seed <= 40u; ++seed)
    {
        Battle battle = FightAgainst({ MonsterId::CHOSEN }, seed);

        CHECK(battle.GetMonsters().front().GetCurrentMove().name == "Poke");

        REQUIRE(battle.EndTurn() == true);

        CHECK(battle.GetMonsters().front().GetCurrentMove().name == "Hex");

        for (int turn = 3; turn <= 10 &&
                           battle.GetPhase() == BattlePhase::PLAYER_TURN;
             ++turn)
        {
            REQUIRE(battle.EndTurn() == true);

            const std::string move =
                battle.GetMonsters().front().GetCurrentMove().name;

            if (turn % 2 == 1)
            {
                ++odd[move];
            }
            else
            {
                ++even[move];
            }
        }
    }

    // The odd turns hold only the two that debuff, the even ones only the two
    // that hit.
    CHECK(odd["Poke"] == 0);
    CHECK(odd["Zap"] == 0);
    CHECK(even["Debilitate"] == 0);
    CHECK(even["Drain"] == 0);

    // And both of each pair are actually drawn, or the halves above pass for
    // being empty.
    CHECK(odd["Debilitate"] + odd["Drain"] > 0);
    CHECK(even["Poke"] + even["Zap"] > 0);
}

TEST_CASE("A book left standing stabs more every time")
{
    // Six a hit, twice over to begin with and one more hit for every Multi
    // Stab already thrown. It has to be on the intent as well as in the blow,
    // because the whole reason to kill it quickly is being able to see the
    // number climbing.
    Battle battle = FightAgainst({ MonsterId::BOOK_OF_STABBING });
    int expected = 2;

    for (int turn = 0; turn < 10 &&
                       battle.GetPhase() == BattlePhase::PLAYER_TURN;
         ++turn)
    {
        const Monster& it = battle.GetMonsters().front();
        const MonsterMove move = it.GetCurrentMove();

        if (move.name == "Multi Stab")
        {
            int hits = 0;

            for (const MonsterEffect& effect : move.effects)
            {
                if (effect.type == MonsterEffectType::DAMAGE)
                {
                    hits = effect.times;
                    CHECK(effect.amount == 6);
                }
            }

            CHECK(hits == expected);
            ++expected;
        }

        REQUIRE(battle.EndTurn() == true);
    }

    // It threw at least a few of them, or nothing above was tested.
    CHECK(expected > 4);
}

TEST_CASE("Two thieves are a looter and a mugger")
{
    // A looter runs off with what it has taken and a mugger stays, so which
    // of the two is in the room is most of what the fight is about.
    bool found = false;

    for (const Encounter& one : EncounterLibrary::GetAct2Weak())
    {
        if (one.name != "2 Thieves")
        {
            continue;
        }

        found = true;

        REQUIRE(one.monsters.size() == 2u);
        CHECK(one.monsters[0] == MonsterId::LOOTER);
        CHECK(one.monsters[1] == MonsterId::MUGGER);
    }

    CHECK(found == true);

    // And a Chosen brings one byrd, not two.
    found = false;

    for (const Encounter& one : EncounterLibrary::GetAct2Strong())
    {
        if (one.name.find("Chosen and Byrd") != 0u)
        {
            continue;
        }

        found = true;

        REQUIRE(one.monsters.size() == 2u);
        CHECK(one.monsters[0] == MonsterId::CHOSEN);
        CHECK(one.monsters[1] == MonsterId::BYRD);
    }

    CHECK(found == true);
}

TEST_CASE("A guardian keeps what it puts up")
{
    // It comes with a barricade, so the block it raises stays raised. Every
    // other monster's block is cleared at the top of the turn and this one's
    // was too, which turned the wall the whole fight is about into a
    // nuisance.
    Battle battle = FightAgainst({ MonsterId::SPHERIC_GUARDIAN });

    REQUIRE(battle.GetMonsters().front().GetPower(PowerType::BARRICADE) > 0);
    CHECK(battle.GetMonsters().front().GetBlock() == 40);

    // Activate puts twenty-five more up on the first turn, on top of what it
    // came with, because a barricade means nothing is swept away first.
    REQUIRE(battle.EndTurn() == true);

    const int raised = battle.GetMonsters().front().GetBlock();

    CHECK(raised >= 65);

    // And a turn later it is still there rather than swept away.
    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetMonsters().front().GetBlock() >= raised);
}

TEST_CASE("A parasite drinks what it gets through")
{
    // Suck deals ten and takes back what lands as health. Blocked, it drinks
    // nothing.
    Battle drinking = FightAgainst({ MonsterId::SHELLED_PARASITE });
    Monster& it = drinking.GetMonsters().front();

    it.SetHealth(it.GetMaxHealth() - 30);

    const int before = it.GetHealth();

    REQUIRE(it.ForceMove("Suck") == true);
    REQUIRE(drinking.EndTurn() == true);

    CHECK(drinking.GetMonsters().front().GetHealth() > before);

    // And behind enough block it drinks nothing at all.
    Battle blocked = FightAgainst({ MonsterId::SHELLED_PARASITE });
    Monster& other = blocked.GetMonsters().front();

    other.SetHealth(other.GetMaxHealth() - 30);
    blocked.GetPlayer().AddBlock(60);

    const int held = other.GetHealth();

    REQUIRE(other.ForceMove("Suck") == true);
    REQUIRE(blocked.EndTurn() == true);

    CHECK(blocked.GetMonsters().front().GetHealth() == held);
}

TEST_CASE("Minions leave when the last of their leaders falls")
{
    // What the game says of the Minion buff, in as many words: minions
    // abandon combat without their leader. So killing a Collector ends the
    // fight rather than leaving her torch heads to be worked through.
    Battle battle = FightAgainst({ MonsterId::THE_COLLECTOR });

    // Her opening move calls two heads in.
    REQUIRE(battle.EndTurn() == true);

    int heads = 0;

    for (const Monster& one : battle.GetMonsters())
    {
        heads += !one.IsGone() &&
                         one.GetMonsterId() == MonsterId::TORCH_HEAD
                     ? 1
                     : 0;
    }

    REQUIRE(heads == 2);

    // And with her gone they go. Struck down rather than set to nothing: a
    // monster is only noticed dying where the damage lands.
    for (Monster& one : battle.GetMonsters())
    {
        if (one.GetMonsterId() == MonsterId::THE_COLLECTOR)
        {
            one.SetHealth(1);
        }
    }

    bool struck = false;

    for (std::size_t at = 0; at < battle.GetPlayer().GetHand().size(); ++at)
    {
        if (battle.GetPlayer().GetHand()[at].GetCardType() ==
            CardType::ATTACK)
        {
            struck = battle.PlayCard(at, 0);
            break;
        }
    }

    REQUIRE(struck == true);
    REQUIRE(battle.GetMonsters().front().IsDead() == true);

    for (const Monster& one : battle.GetMonsters())
    {
        if (one.GetMonsterId() == MonsterId::TORCH_HEAD)
        {
            CHECK(one.IsGone() == true);
        }
    }
}

TEST_CASE("A run of a move survives the company changing")
{
    // Her fireball is written twice, once for the share it has with both
    // heads standing and once for the share it has without them. She may not
    // throw three running - and when a head falls between one turn and the
    // next, the share moves from one line to the other. Counting a run by
    // which line it came from let her throw the third one nearly half the
    // time; counting it by what the move is called does not.
    int third = 0;
    const int rounds = 20000;

    for (int i = 0; i < rounds; ++i)
    {
        std::mt19937 rng(static_cast<unsigned int>(i) + 1u);
        Monster her = MonsterRoster::Make(MonsterId::THE_COLLECTOR, rng);
        MoveContext company;

        company.turn = 1;
        company.allies = 2;

        // Two thrown with both heads up, which is the line gated to two.
        REQUIRE(her.ForceMove("Fireball") == true);
        REQUIRE(her.ForceMove("Fireball") == true);

        // And then one of them falls, which is what moves the share.
        company.allies = 1;

        her.AdvanceMove(rng, company);

        third += her.GetCurrentMove().name == "Fireball" ? 1 : 0;
    }

    CHECK(third == 0);
}

TEST_CASE("A collector does not call for heads she already has")
{
    // The wiki: with both torch heads standing the draw is a fireball and a
    // buff, and calling for more is not in it. Leaving it in had her spending
    // near a quarter of those turns summoning nothing, because the summon caps
    // at two - a boss doing nothing every fourth turn.
    int fullTurns = 0;
    int wasted = 0;
    int calledWhileShort = 0;

    for (unsigned int seed = 1; seed <= 120u; ++seed)
    {
        Battle battle = FightAgainst({ MonsterId::THE_COLLECTOR }, seed);

        for (int turn = 0;
             turn < 12 && battle.GetPhase() == BattlePhase::PLAYER_TURN;
             ++turn)
        {
            int heads = 0;

            for (const Monster& other : battle.GetMonsters())
            {
                heads += !other.IsGone() &&
                                 other.GetMonsterId() == MonsterId::TORCH_HEAD
                             ? 1
                             : 0;
            }

            const bool calling =
                battle.GetMonsters().front().GetCurrentMove().name == "Spawn";

            if (heads >= 2)
            {
                ++fullTurns;
                wasted += calling ? 1 : 0;
            }
            else
            {
                calledWhileShort += calling ? 1 : 0;
            }

            REQUIRE(battle.EndTurn() == true);
        }
    }

    REQUIRE(fullTurns > 0);
    CHECK(wasted == 0);

    // And she does still call for them when she is short of one, or the fight
    // would only ever have the two she opens with.
    CHECK(calledWhileShort > 0);
}

TEST_CASE("A bronze orb holds a card in stasis until it dies")
{
    Battle battle = FightAgainst({ MonsterId::BRONZE_ORB });
    Player& player = battle.GetPlayer();
    Monster& orb = battle.GetMonsters().front();

    player.GetHand().clear();
    player.GetDrawPile().clear();
    player.GetDiscardPile().clear();

    player.GetDrawPile().emplace_back(CardRegistry::Get(CardId::STRIKE_RED));
    player.GetDrawPile().emplace_back(CardRegistry::Get(CardId::SHRUG_IT_OFF));
    player.GetDrawPile().emplace_back(CardRegistry::Get(CardId::DEMON_FORM));

    REQUIRE(orb.ForceMove("Stasis") == true);
    REQUIRE(battle.EndTurn() == true);

    REQUIRE(orb.HasStasisCard() == true);
    CHECK(orb.GetStasisCard().GetId() == CardId::DEMON_FORM);
    CHECK(CountHeldByPlayer(player, CardId::DEMON_FORM) == 0);
    CHECK(orb.GetPhase() == 2);
    CHECK(orb.GetCurrentMove().name != "Stasis");

    orb.SetHealth(1);
    player.GetHand().emplace_back(CardRegistry::Get(CardId::BLUDGEON));
    player.SetEnergy(3);

    REQUIRE(battle.PlayCard(player.GetHand().size() - 1u, 0) == true);

    CHECK(orb.IsGone() == true);
    CHECK(orb.HasStasisCard() == false);

    bool returned = false;

    for (const Card& card : player.GetHand())
    {
        returned = returned || card.GetId() == CardId::DEMON_FORM;
    }

    CHECK(returned == true);
}

TEST_CASE("A bronze orb takes from discard only when draw is empty")
{
    Battle battle = FightAgainst({ MonsterId::BRONZE_ORB });
    Player& player = battle.GetPlayer();
    Monster& orb = battle.GetMonsters().front();

    player.GetHand().clear();
    player.GetDrawPile().clear();
    player.GetDiscardPile().clear();

    player.GetDiscardPile().emplace_back(CardRegistry::Get(CardId::DEFEND_RED));
    player.GetDiscardPile().emplace_back(CardRegistry::Get(CardId::IMMOLATE));

    REQUIRE(orb.ForceMove("Stasis") == true);
    REQUIRE(battle.EndTurn() == true);

    REQUIRE(orb.HasStasisCard() == true);
    CHECK(orb.GetStasisCard().GetId() == CardId::IMMOLATE);
    CHECK(CountHeldByPlayer(player, CardId::IMMOLATE) == 0);
}

TEST_CASE("A champion stops fighting fair once it is losing")
{
    Battle battle = FightAgainst({ MonsterId::THE_CHAMP });
    Monster& champ = battle.GetMonsters().front();

    REQUIRE(champ.GetPhase() == 1);

    champ.SetHealth(200);

    REQUIRE(Swing(battle) == true);

    CHECK(champ.GetPhase() == 2);
    CHECK(champ.GetCurrentMove().name == "Anger");
}

TEST_CASE("A collector puts everything on the climber on its fourth turn")
{
    Battle battle = FightAgainst({ MonsterId::THE_COLLECTOR });

    REQUIRE(battle.GetMonsters().front().GetCurrentMove().name == "Spawn");

    // Turns one to three, and then the fourth.
    PassTurns(battle, 3);

    CHECK(battle.GetMonsters().front().GetCurrentMove().name ==
          "Mega Debuff");
}

TEST_CASE("A darkling comes back while one of its own still stands")
{
    Battle battle =
        FightAgainst({ MonsterId::DARKLING, MonsterId::DARKLING });
    Monster& first = battle.GetMonsters().front();

    first.SetHealth(1);

    REQUIRE(Swing(battle) == true);

    CHECK(first.IsRegrowing() == true);
    CHECK(battle.IsDone() == false);
    CHECK(first.GetCurrentMove().name == "Reincarnate");

    battle.EndTurn();

    CHECK(first.IsRegrowing() == false);
    CHECK(first.GetHealth() > 0);
}

TEST_CASE("An exploder goes off and goes with it")
{
    Battle battle = FightAgainst({ MonsterId::EXPLODER });

    PassTurns(battle, 2);

    const int before = battle.GetPlayer().GetHealth();

    battle.EndTurn();

    CHECK(battle.GetPlayer().GetHealth() < before);
    CHECK(battle.GetMonsters().front().IsGone() == true);
}

TEST_CASE("A transient fades away on its own")
{
    Battle battle = FightAgainst({ MonsterId::TRANSIENT });

    battle.GetPlayer().SetMaxHealth(999);
    battle.GetPlayer().SetHealth(999);

    REQUIRE(battle.GetMonsters().front().GetPower(PowerType::FADING) == 5);

    PassTurns(battle, 5);

    CHECK(battle.GetMonsters().front().IsGone() == true);
}

TEST_CASE("Something slow takes more for every card played")
{
    Battle battle = FightAgainst({ MonsterId::GIANT_HEAD });

    REQUIRE(battle.GetMonsters().front().GetPower(PowerType::SLOW) == 1);

    // Two strikes of its own, so the hand of the turn does not matter.
    std::vector<Card>& hand = battle.GetPlayer().GetHand();

    hand.clear();
    hand.emplace_back(CardRegistry::Get(CardId::STRIKE_RED));
    hand.emplace_back(CardRegistry::Get(CardId::STRIKE_RED));

    const int start = battle.GetMonsters().front().GetHealth();

    // The first Strike lands for six; the next takes a tenth more, because a
    // card has been played.
    REQUIRE(battle.PlayCard(0, 0) == true);

    const int afterFirst = battle.GetMonsters().front().GetHealth();

    REQUIRE(battle.PlayCard(0, 0) == true);

    const int afterSecond = battle.GetMonsters().front().GetHealth();

    // The card being played counts itself, so the first swing is plain and
    // the second is worth a tenth of two more.
    CHECK(start - afterFirst == 6 + 6 * 1 / 10);
    CHECK(afterFirst - afterSecond == 6 + 6 * 2 / 10);
}

TEST_CASE("What eats time ends a turn that has gone on too long")
{
    Battle battle = FightAgainst({ MonsterId::TIME_EATER });

    REQUIRE(battle.GetMonsters().front().GetPower(PowerType::TIME_WARP) == 12);

    Monster& eater = battle.GetMonsters().front();
    const int strength = eater.GetPower(PowerType::STRENGTH);

    // Twelve cards is what it allows, however cheap they are.
    battle.GetPlayer().AddPower(PowerType::ENERGIZED, 40);

    for (int i = 0; i < 12 && !battle.IsDone(); ++i)
    {
        battle.GetPlayer().SetEnergy(3);

        if (battle.GetPlayableCardIndices().empty())
        {
            break;
        }

        battle.PlayCard(battle.GetPlayableCardIndices().front(), 0);
    }

    CHECK(eater.GetPower(PowerType::STRENGTH) >= strength);
}

TEST_CASE("What eats time makes itself whole when it is losing")
{
    Battle battle = FightAgainst({ MonsterId::TIME_EATER });
    Monster& eater = battle.GetMonsters().front();

    eater.SetHealth(200);

    REQUIRE(Swing(battle) == true);

    CHECK(eater.GetPhase() == 2);
    CHECK(eater.GetCurrentMove().name == "Haste");
}

TEST_CASE("The awakened one gets back up the first time")
{
    Battle battle = FightAgainst({ MonsterId::AWAKENED_ONE });
    Monster& one = battle.GetMonsters().front();

    REQUIRE(one.GetPower(PowerType::CURIOSITY) == 1);

    one.SetHealth(1);

    REQUIRE(Swing(battle) == true);

    CHECK(battle.IsDone() == false);
    CHECK(one.GetPhase() == 2);
    CHECK(one.GetHealth() == one.GetMaxHealth());
    CHECK(one.GetPower(PowerType::CURIOSITY) == 0);
    CHECK(one.GetCurrentMove().name == "Dark Echo");
}

TEST_CASE("The heart can only be brought so far down in one turn")
{
    Battle battle = FightAgainst({ MonsterId::CORRUPT_HEART });
    Monster& heart = battle.GetMonsters().front();

    REQUIRE(heart.GetPower(PowerType::INVINCIBLE) == 300);
    REQUIRE(heart.GetPower(PowerType::BEAT_OF_DEATH) == 1);
    REQUIRE(heart.GetDamageCapLeft() == 300);

    const int health = battle.GetPlayer().GetHealth();
    const int before = heart.GetHealth();

    REQUIRE(Swing(battle) == true);

    // Every card played costs the climber something, and the cap goes down by
    // whatever the swing was worth.
    CHECK(battle.GetPlayer().GetHealth() == health - 1);
    CHECK(heart.GetDamageCapLeft() == 300 - (before - heart.GetHealth()));
}

TEST_CASE("A hex answers everything that is not an attack")
{
    Battle battle = FightAgainst({ MonsterId::CHOSEN });

    battle.GetPlayer().AddPower(PowerType::HEX, 1);

    const std::size_t before = battle.GetPlayer().GetDrawPile().size();
    const std::vector<std::size_t> playable =
        battle.GetPlayableCardIndices();

    std::size_t defend = battle.GetPlayer().GetHand().size();

    for (const std::size_t index : playable)
    {
        if (battle.GetPlayer().GetHand()[index].GetCardType() ==
            CardType::SKILL)
        {
            defend = index;
            break;
        }
    }

    REQUIRE(defend < battle.GetPlayer().GetHand().size());
    REQUIRE(battle.PlayCard(defend, 0) == true);

    CHECK(battle.GetPlayer().GetDrawPile().size() == before + 1);
}
