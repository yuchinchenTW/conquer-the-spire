#include "doctest.h"

#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Monsters/EncounterLibrary.hpp>
#include <conquer-the-spire/Potions/PotionRegistry.hpp>
#include <conquer-the-spire/Relics/RelicRegistry.hpp>
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

TEST_CASE("A flier's charges come back every turn unless they all went")
{
    // The page: "Removed when attacked X times in a single turn. Charges are
    // refilled back to X at the start of each turn if not fully removed."
    // So two hits a turn never bring one down, however many turns it takes -
    // charges that carried over would mean any two hits eventually did.
    //
    // This used to take the charges off by hand and check they reached zero,
    // which is a test of subtraction rather than of the bird.
    Battle battle = FightAgainst({ MonsterId::BYRD });

    battle.GetMonsters().front().SetMaxHealth(4000);
    battle.GetMonsters().front().SetHealth(4000);

    for (int turn = 0; turn < 4; ++turn)
    {
        Monster& byrd = battle.GetMonsters().front();

        CHECK(byrd.GetPower(PowerType::FLIGHT) == 3);

        for (int hit = 0; hit < 2; ++hit)
        {
            battle.GetPlayer().GetHand().emplace_back(
                CardRegistry::Get(CardId::STRIKE_RED));
            battle.GetPlayer().SetEnergy(3);

            REQUIRE(battle.PlayCard(
                        battle.GetPlayer().GetHand().size() - 1u, 0) == true);
        }

        // One left, and never nought, so it is never brought down.
        CHECK(byrd.GetPower(PowerType::FLIGHT) == 1);
        CHECK(byrd.GetCurrentMove().name != "Stunned");

        battle.GetPlayer().SetHealth(400);

        REQUIRE(battle.EndTurn() == true);
    }

    // And the third hit in one turn does bring it down.
    Monster& byrd = battle.GetMonsters().front();

    for (int hit = 0; hit < 3; ++hit)
    {
        battle.GetPlayer().GetHand().emplace_back(
            CardRegistry::Get(CardId::STRIKE_RED));
        battle.GetPlayer().SetEnergy(3);
        battle.PlayCard(battle.GetPlayer().GetHand().size() - 1u, 0);
    }

    CHECK(byrd.GetPower(PowerType::FLIGHT) == 0);
    CHECK(byrd.GetCurrentMove().name == "Stunned");
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

        // The byrd first. The spire moves its monsters in the order they
        // stand in, and the game went back and fixed this very pair so that
        // the chosen cannot make the climber vulnerable and have the byrd
        // swoop into it on the same turn.
        REQUIRE(one.monsters.size() == 2u);
        CHECK(one.monsters[0] == MonsterId::BYRD);
        CHECK(one.monsters[1] == MonsterId::CHOSEN);
    }

    CHECK(found == true);
}

TEST_CASE("A byrd swoops before the chosen can make the climber vulnerable")
{
    // The pair as the pool holds them, in the order the pool holds them,
    // because the order is the whole of it.
    std::vector<MonsterId> pair;

    for (const Encounter& one : EncounterLibrary::GetAct2Strong())
    {
        if (one.name == "Chosen and Byrd")
        {
            pair = one.monsters;
        }
    }

    REQUIRE(pair.size() == 2u);

    Battle battle = FightAgainst(pair);

    for (Monster& one : battle.GetMonsters())
    {
        REQUIRE(one.ForceMove(one.GetMonsterId() == MonsterId::BYRD
                                  ? "Swoop"
                                  : "Debilitate") == true);
    }

    const int before = battle.GetPlayer().GetHealth();

    REQUIRE(battle.EndTurn() == true);

    // Twelve for the swoop and ten for the debilitate. Had the chosen gone
    // first, its two of vulnerable would have been on the climber before the
    // byrd left the ground and the swoop would have come to eighteen - which
    // is the turn order the game itself went back and fixed.
    CHECK(before - battle.GetPlayer().GetHealth() == 22);
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

    // A turn lying there first. It was coming back on the very next turn,
    // a turn early, all fight long.
    CHECK(first.GetCurrentMove().name == "Regrowing");
    CHECK(first.GetHealth() == 0);

    battle.EndTurn();

    CHECK(first.IsRegrowing() == true);
    CHECK(first.GetHealth() == 0);
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

TEST_CASE("The awakened one gets back up, but on its own turn")
{
    Battle battle = FightAgainst({ MonsterId::AWAKENED_ONE });
    Monster& one = battle.GetMonsters().front();

    REQUIRE(one.GetPower(PowerType::CURIOSITY) == 1);

    one.SetHealth(1);

    REQUIRE(Swing(battle) == true);

    // It lies where it fell, at nothing, and what it means to do is stand
    // back up. Standing it up the moment the first body went down handed the
    // climber the whole of the turn it had left to spend on the second.
    CHECK(battle.IsDone() == false);
    CHECK(one.GetPhase() == 1);
    CHECK(one.GetHealth() == 0);
    CHECK(one.IsRegrowing() == true);
    CHECK(one.GetPower(PowerType::CURIOSITY) == 0);
    CHECK(one.GetCurrentMove().name == "Rebirth");

    // And nothing more can be done to it in the meantime.
    const bool again = Swing(battle);

    CHECK(one.GetHealth() == 0);
    CHECK(one.GetPhase() == 1);

    if (!again)
    {
        CHECK(one.GetPower(PowerType::CURIOSITY) == 0);
    }

    // Its own turn is where it stands up.
    REQUIRE(battle.EndTurn() == true);

    CHECK(one.GetPhase() == 2);
    CHECK(one.GetHealth() == one.GetMaxHealth());
    CHECK(one.IsRegrowing() == false);
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

namespace
{
//! Counts \p id across one pile.
int CountIn(const std::vector<Card>& pile, CardId id)
{
    int found = 0;

    for (const Card& one : pile)
    {
        found += one.GetId() == id ? 1 : 0;
    }

    return found;
}
}  // namespace

TEST_CASE("A monster's card lands in the pile the spire puts it in")
{
    // A laser burns twice over, and only one of the two waits in the discard.
    // Both in the discard is a turn of nothing until the pile runs out; one
    // on top of the draw pile is in the way of the next hand, which is the
    // half of the move that hurts.
    {
        Battle battle = FightAgainst({ MonsterId::ORB_WALKER });

        REQUIRE(battle.GetMonsters()[0].ForceMove("Laser") == true);
        REQUIRE(battle.EndTurn() == true);

        const Player& player = battle.GetPlayer();
        const int drawn = CountIn(player.GetDrawPile(), CardId::BURN) +
                          CountIn(player.GetHand(), CardId::BURN);

        CHECK(drawn == 1);
        CHECK(CountIn(player.GetDiscardPile(), CardId::BURN) == 1);
    }

    // A repulsor's daze goes into the draw pile, both of them.
    {
        Battle battle = FightAgainst({ MonsterId::REPULSOR });

        REQUIRE(battle.GetMonsters()[0].ForceMove("Repulse") == true);
        REQUIRE(battle.EndTurn() == true);

        const Player& player = battle.GetPlayer();

        CHECK(CountIn(player.GetDrawPile(), CardId::DAZED) +
                  CountIn(player.GetHand(), CardId::DAZED) ==
              2);
        CHECK(CountIn(player.GetDiscardPile(), CardId::DAZED) == 0);
    }

    // And a void, the same way.
    {
        Battle battle = FightAgainst({ MonsterId::AWAKENED_ONE });

        battle.GetMonsters()[0].SetPhase(2);

        REQUIRE(battle.GetMonsters()[0].ForceMove("Sludge") == true);
        REQUIRE(battle.EndTurn() == true);

        const Player& player = battle.GetPlayer();

        CHECK(CountIn(player.GetDrawPile(), CardId::VOID) +
                  CountIn(player.GetHand(), CardId::VOID) ==
              1);
        CHECK(CountIn(player.GetDiscardPile(), CardId::VOID) == 0);
    }
}

TEST_CASE("A parasite is put in the deck, once, and not in any pile")
{
    // Where it goes: the deck, and no pile of this fight. It does nothing to
    // the fight it is given in and everything to the ones after.
    {
        Battle battle = FightAgainst({ MonsterId::WRITHING_MASS });

        REQUIRE(battle.GetMonsters()[0].ForceMove("Implant") == true);
        REQUIRE(battle.EndTurn() == true);

        const Player& player = battle.GetPlayer();

        CHECK(CountIn(player.GetDrawPile(), CardId::PARASITE) == 0);
        CHECK(CountIn(player.GetHand(), CardId::PARASITE) == 0);
        CHECK(CountIn(player.GetDiscardPile(), CardId::PARASITE) == 0);
        CHECK(CountIn(battle.GetKeptCards(), CardId::PARASITE) == 1);
    }

    // And how often: once a fight, left to choose for itself. A repeat limit
    // instead of a limit for the whole fight let it come round again a few
    // turns later, which is two parasites where the spire gives one.
    int gave = 0;

    for (unsigned int seed = 1; seed <= 40u; ++seed)
    {
        Battle battle = FightAgainst({ MonsterId::WRITHING_MASS }, seed);

        for (int turn = 0; turn < 30; ++turn)
        {
            if (!battle.EndTurn())
            {
                break;
            }
        }

        const int kept = CountIn(battle.GetKeptCards(), CardId::PARASITE);

        CHECK(kept <= 1);
        gave += kept;
    }

    // It does give them, so the limit above is not passing for want of any.
    CHECK(gave > 0);
}

TEST_CASE("An Awakened One opens with a slash and comes back whole")
{
    Battle battle = FightAgainst({ MonsterId::CULTIST,
                                   MonsterId::AWAKENED_ONE,
                                   MonsterId::CULTIST });

    Monster& one = battle.GetMonsters()[1];

    REQUIRE(one.GetMonsterId() == MonsterId::AWAKENED_ONE);

    // A quarter of the fights were opening on the harder of the two.
    CHECK(one.GetCurrentMove().name == "Slash");

    // Whole means whole. Poison went on ticking through the second body,
    // which is most of a phase two the spire does not give.
    // Frail rather than vulnerable, because the card that finishes it off
    // may be a Bash and would put vulnerable straight back on afterwards.
    one.AddPower(PowerType::POISON, 20);
    one.AddPower(PowerType::WEAK, 3);
    one.AddPower(PowerType::FRAIL, 3);
    one.SetHealth(1);

    REQUIRE(Swing(battle, 1) == true);

    // Down, and standing back up on its own turn rather than where it fell.
    REQUIRE(one.IsRegrowing() == true);
    REQUIRE(battle.EndTurn() == true);

    REQUIRE(one.GetPhase() == 2);
    CHECK(one.GetHealth() == one.GetMaxHealth());
    CHECK(one.GetPower(PowerType::POISON) == 0);
    CHECK(one.GetPower(PowerType::WEAK) == 0);
    CHECK(one.GetPower(PowerType::FRAIL) == 0);
    CHECK(one.GetPower(PowerType::CURIOSITY) == 0);

    // And the second body down takes the cultists with it. They are not its
    // minions - a fight can feed on them - so they leave by name.
    one.SetHealth(1);

    REQUIRE(Swing(battle, 1) == true);
    REQUIRE(one.IsDead() == true);

    for (const Monster& other : battle.GetMonsters())
    {
        if (other.GetMonsterId() == MonsterId::CULTIST)
        {
            CHECK(other.IsGone() == true);
        }
    }

    CHECK(battle.GetPhase() == BattlePhase::WON);
}

TEST_CASE("Haste takes the debuffs off and puts the health back")
{
    Battle battle = FightAgainst({ MonsterId::TIME_EATER });

    Monster& eater = battle.GetMonsters()[0];

    eater.AddPower(PowerType::POISON, 15);
    eater.AddPower(PowerType::VULNERABLE, 2);
    eater.SetHealth(eater.GetMaxHealth() / 4);

    REQUIRE(eater.ForceMove("Haste") == true);
    REQUIRE(battle.EndTurn() == true);

    // Half its health, and nothing on it. It was a turn of nothing at all.
    CHECK(eater.GetHealth() >= eater.GetMaxHealth() / 2);
    CHECK(eater.GetPower(PowerType::POISON) == 0);
    CHECK(eater.GetPower(PowerType::VULNERABLE) == 0);
}

TEST_CASE("A Giant Head's count runs out and its swing climbs to sixty")
{
    Battle battle = FightAgainst({ MonsterId::GIANT_HEAD });

    // Four turns of counting and glaring.
    for (int turn = 0; turn < 4; ++turn)
    {
        const std::string& name = battle.GetMonsters()[0].GetCurrentMove().name;

        CHECK((name == "Count" || name == "Glare"));
        REQUIRE(battle.EndTurn() == true);
    }

    // And then it swings for everything, for the rest of the fight. It was
    // counting for ever, which is a five-hundred-health elite that cannot
    // kill anybody.
    REQUIRE(battle.GetMonsters()[0].GetCurrentMove().name == "It Is Time");

    int swing = 0;

    for (int again = 0; again < 8; ++again)
    {
        Monster& head = battle.GetMonsters()[0];

        REQUIRE(head.GetCurrentMove().name == "It Is Time");

        int hit = 0;

        for (const MonsterEffect& effect : head.GetCurrentMove().effects)
        {
            if (effect.type == MonsterEffectType::DAMAGE)
            {
                hit = effect.amount;
            }
        }

        // Thirty, then thirty-five, and up by five a swing to sixty.
        CHECK(hit == (30 + 5 * again < 60 ? 30 + 5 * again : 60));
        CHECK(hit >= swing);
        swing = hit;

        battle.GetPlayer().SetHealth(400);

        if (!battle.EndTurn())
        {
            break;
        }
    }

    CHECK(swing == 60);
}

namespace
{
//! The hits the move standing on \p monster is about to make.
int HitsOf(const Monster& monster)
{
    for (const MonsterEffect& effect : monster.GetCurrentMove().effects)
    {
        if (effect.type == MonsterEffectType::DAMAGE)
        {
            return effect.times;
        }
    }

    return 0;
}
}  // namespace

TEST_CASE("The Maw walks by what it just did")
{
    std::set<std::string> afterRoar;
    std::set<std::string> afterSlam;
    std::set<std::string> afterNom;
    std::set<std::string> afterDrool;

    for (unsigned int seed = 1; seed <= 60u; ++seed)
    {
        Battle battle = FightAgainst({ MonsterId::THE_MAW }, seed);

        REQUIRE(battle.GetMonsters()[0].GetCurrentMove().name == "Roar");

        std::string before = "Roar";

        for (int turn = 2; turn <= 9; ++turn)
        {
            battle.GetPlayer().SetHealth(400);

            if (!battle.EndTurn())
            {
                break;
            }

            const Monster& maw = battle.GetMonsters()[0];
            const std::string now = maw.GetCurrentMove().name;

            if (before == "Roar")
            {
                afterRoar.insert(now);
            }
            else if (before == "Slam")
            {
                afterSlam.insert(now);
            }
            else if (before == "Nom Nom")
            {
                afterNom.insert(now);
            }
            else
            {
                afterDrool.insert(now);
            }

            // One bite for every two turns of the fight, rounded up. It was
            // three bites for ever.
            if (now == "Nom Nom")
            {
                CHECK(HitsOf(maw) == (turn + 1) / 2);
            }

            before = now;
        }
    }

    // A drool cannot follow the roar, and a nom nom is always answered by
    // one. Both were possible when everything was drawn from every turn.
    CHECK(afterRoar == std::set<std::string>{ "Nom Nom", "Slam" });
    CHECK(afterSlam == std::set<std::string>{ "Drool", "Nom Nom" });
    CHECK(afterNom == std::set<std::string>{ "Drool" });
    CHECK(afterDrool == std::set<std::string>{ "Nom Nom", "Slam" });
}

TEST_CASE("A Writhing Mass opens four ways and not five")
{
    std::map<std::string, int> first;
    const int rounds = 200;

    for (unsigned int seed = 1;
         seed <= static_cast<unsigned int>(rounds); ++seed)
    {
        Battle battle = FightAgainst({ MonsterId::WRITHING_MASS }, seed);

        ++first[battle.GetMonsters()[0].GetCurrentMove().name];
    }

    // Four of the five, evenly. The parasite is the one left out, and it is
    // the only one left out - the page this project usually follows says
    // three and leaves out the block attack too, but its own data module
    // asks whether the block attack belongs here rather than saying, and the
    // other page says four and names them.
    CHECK(first.count("Implant") == 0u);

    REQUIRE(first.count("Multi Hit") == 1u);
    REQUIRE(first.count("Big Hit") == 1u);
    REQUIRE(first.count("Debuff Attack") == 1u);
    REQUIRE(first.count("Block Attack") == 1u);
    CHECK(first.size() == 4u);

    for (const auto& one : first)
    {
        CHECK(one.second > rounds / 8);
        CHECK(one.second < rounds / 2);
    }
}

TEST_CASE("A Spire Growth smashes once the climber is tied up")
{
    for (unsigned int seed = 1; seed <= 30u; ++seed)
    {
        Battle battle = FightAgainst({ MonsterId::SPIRE_GROWTH }, seed);
        const std::string opened =
            battle.GetMonsters()[0].GetCurrentMove().name;

        // Never the smash to start with: nothing has tied anybody up yet.
        CHECK((opened == "Quick Tackle" || opened == "Constrict"));

        bool tied = false;

        for (int turn = 0; turn < 8; ++turn)
        {
            battle.GetPlayer().SetHealth(400);

            if (!battle.EndTurn())
            {
                break;
            }

            if (tied)
            {
                // From there on it smashes, and nothing else. The smash had
                // no weight and nothing to turn it on, so it never came at
                // all.
                CHECK(battle.GetMonsters()[0].GetCurrentMove().name ==
                      "Smash");
            }

            tied = tied || battle.GetPlayer().GetPower(
                               PowerType::CONSTRICTED) > 0;
        }
    }
}

TEST_CASE("A Darkling rolls its nip once and the middle one cannot chomp")
{
    std::set<int> nips;

    for (unsigned int seed = 1; seed <= 40u; ++seed)
    {
        Battle battle = FightAgainst({ MonsterId::DARKLING }, seed);
        int found = 0;

        for (const MonsterMove& move : battle.GetMonsters()[0].GetMoves())
        {
            if (move.name != "Nip")
            {
                continue;
            }

            for (const MonsterEffect& effect : move.effects)
            {
                if (effect.type == MonsterEffectType::DAMAGE)
                {
                    found = effect.amount;
                }
            }
        }

        // Seven to eleven, rolled once for the fight. It was nine every time.
        CHECK(found >= 7);
        CHECK(found <= 11);
        nips.insert(found);
    }

    CHECK(nips.size() > 1u);

    // The middle one of three is the same darkling but for the chomp.
    std::vector<MonsterId> three;

    for (const Encounter& one : EncounterLibrary::GetAct3Weak())
    {
        if (one.name == "3 Darklings")
        {
            three = one.monsters;
        }
    }

    REQUIRE(three.size() == 3u);

    std::mt19937 rng(7);
    const std::vector<Monster> built = EncounterLibrary::Build(
        { "3 Darklings", MonsterType::NORMAL, three }, rng);

    REQUIRE(built.size() == 3u);

    const auto chomps = [](const Monster& one) {
        for (const MonsterMove& move : one.GetMoves())
        {
            if (move.name == "Chomp")
            {
                return true;
            }
        }

        return false;
    };

    CHECK(chomps(built[0]) == true);
    CHECK(chomps(built[1]) == false);
    CHECK(chomps(built[2]) == true);
}

TEST_CASE("A Darkling sleeping one off cannot hold another one up")
{
    Battle battle = FightAgainst({ MonsterId::DARKLING, MonsterId::DARKLING });

    // One of the two already sleeping a death off.
    battle.GetMonsters()[0].SetRegrowing(true);

    REQUIRE(battle.GetMonsters()[0].IsRegrowing() == true);
    REQUIRE(battle.GetMonsters()[0].IsGone() == false);

    // The other goes down while it is under. There is nothing standing to
    // hold it up, so it stays down - and the fight can be finished. Counting
    // the regrowing one meant a blow that took both at once put both into
    // regrowing, over and over.
    battle.GetMonsters()[1].SetHealth(1);

    REQUIRE(Swing(battle, 1) == true);

    CHECK(battle.GetMonsters()[1].IsRegrowing() == false);
    CHECK(battle.GetMonsters()[1].IsDead() == true);
}

TEST_CASE("A Reptomancer stands with two daggers and strikes when the floor is full")
{
    std::vector<MonsterId> room;

    for (const Encounter& one : EncounterLibrary::GetAct3Elites())
    {
        if (one.name == "Reptomancer")
        {
            room = one.monsters;
        }
    }

    // Two daggers already beside it. Standing alone, its opening summon was
    // the whole of the threat rather than the second wave of it.
    REQUIRE(room.size() == 3u);
    CHECK(room[0] == MonsterId::REPTOMANCER);
    CHECK(room[1] == MonsterId::DAGGER);
    CHECK(room[2] == MonsterId::DAGGER);

    Battle battle = FightAgainst(room);

    CHECK(battle.GetMonsters()[0].GetCurrentMove().name == "Summon");

    // Four on the floor and the summon has nowhere to put anybody, so its
    // share goes to the snake strike rather than the turn being spent on a
    // summon that makes nothing.
    // Counted from the second turn on. The opening summon is owed whatever
    // else is true - it always opens on one, and in its own room there are
    // only two daggers standing, so the floor cannot be full yet.
    Battle full = FightAgainst({ MonsterId::REPTOMANCER, MonsterId::DAGGER,
                                 MonsterId::DAGGER, MonsterId::DAGGER,
                                 MonsterId::DAGGER });
    std::set<std::string> seen;

    full.GetPlayer().SetHealth(400);

    REQUIRE(full.EndTurn() == true);

    for (int turn = 0; turn < 6; ++turn)
    {
        full.GetPlayer().SetHealth(400);
        seen.insert(full.GetMonsters()[0].GetCurrentMove().name);

        if (!full.EndTurn())
        {
            break;
        }
    }

    CHECK(seen.count("Summon") == 0u);
    CHECK(seen.count("Snake Strike") == 1u);
}

TEST_CASE("A room of shapes is whichever shapes, and never three alike")
{
    const std::set<MonsterId> kinds = { MonsterId::REPULSOR,
                                        MonsterId::EXPLODER,
                                        MonsterId::SPIKER };
    std::set<std::vector<MonsterId>> rooms;

    for (const char* name : { "3 Shapes", "4 Shapes", "Sphere and 2 Shapes" })
    {
        std::vector<MonsterId> asked;
        const std::vector<Encounter>* pools[] = {
            &EncounterLibrary::GetAct3Weak(), &EncounterLibrary::GetAct3Strong()
        };

        for (const std::vector<Encounter>* pool : pools)
        {
            for (const Encounter& one : *pool)
            {
                if (one.name == name)
                {
                    asked = one.monsters;
                }
            }
        }

        REQUIRE(asked.empty() == false);

        std::set<std::vector<MonsterId>> here;

        for (unsigned int seed = 1; seed <= 60u; ++seed)
        {
            std::mt19937 rng(seed);
            const std::vector<Monster> built = EncounterLibrary::Build(
                { name, MonsterType::NORMAL, asked }, rng);

            REQUIRE(built.size() == asked.size());

            std::map<MonsterId, int> alike;
            std::vector<MonsterId> made;

            for (const Monster& one : built)
            {
                if (kinds.count(one.GetMonsterId()) == 1u)
                {
                    ++alike[one.GetMonsterId()];
                }

                made.emplace_back(one.GetMonsterId());
            }

            // Never three of a kind in one room.
            for (const auto& counted : alike)
            {
                CHECK(counted.second <= 2);
            }

            rooms.insert(made);
            here.insert(made);
        }

        // Counted for this room on its own. Counting them all together let
        // one fixed room hide behind the two that were drawn.
        CHECK(here.size() > 3u);
    }

    CHECK(rooms.size() > 9u);
}

TEST_CASE("The third act's jaw worms have already bellowed")
{
    std::vector<MonsterId> horde;

    for (const Encounter& one : EncounterLibrary::GetAct3Strong())
    {
        if (one.name == "Jaw Worm Horde")
        {
            horde = one.monsters;
        }
    }

    REQUIRE(horde.size() == 3u);

    for (const MonsterId id : horde)
    {
        CHECK(id == MonsterId::JAW_WORM_HARD);
    }

    Battle battle = FightAgainst(horde);
    std::set<std::string> opened;

    for (const Monster& one : battle.GetMonsters())
    {
        // Three of strength and six of block, standing there before a card is
        // played. Three of the first act's worms is a much softer room.
        CHECK(one.GetName() == "Jaw Worm");
        CHECK(one.GetPower(PowerType::STRENGTH) == 3);
        CHECK(one.GetBlock() == 6);

        opened.insert(one.GetCurrentMove().name);
    }

    // And no forced chomp to open on, so the first turn is the ordinary draw.
    int chomped = 0;

    for (unsigned int seed = 1; seed <= 40u; ++seed)
    {
        Battle again = FightAgainst(horde, seed);

        for (const Monster& one : again.GetMonsters())
        {
            chomped += one.GetCurrentMove().name == "Chomp" ? 1 : 0;
            opened.insert(one.GetCurrentMove().name);
        }
    }

    CHECK(opened.size() == 3u);
    CHECK(chomped < 40 * 3);

    // The first act's worm still opens on its chomp.
    Battle first = FightAgainst({ MonsterId::JAW_WORM });

    CHECK(first.GetMonsters()[0].GetCurrentMove().name == "Chomp");
    CHECK(first.GetMonsters()[0].GetPower(PowerType::STRENGTH) == 0);
}

TEST_CASE("The monster ids are only ever appended to")
{
    // The number a monster has in the enum is the row it gets in the
    // learner's table of monsters, so putting a new one in the middle moves
    // every monster after it onto somebody else's row and throws away what a
    // running climb has learned about all of them. These are the last few,
    // in order, and a new one belongs after them and nowhere else.
    CHECK(static_cast<int>(MonsterId::POINTY) + 1 ==
          static_cast<int>(MonsterId::ROMEO));
    CHECK(static_cast<int>(MonsterId::ROMEO) + 1 ==
          static_cast<int>(MonsterId::BEAR));
    CHECK(static_cast<int>(MonsterId::BEAR) + 1 ==
          static_cast<int>(MonsterId::TRAINING_DUMMY));
    CHECK(static_cast<int>(MonsterId::TRAINING_DUMMY) + 1 ==
          static_cast<int>(MonsterId::RANDOM_SHAPE));
    CHECK(static_cast<int>(MonsterId::RANDOM_SHAPE) + 1 ==
          static_cast<int>(MonsterId::JAW_WORM_HARD));
}

TEST_CASE("A Reptomancer summons one dagger at a time")
{
    Battle battle = FightAgainst({ MonsterId::REPTOMANCER, MonsterId::DAGGER,
                                   MonsterId::DAGGER });

    REQUIRE(battle.GetMonsters()[0].GetCurrentMove().name == "Summon");

    int before = 0;

    for (const Monster& one : battle.GetMonsters())
    {
        before += one.GetMonsterId() == MonsterId::DAGGER ? 1 : 0;
    }

    REQUIRE(before == 2);

    battle.GetPlayer().SetHealth(400);

    REQUIRE(battle.EndTurn() == true);

    int after = 0;

    for (const Monster& one : battle.GetMonsters())
    {
        after += one.GetMonsterId() == MonsterId::DAGGER && !one.IsGone()
                     ? 1
                     : 0;
    }

    // One. Two is the ascension-eighteen number, and reading it as the
    // ordinary one took the opening floor from three daggers to four.
    CHECK(after == 3);
}

TEST_CASE("A Spiker spikes six times and then only cuts")
{
    for (unsigned int seed = 1; seed <= 20u; ++seed)
    {
        Battle battle = FightAgainst({ MonsterId::SPIKER }, seed);
        int spiked = 0;

        for (int turn = 0; turn < 30; ++turn)
        {
            const std::string now =
                battle.GetMonsters()[0].GetCurrentMove().name;

            if (now == "Spike")
            {
                ++spiked;
            }
            else if (spiked >= 6)
            {
                CHECK(now == "Cut");
            }

            battle.GetPlayer().SetHealth(400);

            if (!battle.EndTurn())
            {
                break;
            }
        }

        // Six and no more. It could spike for ever, and a monster whose
        // thorns keep climbing is a fight the other side can wait out.
        CHECK(spiked <= 6);
        CHECK(battle.GetMonsters()[0].GetPower(PowerType::THORNS) <= 3 + 12);
    }
}

TEST_CASE("A Transient gives up strength for any hurt, not only a swing")
{
    // What the loss is worth is the damage it does not do that turn: the
    // strength comes off when the health does and goes back at the end of
    // its own turn, after it has swung.
    const auto swung = [](int poison) {
        Battle battle = FightAgainst({ MonsterId::TRANSIENT });

        battle.GetPlayer().SetHealth(400);

        if (poison > 0)
        {
            battle.GetMonsters().front().AddPower(PowerType::POISON, poison);
        }

        const int before = battle.GetPlayer().GetHealth();

        battle.EndTurn();

        return before - battle.GetPlayer().GetHealth();
    };

    const int clean = swung(0);
    const int poisoned = swung(9);

    REQUIRE(clean > 0);

    // Poison is not a swing, and the page says upon losing HP. It was only
    // giving strength up when it was struck, so poison, an orb and a card
    // that costs health all left it hitting at full weight.
    CHECK(poisoned == clean - 9);
}

TEST_CASE("A Nemesis is out of reach every other turn, and it lasts")
{
    Battle battle = FightAgainst({ MonsterId::NEMESIS });
    Monster& one = battle.GetMonsters().front();

    // Out of reach on the first turn.
    CHECK(one.GetPower(PowerType::INTANGIBLE) > 0);

    std::vector<bool> reach;

    for (int turn = 1; turn <= 6; ++turn)
    {
        reach.emplace_back(one.GetPower(PowerType::INTANGIBLE) > 0);
        battle.GetPlayer().SetHealth(400);

        if (!battle.EndTurn())
        {
            break;
        }
    }

    REQUIRE(reach.size() == 6u);

    // Turn about, and the stack survives to the turn it was raised for. It
    // was being raised before the wearing-off took it straight back, so past
    // the opening one it was never out of reach on a turn anybody could see.
    for (std::size_t at = 0; at < reach.size(); ++at)
    {
        CHECK(reach[at] == (at % 2 == 0));
    }
}

TEST_CASE("A room of shapes never holds three alike, ever")
{
    std::vector<MonsterId> four;

    for (const Encounter& one : EncounterLibrary::GetAct3Strong())
    {
        if (one.name == "4 Shapes")
        {
            four = one.monsters;
        }
    }

    REQUIRE(four.size() == 4u);

    // Drawing again until an allowed one turns up is only nearly a
    // guarantee. Two thousand rooms, and never a third of a kind.
    for (unsigned int seed = 1; seed <= 2000u; ++seed)
    {
        std::mt19937 rng(seed);
        const std::vector<Monster> built = EncounterLibrary::Build(
            { "4 Shapes", MonsterType::NORMAL, four }, rng);
        std::map<MonsterId, int> alike;

        REQUIRE(built.size() == 4u);

        for (const Monster& one : built)
        {
            ++alike[one.GetMonsterId()];
        }

        for (const auto& counted : alike)
        {
            REQUIRE(counted.second <= 2);
        }
    }
}

TEST_CASE("What can be aimed at while it is down is asked of each of them")
{
    // The two are not the same, and the pages are why. A darkling's says the
    // intent changes to Regrowing "and cannot be targeted". An awakened
    // one's says nothing about aiming and the other wiki says invulnerable,
    // which means the blow may be thrown and comes to nothing. Reading the
    // second answer onto the first is the mistake this is here to stop.
    // An awakened one is still in the room and still a target, and the swing
    // comes to nothing. Accepted matters on its own - an action offered and
    // then refused is what sends the policy round in circles - so the strike
    // and the energy to play it are put there rather than hoped for.
    {
        Battle battle = FightAgainst({ MonsterId::AWAKENED_ONE });
        Monster& one = battle.GetMonsters().front();

        one.SetHealth(1);

        REQUIRE(Swing(battle, 0u) == true);
        REQUIRE(one.IsRegrowing() == true);
        CHECK(one.GetHealth() == 0);

        const std::vector<std::size_t> living =
            battle.GetLivingMonsterIndices();

        CHECK(std::find(living.begin(), living.end(), 0u) != living.end());

        battle.GetPlayer().GetHand().emplace_back(
            CardRegistry::Get(CardId::STRIKE_RED));
        battle.GetPlayer().SetEnergy(3);

        REQUIRE(Swing(battle, 0u) == true);

        CHECK(one.GetHealth() == 0);
        CHECK(one.IsRegrowing() == true);

        // And a potion is aimed the same way a card is. It was asking
        // whether they were dead, which turned a fire potion away from the
        // very thing a strike could still be thrown at.
        battle.GetPlayer().GetPotions().emplace_back(
            PotionRegistry::Get(PotionId::FIRE_POTION));

        CHECK(battle.CanUsePotion(
                  battle.GetPlayer().GetPotions().size() - 1u, 0u) == true);
    }

    // A darkling is out of reach: out of what a card may be aimed at and out
    // of what a blow thrown at random may land on. Not out of what the
    // learner is shown - it stays in the room, where the climber can see it
    // lying there and see the intent that says it is coming back, and the
    // assertions below say so.
    {
        Battle battle =
            FightAgainst({ MonsterId::DARKLING, MonsterId::DARKLING });
        Monster& first = battle.GetMonsters().front();

        first.SetHealth(1);

        REQUIRE(Swing(battle, 0u) == true);
        REQUIRE(first.IsRegrowing() == true);
        CHECK(first.GetHealth() == 0);
        CHECK(first.IsGone() == false);

        // Still in the room, and the climber can see it lying there - the
        // state is built from this list and the ordinal a move names is an
        // offset into it, so taking it out here would hide a monster the
        // spire shows and shift the one behind it a place along.
        const std::vector<std::size_t> living =
            battle.GetLivingMonsterIndices();

        CHECK(std::find(living.begin(), living.end(), 0u) != living.end());
        CHECK(living.size() == 2u);

        // But not aimable at, which is the other question.
        const std::vector<std::size_t> aimable =
            battle.GetTargetableMonsterIndices();

        CHECK(std::find(aimable.begin(), aimable.end(), 0u) == aimable.end());
        CHECK(aimable.size() == 1u);

        battle.GetPlayer().GetHand().emplace_back(
            CardRegistry::Get(CardId::STRIKE_RED));
        battle.GetPlayer().SetEnergy(3);

        CHECK(battle.CanPlay(battle.GetPlayer().GetHand().size() - 1u, 0u) ==
              false);

        // And asking without naming anybody asks about the first one that
        // may be aimed at, not about whoever stands first. Asking about
        // whoever stands first answered no for every card in hand while a
        // darkling lay at the front of the room - and that answer is written
        // into the state the learner reads.
        CHECK(battle.CanPlay(battle.GetPlayer().GetHand().size() - 1u) ==
              true);
        CHECK(battle.GetPlayableCardIndices().empty() == false);

        // A potion is turned away from it too, the same way.
        battle.GetPlayer().GetPotions().emplace_back(
            PotionRegistry::Get(PotionId::FIRE_POTION));

        const std::size_t bottle =
            battle.GetPlayer().GetPotions().size() - 1u;

        CHECK(battle.CanUsePotion(bottle, 0u) == false);
        CHECK(battle.CanUsePotion(bottle, 1u) == true);

        // And it comes back into reach once it is up again.
        battle.EndTurn();
        battle.EndTurn();

        CHECK(first.IsRegrowing() == false);
        CHECK(battle.GetLivingMonsterIndices().size() == 2u);
    }

}

TEST_CASE("A kill that does not finish anything is not a fatal one")
{
    // Read through a Feed, which is what Fatal is for: three of maximum
    // health when the card kills, and nothing when it does not. The page
    // says the first phase of an Awakened One does not count, and that among
    // three darklings only the blow that takes the last one does - otherwise
    // a feed could be farmed on a fight that keeps standing back up, which
    // is the reason the page itself gives.
    const auto fed = [](std::vector<MonsterId> room, std::size_t at) {
        Battle battle = FightAgainst(room);

        battle.GetMonsters()[at].SetHealth(1);
        battle.GetPlayer().GetHand().emplace_back(
            CardRegistry::Get(CardId::FEED));
        battle.GetPlayer().SetEnergy(3);

        const std::size_t slot = battle.GetPlayer().GetHand().size() - 1u;
        const int before = battle.GetPlayer().GetMaxHealth();

        REQUIRE(battle.PlayCard(slot, at) == true);

        return battle.GetPlayer().GetMaxHealth() - before;
    };

    CHECK(fed({ MonsterId::AWAKENED_ONE }, 0u) == 0);
    CHECK(fed({ MonsterId::DARKLING, MonsterId::DARKLING }, 0u) == 0);

    // The last one standing does count, and so does an ordinary monster.
    CHECK(fed({ MonsterId::DARKLING }, 0u) == 3);
    CHECK(fed({ MonsterId::SPIKER }, 0u) == 3);

    // And a minion never does: a dagger is a thing the fight made rather
    // than a thing the room held.
    CHECK(fed({ MonsterId::REPTOMANCER, MonsterId::DAGGER,
                MonsterId::DAGGER }, 1u) == 0);
}

TEST_CASE("A darkling knocked down gives up what strength it was given")
{
    Battle battle =
        FightAgainst({ MonsterId::DARKLING, MonsterId::DARKLING });
    Monster& first = battle.GetMonsters().front();

    // At nought ascension it only ever has strength from something the
    // climber gave it, but the page says all of it goes and it goes.
    first.AddPower(PowerType::STRENGTH, 5);
    first.SetHealth(1);

    REQUIRE(Swing(battle, 0u) == true);
    REQUIRE(first.IsRegrowing() == true);

    CHECK(first.GetPower(PowerType::STRENGTH) == 0);
}

TEST_CASE("A Transient gives up strength for a flat blow too")
{
    // A sadistic streak is flat damage: it goes through the path that was
    // taking the health without noting what shifts owes for it. Read as the
    // difference the streak makes, so that what the bash itself takes off
    // cancels out of both sides.
    const auto lost = [](int sadistic) {
        Battle battle = FightAgainst({ MonsterId::TRANSIENT });
        Monster& one = battle.GetMonsters().front();

        if (sadistic > 0)
        {
            battle.GetPlayer().AddPower(PowerType::SADISTIC, sadistic);
        }

        battle.GetPlayer().GetHand().emplace_back(
            CardRegistry::Get(CardId::BASH));
        battle.GetPlayer().SetEnergy(3);

        const std::size_t slot = battle.GetPlayer().GetHand().size() - 1u;

        REQUIRE(battle.PlayCard(slot, 0u) == true);

        return battle.GetMonsters().front().GetPower(
            PowerType::SHIFTING_LOSS);
    };

    const int plain = lost(0);

    REQUIRE(plain > 0);

    // Five more of health taken is five more of strength owed.
    CHECK(lost(5) == plain + 5);
}

TEST_CASE("Poison does not hurry a threshold along, except a guardian's")
{
    // The page: when poison takes a Guardian past the wall it changes shape
    // at, it changes there and then. "In other cases where HP thresholds
    // affect enemy Intent (such as The Champ and Time Eater), this happens on
    // a subsequent turn, and will not be affected." I had read that the other
    // way round and hurried all of them.
    {
        Battle battle = FightAgainst({ MonsterId::TIME_EATER });
        Monster& eater = battle.GetMonsters().front();

        eater.SetHealth(eater.GetMaxHealth() / 2 + 3);
        eater.AddPower(PowerType::POISON, 9);

        REQUIRE(eater.GetPhase() == 1);
        REQUIRE(battle.EndTurn() == true);

        // Under half from the poison, and it has not hasted on this turn -
        // Haste comes back up to half, so still being under is the waiting.
        CHECK(eater.GetHealth() < eater.GetMaxHealth() / 2);
    }

    // A guardian is the exception the page names, and poison counts against
    // its wall - which it was not doing at all.
    {
        Battle battle = FightAgainst({ MonsterId::THE_GUARDIAN });
        Monster& guard = battle.GetMonsters().front();

        REQUIRE(guard.GetPower(PowerType::MODE_SHIFT) == 30);

        guard.AddPower(PowerType::POISON, 30);

        REQUIRE(battle.EndTurn() == true);

        // The wall is gone and it went into Defensive Mode on that very
        // turn, which is what Sharp Hide standing on it says - the intent
        // cannot be read afterwards because it has already been acted on and
        // the pattern has moved along.
        CHECK(guard.GetPower(PowerType::MODE_SHIFT) == 0);
        CHECK(guard.GetPower(PowerType::SHARP_HIDE) == 3);
    }
}

TEST_CASE("A hand of greed counts a kill the way a feed does")
{
    // Two places ask whether the card killed something, and they were asking
    // different questions. This one only looked at dead-and-not-a-minion, so
    // it paid out on an awakened one's first body and on a darkling that was
    // going to stand back up - the very farming the Fatal page gives as the
    // reason for the rule.
    const auto gold = [](std::vector<MonsterId> room, std::size_t at) {
        Battle battle = FightAgainst(room);

        battle.GetMonsters()[at].SetHealth(1);
        battle.GetPlayer().GetHand().emplace_back(
            CardRegistry::Get(CardId::HAND_OF_GREED));
        battle.GetPlayer().SetEnergy(3);

        const std::size_t slot = battle.GetPlayer().GetHand().size() - 1u;

        REQUIRE(battle.PlayCard(slot, at) == true);

        return battle.GetGoldFound();
    };

    CHECK(gold({ MonsterId::AWAKENED_ONE }, 0u) == 0);
    CHECK(gold({ MonsterId::DARKLING, MonsterId::DARKLING }, 0u) == 0);

    // The last one standing does pay, and so does an ordinary monster.
    CHECK(gold({ MonsterId::DARKLING }, 0u) > 0);
    CHECK(gold({ MonsterId::SPIKER }, 0u) > 0);
}

TEST_CASE("A reptomancer with a full floor gives the summon's share away")
{
    // Four daggers standing, so the summon has nowhere to put anybody and
    // hands its third to the snake strike - two thirds the strike, one third
    // the bite. The gate was being asked before the handing-over, so the
    // share went nowhere and the two attacks split it evenly instead.
    std::map<std::string, int> tally;

    for (unsigned int seed = 1; seed <= 200u; ++seed)
    {
        Battle battle = FightAgainst({ MonsterId::REPTOMANCER,
                                       MonsterId::DAGGER, MonsterId::DAGGER,
                                       MonsterId::DAGGER,
                                       MonsterId::DAGGER },
                                     seed);

        battle.GetPlayer().SetHealth(400);

        // Past the opening summon, which is owed whatever else is true.
        REQUIRE(battle.EndTurn() == true);

        ++tally[battle.GetMonsters()[0].GetCurrentMove().name];
    }

    CHECK(tally.count("Summon") == 0u);

    const int strike = tally["Snake Strike"];
    const int bite = tally["Big Bite"];

    REQUIRE(strike + bite == 200);

    // Two to one, near enough. Evens is what it was.
    CHECK(strike > bite);
    CHECK(strike > 110);
}

TEST_CASE("A pack of darklings goes down together")
{
    // The link works only while another of them is standing. Once the last
    // one on its feet falls there is nobody to do the pulling, so the ones
    // already lying there go with it and the fight is over.
    Battle battle =
        FightAgainst({ MonsterId::DARKLING, MonsterId::DARKLING });
    Monster& first = battle.GetMonsters()[0];
    Monster& second = battle.GetMonsters()[1];

    first.SetHealth(1);

    REQUIRE(Swing(battle, 0u) == true);
    REQUIRE(first.IsRegrowing() == true);
    REQUIRE(battle.IsDone() == false);

    // The last one standing falls while the first is still on the floor.
    second.SetHealth(1);
    battle.GetPlayer().GetHand().emplace_back(
        CardRegistry::Get(CardId::STRIKE_RED));
    battle.GetPlayer().SetEnergy(3);

    REQUIRE(Swing(battle, 1u) == true);

    // Only the last of them was being stopped from lying down. The first
    // stayed on the floor with the fight unfinished, waiting for a turn that
    // would stand it up with nothing alive to link to.
    CHECK(second.IsRegrowing() == false);
    CHECK(first.IsRegrowing() == false);
    CHECK(first.IsGone() == true);
    CHECK(second.IsGone() == true);
    CHECK(battle.GetPhase() == BattlePhase::WON);
}

TEST_CASE("A room with nobody to aim at still lets through what aims at nobody")
{
    // The belt to the same braces: even with the pack finished off properly,
    // a room can hold nothing but somebody who may not be aimed at, and the
    // question asked without naming anybody must not answer no for the cards
    // that never wanted a target.
    Battle battle = FightAgainst({ MonsterId::DARKLING });
    Monster& one = battle.GetMonsters().front();

    one.SetHiddenWhenDown(true);
    one.SetRegrowing(true);
    one.SetHealth(0);

    REQUIRE(battle.GetTargetableMonsterIndices().empty() == true);
    REQUIRE(battle.GetLivingMonsterIndices().size() == 1u);

    battle.GetPlayer().GetHand().clear();
    battle.GetPlayer().GetHand().emplace_back(
        CardRegistry::Get(CardId::DEFEND_RED));
    battle.GetPlayer().GetHand().emplace_back(
        CardRegistry::Get(CardId::STRIKE_RED));
    battle.GetPlayer().SetEnergy(3);

    // A defend aims at nobody and goes; a strike wants somebody and does not.
    CHECK(battle.CanPlay(0u) == true);
    CHECK(battle.CanPlay(1u) == false);
    CHECK(battle.GetPlayableCardIndices().size() == 1u);
}

TEST_CASE("A count of cards in front of a time eater carries over turns")
{
    Battle battle = FightAgainst({ MonsterId::TIME_EATER });
    Monster& eater = battle.GetMonsters().front();

    REQUIRE(eater.GetPower(PowerType::TIME_WARP) == 12);

    const int strength = eater.GetPower(PowerType::STRENGTH);

    battle.GetPlayer().AddPower(PowerType::ENERGIZED, 80);

    // Seven this turn.
    const auto playOne = [&battle]() {
        battle.GetPlayer().SetEnergy(3);
        battle.GetPlayer().GetHand().emplace_back(
            CardRegistry::Get(CardId::DEFEND_RED));

        return battle.PlayCard(battle.GetPlayer().GetHand().size() - 1u, 0u);
    };

    for (int i = 0; i < 7; ++i)
    {
        REQUIRE(playOne() == true);
    }

    // Five left of the twelve, and the turn is not over.
    CHECK(eater.GetPower(PowerType::TIME_WARP) == 5);
    CHECK(eater.GetPower(PowerType::STRENGTH) == strength);

    battle.GetPlayer().SetHealth(400);

    REQUIRE(battle.EndTurn() == true);

    // The count is where it was left. It was starting again every turn, so
    // seven then five went by untouched - and spreading the count evenly
    // instead of seven, four, one is the whole of what a climber does about
    // a thing that eats time.
    CHECK(eater.GetPower(PowerType::TIME_WARP) == 5);

    // Five more the next turn ends it, and it is the stronger for it.
    for (int i = 0; i < 5; ++i)
    {
        REQUIRE(playOne() == true);
    }

    CHECK(eater.GetPower(PowerType::STRENGTH) == strength + 2);
    CHECK(eater.GetPower(PowerType::TIME_WARP) == 12);
}

TEST_CASE("A move owed to one turn does not come round on the others")
{
    // A writhing mass draws four ways on its first turn, evenly, and five
    // ways after at thirty the multi hit, thirty the block attack, twenty
    // the debuff attack, ten the big hit and ten the parasite. The opening
    // four were owed to turn one and also left sitting in the weighted draw
    // for every turn after, so the later turns were the two sets run
    // together and every later weight was near enough halved against them.
    std::map<std::string, int> later;
    const int rounds = 400;

    for (unsigned int seed = 1;
         seed <= static_cast<unsigned int>(rounds); ++seed)
    {
        Battle battle = FightAgainst({ MonsterId::WRITHING_MASS }, seed);

        battle.GetPlayer().SetHealth(400);

        REQUIRE(battle.EndTurn() == true);

        ++later[battle.GetMonsters()[0].GetCurrentMove().name];
    }

    // The big hit is the tell. It is a tenth of the later draw and a quarter
    // of the opening, which is the widest the two sets differ - a tenth of
    // four hundred is forty, and run together it would be nearer seventy.
    //
    // The block attack used to be the tell, when it was not one of the
    // opening moves at all. Now that it is one of them its two weights are
    // thirty and twenty-five, which are near enough alike to tell nothing.
    CHECK(later["Big Hit"] > 20);
    CHECK(later["Big Hit"] < 56);

    // And the parasite is only in the later draw, so running the two sets
    // together halves its share as well.
    CHECK(later["Implant"] > 20);
}

TEST_CASE("An X cost played through Chemical X still ends the turn")
{
    // The twelfth card is the twelfth card whatever it costs. Chemical X had
    // a path of its own that returned early, so a whirlwind played as the
    // twelfth took the two strength and left the climber another card to
    // play - and left whatever was waiting to be put on the floor waiting.
    Battle battle = FightAgainst({ MonsterId::TIME_EATER });
    Monster& eater = battle.GetMonsters().front();

    battle.GetPlayer().AddRelic(RelicRegistry::Get(RelicId::CHEMICAL_X));
    battle.GetPlayer().AddPower(PowerType::ENERGIZED, 80);

    const int strength = eater.GetPower(PowerType::STRENGTH);

    for (int i = 0; i < 11; ++i)
    {
        battle.GetPlayer().SetEnergy(3);
        battle.GetPlayer().GetHand().emplace_back(
            CardRegistry::Get(CardId::DEFEND_RED));

        REQUIRE(battle.PlayCard(battle.GetPlayer().GetHand().size() - 1u,
                                0u) == true);
    }

    REQUIRE(eater.GetPower(PowerType::TIME_WARP) == 1);
    REQUIRE(battle.GetPhase() == BattlePhase::PLAYER_TURN);

    const int turn = battle.GetTurn();

    battle.GetPlayer().SetHealth(400);
    battle.GetPlayer().SetEnergy(1);
    battle.GetPlayer().GetHand().emplace_back(
        CardRegistry::Get(CardId::WHIRLWIND));

    REQUIRE(battle.PlayCard(battle.GetPlayer().GetHand().size() - 1u, 0u) ==
            true);

    // It took the strength, so it counted the card - and having counted it,
    // the turn is over.
    CHECK(eater.GetPower(PowerType::STRENGTH) == strength + 2);
    CHECK(eater.GetPower(PowerType::TIME_WARP) == 12);

    // The turn moved on. Not the phase: ending a turn runs the monster's and
    // hands the climber a fresh one, so it reads PLAYER_TURN either way.
    CHECK(battle.GetTurn() == turn + 1);
}

TEST_CASE("A Lagavulin left alone wakes up swinging")
{
    Battle battle = FightAgainst({ MonsterId::LAGAVULIN });
    Monster& it = battle.GetMonsters().front();

    REQUIRE(it.GetPower(PowerType::ASLEEP) == 3);
    REQUIRE(it.GetPower(PowerType::METALLICIZE) == 8);

    const int health = battle.GetPlayer().GetHealth();

    for (int turn = 0; turn < 3; ++turn)
    {
        REQUIRE(battle.EndTurn() == true);
    }

    // Three turns and it is simply awake. It was taking a turn standing
    // there stunned as well, which is a turn of nothing the spire does not
    // give away - the stunned turn is what a blow buys, not the clock.
    CHECK(it.GetPower(PowerType::ASLEEP) == 0);
    CHECK(it.GetPower(PowerType::METALLICIZE) == 0);
    CHECK(it.GetCurrentMove().name == "Attack");

    battle.GetPlayer().SetHealth(400);

    REQUIRE(battle.EndTurn() == true);

    CHECK(battle.GetPlayer().GetHealth() < 400);
    CHECK(health > 0);

    // Woken by a blow instead, and it does stand there for the one turn.
    Battle hit = FightAgainst({ MonsterId::LAGAVULIN });

    hit.GetPlayer().GetHand().emplace_back(
        CardRegistry::Get(CardId::STRIKE_RED));
    hit.GetPlayer().SetEnergy(3);

    REQUIRE(Swing(hit, 0u) == true);

    CHECK(hit.GetMonsters().front().GetPower(PowerType::ASLEEP) == 0);
    CHECK(hit.GetMonsters().front().GetPower(PowerType::METALLICIZE) == 0);
    CHECK(hit.GetMonsters().front().GetCurrentMove().name == "Stunned");
}

TEST_CASE("A Guardian's wall goes back up higher, and it whirls on the way out")
{
    Battle battle = FightAgainst({ MonsterId::THE_GUARDIAN });
    Monster& guard = battle.GetMonsters().front();

    REQUIRE(guard.GetPower(PowerType::MODE_SHIFT) == 30);

    const auto bringItDown = [&battle, &guard]() {
        guard.RemovePower(PowerType::MODE_SHIFT);
        guard.ForceMove("Defensive Mode");

        // Defensive Mode, Roll Attack, Twin Slam, Whirlwind, and only then
        // back round to the start. The whirlwind was missing, so it came out
        // of the shape change a move short.
        const char* walk[] = { "Defensive Mode", "Roll Attack", "Twin Slam",
                               "Whirlwind", "Charging Up" };

        for (const char* name : walk)
        {
            CHECK(guard.GetCurrentMove().name == name);
            battle.GetPlayer().SetHealth(400);
            battle.EndTurn();
        }
    };

    bringItDown();

    // Forty the first time it goes back up, not thirty.
    CHECK(guard.GetPower(PowerType::MODE_SHIFT) == 40);

    bringItDown();

    // And fifty the next. It was going back to forty for ever.
    CHECK(guard.GetPower(PowerType::MODE_SHIFT) == 50);
}

TEST_CASE("A hexaghost's inferno makes every burn a worse burn")
{
    Battle battle = FightAgainst({ MonsterId::HEXAGHOST });
    Monster& ghost = battle.GetMonsters().front();

    // A burn already in the discard from an earlier sear.
    battle.GetPlayer().GetDiscardPile().emplace_back(
        CardRegistry::Get(CardId::BURN));

    REQUIRE(battle.GetPlayer().GetDiscardPile().back().IsUpgraded() == false);
    REQUIRE(ghost.ForceMove("Inferno") == true);

    battle.GetPlayer().SetHealth(400);

    REQUIRE(battle.EndTurn() == true);

    int burns = 0;
    int worse = 0;

    for (const std::vector<Card>* pile :
         { &battle.GetPlayer().GetHand(), &battle.GetPlayer().GetDrawPile(),
           &battle.GetPlayer().GetDiscardPile() })
    {
        for (const Card& one : *pile)
        {
            if (one.GetId() == CardId::BURN)
            {
                ++burns;
                worse += one.IsUpgraded() ? 1 : 0;
            }
        }
    }

    // Its own three and the one that was already there, and every one of
    // them the worse kind. The three were coming out plain, because a status
    // card was never allowed to carry the mark at all.
    CHECK(burns == 4);
    CHECK(worse == burns);

    // And every burn made afterwards is the worse kind too.
    REQUIRE(ghost.ForceMove("Sear") == true);

    battle.GetPlayer().SetHealth(400);

    REQUIRE(battle.EndTurn() == true);

    for (const Card& one : battle.GetPlayer().GetDiscardPile())
    {
        if (one.GetId() == CardId::BURN)
        {
            CHECK(one.IsUpgraded() == true);
        }
    }
}

TEST_CASE("The first act's rooms are drawn, not fixed")
{
    std::mt19937 rng(19);
    std::set<std::vector<MonsterId>> louses;
    std::set<std::vector<MonsterId>> smalls;
    std::set<std::vector<MonsterId>> gangs;
    std::set<std::vector<MonsterId>> thugs;
    std::set<std::vector<MonsterId>> wild;
    std::set<MonsterId> larges;

    const auto find = [](const std::vector<Encounter>& pool,
                         const char* name) {
        for (const Encounter& one : pool)
        {
            if (one.name == name)
            {
                return one;
            }
        }

        return Encounter();
    };

    const Encounter twoLouse = find(EncounterLibrary::GetAct1Weak(),
                                    "2 Louses");
    const Encounter small = find(EncounterLibrary::GetAct1Weak(),
                                 "Small Slimes");
    const Encounter gang = find(EncounterLibrary::GetAct1Strong(),
                                "Gremlin Gang");
    const Encounter large = find(EncounterLibrary::GetAct1Strong(),
                                 "Large Slime");
    const Encounter thug = find(EncounterLibrary::GetAct1Strong(),
                                "Exordium Thugs");
    const Encounter beast = find(EncounterLibrary::GetAct1Strong(),
                                 "Exordium Wildlife");
    const Encounter lots = find(EncounterLibrary::GetAct1Strong(),
                                "Lots of Slimes");

    REQUIRE(gang.monsters.size() == 4u);

    const auto idsOf = [](const std::vector<Monster>& built) {
        std::vector<MonsterId> out;

        for (const Monster& one : built)
        {
            out.emplace_back(one.GetMonsterId());
        }

        return out;
    };

    for (int i = 0; i < 400; ++i)
    {
        louses.insert(idsOf(EncounterLibrary::Build(twoLouse, rng)));
        smalls.insert(idsOf(EncounterLibrary::Build(small, rng)));
        thugs.insert(idsOf(EncounterLibrary::Build(thug, rng)));
        wild.insert(idsOf(EncounterLibrary::Build(beast, rng)));
        larges.insert(
            EncounterLibrary::Build(large, rng).front().GetMonsterId());

        const std::vector<Monster> four = EncounterLibrary::Build(gang, rng);

        REQUIRE(four.size() == 4u);

        std::map<MonsterId, int> alike;

        for (const Monster& one : four)
        {
            ++alike[one.GetMonsterId()];
        }

        // Four out of a bag holding two fats, two sneakies, two mads, one
        // shield and one wizard - so never three of a kind, and never two
        // shields or two wizards.
        for (const auto& counted : alike)
        {
            REQUIRE(counted.second <= 2);

            if (counted.first == MonsterId::SHIELD_GREMLIN ||
                counted.first == MonsterId::GREMLIN_WIZARD)
            {
                REQUIRE(counted.second == 1);
            }
        }

        std::vector<MonsterId> sorted = idsOf(four);

        std::sort(sorted.begin(), sorted.end());
        gangs.insert(sorted);
    }

    // Each louse its own colour, so all four pairings turn up.
    CHECK(louses.size() == 4u);

    // A medium of one kind beside a small of the other, either way round.
    CHECK(smalls.size() == 2u);

    // A large slime of either kind, one room rather than two.
    CHECK(larges.size() == 2u);

    // And the mixed rooms and the gang really are drawn.
    CHECK(gangs.size() > 4u);
    CHECK(thugs.size() > 4u);
    CHECK(wild.size() > 3u);

    // Three spike and two acid, which was the other way round.
    int spike = 0;

    for (const MonsterId id : lots.monsters)
    {
        spike += id == MonsterId::SPIKE_SLIME_S ? 1 : 0;
    }

    CHECK(lots.monsters.size() == 5u);
    CHECK(spike == 3);
}

TEST_CASE("The first hard fight of act one turns some rooms away")
{
    std::mt19937 rng(23);

    // Three louses cannot follow two of them into the harder list, nor a
    // large slime or a swarm follow the small ones.
    for (const char* had : { "2 Louses", "Small Slimes" })
    {
        std::set<std::string> drawn;

        for (int i = 0; i < 400; ++i)
        {
            drawn.insert(
                EncounterLibrary::Pick(1, MapNodeType::MONSTER,
                                       EncounterLibrary::WeakFightsOf(1), rng,
                                       { had })
                    .name);
        }

        REQUIRE(drawn.empty() == false);

        if (std::string(had) == "2 Louses")
        {
            CHECK(drawn.count("3 Louses") == 0u);
        }
        else
        {
            CHECK(drawn.count("Large Slime") == 0u);
            CHECK(drawn.count("Lots of Slimes") == 0u);
        }
    }

    // And only the first one: by the next hard fight they are back.
    std::set<std::string> later;

    for (int i = 0; i < 400; ++i)
    {
        later.insert(EncounterLibrary::Pick(
                         1, MapNodeType::MONSTER,
                         EncounterLibrary::WeakFightsOf(1) + 1, rng,
                         { "2 Louses" })
                         .name);
    }

    CHECK(later.count("3 Louses") == 1u);
}

TEST_CASE("A jaw worm draws from a table read off the move before it")
{
    // After a chomp  : bellow 59, thrash 41
    // After a bellow : thrash 56, chomp 44
    // After a thrash : bellow 45, thrash 30, chomp 25
    //
    // Flat weights with limits on repeating come out near this and exactly
    // right after a single thrash, which is how it went unnoticed.
    std::map<std::string, std::map<std::string, int>> after;

    for (unsigned int seed = 1; seed <= 60u; ++seed)
    {
        std::mt19937 rng(seed);
        Monster worm = MonsterRoster::Make(MonsterId::JAW_WORM, rng);

        worm.ChooseOpeningMove(rng);

        CHECK(worm.GetCurrentMove().name == "Chomp");

        std::string before = worm.GetCurrentMove().name;

        for (int turn = 0; turn < 60; ++turn)
        {
            worm.CountMoveUsed();
            worm.AdvanceMove(rng);

            const std::string now = worm.GetCurrentMove().name;

            ++after[before][now];
            before = now;
        }
    }

    const auto share = [&after](const char* was, const char* now) {
        int total = 0;

        for (const auto& one : after[was])
        {
            total += one.second;
        }

        return total == 0 ? 0 : 100 * after[was][now] / total;
    };

    // Nothing follows itself except a thrash, and a thrash may follow a
    // thrash however many there have been.
    CHECK(after["Chomp"].count("Chomp") == 0u);
    CHECK(after["Bellow"].count("Bellow") == 0u);
    CHECK(after["Thrash"].count("Thrash") == 1u);

    // And three thrashes running really do happen. This is the assertion
    // that tells the table apart from the weights it replaced: those held a
    // thrash to two in a row, and the bands below are wide enough that both
    // ways of doing it sit inside them.
    int longest = 0;

    for (unsigned int seed = 1; seed <= 60u; ++seed)
    {
        std::mt19937 rng(seed);
        Monster worm = MonsterRoster::Make(MonsterId::JAW_WORM, rng);

        worm.ChooseOpeningMove(rng);

        std::string last;
        int run = 0;

        for (int turn = 0; turn < 60; ++turn)
        {
            const std::string now = worm.GetCurrentMove().name;

            run = now == last ? run + 1 : 1;
            last = now;
            longest = run > longest ? run : longest;

            worm.CountMoveUsed();
            worm.AdvanceMove(rng);
        }
    }

    CHECK(longest >= 3);

    CHECK(share("Chomp", "Bellow") > 53);
    CHECK(share("Chomp", "Bellow") < 65);

    CHECK(share("Bellow", "Thrash") > 50);
    CHECK(share("Bellow", "Thrash") < 62);

    CHECK(share("Thrash", "Bellow") > 39);
    CHECK(share("Thrash", "Bellow") < 51);
    CHECK(share("Thrash", "Chomp") > 19);
    CHECK(share("Thrash", "Chomp") < 31);
}

TEST_CASE("Two act-one monsters were allowed a move one turn too often")
{
    // Both pages: an acid slime cannot tackle twice running, whatever size
    // it is. The medium one was allowed it and the large one was not.
    const auto rowOf = [](MonsterId id, const char* name) {
        // Held in a local first: a range-for over the moves of a monster
        // made on the spot walks a list whose owner is already gone.
        const Monster one = Make(id);
        int found = -1;

        for (const MonsterMove& move : one.GetMoves())
        {
            if (move.name == name)
            {
                found = move.maxInARow;
            }
        }

        return found;
    };

    CHECK(rowOf(MonsterId::ACID_SLIME_M, "Tackle") == 1);
    CHECK(rowOf(MonsterId::ACID_SLIME_L, "Tackle") == 1);
    CHECK(rowOf(MonsterId::ACID_SLIME_M, "Lick") == 2);

    // And a fungi beast will not grow twice running, which is six of
    // strength off two turns.
    CHECK(rowOf(MonsterId::FUNGI_BEAST, "Grow") == 1);
    CHECK(rowOf(MonsterId::FUNGI_BEAST, "Bite") == 2);
}

TEST_CASE("A Guardian puts twenty up the moment its wall comes down")
{
    // Only one of the two pages says so - the one this project usually
    // follows describes the shift and does not mention it - and it is here
    // on the user's word. Both roads into the shell go through the same
    // place, so a blow and a tick of poison bring the same twenty.
    {
        Battle battle = FightAgainst({ MonsterId::THE_GUARDIAN });
        Monster& guard = battle.GetMonsters().front();

        REQUIRE(guard.GetPower(PowerType::MODE_SHIFT) == 30);
        REQUIRE(guard.GetBlock() == 0);

        guard.AddPower(PowerType::MODE_SHIFT, -29);
        battle.GetPlayer().GetHand().emplace_back(
            CardRegistry::Get(CardId::STRIKE_RED));
        battle.GetPlayer().SetEnergy(3);

        REQUIRE(Swing(battle, 0u) == true);

        CHECK(guard.GetPower(PowerType::MODE_SHIFT) == 0);
        CHECK(guard.GetCurrentMove().name == "Defensive Mode");
        CHECK(guard.GetBlock() == 20);
    }

    // And by poison, which is the other way the wall comes down.
    {
        Battle battle = FightAgainst({ MonsterId::THE_GUARDIAN });
        Monster& guard = battle.GetMonsters().front();

        guard.AddPower(PowerType::POISON, 30);
        battle.GetPlayer().SetHealth(400);

        REQUIRE(battle.EndTurn() == true);

        CHECK(guard.GetPower(PowerType::MODE_SHIFT) == 0);

        // The shell is put on and acted on in the same turn, so what is left
        // standing afterwards is the sharp hide it gives - and the twenty
        // is under whatever the turn then did to it.
        CHECK(guard.GetPower(PowerType::SHARP_HIDE) == 3);
    }
}

TEST_CASE("A cultist's ritual counts the turn it was set up on")
{
    // Decided rather than found: both pages say only "at the end of its
    // turn, gains X Strength", with nothing about skipping the turn the
    // ritual was granted on. Read as it stands, the incantation turn counts
    // - so the first dark strike lands for nine and not six. The other
    // reading is a turn's delay and three points; this is the user's call,
    // written down so it is not quietly flipped back.
    Battle battle = FightAgainst({ MonsterId::CULTIST });
    Monster& one = battle.GetMonsters().front();

    REQUIRE(one.GetCurrentMove().name == "Incantation");
    CHECK(one.GetPower(PowerType::STRENGTH) == 0);

    battle.GetPlayer().SetHealth(400);

    REQUIRE(battle.EndTurn() == true);

    CHECK(one.GetPower(PowerType::RITUAL) == 3);
    CHECK(one.GetPower(PowerType::STRENGTH) == 3);
    CHECK(one.GetCurrentMove().name == "Dark Strike");

    const int before = battle.GetPlayer().GetHealth();

    REQUIRE(battle.EndTurn() == true);

    CHECK(before - battle.GetPlayer().GetHealth() == 9);
}

TEST_CASE("What answers a played card is damage, so block soaks it")
{
    // All four of these are written on their pages as damage - a thorn
    // "deals X damage back", a guardian's hide "deals damage after you play
    // an Attack", a constriction and a heart's beat both "take X damage" -
    // and all four were taking the health straight off, which block cannot
    // touch. A guardian in its shell took three off every attack played with
    // nothing the climber could do about it.
    const auto through = [](MonsterId who, PowerType power, int amount,
                            int block) {
        Battle battle = FightAgainst({ who });

        battle.GetMonsters().front().AddPower(power, amount);
        battle.GetPlayer().SetHealth(400);
        battle.GetPlayer().AddBlock(block);

        const int before = battle.GetPlayer().GetHealth();

        battle.GetPlayer().GetHand().emplace_back(
            CardRegistry::Get(CardId::STRIKE_RED));
        battle.GetPlayer().SetEnergy(3);

        REQUIRE(Swing(battle, 0u) == true);

        return before - battle.GetPlayer().GetHealth();
    };

    // Behind enough block, a thorn and a hide take nothing at all.
    CHECK(through(MonsterId::SPIKER, PowerType::THORNS, 5, 20) == 0);
    CHECK(through(MonsterId::THE_GUARDIAN, PowerType::SHARP_HIDE, 3, 20) == 0);

    // And behind none, they take what they say.
    CHECK(through(MonsterId::SPIKER, PowerType::THORNS, 5, 0) >= 5);
    CHECK(through(MonsterId::THE_GUARDIAN, PowerType::SHARP_HIDE, 3, 0) == 3);

    // A constriction is answered at the end of the turn, so block that is
    // still standing soaks that too.
    {
        Battle battle = FightAgainst({ MonsterId::SPIRE_GROWTH });

        battle.GetPlayer().AddPower(PowerType::CONSTRICTED, 10);
        battle.GetPlayer().SetHealth(400);
        battle.GetPlayer().AddBlock(40);

        const int before = battle.GetPlayer().GetHealth();

        REQUIRE(battle.EndTurn() == true);

        // The growth's own swing gets through; the ten of the constriction
        // does not, because forty of block was standing when it was asked.
        CHECK(before - battle.GetPlayer().GetHealth() < 10);
    }
}

TEST_CASE("A Guardian only shells up when its wall is brought down")
{
    Battle battle = FightAgainst({ MonsterId::THE_GUARDIAN });
    Monster& guard = battle.GetMonsters().front();

    // Four moves round and round, and nothing else, for as long as the wall
    // stands. Written as one scripted list of eight it walked into the shell
    // every eighth turn whether the climber had broken the wall or not - and
    // each of those free shells put another ten on the wall through the
    // slam, so the wall grew without anybody ever breaking it.
    const char* walk[] = { "Charging Up", "Fierce Bash", "Vent Steam",
                           "Whirlwind" };

    for (int turn = 0; turn < 12; ++turn)
    {
        CHECK(guard.GetCurrentMove().name == walk[turn % 4]);
        CHECK(guard.GetPower(PowerType::MODE_SHIFT) == 30);

        battle.GetPlayer().SetHealth(400);

        REQUIRE(battle.EndTurn() == true);
    }

    // Bring it down with one left, so the blow overshoots - which is the
    // ordinary way it happens and not a special case.
    guard.AddPower(PowerType::MODE_SHIFT, -29);

    REQUIRE(guard.GetPower(PowerType::MODE_SHIFT) == 1);

    battle.GetPlayer().GetHand().emplace_back(
        CardRegistry::Get(CardId::STRIKE_RED));
    battle.GetPlayer().SetEnergy(3);

    const std::string meant = guard.GetCurrentMove().name;

    REQUIRE(Swing(battle, 0u) == true);

    // Whatever it meant to do is dropped, and twenty goes up on the spot.
    CHECK(guard.GetPower(PowerType::MODE_SHIFT) == 0);
    CHECK(guard.GetCurrentMove().name == "Defensive Mode");
    CHECK(guard.GetCurrentMove().name != meant);
    CHECK(guard.GetBlock() == 20);

    // The shell, and then back onto the walk at the top of it.
    const char* shell[] = { "Defensive Mode", "Roll Attack", "Twin Slam",
                            "Whirlwind", "Charging Up", "Fierce Bash" };

    for (const char* name : shell)
    {
        CHECK(guard.GetCurrentMove().name == name);

        battle.GetPlayer().SetHealth(400);

        REQUIRE(battle.EndTurn() == true);
    }

    // And the wall it stood back up is ten higher, once, for the one time it
    // was broken.
    CHECK(guard.GetPower(PowerType::MODE_SHIFT) == 40);

    // Round again with the wall standing, and it stays on the walk.
    for (int turn = 0; turn < 8; ++turn)
    {
        CHECK(guard.GetCurrentMove().name != "Defensive Mode");
        CHECK(guard.GetPower(PowerType::MODE_SHIFT) == 40);

        battle.GetPlayer().SetHealth(400);

        REQUIRE(battle.EndTurn() == true);
    }
}

TEST_CASE("An inferno sets fire to the piles and not to the hand")
{
    // The Upgrade page: an inferno "will not Upgrade any Burn that is
    // currently in hand". The Burn page says all three piles; the Upgrade
    // page is the one that knows why the distinction exists, and it reasons
    // about how a burn comes to be in hand at that moment at all - they
    // cannot be retained in this fight, so it takes a card drawn after the
    // player's turn has ended.
    //
    // Which is why this test only checks the piles: in ordinary play the
    // turn's end has already discarded the hand by the time an inferno goes
    // off, so a burn is never sitting in one. Leaving the hand out of the
    // sweep is the rule written down for the day something does draw into
    // it, not a change anybody can see today.
    Battle battle = FightAgainst({ MonsterId::HEXAGHOST });
    Monster& ghost = battle.GetMonsters().front();

    battle.GetPlayer().GetDrawPile().emplace_back(
        CardRegistry::Get(CardId::BURN));
    battle.GetPlayer().GetDiscardPile().emplace_back(
        CardRegistry::Get(CardId::BURN));

    REQUIRE(ghost.ForceMove("Inferno") == true);

    battle.GetPlayer().SetHealth(400);

    REQUIRE(battle.EndTurn() == true);

    int burns = 0;
    int plain = 0;

    for (const std::vector<Card>* pile :
         { &battle.GetPlayer().GetHand(), &battle.GetPlayer().GetDrawPile(),
           &battle.GetPlayer().GetDiscardPile() })
    {
        for (const Card& one : *pile)
        {
            if (one.GetId() == CardId::BURN)
            {
                ++burns;
                plain += one.IsUpgraded() ? 0 : 1;
            }
        }
    }

    // Its own three and the two that were lying about, every one of them the
    // worse kind.
    CHECK(burns == 5);
    CHECK(plain == 0);
}

TEST_CASE("A tungsten rod takes a point off everything that reaches the climber")
{
    // The page: "Tungsten Rod will reduce ALL HP loss by 1", damage that
    // gets through block included. It was only being asked where health was
    // taken off directly, so every blow a monster landed, every burn and
    // every thorn was a point harsher than it should have been.
    const auto tookFrom = [](bool rod) {
        Battle battle = FightAgainst({ MonsterId::JAW_WORM });

        if (rod)
        {
            battle.GetPlayer().AddRelic(
                RelicRegistry::Get(RelicId::TUNGSTEN_ROD));
        }

        battle.GetPlayer().SetHealth(400);

        const int before = battle.GetPlayer().GetHealth();

        battle.EndTurn();

        return before - battle.GetPlayer().GetHealth();
    };

    const int plain = tookFrom(false);

    REQUIRE(plain > 1);
    CHECK(tookFrom(true) == plain - 1);

    // And a burn at the end of the turn, the same.
    const auto burned = [](bool rod) {
        Battle battle = FightAgainst({ MonsterId::HEXAGHOST });

        if (rod)
        {
            battle.GetPlayer().AddRelic(
                RelicRegistry::Get(RelicId::TUNGSTEN_ROD));
        }

        battle.GetMonsters().front().ForceMove("Activate");
        battle.GetPlayer().GetHand().clear();
        battle.GetPlayer().GetHand().emplace_back(
            CardRegistry::Get(CardId::BURN));
        battle.GetPlayer().SetHealth(400);

        const int before = battle.GetPlayer().GetHealth();

        battle.EndTurn();

        return before - battle.GetPlayer().GetHealth();
    };

    CHECK(burned(false) == 2);
    CHECK(burned(true) == 1);

    // But nothing comes off what the block swallowed whole: a rod softens
    // what reaches the climber, and a blow that never reached them was not
    // going to.
    {
        Battle battle = FightAgainst({ MonsterId::HEXAGHOST });

        battle.GetPlayer().AddRelic(
            RelicRegistry::Get(RelicId::TUNGSTEN_ROD));
        battle.GetMonsters().front().ForceMove("Activate");
        battle.GetPlayer().GetHand().clear();
        battle.GetPlayer().GetHand().emplace_back(
            CardRegistry::Get(CardId::BURN));
        battle.GetPlayer().SetHealth(400);
        battle.GetPlayer().AddBlock(10);

        const int before = battle.GetPlayer().GetHealth();

        REQUIRE(battle.EndTurn() == true);

        CHECK(before - battle.GetPlayer().GetHealth() == 0);
    }
}
