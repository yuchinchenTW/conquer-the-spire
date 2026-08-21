#include "doctest.h"

#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Events/EventLibrary.hpp>
#include <conquer-the-spire/Relics/RelicRegistry.hpp>
#include <conquer-the-spire/Run/Run.hpp>

#include <algorithm>
#include <cstddef>
#include <map>
#include <random>
#include <set>
#include <vector>

using namespace ConquerTheSpire;

namespace
{
//! Opens a run standing in the room \p id.
Run RunInRoom(EventId id, CardColor color = CardColor::RED,
              unsigned int seed = 5)
{
    Run run(color, seed);
    run.StartEvent(id);

    return run;
}

//! Returns the slot the card \p id sits in, or the size of the deck.
std::size_t SlotOf(const Run& run, CardId id)
{
    const std::vector<Card>& deck = run.GetDeck();

    for (std::size_t i = 0; i < deck.size(); ++i)
    {
        if (deck[i].GetId() == id)
        {
            return i;
        }
    }

    return deck.size();
}

//! Counts the cards of \p type in the deck.
int CountType(const Run& run, CardType type)
{
    int count = 0;

    for (const auto& card : run.GetDeck())
    {
        if (card.GetCardType() == type)
        {
            ++count;
        }
    }

    return count;
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
}  // namespace

TEST_CASE("Every room of the act is built with something to choose")
{
    std::vector<EventId> all = EventLibrary::GetAct1Rooms();
    const std::vector<EventId>& shrines = EventLibrary::GetShrines();

    all.insert(all.end(), shrines.begin(), shrines.end());

    CHECK(all.size() == 24u);

    for (const EventId id : all)
    {
        const Event event = EventLibrary::Get(id);

        CHECK(event.GetId() == id);
        CHECK(event.GetName().empty() == false);
        CHECK(event.GetKind() != EventKind::INVALID);
        CHECK(event.GetOptions().empty() == false);
        CHECK(event.IsDone() == false);
    }
}

TEST_CASE("The fish is eaten one way or another")
{
    const Event fish = EventLibrary::Get(EventId::BIG_FISH);

    // There is nothing to walk away with.
    REQUIRE(fish.GetOptions().size() == 3u);

    SUBCASE("The banana heals a third of the whole")
    {
        Run run = RunInRoom(EventId::BIG_FISH);
        Player& player = run.GetPlayer();

        player.LoseHealth(40);

        const int hurt = player.GetHealth();

        REQUIRE(run.ChooseEventOption(0) == true);

        CHECK(player.GetHealth() == hurt + 80 / 3);
        CHECK(run.HasEvent() == false);
    }

    SUBCASE("The donut is worth five")
    {
        Run run = RunInRoom(EventId::BIG_FISH);

        REQUIRE(run.ChooseEventOption(1) == true);

        CHECK(run.GetPlayer().GetMaxHealth() == 85);
        CHECK(run.GetPlayer().GetHealth() == 85);
    }

    SUBCASE("The box costs a regret")
    {
        Run run = RunInRoom(EventId::BIG_FISH);

        REQUIRE(run.ChooseEventOption(2) == true);

        CHECK(run.GetPlayer().GetRelics().size() == 2u);
        CHECK(SlotOf(run, CardId::REGRET) < run.GetDeck().size());
    }
}

TEST_CASE("The idol sends a boulder after whoever takes it")
{
    Run run = RunInRoom(EventId::GOLDEN_IDOL);

    REQUIRE(run.ChooseEventOption(0) == true);

    CHECK(run.GetPlayer().HasRelic(RelicId::GOLDEN_IDOL) == true);

    // The room is not done with the climber yet.
    REQUIRE(run.HasEvent() == true);
    REQUIRE(run.GetEvent().GetStage() == 1);
    REQUIRE(run.GetEvent().GetOptions().size() == 3u);

    const int before = run.GetPlayer().GetHealth();

    REQUIRE(run.ChooseEventOption(1) == true);

    // Smashing through it costs a quarter of the whole.
    CHECK(run.GetPlayer().GetHealth() == before - 80 / 4);
    CHECK(run.HasEvent() == false);
}

TEST_CASE("Hiding from the boulder costs a slice of the whole")
{
    Run run = RunInRoom(EventId::GOLDEN_IDOL);

    REQUIRE(run.ChooseEventOption(0) == true);
    REQUIRE(run.ChooseEventOption(2) == true);

    CHECK(run.GetPlayer().GetMaxHealth() == 80 - 80 * 8 / 100);
    CHECK(run.GetPlayer().GetHealth() <= run.GetPlayer().GetMaxHealth());
}

TEST_CASE("The cleric wants paying")
{
    Run run = RunInRoom(EventId::THE_CLERIC);

    REQUIRE(run.GetGold() == 99);

    Player& player = run.GetPlayer();
    player.LoseHealth(50);

    const int hurt = player.GetHealth();

    REQUIRE(run.CanChooseEventOption(0) == true);
    REQUIRE(run.ChooseEventOption(0) == true);

    CHECK(run.GetGold() == 99 - 35);
    CHECK(player.GetHealth() == hurt + 80 / 4);
}

TEST_CASE("An empty purse cannot pay the cleric")
{
    Run run = RunInRoom(EventId::THE_CLERIC);

    REQUIRE(run.SpendGold(99) == true);

    CHECK(run.CanChooseEventOption(0) == false);
    CHECK(run.CanChooseEventOption(1) == false);
    CHECK(run.ChooseEventOption(0) == false);

    // Walking away is always open.
    CHECK(run.CanChooseEventOption(2) == true);
}

TEST_CASE("The statue only breaks for something heavy")
{
    Run run = RunInRoom(EventId::WING_STATUE);
    const std::size_t destroy = OptionOf(run.GetEvent(), "Destroy");

    REQUIRE(destroy < run.GetEvent().GetOptions().size());

    // A starting deck swings for six and eight.
    CHECK(run.CanChooseEventOption(destroy) == false);

    run.AddCardToDeck(CardRegistry::Get(CardId::CLOTHESLINE));

    CHECK(run.CanChooseEventOption(destroy) == true);

    const int purse = run.GetGold();

    REQUIRE(run.ChooseEventOption(destroy) == true);

    CHECK(run.GetGold() >= purse + 50);
    CHECK(run.GetGold() <= purse + 80);
}

TEST_CASE("Praying at the statue takes a card and seven health")
{
    Run run = RunInRoom(EventId::WING_STATUE);

    const std::size_t before = run.GetDeck().size();
    const int health = run.GetPlayer().GetHealth();
    const CardId named = run.GetDeck()[3].GetId();

    REQUIRE(run.ChooseEventOption(0, { 3 }) == true);

    CHECK(run.GetDeck().size() == before - 1);
    CHECK(run.GetPlayer().GetHealth() == health - 7);
    static_cast<void>(named);
}

TEST_CASE("The goop pays for what it takes")
{
    Run run = RunInRoom(EventId::WORLD_OF_GOOP);

    const int health = run.GetPlayer().GetHealth();

    REQUIRE(run.ChooseEventOption(0) == true);

    CHECK(run.GetGold() == 99 + 75);
    CHECK(run.GetPlayer().GetHealth() == health - 11);
}

TEST_CASE("Walking past the goop still costs something")
{
    Run run = RunInRoom(EventId::WORLD_OF_GOOP);

    REQUIRE(run.ChooseEventOption(1) == true);

    CHECK(run.GetGold() <= 99 - 20);
    CHECK(run.GetGold() >= 99 - 50);
}

TEST_CASE("Reaching into the ooze costs more every time")
{
    // A seed the ooze keeps its relic for a while under.
    Run run(CardColor::RED, 3);
    run.StartEvent(EventId::SCRAP_OOZE);

    int health = run.GetPlayer().GetHealth();
    int expected = 3;

    while (run.HasEvent() && expected < 8)
    {
        REQUIRE(run.ChooseEventOption(0) == true);

        CHECK(run.GetPlayer().GetHealth() == health - expected);

        health = run.GetPlayer().GetHealth();
        ++expected;
    }

    CHECK(expected > 3);
}

TEST_CASE("The ooze does hand something over in the end")
{
    bool handedOver = false;

    for (unsigned int seed = 1; seed < 12 && !handedOver; ++seed)
    {
        Run run(CardColor::RED, seed);
        run.StartEvent(EventId::SCRAP_OOZE);

        for (int i = 0; i < 9 && run.HasEvent(); ++i)
        {
            run.ChooseEventOption(0);
        }

        // Either it paid out, which closes the room, or the climber is still
        // reaching in.
        if (run.GetPlayer().GetRelics().size() > 1u)
        {
            handedOver = true;
            CHECK(run.HasEvent() == false);
        }
    }

    CHECK(handedOver == true);
}

TEST_CASE("The body is searched three times at most")
{
    int fights = 0;
    int emptied = 0;

    for (unsigned int seed = 1; seed < 20; ++seed)
    {
        Run run(CardColor::RED, seed);
        run.StartEvent(EventId::DEAD_ADVENTURER);

        int searches = 0;

        while (run.HasEvent() && searches < 5)
        {
            REQUIRE(run.ChooseEventOption(0) == true);
            ++searches;
        }

        CHECK(searches <= 3);

        if (run.HasPendingFight())
        {
            ++fights;

            Battle battle = run.StartPendingBattle();

            CHECK(battle.GetMonsters().empty() == false);
        }
        else
        {
            ++emptied;
        }
    }

    // Both endings turn up over twenty tries.
    CHECK(fights > 0);
    CHECK(emptied > 0);
}

TEST_CASE("Stomping the mushrooms is a fight worth an odd relic")
{
    Run run = RunInRoom(EventId::HYPNOTIZING_COLORED_MUSHROOMS);

    REQUIRE(run.ChooseEventOption(0) == true);
    REQUIRE(run.HasPendingFight() == true);

    Battle battle = run.StartPendingBattle();

    REQUIRE(battle.GetMonsters().size() == 3u);

    // Hand the fight to the climber so the prize can be checked.
    for (auto& monster : battle.GetMonsters())
    {
        monster.LoseHealth(monster.GetHealth());
    }

    battle.EndTurn();
    run.FinishBattle(battle);

    CHECK(run.GetPlayer().HasRelic(RelicId::ODD_MUSHROOM) == true);
    CHECK(run.HasPendingFight() == false);
}

TEST_CASE("Eating the mushrooms leaves a parasite behind")
{
    Run run = RunInRoom(EventId::HYPNOTIZING_COLORED_MUSHROOMS);

    run.GetPlayer().LoseHealth(40);

    const int hurt = run.GetPlayer().GetHealth();

    REQUIRE(run.ChooseEventOption(1) == true);

    CHECK(run.GetPlayer().GetHealth() == hurt + 80 / 4);
    CHECK(SlotOf(run, CardId::PARASITE) < run.GetDeck().size());
}

TEST_CASE("A fountain washes the curses out of a deck")
{
    Run run(CardColor::RED, 5);

    run.AddCardToDeck(CardRegistry::Get(CardId::REGRET));
    run.AddCardToDeck(CardRegistry::Get(CardId::DOUBT));
    run.AddCardToDeck(CardRegistry::Get(CardId::ASCENDERS_BANE));

    REQUIRE(CountType(run, CardType::CURSE) == 3);
    REQUIRE(EventLibrary::CanAppear(EventId::THE_DIVINE_FOUNTAIN,
                                    run.GetPlayer()) == true);

    run.StartEvent(EventId::THE_DIVINE_FOUNTAIN);

    REQUIRE(run.ChooseEventOption(0) == true);

    // Everything but the one that cannot be washed off.
    CHECK(CountType(run, CardType::CURSE) == 1);
    CHECK(SlotOf(run, CardId::ASCENDERS_BANE) < run.GetDeck().size());
}

TEST_CASE("A clean deck never finds the fountain")
{
    const Run run(CardColor::RED, 5);

    CHECK(EventLibrary::CanAppear(EventId::THE_DIVINE_FOUNTAIN,
                                  run.GetPlayer()) == false);
}

TEST_CASE("The shrines work on the card they are pointed at")
{
    SUBCASE("A purifier takes it out")
    {
        Run run = RunInRoom(EventId::PURIFIER);

        const std::size_t before = run.GetDeck().size();

        REQUIRE(run.ChooseEventOption(0, { 0 }) == true);

        CHECK(run.GetDeck().size() == before - 1);
    }

    SUBCASE("A shrine upgrades it")
    {
        Run run = RunInRoom(EventId::UPGRADE_SHRINE);

        REQUIRE(run.GetDeck()[0].IsUpgraded() == false);
        REQUIRE(run.ChooseEventOption(0, { 0 }) == true);

        CHECK(run.GetDeck()[0].IsUpgraded() == true);
    }

    SUBCASE("A duplicator hands over a second one")
    {
        Run run = RunInRoom(EventId::DUPLICATOR);

        const std::size_t before = run.GetDeck().size();
        const CardId named = run.GetDeck()[0].GetId();

        REQUIRE(run.ChooseEventOption(0, { 0 }) == true);

        CHECK(run.GetDeck().size() == before + 1);
        CHECK(run.GetDeck().back().GetId() == named);
    }

    SUBCASE("A transmogrifier hands back something else")
    {
        Run run = RunInRoom(EventId::TRANSMOGRIFIER);

        const std::size_t before = run.GetDeck().size();
        const CardId named = run.GetDeck()[0].GetId();

        REQUIRE(run.ChooseEventOption(0, { 0 }) == true);

        CHECK(run.GetDeck().size() == before);
        CHECK(run.GetDeck().back().GetId() != named);
        CHECK(run.GetDeck().back().GetColor() == CardColor::RED);
    }
}

TEST_CASE("The bonfire pays by what was burned")
{
    SUBCASE("A common card is worth five health")
    {
        Run run = RunInRoom(EventId::BONFIRE_SPIRITS);

        run.AddCardToDeck(CardRegistry::Get(CardId::CLEAVE));
        run.GetPlayer().LoseHealth(30);

        const int hurt = run.GetPlayer().GetHealth();
        const std::size_t slot = SlotOf(run, CardId::CLEAVE);

        REQUIRE(run.ChooseEventOption(0, { slot }) == true);

        CHECK(run.GetPlayer().GetHealth() == hurt + 5);
        CHECK(SlotOf(run, CardId::CLEAVE) == run.GetDeck().size());
    }

    SUBCASE("A rare card is worth ten more of the whole")
    {
        Run run = RunInRoom(EventId::BONFIRE_SPIRITS);

        run.AddCardToDeck(CardRegistry::Get(CardId::DEMON_FORM));
        run.GetPlayer().LoseHealth(30);

        const std::size_t slot = SlotOf(run, CardId::DEMON_FORM);

        REQUIRE(run.ChooseEventOption(0, { slot }) == true);

        CHECK(run.GetPlayer().GetMaxHealth() == 90);
        CHECK(run.GetPlayer().GetHealth() == 90);
    }

    SUBCASE("A curse is worth nothing but a poop")
    {
        Run run = RunInRoom(EventId::BONFIRE_SPIRITS);

        run.AddCardToDeck(CardRegistry::Get(CardId::REGRET));

        const std::size_t slot = SlotOf(run, CardId::REGRET);

        REQUIRE(run.ChooseEventOption(0, { slot }) == true);

        CHECK(run.GetPlayer().HasRelic(RelicId::SPIRIT_POOP) == true);
    }
}

TEST_CASE("The woman in blue sells three for forty")
{
    Run run = RunInRoom(EventId::THE_WOMAN_IN_BLUE);

    REQUIRE(run.ChooseEventOption(2) == true);

    CHECK(run.GetGold() == 99 - 40);
    CHECK(run.GetPlayer().GetPotions().size() == 3u);
}

TEST_CASE("The forge trades a curse for a pair of tongs")
{
    Run run = RunInRoom(EventId::OMINOUS_FORGE);

    REQUIRE(run.ChooseEventOption(1) == true);

    CHECK(run.GetPlayer().HasRelic(RelicId::WARPED_TONGS) == true);
    CHECK(SlotOf(run, CardId::PAIN) < run.GetDeck().size());
}

TEST_CASE("The serpent pays well for a doubt")
{
    Run run = RunInRoom(EventId::THE_SSSSSERPENT);

    REQUIRE(run.ChooseEventOption(0) == true);

    CHECK(run.GetGold() == 99 + 175);
    CHECK(SlotOf(run, CardId::DOUBT) < run.GetDeck().size());
}

TEST_CASE("The light upgrades two cards and burns for a fifth")
{
    Run run = RunInRoom(EventId::SHINING_LIGHT);

    const int health = run.GetPlayer().GetHealth();

    REQUIRE(run.ChooseEventOption(0) == true);

    int upgraded = 0;

    for (const auto& card : run.GetDeck())
    {
        if (card.IsUpgraded())
        {
            ++upgraded;
        }
    }

    CHECK(upgraded == 2);
    CHECK(run.GetPlayer().GetHealth() == health - 80 / 5);
}

TEST_CASE("A face is traded for one of the five on offer")
{
    std::set<RelicId> seen;

    for (unsigned int seed = 1; seed < 30; ++seed)
    {
        Run run(CardColor::RED, seed);
        run.StartEvent(EventId::FACE_TRADER);

        REQUIRE(run.ChooseEventOption(1) == true);

        for (const auto& relic : run.GetPlayer().GetRelics())
        {
            if (relic.GetId() != RelicId::BURNING_BLOOD)
            {
                seen.insert(relic.GetId());
            }
        }
    }

    const std::set<RelicId> faces = {
        RelicId::FACE_OF_CLERIC, RelicId::SSSERPENT_HEAD,
        RelicId::GREMLIN_VISAGE, RelicId::NLOTHS_HUNGRY_FACE,
        RelicId::CULTIST_HEADPIECE
    };

    CHECK(seen.empty() == false);

    for (const RelicId id : seen)
    {
        CHECK(faces.count(id) == 1u);
    }
}

TEST_CASE("Touching the face trader is paid for in health")
{
    Run run = RunInRoom(EventId::FACE_TRADER);

    const int health = run.GetPlayer().GetHealth();

    REQUIRE(run.ChooseEventOption(0) == true);

    CHECK(run.GetPlayer().GetHealth() == health - 8);
    CHECK(run.GetGold() == 99 + 75);
}

TEST_CASE("The wheel lands on each of its six faces")
{
    std::map<std::string, int> tally;

    for (unsigned int seed = 1; seed < 60; ++seed)
    {
        Run run(CardColor::RED, seed);
        run.StartEvent(EventId::WHEEL_OF_CHANGE);

        const int gold = run.GetGold();
        const int health = run.GetPlayer().GetHealth();
        const std::size_t deck = run.GetDeck().size();
        const std::size_t relics = run.GetPlayer().GetRelics().size();

        REQUIRE(run.ChooseEventOption(0) == true);

        if (run.GetGold() > gold)
        {
            ++tally["gold"];
        }

        if (run.GetPlayer().GetHealth() < health)
        {
            ++tally["hurt"];
        }

        if (run.GetPlayer().GetRelics().size() > relics)
        {
            ++tally["relic"];
        }

        if (run.GetDeck().size() < deck)
        {
            ++tally["removed"];
        }

        if (run.GetDeck().size() > deck)
        {
            ++tally["cursed"];
        }
    }

    CHECK(tally["gold"] > 0);
    CHECK(tally["hurt"] > 0);
    CHECK(tally["relic"] > 0);
    CHECK(tally["removed"] > 0);
    CHECK(tally["cursed"] > 0);
}

TEST_CASE("Neow offers four blessings and no bargain that undoes itself")
{
    for (unsigned int seed = 1; seed < 40; ++seed)
    {
        Run run(CardColor::GREEN, seed);
        const Event& neow = run.StartNeow();

        REQUIRE(neow.GetOptions().size() == 4u);

        const std::vector<EventEffect>& bargain = neow.GetOptions()[2].effects;

        REQUIRE(bargain.size() == 2u);

        const bool undoes =
            (bargain[0].type == EventEffectType::LOSE_ALL_GOLD &&
             bargain[1].type == EventEffectType::GAIN_GOLD) ||
            (bargain[0].type == EventEffectType::LOSE_MAX_HEALTH &&
             bargain[1].type == EventEffectType::GAIN_MAX_HEALTH) ||
            (bargain[0].type == EventEffectType::GAIN_RANDOM_CURSE &&
             bargain[1].type == EventEffectType::REMOVE_CARDS);

        CHECK(undoes == false);
    }
}

TEST_CASE("Neow's last blessing swaps the starter relic for a boss one")
{
    Run run(CardColor::RED, 9);

    run.StartNeow();

    REQUIRE(run.GetPlayer().HasRelic(RelicId::BURNING_BLOOD) == true);
    REQUIRE(run.ChooseEventOption(3) == true);

    CHECK(run.GetPlayer().HasRelic(RelicId::BURNING_BLOOD) == false);
    REQUIRE(run.GetPlayer().GetRelics().size() == 1u);
    CHECK(run.GetPlayer().GetRelics().front().GetTier() == RelicTier::BOSS);
}

TEST_CASE("A room that turns up once is never handed out twice")
{
    Run run(CardColor::BLUE, 21);

    std::vector<EventId> oneTime;

    for (int i = 0; i < 40; ++i)
    {
        const Event& event = run.StartEvent();

        if (event.GetId() == EventId::INVALID)
        {
            continue;
        }

        if (event.GetKind() == EventKind::ONE_TIME)
        {
            CHECK(std::find(oneTime.begin(), oneTime.end(), event.GetId()) ==
                  oneTime.end());
            oneTime.emplace_back(event.GetId());
        }

        // Walk out of whatever it is, one way or another.
        while (run.HasEvent())
        {
            const std::size_t last = run.GetEvent().GetOptions().size() - 1;

            if (!run.ChooseEventOption(last))
            {
                break;
            }
        }
    }

    CHECK(oneTime.empty() == false);
}
