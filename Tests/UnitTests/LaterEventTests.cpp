#include "doctest.h"

#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Events/EventLibrary.hpp>
#include <conquer-the-spire/Relics/RelicRegistry.hpp>
#include <conquer-the-spire/Run/Run.hpp>

#include <algorithm>
#include <cstddef>
#include <random>
#include <string>
#include <vector>

using namespace ConquerTheSpire;

namespace
{
//! Opens a run of \p act standing in the room \p id.
Run RunInRoom(EventId id, int act = 2, unsigned int seed = 5)
{
    Run run(CardColor::RED, seed);

    if (act != 1)
    {
        run.BeginAct(act);
    }

    run.StartEvent(id);

    return run;
}

//! Returns the index of the option labelled \p label.
std::size_t OptionOf(const Event& event, const std::string& label)
{
    const std::vector<EventOption>& options = event.GetOptions();

    for (std::size_t i = 0; i < options.size(); ++i)
    {
        if (options[i].label == label)
        {
            return i;
        }
    }

    return options.size();
}

//! Counts the cards of \p id in the deck.
int Count(const Run& run, CardId id)
{
    int count = 0;

    for (const auto& card : run.GetDeck())
    {
        if (card.GetId() == id)
        {
            ++count;
        }
    }

    return count;
}

//! Counts the upgraded cards of the deck.
int Upgraded(const Run& run)
{
    int count = 0;

    for (const auto& card : run.GetDeck())
    {
        if (card.IsUpgraded())
        {
            ++count;
        }
    }

    return count;
}
}  // namespace

TEST_CASE("Every room of the later acts is built with something to choose")
{
    std::vector<EventId> all = EventLibrary::GetAct2Rooms();
    const std::vector<EventId>& third = EventLibrary::GetAct3Rooms();

    all.insert(all.end(), third.begin(), third.end());

    for (const EventId id : all)
    {
        const Event event = EventLibrary::Get(id);

        CHECK(event.GetId() == id);
        CHECK(event.GetName().empty() == false);
        CHECK(event.GetKind() != EventKind::INVALID);
        CHECK(event.GetOptions().empty() == false);
    }

    // Each act draws from its own list.
    const std::vector<EventId>& second = EventLibrary::GetAct2Rooms();

    CHECK(std::find(second.begin(), second.end(), EventId::VAMPIRES) !=
          second.end());
    CHECK(std::find(second.begin(), second.end(), EventId::MIND_BLOOM) ==
          second.end());
    CHECK(std::find(third.begin(), third.end(), EventId::MIND_BLOOM) !=
          third.end());
}

TEST_CASE("An act draws its rooms from its own list")
{
    Run run(CardColor::RED, 21);

    run.BeginAct(2);

    const std::vector<EventId>& second = EventLibrary::GetAct2Rooms();
    const std::vector<EventId>& shrines = EventLibrary::GetShrines();

    for (int i = 0; i < 20; ++i)
    {
        const Event& event = run.StartEvent();

        if (event.GetId() == EventId::INVALID)
        {
            continue;
        }

        const bool known =
            std::find(second.begin(), second.end(), event.GetId()) !=
                second.end() ||
            std::find(shrines.begin(), shrines.end(), event.GetId()) !=
                shrines.end();

        CHECK(known == true);

        while (run.HasEvent())
        {
            const std::size_t last = run.GetEvent().GetOptions().size() - 1;

            if (!run.ChooseEventOption(last))
            {
                break;
            }
        }
    }
}

TEST_CASE("Simple writing sharpens the plain cards of a deck")
{
    Run run = RunInRoom(EventId::ANCIENT_WRITING);

    const std::size_t simplicity = OptionOf(run.GetEvent(), "Simplicity");

    REQUIRE(simplicity < run.GetEvent().GetOptions().size());
    REQUIRE(run.ChooseEventOption(simplicity) == true);

    // A starting deck is nothing but plain cards.
    CHECK(Upgraded(run) == static_cast<int>(run.GetDeck().size()));
}

TEST_CASE("A council of ghosts is paid for with half of the whole")
{
    Run run = RunInRoom(EventId::COUNCIL_OF_GHOSTS);

    const int whole = run.GetPlayer().GetMaxHealth();

    REQUIRE(run.ChooseEventOption(0) == true);

    CHECK(run.GetPlayer().GetMaxHealth() == whole - whole / 2);
    CHECK(Count(run, CardId::APPARITION) == 5);
}

TEST_CASE("A cursed tome costs sixteen and hands over one of three books")
{
    Run run = RunInRoom(EventId::CURSED_TOME);

    const int health = run.GetPlayer().GetHealth();

    REQUIRE(run.ChooseEventOption(0) == true);

    CHECK(run.GetPlayer().GetHealth() == health - 6);
    REQUIRE(run.HasEvent() == true);
    REQUIRE(run.ChooseEventOption(0) == true);

    CHECK(run.GetPlayer().GetHealth() == health - 16);

    const bool book = run.GetPlayer().HasRelic(RelicId::ENCHIRIDION) ||
                      run.GetPlayer().HasRelic(RelicId::NILRYS_CODEX) ||
                      run.GetPlayer().HasRelic(RelicId::NECRONOMICON);

    CHECK(book == true);
}

TEST_CASE("An altar only takes an idol from whoever has one")
{
    Run without = RunInRoom(EventId::FORGOTTEN_ALTAR);

    CHECK(without.CanChooseEventOption(0) == false);

    Run with = RunInRoom(EventId::FORGOTTEN_ALTAR);

    with.AddRelic(RelicId::GOLDEN_IDOL);

    REQUIRE(with.CanChooseEventOption(0) == true);
    REQUIRE(with.ChooseEventOption(0) == true);

    CHECK(with.GetPlayer().HasRelic(RelicId::GOLDEN_IDOL) == false);
    CHECK(with.GetPlayer().HasRelic(RelicId::BLOODY_IDOL) == true);
}

TEST_CASE("The bandits take everything, or take a beating")
{
    Run paid = RunInRoom(EventId::MASKED_BANDITS);

    paid.AddGold(200);

    REQUIRE(paid.ChooseEventOption(0) == true);

    CHECK(paid.GetGold() == 0);

    Run fought = RunInRoom(EventId::MASKED_BANDITS);

    REQUIRE(fought.ChooseEventOption(1) == true);
    REQUIRE(fought.HasPendingFight() == true);

    Battle battle = fought.StartPendingBattle();

    REQUIRE(battle.GetMonsters().size() == 3u);

    for (auto& monster : battle.GetMonsters())
    {
        monster.SetHealth(0);
    }

    battle.EndTurn();
    fought.FinishBattle(battle);

    CHECK(fought.GetPlayer().HasRelic(RelicId::RED_MASK) == true);
}

TEST_CASE("The colosseum keeps its door open for a second fight")
{
    Run run = RunInRoom(EventId::THE_COLOSSEUM);

    REQUIRE(run.ChooseEventOption(0) == true);
    REQUIRE(run.HasPendingFight() == true);

    // The room is not done: whoever wins is asked to stay.
    CHECK(run.HasEvent() == true);
    CHECK(run.GetEvent().GetStage() == 1);

    Battle first = run.StartPendingBattle();

    for (auto& monster : first.GetMonsters())
    {
        monster.SetHealth(0);
    }

    first.EndTurn();
    run.FinishBattle(first);

    const int purse = run.GetGold();
    const std::size_t carried = run.GetPlayer().GetRelics().size();

    REQUIRE(run.ChooseEventOption(OptionOf(run.GetEvent(), "Victory")) ==
            true);

    CHECK(run.GetGold() == purse + 100);
    CHECK(run.GetPlayer().GetRelics().size() == carried + 2);
    CHECK(run.HasPendingFight() == true);
}

TEST_CASE("A library offers twenty cards or a good sleep")
{
    Run read = RunInRoom(EventId::THE_LIBRARY);

    REQUIRE(read.ChooseEventOption(0) == true);

    REQUIRE(read.GetRewards().empty() == false);
    CHECK(read.GetRewards().front().kind == RewardKind::CARD_CHOICE);
    CHECK(read.GetRewards().front().cards.size() > 10u);

    Run slept = RunInRoom(EventId::THE_LIBRARY);

    slept.GetPlayer().LoseHealth(50);

    const int hurt = slept.GetPlayer().GetHealth();

    REQUIRE(slept.ChooseEventOption(1) == true);

    CHECK(slept.GetPlayer().GetHealth() == hurt + 80 * 33 / 100);
}

TEST_CASE("A mausoleum is a coffin worth opening, mostly")
{
    int cursed = 0;
    int clean = 0;

    for (unsigned int seed = 1; seed < 20; ++seed)
    {
        Run run(CardColor::RED, seed);

        run.BeginAct(2);
        run.StartEvent(EventId::THE_MAUSOLEUM);

        REQUIRE(run.ChooseEventOption(0) == true);

        CHECK(run.GetPlayer().GetRelics().size() >= 2u);

        if (Count(run, CardId::WRITHE) > 0)
        {
            ++cursed;
        }
        else
        {
            ++clean;
        }
    }

    // Half the time, either way.
    CHECK(cursed > 0);
    CHECK(clean > 0);
}

TEST_CASE("The vampires trade every plain strike for a bite")
{
    Run run = RunInRoom(EventId::VAMPIRES);

    const int whole = run.GetPlayer().GetMaxHealth();
    const int strikes = Count(run, CardId::STRIKE_RED);

    REQUIRE(strikes > 0);

    const std::size_t accept = OptionOf(run.GetEvent(), "Accept");

    REQUIRE(run.ChooseEventOption(accept) == true);

    CHECK(Count(run, CardId::STRIKE_RED) == 0);
    CHECK(Count(run, CardId::BITE) == 5);
    CHECK(run.GetPlayer().GetMaxHealth() == whole - whole * 30 / 100);
}

TEST_CASE("A blood vial buys the bites for nothing")
{
    Run run = RunInRoom(EventId::VAMPIRES);

    run.AddRelic(RelicId::BLOOD_VIAL);

    const int whole = run.GetPlayer().GetMaxHealth();

    REQUIRE(run.CanChooseEventOption(0) == true);
    REQUIRE(run.ChooseEventOption(0) == true);

    CHECK(run.GetPlayer().HasRelic(RelicId::BLOOD_VIAL) == false);
    CHECK(Count(run, CardId::BITE) == 5);
    CHECK(run.GetPlayer().GetMaxHealth() == whole);
}

TEST_CASE("A nest pays in gold or in a dagger")
{
    Run gold = RunInRoom(EventId::THE_NEST);

    const int purse = gold.GetGold();

    REQUIRE(gold.ChooseEventOption(0) == true);

    CHECK(gold.GetGold() == purse + 99);

    Run dagger = RunInRoom(EventId::THE_NEST);
    const int health = dagger.GetPlayer().GetHealth();

    REQUIRE(dagger.ChooseEventOption(1) == true);

    CHECK(dagger.GetPlayer().GetHealth() == health - 6);
    CHECK(Count(dagger, CardId::RITUAL_DAGGER) == 1);
}

TEST_CASE("A beggar and a vagrant both want paying")
{
    Run beggar = RunInRoom(EventId::OLD_BEGGAR);

    CHECK(beggar.CanChooseEventOption(0) == true);

    const std::size_t held = beggar.GetDeck().size();

    REQUIRE(beggar.ChooseEventOption(0) == true);

    CHECK(beggar.GetGold() == 99 - 75);
    CHECK(beggar.GetDeck().size() == held - 1);

    Run vagrant = RunInRoom(EventId::PLEADING_VAGRANT);

    // Eighty-five is affordable at the start of a run, and not once some of
    // it has been spent.
    CHECK(vagrant.CanChooseEventOption(0) == true);

    REQUIRE(vagrant.SpendGold(20) == true);

    CHECK(vagrant.CanChooseEventOption(0) == false);

    const std::size_t carried = vagrant.GetPlayer().GetRelics().size();

    REQUIRE(vagrant.ChooseEventOption(1) == true);

    CHECK(vagrant.GetPlayer().GetRelics().size() == carried + 1);
    CHECK(Count(vagrant, CardId::SHAME) == 1);
}

TEST_CASE("Falling costs a card of the kind that was reached for")
{
    Run run = RunInRoom(EventId::FALLING, 3);

    const int skills = Count(run, CardId::DEFEND_RED);

    REQUIRE(skills > 0);
    REQUIRE(run.ChooseEventOption(0) == true);

    CHECK(Count(run, CardId::DEFEND_RED) == skills - 1);
}

TEST_CASE("Mind bloom offers a boss, a whetstone or a fortune")
{
    Run war = RunInRoom(EventId::MIND_BLOOM, 3);

    REQUIRE(war.ChooseEventOption(0) == true);
    REQUIRE(war.HasPendingFight() == true);

    Battle battle = war.StartPendingBattle();

    CHECK(battle.GetMonsters().empty() == false);

    Run awake = RunInRoom(EventId::MIND_BLOOM, 3);

    REQUIRE(awake.ChooseEventOption(1) == true);

    CHECK(Upgraded(awake) == static_cast<int>(awake.GetDeck().size()));
    CHECK(awake.GetPlayer().HasRelic(RelicId::MARK_OF_THE_BLOOM) == true);

    Run rich = RunInRoom(EventId::MIND_BLOOM, 3);
    const int purse = rich.GetGold();

    REQUIRE(rich.ChooseEventOption(2) == true);

    CHECK(rich.GetGold() == purse + 999);
    CHECK(Count(rich, CardId::NORMALITY) == 2);
}

TEST_CASE("A sphere is a fight for something rare")
{
    Run run = RunInRoom(EventId::MYSTERIOUS_SPHERE, 3);

    REQUIRE(run.ChooseEventOption(0) == true);
    REQUIRE(run.HasPendingFight() == true);

    Battle battle = run.StartPendingBattle();

    REQUIRE(battle.GetMonsters().size() == 2u);

    for (auto& monster : battle.GetMonsters())
    {
        monster.SetHealth(0);
    }

    battle.EndTurn();
    run.FinishBattle(battle);

    // Something rare came out of it.
    bool rare = false;

    for (const auto& relic : run.GetPlayer().GetRelics())
    {
        rare = rare || relic.GetTier() == RelicTier::RARE;
    }

    CHECK(rare == true);
}

TEST_CASE("A sensory stone charges by the handful")
{
    Run run = RunInRoom(EventId::SENSORY_STONE, 3);

    const int health = run.GetPlayer().GetHealth();

    REQUIRE(run.ChooseEventOption(2) == true);

    CHECK(run.GetPlayer().GetHealth() == health - 10);

    int picks = 0;

    for (const auto& reward : run.GetRewards())
    {
        if (reward.kind == RewardKind::CARD_CHOICE)
        {
            ++picks;

            for (const CardId id : reward.cards)
            {
                CHECK(CardRegistry::Get(id).GetColor() ==
                      CardColor::COLORLESS);
            }
        }
    }

    CHECK(picks == 3);
}

TEST_CASE("A head is worth a whole skin or a golden idol")
{
    Run jump = RunInRoom(EventId::THE_MOAI_HEAD, 3);

    jump.GetPlayer().LoseHealth(50);

    const int whole = jump.GetPlayer().GetMaxHealth();

    REQUIRE(jump.ChooseEventOption(0) == true);

    CHECK(jump.GetPlayer().GetMaxHealth() == whole - whole * 12 / 100);
    CHECK(jump.GetPlayer().GetHealth() == jump.GetPlayer().GetMaxHealth());

    Run idol = RunInRoom(EventId::THE_MOAI_HEAD, 3);

    CHECK(idol.CanChooseEventOption(1) == false);

    idol.AddRelic(RelicId::GOLDEN_IDOL);

    const int purse = idol.GetGold();

    REQUIRE(idol.ChooseEventOption(1) == true);

    CHECK(idol.GetGold() == purse + 333);
    CHECK(idol.GetPlayer().HasRelic(RelicId::GOLDEN_IDOL) == false);
}

TEST_CASE("A tomb pays whoever wears the mask and sells it to whoever does not")
{
    Run wearing = RunInRoom(EventId::TOMB_OF_LORD_RED_MASK, 3);

    wearing.AddRelic(RelicId::RED_MASK);

    const int purse = wearing.GetGold();

    REQUIRE(wearing.CanChooseEventOption(0) == true);
    REQUIRE(wearing.ChooseEventOption(0) == true);

    CHECK(wearing.GetGold() == purse + 222);

    Run buying = RunInRoom(EventId::TOMB_OF_LORD_RED_MASK, 3);

    CHECK(buying.CanChooseEventOption(0) == false);

    REQUIRE(buying.ChooseEventOption(1) == true);

    CHECK(buying.GetGold() == 0);
    CHECK(buying.GetPlayer().HasRelic(RelicId::RED_MASK) == true);
}

TEST_CASE("Winding halls charge in health whichever way is taken")
{
    Run madness = RunInRoom(EventId::WINDING_HALLS, 3);
    const int whole = madness.GetPlayer().GetMaxHealth();

    REQUIRE(madness.ChooseEventOption(0) == true);

    CHECK(madness.GetPlayer().GetMaxHealth() == whole - whole * 12 / 100);
    CHECK(Count(madness, CardId::MADNESS) == 2);

    Run focus = RunInRoom(EventId::WINDING_HALLS, 3);

    focus.GetPlayer().LoseHealth(40);

    const int hurt = focus.GetPlayer().GetHealth();

    REQUIRE(focus.ChooseEventOption(1) == true);

    CHECK(focus.GetPlayer().GetHealth() == hurt + 80 / 4);
    CHECK(Count(focus, CardId::WRITHE) == 1);

    Run back = RunInRoom(EventId::WINDING_HALLS, 3);

    REQUIRE(back.ChooseEventOption(2) == true);

    CHECK(back.GetPlayer().GetMaxHealth() == 80 - 80 * 5 / 100);
}

TEST_CASE("A designer charges by the job")
{
    Run run = RunInRoom(EventId::DESIGNER_IN_SPIRE);

    run.AddGold(100);

    REQUIRE(run.CanChooseEventOption(0) == true);
    REQUIRE(run.ChooseEventOption(0, { 0 }) == true);

    CHECK(run.GetGold() == 199 - 40);
    CHECK(run.GetDeck()[0].IsUpgraded() == true);

    Run punched = RunInRoom(EventId::DESIGNER_IN_SPIRE);
    const int health = punched.GetPlayer().GetHealth();

    REQUIRE(punched.ChooseEventOption(3) == true);

    CHECK(punched.GetPlayer().GetHealth() == health - 3);
}

TEST_CASE("A skull charges more every time the same question is asked")
{
    Run run = RunInRoom(EventId::KNOWING_SKULL);

    const int health = run.GetPlayer().GetHealth();

    // A tenth of eighty is eight, so the first answer costs eight.
    REQUIRE(run.ChooseEventOption(1) == true);

    CHECK(run.GetGold() == 99 + 90);
    CHECK(run.GetPlayer().GetHealth() == health - 8);

    REQUIRE(run.HasEvent() == true);
    REQUIRE(run.ChooseEventOption(1) == true);

    CHECK(run.GetPlayer().GetHealth() == health - 8 - 9);
}

TEST_CASE("N'loth takes a relic and gives its own")
{
    Run run = RunInRoom(EventId::NLOTH);

    const std::size_t carried = run.GetPlayer().GetRelics().size();

    REQUIRE(run.ChooseEventOption(0) == true);

    CHECK(run.GetPlayer().HasRelic(RelicId::NLOTHS_GIFT) == true);
    CHECK(run.GetPlayer().GetRelics().size() == carried);
}

TEST_CASE("A joust takes the stake and sometimes pays out")
{
    int won = 0;
    int lost = 0;

    for (unsigned int seed = 1; seed < 20; ++seed)
    {
        Run run(CardColor::RED, seed);

        run.BeginAct(2);
        run.StartEvent(EventId::THE_JOUST);

        REQUIRE(run.CanChooseEventOption(0) == true);
        REQUIRE(run.ChooseEventOption(0) == true);

        if (run.GetGold() > 99)
        {
            ++won;
        }
        else
        {
            CHECK(run.GetGold() == 99 - 50);
            ++lost;
        }
    }

    // The favourite comes in most of the time.
    CHECK(won > lost);
}

TEST_CASE("A portal is a way straight up to the boss")
{
    Run run = RunInRoom(EventId::SECRET_PORTAL, 3);

    REQUIRE(run.ChooseEventOption(0) == true);

    CHECK(run.IsAtBoss() == true);
    CHECK(run.GetAvailableColumns().empty() == true);
}

TEST_CASE("An augmenter hands over what it is asked for")
{
    Run jax = RunInRoom(EventId::AUGMENTER);

    REQUIRE(jax.ChooseEventOption(0) == true);

    CHECK(Count(jax, CardId::JAX) == 1);

    Run mutagen = RunInRoom(EventId::AUGMENTER);

    REQUIRE(mutagen.ChooseEventOption(2) == true);

    CHECK(mutagen.GetPlayer().HasRelic(RelicId::MUTAGENIC_STRENGTH) == true);
}
