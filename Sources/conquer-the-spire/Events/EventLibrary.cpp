// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Events/EventLibrary.hpp>

#include <algorithm>
#include <utility>

namespace ConquerTheSpire
{
namespace
{
//! Rolls a number between \p low and \p high, both included.
int Roll(std::mt19937& rng, int low, int high)
{
    std::uniform_int_distribution<int> pick(low, high);

    return pick(rng);
}

//! Returns one of \p options at random.
template <class T>
const T& PickOne(const std::vector<T>& options, std::mt19937& rng)
{
    std::uniform_int_distribution<std::size_t> pick(0, options.size() - 1);

    return options[pick(rng)];
}

//! How much maximum health Neow's second blessing hands a character.
int NeowHealth(CardColor character)
{
    switch (character)
    {
        case CardColor::GREEN:
            return 6;

        case CardColor::BLUE:
            return 7;

        default:
            return 8;
    }
}

//! How much maximum health the third blessing takes, and gives.
int NeowLoss(CardColor character)
{
    return character == CardColor::RED ? 8 : 7;
}

int NeowGain(CardColor character)
{
    switch (character)
    {
        case CardColor::GREEN:
            return 12;

        case CardColor::BLUE:
            return 14;

        default:
            return 16;
    }
}

//! Returns true when the deck holds a card the merchant would take out.
bool HasRemovableCard(const Player& player)
{
    for (const auto& card : player.GetDeck())
    {
        if (card.GetRarity() != CardRarity::BASIC &&
            card.GetCardType() != CardType::CURSE)
        {
            return true;
        }
    }

    return false;
}

Event MakeBigFish()
{
    Event event(EventId::BIG_FISH, "Big Fish", EventKind::ONE_TIME);

    // There is nothing to walk away with here: the fish is eaten one way or
    // another.
    event.AddStage({ EventOption("Banana", { EventEffect::HealPercent(33) }),
                     EventOption("Donut", { EventEffect::GainMaxHealth(5) }),
                     EventOption("Box", { EventEffect::RandomRelic(),
                                          EventEffect::GainCurse(
                                              CardId::REGRET) }) });

    return event;
}

Event MakeDeadAdventurer()
{
    Event event(EventId::DEAD_ADVENTURER, "Dead Adventurer",
                EventKind::ONE_TIME);

    // Searching again keeps the room open, which is what makes it a gamble.
    event.AddStage({ EventOption("Search", { EventEffect::Search() }, 0),
                     EventOption("Leave", {}) });

    return event;
}

Event MakeFaceTrader()
{
    Event event(EventId::FACE_TRADER, "Face Trader", EventKind::ONE_TIME);

    event.AddStage({ EventOption("Touch",
                                 { EventEffect::DamagePercent(10),
                                   EventEffect::Gold(75) }),
                     EventOption("Trade", { EventEffect::TradeFace() }),
                     EventOption("Leave", {}) });

    return event;
}

Event MakeGoldenIdol()
{
    Event event(EventId::GOLDEN_IDOL, "Golden Idol", EventKind::ONE_TIME);

    event.AddStage(
        { EventOption("Take", { EventEffect::GainRelic(RelicId::GOLDEN_IDOL) },
                      1),
          EventOption("Leave", {}) });

    // Whatever is taken off the pedestal sends a boulder after it.
    event.AddStage(
        { EventOption("Outrun", { EventEffect::GainCurse(CardId::INJURY) }),
          EventOption("Smash", { EventEffect::DamagePercent(25) }),
          EventOption("Hide", { EventEffect::LoseMaxHealthPercent(8) }) });

    return event;
}

Event MakeMushrooms()
{
    Event event(EventId::HYPNOTIZING_COLORED_MUSHROOMS,
                "Hypnotizing Colored Mushrooms", EventKind::ONE_TIME);

    event.AddStage(
        { EventOption("Stomp",
                      { EventEffect::Fight({ MonsterId::FUNGI_BEAST,
                                             MonsterId::FUNGI_BEAST,
                                             MonsterId::FUNGI_BEAST },
                                           RelicId::ODD_MUSHROOM) }),
          EventOption("Eat", { EventEffect::HealPercent(25),
                               EventEffect::GainCurse(CardId::PARASITE) }) });

    return event;
}

Event MakeLivingWall()
{
    Event event(EventId::LIVING_WALL, "Living Wall", EventKind::ONE_TIME);

    event.AddStage({ EventOption("Forget", { EventEffect::RemoveCards(1) }),
                     EventOption("Change", { EventEffect::TransformCards(1) }),
                     EventOption("Grow", { EventEffect::UpgradeCards(1) }) });

    return event;
}

Event MakeScrapOoze()
{
    Event event(EventId::SCRAP_OOZE, "Scrap Ooze", EventKind::ONE_TIME);

    // Reaching in again costs another point of health and pays out a tenth
    // more often.
    event.AddStage(
        { EventOption("Reach Inside", { EventEffect::Reach() }, 0),
          EventOption("Leave", {}) });

    return event;
}

Event MakeShiningLight()
{
    Event event(EventId::SHINING_LIGHT, "Shining Light", EventKind::ONE_TIME);

    event.AddStage({ EventOption("Enter",
                                 { EventEffect::UpgradeRandom(2),
                                   EventEffect::DamagePercent(20) }),
                     EventOption("Leave", {}) });

    return event;
}

Event MakeCleric()
{
    Event event(EventId::THE_CLERIC, "The Cleric", EventKind::ONE_TIME);

    std::vector<EventOption> options;

    options.emplace_back(
        EventOption("Heal", { EventEffect::HealPercent(25) }).Costs(35));
    options.emplace_back(
        EventOption("Purify", { EventEffect::RemoveCards(1) }).Costs(50));
    options.emplace_back(EventOption("Leave", {}));

    event.AddStage(std::move(options));

    return event;
}

Event MakeSerpent()
{
    Event event(EventId::THE_SSSSSERPENT, "The Ssssserpent",
                EventKind::ONE_TIME);

    event.AddStage({ EventOption("Agree",
                                 { EventEffect::Gold(175),
                                   EventEffect::GainCurse(CardId::DOUBT) }),
                     EventOption("Disagree", {}) });

    return event;
}

Event MakeWingStatue()
{
    Event event(EventId::WING_STATUE, "Wing Statue", EventKind::ONE_TIME);

    std::vector<EventOption> options;

    options.emplace_back(EventOption(
        "Pray", { EventEffect::RemoveCards(1), EventEffect::Damage(7) }));
    options.emplace_back(
        EventOption("Destroy", { EventEffect::GoldRange(50, 80) })
            .Needs(EventRequirement::HAS_HEAVY_ATTACK, 10));
    options.emplace_back(EventOption("Leave", {}));

    event.AddStage(std::move(options));

    return event;
}

Event MakeWorldOfGoop()
{
    Event event(EventId::WORLD_OF_GOOP, "World of Goop", EventKind::ONE_TIME);

    event.AddStage({ EventOption("Gather Gold",
                                 { EventEffect::Gold(75),
                                   EventEffect::Damage(11) }),
                     EventOption("Leave It",
                                 { EventEffect::LoseGold(20, 50) }) });

    return event;
}

Event MakeAncientWriting()
{
    Event event(EventId::ANCIENT_WRITING, "Ancient Writing",
                EventKind::ONE_TIME);

    event.AddStage({ EventOption("Elegance", { EventEffect::RemoveCards(1) }),
                     EventOption("Simplicity",
                                 { EventEffect::UpgradeAllBasic() }) });

    return event;
}

Event MakeAugmenter()
{
    Event event(EventId::AUGMENTER, "Augmenter", EventKind::ONE_TIME);

    event.AddStage(
        { EventOption("Test J.A.X.", { EventEffect::GainCard(CardId::JAX) }),
          EventOption("Become Test Subject",
                      { EventEffect::TransformCards(2) }),
          EventOption("Ingest Mutagens",
                      { EventEffect::GainRelic(
                          RelicId::MUTAGENIC_STRENGTH) }) });

    return event;
}

Event MakeCouncilOfGhosts()
{
    Event event(EventId::COUNCIL_OF_GHOSTS, "Council of Ghosts",
                EventKind::ONE_TIME);

    event.AddStage(
        { EventOption("Accept",
                      { EventEffect::LoseMaxHealthPercent(50),
                        EventEffect::GainCards(CardId::APPARITION, 5) }),
          EventOption("Refuse", {}) });

    return event;
}

Event MakeCursedTome()
{
    Event event(EventId::CURSED_TOME, "Cursed Tome", EventKind::ONE_TIME);

    // Reading it through costs six, and then it is a question of whether to
    // take the thing.
    event.AddStage({ EventOption("Read", { EventEffect::Damage(6) }, 1),
                     EventOption("Leave", {}) });

    event.AddStage(
        { EventOption("Take",
                      { EventEffect::Damage(10),
                        EventEffect::OneOfRelics({ RelicId::ENCHIRIDION,
                                                   RelicId::NILRYS_CODEX,
                                                   RelicId::NECRONOMICON }) }),
          EventOption("Stop", { EventEffect::Damage(3) }) });

    return event;
}

Event MakeForgottenAltar()
{
    Event event(EventId::FORGOTTEN_ALTAR, "Forgotten Altar",
                EventKind::ONE_TIME);

    std::vector<EventOption> options;

    options.emplace_back(
        EventOption("Offer: Golden Idol",
                    { EventEffect::LoseRelic(RelicId::GOLDEN_IDOL),
                      EventEffect::GainRelic(RelicId::BLOODY_IDOL) })
            .NeedsRelic(RelicId::GOLDEN_IDOL));
    options.emplace_back(EventOption("Sacrifice",
                                     { EventEffect::DamagePercent(25),
                                       EventEffect::GainMaxHealth(5) }));
    options.emplace_back(
        EventOption("Desecrate", { EventEffect::GainCurse(CardId::DECAY) }));

    event.AddStage(std::move(options));

    return event;
}

Event MakeMaskedBandits()
{
    Event event(EventId::MASKED_BANDITS, "Masked Bandits",
                EventKind::ONE_TIME);

    event.AddStage(
        { EventOption("Pay", { EventEffect::LoseAllGold() }),
          EventOption("Fight",
                      { EventEffect::Fight({ MonsterId::POINTY,
                                             MonsterId::ROMEO,
                                             MonsterId::BEAR },
                                           RelicId::RED_MASK) }) });

    return event;
}

Event MakeOldBeggar()
{
    Event event(EventId::OLD_BEGGAR, "Old Beggar", EventKind::ONE_TIME);

    std::vector<EventOption> options;

    options.emplace_back(
        EventOption("Offer Gold", { EventEffect::RemoveCards(1) }).Costs(75));
    options.emplace_back(EventOption("Leave", {}));

    event.AddStage(std::move(options));

    return event;
}

Event MakePleadingVagrant()
{
    Event event(EventId::PLEADING_VAGRANT, "Pleading Vagrant",
                EventKind::ONE_TIME);

    std::vector<EventOption> options;

    options.emplace_back(
        EventOption("Offer Gold", { EventEffect::RandomRelic() }).Costs(85));
    options.emplace_back(
        EventOption("Rob", { EventEffect::RandomRelic(),
                             EventEffect::GainCurse(CardId::SHAME) }));
    options.emplace_back(EventOption("Leave", {}));

    event.AddStage(std::move(options));

    return event;
}

Event MakeColosseum()
{
    Event event(EventId::THE_COLOSSEUM, "The Colosseum", EventKind::ONE_TIME);

    // The first pair are nothing; whoever stays for the second gets paid.
    event.AddStage({ EventOption("Fight",
                                 { EventEffect::Fight(
                                     { MonsterId::BLUE_SLAVER,
                                       MonsterId::RED_SLAVER }) },
                                 1) });

    event.AddStage(
        { EventOption("Cowardice", {}),
          EventOption("Victory",
                      { EventEffect::Fight({ MonsterId::TASKMASTER,
                                             MonsterId::GREMLIN_NOB }),
                        EventEffect::Gold(100),
                        EventEffect::RandomRelic(RelicTier::RARE),
                        EventEffect::RandomRelic(RelicTier::UNCOMMON) }) });

    return event;
}

Event MakeLibrary()
{
    Event event(EventId::THE_LIBRARY, "The Library", EventKind::ONE_TIME);

    event.AddStage(
        { EventOption("Read",
                      { EventEffect::CardReward(20, CardColor::INVALID,
                                                CardRarity::INVALID) }),
          EventOption("Sleep", { EventEffect::HealPercent(33) }) });

    return event;
}

Event MakeMausoleum()
{
    Event event(EventId::THE_MAUSOLEUM, "The Mausoleum", EventKind::ONE_TIME);

    event.AddStage({ EventOption("Open Coffin",
                                 { EventEffect::RandomRelic(),
                                   EventEffect::MaybeCurse(CardId::WRITHE,
                                                           50) }),
                     EventOption("Leave", {}) });

    return event;
}

Event MakeNest()
{
    Event event(EventId::THE_NEST, "The Nest", EventKind::ONE_TIME);

    event.AddStage(
        { EventOption("Smash and Grab", { EventEffect::Gold(99) }),
          EventOption("Stay in Line",
                      { EventEffect::Damage(6),
                        EventEffect::GainCard(CardId::RITUAL_DAGGER) }) });

    return event;
}

Event MakeVampires()
{
    Event event(EventId::VAMPIRES, "Vampires", EventKind::ONE_TIME);

    std::vector<EventOption> options;

    options.emplace_back(
        EventOption("Offer: Blood Vial",
                    { EventEffect::LoseRelic(RelicId::BLOOD_VIAL),
                      EventEffect::ReplaceEvery(CardId::STRIKE_RED,
                                                CardId::BITE, 5) })
            .NeedsRelic(RelicId::BLOOD_VIAL));
    options.emplace_back(
        EventOption("Accept",
                    { EventEffect::LoseMaxHealthPercent(30),
                      EventEffect::ReplaceEvery(CardId::STRIKE_RED,
                                                CardId::BITE, 5) }));
    options.emplace_back(EventOption("Refuse", {}));

    event.AddStage(std::move(options));

    return event;
}

Event MakeFalling()
{
    Event event(EventId::FALLING, "Falling", EventKind::ONE_TIME);

    event.AddStage(
        { EventOption("Land",
                      { EventEffect::RemoveRandomOfType(CardType::SKILL) }),
          EventOption("Channel",
                      { EventEffect::RemoveRandomOfType(CardType::POWER) }),
          EventOption("Strike",
                      { EventEffect::RemoveRandomOfType(
                          CardType::ATTACK) }) });

    return event;
}

Event MakeMindBloom()
{
    Event event(EventId::MIND_BLOOM, "Mind Bloom", EventKind::ONE_TIME);

    event.AddStage(
        { EventOption("I am War", { EventEffect::FightOldBoss() }),
          EventOption("I am Awake",
                      { EventEffect::UpgradeAll(),
                        EventEffect::GainRelic(
                            RelicId::MARK_OF_THE_BLOOM) }),
          EventOption("I am Rich",
                      { EventEffect::Gold(999),
                        EventEffect::GainCards(CardId::NORMALITY, 2) }) });

    return event;
}

Event MakeMysteriousSphere()
{
    Event event(EventId::MYSTERIOUS_SPHERE, "Mysterious Sphere",
                EventKind::ONE_TIME);

    event.AddStage(
        { EventOption("Open Sphere",
                      { EventEffect::FightFor({ MonsterId::ORB_WALKER,
                                                MonsterId::ORB_WALKER },
                                              RelicTier::RARE) }),
          EventOption("Leave", {}) });

    return event;
}

Event MakeSensoryStone()
{
    Event event(EventId::SENSORY_STONE, "Sensory Stone", EventKind::ONE_TIME);

    event.AddStage(
        { EventOption("Recall one",
                      { EventEffect::CardReward(3, CardColor::COLORLESS,
                                                CardRarity::INVALID) }),
          EventOption("Recall two",
                      { EventEffect::Damage(5),
                        EventEffect::CardReward(3, CardColor::COLORLESS,
                                                CardRarity::INVALID),
                        EventEffect::CardReward(3, CardColor::COLORLESS,
                                                CardRarity::INVALID) }),
          EventOption("Recall three",
                      { EventEffect::Damage(10),
                        EventEffect::CardReward(3, CardColor::COLORLESS,
                                                CardRarity::INVALID),
                        EventEffect::CardReward(3, CardColor::COLORLESS,
                                                CardRarity::INVALID),
                        EventEffect::CardReward(3, CardColor::COLORLESS,
                                                CardRarity::INVALID) }) });

    return event;
}

Event MakeMoaiHead()
{
    Event event(EventId::THE_MOAI_HEAD, "The Moai Head", EventKind::ONE_TIME);

    std::vector<EventOption> options;

    options.emplace_back(EventOption("Jump Inside",
                                     { EventEffect::HealFull(),
                                       EventEffect::LoseMaxHealthPercent(
                                           12) }));
    options.emplace_back(
        EventOption("Offer: Golden Idol",
                    { EventEffect::LoseRelic(RelicId::GOLDEN_IDOL),
                      EventEffect::Gold(333) })
            .NeedsRelic(RelicId::GOLDEN_IDOL));
    options.emplace_back(EventOption("Leave", {}));

    event.AddStage(std::move(options));

    return event;
}

Event MakeTombOfLordRedMask()
{
    Event event(EventId::TOMB_OF_LORD_RED_MASK, "Tomb of Lord Red Mask",
                EventKind::ONE_TIME);

    std::vector<EventOption> options;

    options.emplace_back(EventOption("Don the Red Mask",
                                     { EventEffect::Gold(222) })
                             .NeedsRelic(RelicId::RED_MASK));
    options.emplace_back(
        EventOption("Offer all Gold",
                    { EventEffect::LoseAllGold(),
                      EventEffect::GainRelic(RelicId::RED_MASK) }));
    options.emplace_back(EventOption("Leave", {}));

    event.AddStage(std::move(options));

    return event;
}

Event MakeWindingHalls()
{
    Event event(EventId::WINDING_HALLS, "Winding Halls", EventKind::ONE_TIME);

    event.AddStage(
        { EventOption("Embrace Madness",
                      { EventEffect::LoseMaxHealthPercent(12),
                        EventEffect::GainCards(CardId::MADNESS, 2) }),
          EventOption("Focus", { EventEffect::HealPercent(25),
                                 EventEffect::GainCurse(CardId::WRITHE) }),
          EventOption("Retrace Your Steps",
                      { EventEffect::LoseMaxHealthPercent(5) }) });

    return event;
}

Event MakeDesignerInSpire()
{
    Event event(EventId::DESIGNER_IN_SPIRE, "Designer In-Spire",
                EventKind::SHRINE);

    std::vector<EventOption> options;

    options.emplace_back(
        EventOption("Adjustments", { EventEffect::UpgradeCards(1) })
            .Costs(40));
    options.emplace_back(
        EventOption("Clean Up", { EventEffect::RemoveCards(1) }).Costs(60));
    options.emplace_back(
        EventOption("Full Service", { EventEffect::RemoveCards(1),
                                      EventEffect::UpgradeRandom(1) })
            .Costs(90));
    options.emplace_back(EventOption("Punch", { EventEffect::Damage(3) }));

    event.AddStage(std::move(options));

    return event;
}

Event MakeKnowingSkull()
{
    Event event(EventId::KNOWING_SKULL, "Knowing Skull", EventKind::SHRINE);

    // Every answer costs a little more than the last one of its kind.
    event.AddStage(
        { EventOption("A Pick Me Up?",
                      { EventEffect::SkullToll(), EventEffect::Potions(1) },
                      0),
          EventOption("Riches?",
                      { EventEffect::SkullToll(), EventEffect::Gold(90) }, 0),
          EventOption("Success?",
                      { EventEffect::SkullToll(),
                        EventEffect::CardReward(3, CardColor::COLORLESS,
                                                CardRarity::UNCOMMON) },
                      0),
          EventOption("How do I leave?", { EventEffect::SkullToll() }) });

    return event;
}

Event MakeNloth()
{
    Event event(EventId::NLOTH, "N'loth", EventKind::SHRINE);

    event.AddStage(
        { EventOption("Offer a relic",
                      { EventEffect::LoseRelic(RelicId::INVALID),
                        EventEffect::GainRelic(RelicId::NLOTHS_GIFT) }),
          EventOption("Leave", {}) });

    return event;
}

Event MakeSecretPortal()
{
    Event event(EventId::SECRET_PORTAL, "Secret Portal", EventKind::SHRINE);

    event.AddStage({ EventOption("Enter", { EventEffect::ToTheBoss() }),
                     EventOption("Leave", {}) });

    return event;
}

Event MakeJoust()
{
    Event event(EventId::THE_JOUST, "The Joust", EventKind::SHRINE);

    std::vector<EventOption> options;

    options.emplace_back(
        EventOption("Murderer", { EventEffect::Wager(50, 70, 100) })
            .Needs(EventRequirement::GOLD, 50));
    options.emplace_back(
        EventOption("Owner", { EventEffect::Wager(50, 30, 250) })
            .Needs(EventRequirement::GOLD, 50));
    options.emplace_back(EventOption("Leave", {}));

    event.AddStage(std::move(options));

    return event;
}

Event MakeBonfireSpirits()
{
    Event event(EventId::BONFIRE_SPIRITS, "Bonfire Spirits",
                EventKind::SHRINE);

    event.AddStage({ EventOption("Offer", { EventEffect::BurnOffering() }) });

    return event;
}

Event MakeDuplicator()
{
    Event event(EventId::DUPLICATOR, "Duplicator", EventKind::SHRINE);

    event.AddStage({ EventOption("Pray", { EventEffect::DuplicateCard() }),
                     EventOption("Leave", {}) });

    return event;
}

Event MakeGoldenShrine()
{
    Event event(EventId::GOLDEN_SHRINE, "Golden Shrine", EventKind::SHRINE);

    event.AddStage({ EventOption("Pray", { EventEffect::Gold(100) }),
                     EventOption("Desecrate",
                                 { EventEffect::Gold(275),
                                   EventEffect::GainCurse(CardId::REGRET) }),
                     EventOption("Leave", {}) });

    return event;
}

Event MakeLab()
{
    Event event(EventId::LAB, "Lab", EventKind::SHRINE);

    event.AddStage({ EventOption("Search", { EventEffect::Potions(3) }) });

    return event;
}

Event MakeOminousForge()
{
    Event event(EventId::OMINOUS_FORGE, "Ominous Forge", EventKind::SHRINE);

    event.AddStage(
        { EventOption("Forge", { EventEffect::UpgradeCards(1) }),
          EventOption("Rummage",
                      { EventEffect::GainRelic(RelicId::WARPED_TONGS),
                        EventEffect::GainCurse(CardId::PAIN) }),
          EventOption("Leave", {}) });

    return event;
}

Event MakePurifier()
{
    Event event(EventId::PURIFIER, "Purifier", EventKind::SHRINE);

    event.AddStage({ EventOption("Pray", { EventEffect::RemoveCards(1) }),
                     EventOption("Leave", {}) });

    return event;
}

Event MakeDivineFountain()
{
    Event event(EventId::THE_DIVINE_FOUNTAIN, "The Divine Fountain",
                EventKind::SHRINE);

    event.AddStage({ EventOption("Drink", { EventEffect::CleanseCurses() }),
                     EventOption("Leave", {}) });

    return event;
}

Event MakeWomanInBlue()
{
    Event event(EventId::THE_WOMAN_IN_BLUE, "The Woman in Blue",
                EventKind::SHRINE);

    std::vector<EventOption> options;

    options.emplace_back(
        EventOption("Buy 1 Potion", { EventEffect::Potions(1) }).Costs(20));
    options.emplace_back(
        EventOption("Buy 2 Potions", { EventEffect::Potions(2) }).Costs(30));
    options.emplace_back(
        EventOption("Buy 3 Potions", { EventEffect::Potions(3) }).Costs(40));
    options.emplace_back(EventOption("Leave", {}));

    event.AddStage(std::move(options));

    return event;
}

Event MakeTransmogrifier()
{
    Event event(EventId::TRANSMOGRIFIER, "Transmogrifier", EventKind::SHRINE);

    event.AddStage({ EventOption("Pray", { EventEffect::TransformCards(1) }),
                     EventOption("Leave", {}) });

    return event;
}

Event MakeUpgradeShrine()
{
    Event event(EventId::UPGRADE_SHRINE, "Upgrade Shrine", EventKind::SHRINE);

    event.AddStage({ EventOption("Pray", { EventEffect::UpgradeCards(1) }),
                     EventOption("Leave", {}) });

    return event;
}

Event MakeWeMeetAgain()
{
    Event event(EventId::WE_MEET_AGAIN, "We Meet Again!", EventKind::SHRINE);

    std::vector<EventOption> options;

    options.emplace_back(
        EventOption("Give Potion",
                    { EventEffect::LosePotion(), EventEffect::RandomRelic() })
            .Needs(EventRequirement::HAS_POTION));
    options.emplace_back(
        EventOption("Give Gold", { EventEffect::LoseGold(50, 150),
                                   EventEffect::RandomRelic() })
            .Needs(EventRequirement::GOLD, 50));
    options.emplace_back(
        EventOption("Give Card",
                    { EventEffect::LoseCard(), EventEffect::RandomRelic() })
            .Needs(EventRequirement::HAS_REMOVABLE_CARD));
    options.emplace_back(EventOption("Attack", {}));

    event.AddStage(std::move(options));

    return event;
}

Event MakeWheelOfChange()
{
    Event event(EventId::WHEEL_OF_CHANGE, "Wheel of Change",
                EventKind::SHRINE);

    event.AddStage({ EventOption("Spin", { EventEffect::Spin() }) });

    return event;
}
}  // namespace

const std::vector<EventId>& EventLibrary::GetAct1Rooms()
{
    static const std::vector<EventId> rooms = {
        EventId::BIG_FISH,
        EventId::DEAD_ADVENTURER,
        EventId::FACE_TRADER,
        EventId::GOLDEN_IDOL,
        EventId::HYPNOTIZING_COLORED_MUSHROOMS,
        EventId::LIVING_WALL,
        EventId::SCRAP_OOZE,
        EventId::SHINING_LIGHT,
        EventId::THE_CLERIC,
        EventId::THE_SSSSSERPENT,
        EventId::WING_STATUE,
        EventId::WORLD_OF_GOOP
    };

    return rooms;
}

const std::vector<EventId>& EventLibrary::GetAct2Rooms()
{
    static const std::vector<EventId> rooms = {
        EventId::ANCIENT_WRITING,   EventId::AUGMENTER,
        EventId::COUNCIL_OF_GHOSTS, EventId::CURSED_TOME,
        EventId::FORGOTTEN_ALTAR,   EventId::MASKED_BANDITS,
        EventId::OLD_BEGGAR,        EventId::PLEADING_VAGRANT,
        EventId::THE_COLOSSEUM,     EventId::THE_LIBRARY,
        EventId::THE_MAUSOLEUM,     EventId::THE_NEST,
        EventId::VAMPIRES,          EventId::FACE_TRADER,
        EventId::KNOWING_SKULL,     EventId::NLOTH,
        EventId::THE_JOUST,         EventId::DESIGNER_IN_SPIRE
    };

    return rooms;
}

const std::vector<EventId>& EventLibrary::GetAct3Rooms()
{
    static const std::vector<EventId> rooms = {
        EventId::FALLING,        EventId::MIND_BLOOM,
        EventId::MYSTERIOUS_SPHERE, EventId::SENSORY_STONE,
        EventId::THE_MOAI_HEAD,  EventId::TOMB_OF_LORD_RED_MASK,
        EventId::WINDING_HALLS,  EventId::SECRET_PORTAL,
        EventId::DESIGNER_IN_SPIRE
    };

    return rooms;
}

const std::vector<EventId>& EventLibrary::GetShrines()
{
    static const std::vector<EventId> shrines = {
        EventId::BONFIRE_SPIRITS,     EventId::DUPLICATOR,
        EventId::GOLDEN_SHRINE,       EventId::LAB,
        EventId::OMINOUS_FORGE,       EventId::PURIFIER,
        EventId::THE_DIVINE_FOUNTAIN, EventId::THE_WOMAN_IN_BLUE,
        EventId::TRANSMOGRIFIER,      EventId::UPGRADE_SHRINE,
        EventId::WE_MEET_AGAIN,       EventId::WHEEL_OF_CHANGE
    };

    return shrines;
}

Event EventLibrary::Get(EventId id)
{
    switch (id)
    {
        case EventId::BIG_FISH:
            return MakeBigFish();

        case EventId::DEAD_ADVENTURER:
            return MakeDeadAdventurer();

        case EventId::FACE_TRADER:
            return MakeFaceTrader();

        case EventId::GOLDEN_IDOL:
            return MakeGoldenIdol();

        case EventId::HYPNOTIZING_COLORED_MUSHROOMS:
            return MakeMushrooms();

        case EventId::LIVING_WALL:
            return MakeLivingWall();

        case EventId::SCRAP_OOZE:
            return MakeScrapOoze();

        case EventId::SHINING_LIGHT:
            return MakeShiningLight();

        case EventId::THE_CLERIC:
            return MakeCleric();

        case EventId::THE_SSSSSERPENT:
            return MakeSerpent();

        case EventId::WING_STATUE:
            return MakeWingStatue();

        case EventId::WORLD_OF_GOOP:
            return MakeWorldOfGoop();

        case EventId::ANCIENT_WRITING:
            return MakeAncientWriting();

        case EventId::AUGMENTER:
            return MakeAugmenter();

        case EventId::COUNCIL_OF_GHOSTS:
            return MakeCouncilOfGhosts();

        case EventId::CURSED_TOME:
            return MakeCursedTome();

        case EventId::FORGOTTEN_ALTAR:
            return MakeForgottenAltar();

        case EventId::MASKED_BANDITS:
            return MakeMaskedBandits();

        case EventId::OLD_BEGGAR:
            return MakeOldBeggar();

        case EventId::PLEADING_VAGRANT:
            return MakePleadingVagrant();

        case EventId::THE_COLOSSEUM:
            return MakeColosseum();

        case EventId::THE_LIBRARY:
            return MakeLibrary();

        case EventId::THE_MAUSOLEUM:
            return MakeMausoleum();

        case EventId::THE_NEST:
            return MakeNest();

        case EventId::VAMPIRES:
            return MakeVampires();

        case EventId::FALLING:
            return MakeFalling();

        case EventId::MIND_BLOOM:
            return MakeMindBloom();

        case EventId::MYSTERIOUS_SPHERE:
            return MakeMysteriousSphere();

        case EventId::SENSORY_STONE:
            return MakeSensoryStone();

        case EventId::THE_MOAI_HEAD:
            return MakeMoaiHead();

        case EventId::TOMB_OF_LORD_RED_MASK:
            return MakeTombOfLordRedMask();

        case EventId::WINDING_HALLS:
            return MakeWindingHalls();

        case EventId::DESIGNER_IN_SPIRE:
            return MakeDesignerInSpire();

        case EventId::KNOWING_SKULL:
            return MakeKnowingSkull();

        case EventId::NLOTH:
            return MakeNloth();

        case EventId::SECRET_PORTAL:
            return MakeSecretPortal();

        case EventId::THE_JOUST:
            return MakeJoust();

        case EventId::BONFIRE_SPIRITS:
            return MakeBonfireSpirits();

        case EventId::DUPLICATOR:
            return MakeDuplicator();

        case EventId::GOLDEN_SHRINE:
            return MakeGoldenShrine();

        case EventId::LAB:
            return MakeLab();

        case EventId::OMINOUS_FORGE:
            return MakeOminousForge();

        case EventId::PURIFIER:
            return MakePurifier();

        case EventId::THE_DIVINE_FOUNTAIN:
            return MakeDivineFountain();

        case EventId::THE_WOMAN_IN_BLUE:
            return MakeWomanInBlue();

        case EventId::TRANSMOGRIFIER:
            return MakeTransmogrifier();

        case EventId::UPGRADE_SHRINE:
            return MakeUpgradeShrine();

        case EventId::WE_MEET_AGAIN:
            return MakeWeMeetAgain();

        case EventId::WHEEL_OF_CHANGE:
            return MakeWheelOfChange();

        default:
            return Event();
    }
}

Event EventLibrary::MakeNeow(CardColor character, std::mt19937& rng)
{
    Event event(EventId::NEOW, "Neow", EventKind::ONE_TIME);

    // The first blessing is about the deck.
    const std::vector<EventEffect> first = {
        EventEffect::RemoveCards(1), EventEffect::TransformCards(1),
        EventEffect::UpgradeCards(1),
        EventEffect::CardReward(3, character, CardRarity::INVALID),
        EventEffect::CardReward(1, CardColor::COLORLESS,
                                CardRarity::UNCOMMON),
        EventEffect::CardReward(1, character, CardRarity::RARE)
    };

    // The second is whatever else a climber could want.
    const std::vector<EventEffect> second = {
        EventEffect::GainMaxHealth(NeowHealth(character)),
        EventEffect::GainRelic(RelicId::NEOWS_LAMENT),
        EventEffect::RandomRelic(RelicTier::COMMON), EventEffect::Gold(100),
        EventEffect::Potions(3)
    };

    // The third asks for something in return. A pair that would undo itself
    // is rolled again.
    const std::vector<EventEffect> costs = {
        EventEffect::LoseMaxHealth(NeowLoss(character)),
        EventEffect::DamageCurrentPercent(30), EventEffect::RandomCurse(),
        EventEffect::LoseAllGold()
    };

    const std::vector<EventEffect> gifts = {
        EventEffect::RemoveCards(2),
        EventEffect::TransformCards(2),
        EventEffect::Gold(250),
        EventEffect::CardReward(1, character, CardRarity::RARE),
        EventEffect::CardReward(1, CardColor::COLORLESS, CardRarity::RARE),
        EventEffect::RandomRelic(RelicTier::RARE),
        EventEffect::GainMaxHealth(NeowGain(character))
    };

    EventEffect cost = PickOne(costs, rng);
    EventEffect gift = PickOne(gifts, rng);

    for (int tries = 0; tries < 8; ++tries)
    {
        const bool undoesItself =
            (cost.type == EventEffectType::LOSE_ALL_GOLD &&
             gift.type == EventEffectType::GAIN_GOLD) ||
            (cost.type == EventEffectType::LOSE_MAX_HEALTH &&
             gift.type == EventEffectType::GAIN_MAX_HEALTH) ||
            (cost.type == EventEffectType::GAIN_RANDOM_CURSE &&
             gift.type == EventEffectType::REMOVE_CARDS);

        if (!undoesItself)
        {
            break;
        }

        gift = PickOne(gifts, rng);
    }

    std::vector<EventOption> options;

    options.emplace_back(EventOption("Blessing", { PickOne(first, rng) }));
    options.emplace_back(EventOption("Gift", { PickOne(second, rng) }));
    options.emplace_back(EventOption("Bargain", { cost, gift }));
    options.emplace_back(EventOption("Swap", { EventEffect::BossSwap() }));

    event.AddStage(std::move(options));

    return event;
}

bool EventLibrary::CanAppear(EventId id, const Player& player)
{
    if (id == EventId::THE_DIVINE_FOUNTAIN)
    {
        // The fountain has nothing to wash off a clean deck.
        for (const auto& card : player.GetDeck())
        {
            if (card.GetCardType() == CardType::CURSE)
            {
                return true;
            }
        }

        return false;
    }

    if (id == EventId::WE_MEET_AGAIN)
    {
        // The old friend wants something worth having.
        return HasRemovableCard(player) || !player.GetPotions().empty();
    }

    return true;
}

Event EventLibrary::Pick(int act, const Player& player,
                         const std::vector<EventId>& seen, std::mt19937& rng)
{
    std::vector<EventId> pool;

    // Each act has its own rooms, and every one of them turns up once.
    const std::vector<EventId>* rooms = &GetAct1Rooms();

    if (act == 2)
    {
        rooms = &GetAct2Rooms();
    }
    else if (act >= 3)
    {
        rooms = &GetAct3Rooms();
    }

    for (const EventId id : *rooms)
    {
        if (std::find(seen.begin(), seen.end(), id) == seen.end() &&
            CanAppear(id, player))
        {
            pool.emplace_back(id);
        }
    }

    for (const EventId id : GetShrines())
    {
        if (CanAppear(id, player))
        {
            pool.emplace_back(id);
        }
    }

    if (pool.empty())
    {
        return Event();
    }

    return Get(pool[static_cast<std::size_t>(
        Roll(rng, 0, static_cast<int>(pool.size()) - 1))]);
}
}  // namespace ConquerTheSpire
