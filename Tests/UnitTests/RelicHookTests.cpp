#include "doctest.h"

#include <conquer-the-spire/Cards/CardRegistry.hpp>
#include <conquer-the-spire/Monsters/MonsterRoster.hpp>
#include <conquer-the-spire/Relics/RelicRegistry.hpp>
#include <conquer-the-spire/Run/Run.hpp>

#include <algorithm>
#include <cstddef>
#include <random>
#include <vector>

using namespace ConquerTheSpire;

namespace
{
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

//! Walks a run into the last act, whose rooms are always the same three.
Run RunInFinalAct(const std::vector<RelicId>& relics = {})
{
    Run run(CardColor::RED, 5);

    run.TakeKey(KeyType::RUBY);
    run.TakeKey(KeyType::EMERALD);
    run.TakeKey(KeyType::SAPPHIRE);
    run.BeginAct(4);

    for (const RelicId id : relics)
    {
        run.AddRelic(id);
    }

    return run;
}

//! Opens a fight against \p monsters for a player carrying \p relics.
Battle FightWith(const std::vector<RelicId>& relics,
                 std::vector<MonsterId> monsters, unsigned int seed = 4)
{
    std::mt19937 rng(seed);
    Player player("Ironclad", 80);
    player.SetColor(CardColor::RED);

    for (auto& card : CardRegistry::MakeStarterDeck(CardColor::RED))
    {
        player.AddCardToDeck(std::move(card));
    }

    for (const RelicId id : relics)
    {
        player.AddRelic(RelicRegistry::Get(id));
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
}  // namespace

TEST_CASE("A whetstone and a war paint sharpen what they are for")
{
    Run whetstone(CardColor::RED, 5);

    REQUIRE(Upgraded(whetstone) == 0);

    whetstone.AddRelic(RelicId::WHETSTONE);

    CHECK(Upgraded(whetstone) == 2);

    for (const auto& card : whetstone.GetDeck())
    {
        if (card.IsUpgraded())
        {
            CHECK(card.GetCardType() == CardType::ATTACK);
        }
    }

    Run paint(CardColor::RED, 5);
    paint.AddRelic(RelicId::WAR_PAINT);

    CHECK(Upgraded(paint) == 2);

    for (const auto& card : paint.GetDeck())
    {
        if (card.IsUpgraded())
        {
            CHECK(card.GetCardType() == CardType::SKILL);
        }
    }
}

TEST_CASE("An astrolabe turns three cards into something sharper")
{
    Run run(CardColor::RED, 5);

    const std::size_t before = run.GetDeck().size();

    run.AddRelic(RelicId::ASTROLABE);

    CHECK(run.GetDeck().size() == before);
    CHECK(Upgraded(run) == 3);
}

TEST_CASE("A mirror hands over a second copy and a cage takes two away")
{
    Run mirror(CardColor::RED, 5);
    const std::size_t held = mirror.GetDeck().size();

    mirror.AddRelic(RelicId::DOLLYS_MIRROR);

    CHECK(mirror.GetDeck().size() == held + 1);

    Run cage(CardColor::RED, 5);
    cage.AddRelic(RelicId::EMPTY_CAGE);

    CHECK(cage.GetDeck().size() == held - 2);
}

TEST_CASE("Pandora's box leaves no strike and no defend behind")
{
    Run run(CardColor::RED, 5);
    const std::size_t held = run.GetDeck().size();

    run.AddRelic(RelicId::PANDORAS_BOX);

    CHECK(run.GetDeck().size() == held);

    for (const auto& card : run.GetDeck())
    {
        CHECK(card.GetRarity() != CardRarity::BASIC);
    }
}

TEST_CASE("A cauldron brews as much as the belt will hold")
{
    Run run(CardColor::RED, 5);

    run.AddRelic(RelicId::CAULDRON);

    CHECK(static_cast<int>(run.GetPlayer().GetPotions().size()) ==
          run.GetPlayer().GetPotionSlots());
}

TEST_CASE("A tiny house hands over a little of everything")
{
    Run run(CardColor::RED, 5);

    const int gold = run.GetGold();
    const int health = run.GetPlayer().GetMaxHealth();
    const std::size_t held = run.GetDeck().size();

    run.AddRelic(RelicId::TINY_HOUSE);

    CHECK(run.GetGold() == gold + 50);
    CHECK(run.GetPlayer().GetMaxHealth() == health + 5);
    CHECK(run.GetDeck().size() == held + 1);
    CHECK(run.GetPlayer().GetPotions().empty() == false);
    CHECK(Upgraded(run) >= 1);
}

TEST_CASE("An old coin is worth three hundred and a waffle is worth seven")
{
    Run coin(CardColor::RED, 5);
    const int purse = coin.GetGold();

    coin.AddRelic(RelicId::OLD_COIN);

    CHECK(coin.GetGold() == purse + 300);

    Run waffle(CardColor::RED, 5);
    waffle.GetPlayer().LoseHealth(40);
    waffle.AddRelic(RelicId::LEES_WAFFLE);

    CHECK(waffle.GetPlayer().GetMaxHealth() == 87);
    CHECK(waffle.GetPlayer().GetHealth() == 87);
}

TEST_CASE("A calling bell is paid for with a curse")
{
    Run run(CardColor::RED, 5);

    run.AddRelic(RelicId::CALLING_BELL);

    CHECK(CountType(run, CardType::CURSE) == 1);

    // The bell itself, and the three it rang for.
    CHECK(run.GetPlayer().GetRelics().size() == 5u);
}

TEST_CASE("An orrery leaves five picks on the pile")
{
    Run run(CardColor::RED, 5);

    run.AddRelic(RelicId::ORRERY);

    int picks = 0;

    for (const auto& reward : run.GetRewards())
    {
        if (reward.kind == RewardKind::CARD_CHOICE)
        {
            ++picks;
        }
    }

    CHECK(picks == 5);
}

TEST_CASE("An omamori turns the next two curses away")
{
    Run run(CardColor::RED, 5);

    run.AddRelic(RelicId::OMAMORI);

    run.AddCardToDeck(CardRegistry::Get(CardId::REGRET));
    run.AddCardToDeck(CardRegistry::Get(CardId::DOUBT));

    CHECK(CountType(run, CardType::CURSE) == 0);

    run.AddCardToDeck(CardRegistry::Get(CardId::SHAME));

    CHECK(CountType(run, CardType::CURSE) == 1);
}

TEST_CASE("A darkstone periapt grows on what a curse costs")
{
    Run run(CardColor::RED, 5);

    run.AddRelic(RelicId::DARKSTONE_PERIAPT);

    const int health = run.GetPlayer().GetMaxHealth();

    run.AddCardToDeck(CardRegistry::Get(CardId::REGRET));

    CHECK(run.GetPlayer().GetMaxHealth() == health + 6);
}

TEST_CASE("The eggs sharpen whatever of their kind comes in")
{
    Run molten(CardColor::RED, 5);
    molten.AddRelic(RelicId::MOLTEN_EGG);
    molten.AddCardToDeck(CardRegistry::Get(CardId::CLEAVE));

    CHECK(molten.GetDeck().back().IsUpgraded() == true);

    Run toxic(CardColor::RED, 5);
    toxic.AddRelic(RelicId::TOXIC_EGG);
    toxic.AddCardToDeck(CardRegistry::Get(CardId::CLEAVE));

    CHECK(toxic.GetDeck().back().IsUpgraded() == false);

    toxic.AddCardToDeck(CardRegistry::Get(CardId::SHRUG_IT_OFF));

    CHECK(toxic.GetDeck().back().IsUpgraded() == true);

    Run frozen(CardColor::RED, 5);
    frozen.AddRelic(RelicId::FROZEN_EGG);
    frozen.AddCardToDeck(CardRegistry::Get(CardId::INFLAME));

    CHECK(frozen.GetDeck().back().IsUpgraded() == true);
}

TEST_CASE("A ceramic fish pays for every card that comes in")
{
    Run run(CardColor::RED, 5);

    run.AddRelic(RelicId::CERAMIC_FISH);

    const int purse = run.GetGold();

    run.AddCardToDeck(CardRegistry::Get(CardId::CLEAVE));

    CHECK(run.GetGold() == purse + 9);
}

TEST_CASE("Ectoplasm turns gold down and a bloody idol turns it into health")
{
    Run ecto(CardColor::RED, 5);
    const int purse = ecto.GetGold();

    ecto.AddRelic(RelicId::ECTOPLASM);
    ecto.AddGold(100);

    CHECK(ecto.GetGold() == purse);

    Run idol(CardColor::RED, 5);
    idol.AddRelic(RelicId::BLOODY_IDOL);
    idol.GetPlayer().LoseHealth(30);

    const int hurt = idol.GetPlayer().GetHealth();

    idol.AddGold(20);

    CHECK(idol.GetPlayer().GetHealth() == hurt + 5);
}

TEST_CASE("A maw bank pays for the climbing until its holder spends")
{
    Run run(CardColor::RED, 5);

    run.AddRelic(RelicId::MAW_BANK);

    const int purse = run.GetGold();

    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);

    CHECK(run.GetGold() == purse + 12);

    REQUIRE(run.SpendGold(10) == true);

    const int after = run.GetGold();

    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);

    CHECK(run.GetGold() == after);
}

TEST_CASE("Wing boots step over a path that is not there, three times")
{
    Run run(CardColor::RED, 5);

    run.AddRelic(RelicId::WING_BOOTS);

    CHECK(run.GetPathSkips() == 3);

    for (int i = 0; i < 3; ++i)
    {
        const std::vector<int> allowed = run.GetAvailableColumns();
        int off = 0;

        while (std::find(allowed.begin(), allowed.end(), off) != allowed.end())
        {
            ++off;
        }

        REQUIRE(run.Travel(off, true) == true);
        CHECK(run.GetColumn() == off);
    }

    CHECK(run.GetPathSkips() == 0);

    const std::vector<int> allowed = run.GetAvailableColumns();
    int off = 0;

    while (std::find(allowed.begin(), allowed.end(), off) != allowed.end())
    {
        ++off;
    }

    CHECK(run.Travel(off, true) == false);
}

TEST_CASE("A rest site pays out what the relics of resting promise")
{
    Run run = RunInFinalAct({ RelicId::REGAL_PILLOW,
                              RelicId::ANCIENT_TEA_SET,
                              RelicId::ETERNAL_FEATHER });

    run.GetPlayer().LoseHealth(60);

    const int hurt = run.GetPlayer().GetHealth();

    // The first room of the last act is always a rest site.
    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);
    REQUIRE(run.GetCurrentNodeType() == MapNodeType::REST);

    // A feather pays three for every five cards on the way in.
    const int feathered =
        static_cast<int>(run.GetDeck().size()) / 5 * 3;

    CHECK(run.GetPlayer().GetHealth() == hurt + feathered);
    CHECK(run.GetPlayer().GetBonusEnergy() == 2);

    const int rested = run.GetPlayer().GetHealth();

    REQUIRE(run.Rest() == true);

    // A third of the whole, and fifteen more for the pillow.
    CHECK(run.GetPlayer().GetHealth() == rested + 80 * 30 / 100 + 15);
}

TEST_CASE("A dripper and a hammer take the rest site away")
{
    Run dripper = RunInFinalAct({ RelicId::COFFEE_DRIPPER });

    CHECK(dripper.Rest() == false);

    Run hammer = RunInFinalAct({ RelicId::FUSION_HAMMER });

    CHECK(hammer.Smith(0) == false);
}

TEST_CASE("A pipe, a shovel and a girya open the other three options")
{
    Run pipe = RunInFinalAct();

    // Without the pipe there is nothing to smoke.
    CHECK(pipe.Toke(0) == false);

    pipe.AddRelic(RelicId::PEACE_PIPE);

    const std::size_t held = pipe.GetDeck().size();

    CHECK(pipe.Toke(0) == true);
    CHECK(pipe.GetDeck().size() == held - 1);

    Run shovel = RunInFinalAct();

    CHECK(shovel.Dig() == RelicId::INVALID);

    shovel.AddRelic(RelicId::SHOVEL);

    const std::size_t carried = shovel.GetPlayer().GetRelics().size();

    CHECK(shovel.Dig() != RelicId::INVALID);
    CHECK(shovel.GetPlayer().GetRelics().size() == carried + 1);

    Run girya = RunInFinalAct();

    CHECK(girya.Lift() == false);

    girya.AddRelic(RelicId::GIRYA);

    CHECK(girya.Lift() == true);
    CHECK(girya.Lift() == true);
    CHECK(girya.Lift() == true);
    CHECK(girya.Lift() == false);
    CHECK(girya.GetLifts() == 3);
    CHECK(girya.GetPlayer().GetLiftedStrength() == 3);
}

TEST_CASE("A dream catcher leaves a pick behind for whoever slept")
{
    Run run = RunInFinalAct({ RelicId::DREAM_CATCHER });

    REQUIRE(run.Rest() == true);

    bool pick = false;

    for (const auto& reward : run.GetRewards())
    {
        pick = pick || reward.kind == RewardKind::CARD_CHOICE;
    }

    CHECK(pick == true);
}

TEST_CASE("A meal ticket is worth fifteen at the door of a shop")
{
    Run run = RunInFinalAct({ RelicId::MEAL_TICKET });

    run.GetPlayer().LoseHealth(40);

    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);

    const int atRest = run.GetPlayer().GetHealth();

    // The second room of the last act is always the merchant.
    REQUIRE(run.Travel(run.GetAvailableColumns().front()) == true);
    REQUIRE(run.GetCurrentNodeType() == MapNodeType::MERCHANT);

    CHECK(run.GetPlayer().GetHealth() == atRest + 15);
}

TEST_CASE("What lifting put on is there from the first turn")
{
    Battle battle = FightWith({ RelicId::GIRYA }, { MonsterId::CULTIST });

    CHECK(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 0);

    Player lifted("Ironclad", 80);
    lifted.SetColor(CardColor::RED);

    for (auto& card : CardRegistry::MakeStarterDeck(CardColor::RED))
    {
        lifted.AddCardToDeck(std::move(card));
    }

    lifted.Lift(2);

    std::mt19937 rng(4);
    std::vector<Monster> monsters;
    monsters.emplace_back(MonsterRoster::Make(MonsterId::CULTIST, rng));

    Battle strong(std::move(lifted), std::move(monsters), 4);
    strong.Start();

    CHECK(strong.GetPlayer().GetPower(PowerType::STRENGTH) == 2);
}

TEST_CASE("A doll answers for every curse the deck carries")
{
    std::mt19937 rng(4);
    Player player("Ironclad", 80);
    player.SetColor(CardColor::RED);

    for (auto& card : CardRegistry::MakeStarterDeck(CardColor::RED))
    {
        player.AddCardToDeck(std::move(card));
    }

    player.AddCardToDeck(CardRegistry::Get(CardId::REGRET));
    player.AddCardToDeck(CardRegistry::Get(CardId::DOUBT));
    player.AddRelic(RelicRegistry::Get(RelicId::DU_VU_DOLL));

    std::vector<Monster> monsters;
    monsters.emplace_back(MonsterRoster::Make(MonsterId::CULTIST, rng));

    Battle battle(std::move(player), std::move(monsters), 4);
    battle.Start();

    CHECK(battle.GetPlayer().GetPower(PowerType::STRENGTH) == 2);
}

TEST_CASE("A mark of pain buys its energy with wounds")
{
    Battle battle = FightWith({ RelicId::MARK_OF_PAIN },
                              { MonsterId::CULTIST });

    int wounds = 0;

    for (const auto& card : battle.GetPlayer().GetDrawPile())
    {
        if (card.GetId() == CardId::WOUND)
        {
            ++wounds;
        }
    }

    for (const auto& card : battle.GetPlayer().GetHand())
    {
        if (card.GetId() == CardId::WOUND)
        {
            ++wounds;
        }
    }

    CHECK(wounds == 2);
}

TEST_CASE("A pantograph is worth something where it is needed")
{
    Battle plain = FightWith({ RelicId::PANTOGRAPH },
                             { MonsterId::CULTIST });

    plain.GetPlayer().LoseHealth(40);

    Battle boss = FightWith({ RelicId::PANTOGRAPH },
                            { MonsterId::HEXAGHOST });

    CHECK(boss.IsBossFight() == true);

    // The heal lands as the fight opens, so a fight that opened hurt shows
    // it: build one that is already hurt.
    Player hurt("Ironclad", 80);
    hurt.SetColor(CardColor::RED);

    for (auto& card : CardRegistry::MakeStarterDeck(CardColor::RED))
    {
        hurt.AddCardToDeck(std::move(card));
    }

    hurt.AddRelic(RelicRegistry::Get(RelicId::PANTOGRAPH));
    hurt.LoseHealth(40);

    std::mt19937 rng(4);
    std::vector<Monster> monsters;
    monsters.emplace_back(MonsterRoster::Make(MonsterId::HEXAGHOST, rng));

    Battle healed(std::move(hurt), std::move(monsters), 4);
    healed.Start();

    CHECK(healed.GetPlayer().GetHealth() == 65);
}

TEST_CASE("A slaver's collar is only worth carrying to the harder fights")
{
    Battle plain = FightWith({ RelicId::SLAVERS_COLLAR },
                             { MonsterId::CULTIST });

    CHECK(plain.GetPlayer().GetEnergy() == 3);

    Battle elite = FightWith({ RelicId::SLAVERS_COLLAR },
                             { MonsterId::GREMLIN_NOB });

    CHECK(elite.GetPlayer().GetEnergy() == 4);
}

TEST_CASE("A tea set is worth two energy for the first turn only")
{
    std::mt19937 rng(4);
    Player player("Ironclad", 80);
    player.SetColor(CardColor::RED);

    for (auto& card : CardRegistry::MakeStarterDeck(CardColor::RED))
    {
        player.AddCardToDeck(std::move(card));
    }

    player.SetBonusEnergy(2);

    std::vector<Monster> monsters;
    monsters.emplace_back(MonsterRoster::Make(MonsterId::CULTIST, rng));

    Battle battle(std::move(player), std::move(monsters), 4);
    battle.Start();

    CHECK(battle.GetPlayer().GetEnergy() == 5);

    battle.EndTurn();

    CHECK(battle.GetPlayer().GetEnergy() == 3);
}

TEST_CASE("What is in a bottle is in hand from the first turn")
{
    std::mt19937 rng(4);
    Player player("Ironclad", 80);
    player.SetColor(CardColor::RED);

    for (auto& card : CardRegistry::MakeStarterDeck(CardColor::RED))
    {
        player.AddCardToDeck(std::move(card));
    }

    player.AddCardToDeck(CardRegistry::Get(CardId::DEMON_FORM));
    player.BottleCard(CardId::DEMON_FORM);
    player.AddRelic(RelicRegistry::Get(RelicId::BOTTLED_TORNADO));

    std::vector<Monster> monsters;
    monsters.emplace_back(MonsterRoster::Make(MonsterId::CULTIST, rng));

    Battle battle(std::move(player), std::move(monsters), 4);
    battle.Start();

    bool held = false;

    for (const auto& card : battle.GetPlayer().GetHand())
    {
        held = held || card.GetId() == CardId::DEMON_FORM;
    }

    CHECK(held == true);
}

TEST_CASE("A bottle only holds what it is meant to")
{
    Run run(CardColor::RED, 5);

    run.AddRelic(RelicId::BOTTLED_FLAME);

    // The pickup bottled a Strike on its own.
    CHECK(run.GetPlayer().GetBottledCards().empty() == false);

    // And a defend is not something a flame will hold.
    for (std::size_t i = 0; i < run.GetDeck().size(); ++i)
    {
        if (run.GetDeck()[i].GetCardType() == CardType::SKILL)
        {
            CHECK(run.BottleCard(i) == false);
            break;
        }
    }
}

TEST_CASE("A necronomicon reads the first heavy attack out twice")
{
    std::mt19937 rng(4);
    Player player("Ironclad", 80);
    player.SetColor(CardColor::RED);

    for (auto& card : CardRegistry::MakeStarterDeck(CardColor::RED))
    {
        player.AddCardToDeck(std::move(card));
    }

    player.AddRelic(RelicRegistry::Get(RelicId::NECRONOMICON));

    std::vector<Monster> monsters;
    monsters.emplace_back(MonsterRoster::Make(MonsterId::TRAINING_DUMMY, rng));

    Battle battle(std::move(player), std::move(monsters), 4);
    battle.Start();

    // Bash costs two, so the book takes an interest.
    std::vector<Card>& hand = battle.GetPlayer().GetHand();

    hand.clear();
    hand.emplace_back(CardRegistry::Get(CardId::BASH));

    const int before = battle.GetMonsters().front().GetHealth();

    REQUIRE(battle.PlayCard(0, 0) == true);

    // Eight, and then twelve, because the first read left the dummy
    // vulnerable.
    CHECK(before - battle.GetMonsters().front().GetHealth() == 20);
}

TEST_CASE("A codex leaves something in the pile at the end of a turn")
{
    Battle battle = FightWith({ RelicId::NILRYS_CODEX },
                              { MonsterId::CULTIST });

    const std::size_t before = battle.GetPlayer().GetDrawPile().size();

    battle.EndTurn();

    // A turn draws five back, so the pile is one short of that.
    CHECK(battle.GetPlayer().GetDrawPile().size() + 5 == before + 1);
}

TEST_CASE("A runic dome hides what the monsters mean to do")
{
    Battle plain = FightWith({}, { MonsterId::CULTIST });
    Battle domed = FightWith({ RelicId::RUNIC_DOME },
                             { MonsterId::CULTIST });

    CHECK(plain.AreIntentsVisible() == true);
    CHECK(domed.AreIntentsVisible() == false);

    Battle eyed = FightWith({ RelicId::FROZEN_EYE },
                            { MonsterId::CULTIST });

    CHECK(plain.IsDrawPileOrdered() == false);
    CHECK(eyed.IsDrawPileOrdered() == true);
}

TEST_CASE("A gambling chip trades a hand in, once")
{
    Battle battle = FightWith({ RelicId::GAMBLING_CHIP },
                              { MonsterId::CULTIST });

    const std::size_t held = battle.GetPlayer().GetHand().size();

    REQUIRE(battle.Gamble(2) == true);

    CHECK(battle.GetPlayer().GetHand().size() == held);
    CHECK(battle.Gamble(2) == false);
}

TEST_CASE("A bloom means no healing at all")
{
    Battle battle = FightWith({ RelicId::MARK_OF_THE_BLOOM },
                              { MonsterId::CULTIST });

    battle.GetPlayer().LoseHealth(30);

    const int hurt = battle.GetPlayer().GetHealth();

    battle.GetPlayer().AddPower(PowerType::REGENERATION, 5);
    battle.EndTurn();

    CHECK(battle.GetPlayer().GetHealth() <= hurt);

    Run run(CardColor::RED, 5);

    run.AddRelic(RelicId::MARK_OF_THE_BLOOM);
    run.GetPlayer().LoseHealth(30);

    const int hurtRun = run.GetPlayer().GetHealth();

    run.AddRelic(RelicId::BLOODY_IDOL);
    run.AddGold(20);

    CHECK(run.GetPlayer().GetHealth() == hurtRun);
}

TEST_CASE("A preserved insect leaves an elite a quarter short")
{
    Run plain(CardColor::RED, 5);
    std::mt19937 rng(4);

    const Monster whole = MonsterRoster::Make(MonsterId::GREMLIN_NOB, rng);

    Run run(CardColor::RED, 5);
    run.AddRelic(RelicId::PRESERVED_INSECT);

    std::vector<Monster> monsters;
    std::mt19937 same(4);
    monsters.emplace_back(MonsterRoster::Make(MonsterId::GREMLIN_NOB, same));

    Battle battle = run.StartBattle(std::move(monsters));

    CHECK(battle.GetMonsters().front().GetMaxHealth() ==
          whole.GetMaxHealth() * 3 / 4);
}
