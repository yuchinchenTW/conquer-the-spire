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

    CHECK(guardian.GetBlock() == 40);
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

    int gremlins = 0;

    for (const auto& monster : battle.GetMonsters())
    {
        if (monster.GetMonsterId() == MonsterId::MAD_GREMLIN)
        {
            ++gremlins;
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
