// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Relics/RelicRegistry.hpp>

#include <utility>

namespace ConquerTheSpire
{
namespace
{
using CE = CardEffect;
using RT = RelicTrigger;

//! A relic that only matters outside a battle - on the map, in a shop, at a
//! rest site, or on the deck itself - carries no trigger.
Relic Carried(RelicId id, const char* name, RelicTier tier)
{
    return Relic(id, name, tier);
}

Relic Reacting(RelicId id, const char* name, RelicTier tier,
               std::vector<RelicTrigger> triggers)
{
    return Relic(id, name, tier, std::move(triggers));
}

constexpr RelicId LAST_RELIC_ID = RelicId::WARPED_TONGS;
}  // namespace

Relic RelicRegistry::Get(RelicId id)
{
    switch (id)
    {
        // Starter

        case RelicId::BURNING_BLOOD:
            // Heals after a battle, which is the run's business.
            return Carried(id, "Burning Blood", RelicTier::STARTER);

        case RelicId::RING_OF_THE_SNAKE:
            return Reacting(id, "Ring of the Snake", RelicTier::STARTER,
                            { RT::OnTurn(1, { CE::Draw(2) }) });

        case RelicId::CRACKED_CORE:
            return Reacting(id, "Cracked Core", RelicTier::STARTER,
                            { RT::On(RelicHook::BATTLE_START,
                                     { CE::ChannelOrb(OrbType::LIGHTNING) }) });

        case RelicId::PURE_WATER:
            return Reacting(
                id, "Pure Water", RelicTier::STARTER,
                { RT::On(RelicHook::BATTLE_START,
                         { CE::AddCard(CardId::MIRACLE, CardPile::HAND) }) });

        // Common

        case RelicId::AKABEKO:
            // The extra damage on the first attack is worked out by the battle.
            return Carried(id, "Akabeko", RelicTier::COMMON);

        case RelicId::ANCHOR:
            return Reacting(id, "Anchor", RelicTier::COMMON,
                            { RT::On(RelicHook::BATTLE_START,
                                     { CE::Block(10) }) });

        case RelicId::ANCIENT_TEA_SET:
            return Carried(id, "Ancient Tea Set", RelicTier::COMMON);

        case RelicId::ART_OF_WAR:
            // The battle watches whether an attack was played last turn.
            return Carried(id, "Art of War", RelicTier::COMMON);

        case RelicId::BAG_OF_MARBLES:
            return Reacting(id, "Bag of Marbles", RelicTier::COMMON,
                            { RT::On(RelicHook::BATTLE_START,
                                     { CE::ApplyPowerToAll(
                                         PowerType::VULNERABLE, 1) }) });

        case RelicId::BAG_OF_PREPARATION:
            return Reacting(id, "Bag of Preparation", RelicTier::COMMON,
                            { RT::On(RelicHook::BATTLE_START,
                                     { CE::Draw(2) }) });

        case RelicId::BLOOD_VIAL:
            return Reacting(id, "Blood Vial", RelicTier::COMMON,
                            { RT::On(RelicHook::BATTLE_START,
                                     { CE::Heal(2) }) });

        case RelicId::BRONZE_SCALES:
            return Reacting(id, "Bronze Scales", RelicTier::COMMON,
                            { RT::On(RelicHook::BATTLE_START,
                                     { CE::ApplyPowerToSelf(PowerType::THORNS,
                                                            3) }) });

        case RelicId::CENTENNIAL_PUZZLE:
            // Only the first time health is lost, which the battle tracks.
            return Carried(id, "Centennial Puzzle", RelicTier::COMMON);

        case RelicId::CERAMIC_FISH:
            return Carried(id, "Ceramic Fish", RelicTier::COMMON);

        case RelicId::DREAM_CATCHER:
            return Carried(id, "Dream Catcher", RelicTier::COMMON);

        case RelicId::HAPPY_FLOWER:
            return Reacting(id, "Happy Flower", RelicTier::COMMON,
                            { RT::Every(RelicHook::TURN_START, 3,
                                        { CE::Energy(1) }) });

        case RelicId::JUZU_BRACELET:
            return Carried(id, "Juzu Bracelet", RelicTier::COMMON);

        case RelicId::LANTERN:
            return Reacting(id, "Lantern", RelicTier::COMMON,
                            { RT::OnTurn(1, { CE::Energy(1) }) });

        case RelicId::MAW_BANK:
            return Carried(id, "Maw Bank", RelicTier::COMMON);

        case RelicId::MEAL_TICKET:
            return Carried(id, "Meal Ticket", RelicTier::COMMON);

        case RelicId::NUNCHAKU:
            return Reacting(id, "Nunchaku", RelicTier::COMMON,
                            { RT::Every(RelicHook::ATTACK_PLAYED, 10,
                                        { CE::Energy(1) }) });

        case RelicId::ODDLY_SMOOTH_STONE:
            return Reacting(id, "Oddly Smooth Stone", RelicTier::COMMON,
                            { RT::On(RelicHook::BATTLE_START,
                                     { CE::ApplyPowerToSelf(
                                         PowerType::DEXTERITY, 1) }) });

        case RelicId::OMAMORI:
            return Carried(id, "Omamori", RelicTier::COMMON);

        case RelicId::ORICHALCUM:
            return Reacting(
                id, "Orichalcum", RelicTier::COMMON,
                { RT::On(RelicHook::TURN_END,
                         { CE::Block(6).If(
                             EffectCondition::PLAYER_HAS_NO_BLOCK) }) });

        case RelicId::PEN_NIB:
            // The real relic only doubles the tenth attack; this doubles the
            // attacks for the rest of the turn.
            return Reacting(id, "Pen Nib", RelicTier::COMMON,
                            { RT::Every(RelicHook::ATTACK_PLAYED, 10,
                                        { CE::ApplyPowerToSelf(
                                            PowerType::DOUBLE_DAMAGE, 1) }) });

        case RelicId::POTION_BELT:
            return Carried(id, "Potion Belt", RelicTier::COMMON);

        case RelicId::PRESERVED_INSECT:
            return Carried(id, "Preserved Insect", RelicTier::COMMON);

        case RelicId::REGAL_PILLOW:
            return Carried(id, "Regal Pillow", RelicTier::COMMON);

        case RelicId::SMILING_MASK:
            return Carried(id, "Smiling Mask", RelicTier::COMMON);

        case RelicId::STRAWBERRY:
            return Carried(id, "Strawberry", RelicTier::COMMON);

        case RelicId::THE_BOOT:
            // The battle raises small hits to 5 for this one.
            return Carried(id, "The Boot", RelicTier::COMMON);

        case RelicId::TINY_CHEST:
            return Carried(id, "Tiny Chest", RelicTier::COMMON);

        case RelicId::TOY_ORNITHOPTER:
            return Carried(id, "Toy Ornithopter", RelicTier::COMMON);

        case RelicId::VAJRA:
            return Reacting(id, "Vajra", RelicTier::COMMON,
                            { RT::On(RelicHook::BATTLE_START,
                                     { CE::ApplyPowerToSelf(
                                         PowerType::STRENGTH, 1) }) });

        case RelicId::WAR_PAINT:
            return Carried(id, "War Paint", RelicTier::COMMON);

        case RelicId::WHETSTONE:
            return Carried(id, "Whetstone", RelicTier::COMMON);

        // Uncommon

        case RelicId::BLUE_CANDLE:
            // The battle lets curses be played while this is carried.
            return Carried(id, "Blue Candle", RelicTier::UNCOMMON);

        case RelicId::BOTTLED_FLAME:
            return Carried(id, "Bottled Flame", RelicTier::UNCOMMON);

        case RelicId::BOTTLED_LIGHTNING:
            return Carried(id, "Bottled Lightning", RelicTier::UNCOMMON);

        case RelicId::BOTTLED_TORNADO:
            return Carried(id, "Bottled Tornado", RelicTier::UNCOMMON);

        case RelicId::DARKSTONE_PERIAPT:
            return Carried(id, "Darkstone Periapt", RelicTier::UNCOMMON);

        case RelicId::ETERNAL_FEATHER:
            return Carried(id, "Eternal Feather", RelicTier::UNCOMMON);

        case RelicId::FROZEN_EGG:
            return Carried(id, "Frozen Egg", RelicTier::UNCOMMON);

        case RelicId::GREMLIN_HORN:
            return Reacting(id, "Gremlin Horn", RelicTier::UNCOMMON,
                            { RT::On(RelicHook::ENEMY_KILLED,
                                     { CE::Energy(1), CE::Draw(1) }) });

        case RelicId::HORN_CLEAT:
            return Reacting(id, "Horn Cleat", RelicTier::UNCOMMON,
                            { RT::OnTurn(2, { CE::Block(14) }) });

        case RelicId::INK_BOTTLE:
            return Reacting(id, "Ink Bottle", RelicTier::UNCOMMON,
                            { RT::Every(RelicHook::CARD_PLAYED, 10,
                                        { CE::Draw(1) }) });

        case RelicId::KUNAI:
            return Reacting(id, "Kunai", RelicTier::UNCOMMON,
                            { RT::EveryInTurn(RelicHook::ATTACK_PLAYED, 3,
                                        { CE::ApplyPowerToSelf(
                                            PowerType::DEXTERITY, 1) }) });

        case RelicId::LETTER_OPENER:
            return Reacting(id, "Letter Opener", RelicTier::UNCOMMON,
                            { RT::EveryInTurn(RelicHook::SKILL_PLAYED, 3,
                                        { CE::DamageAll(5) }) });

        case RelicId::MATRYOSHKA:
            return Carried(id, "Matryoshka", RelicTier::UNCOMMON);

        case RelicId::MEAT_ON_THE_BONE:
            return Carried(id, "Meat on the Bone", RelicTier::UNCOMMON);

        case RelicId::MERCURY_HOURGLASS:
            return Reacting(id, "Mercury Hourglass", RelicTier::UNCOMMON,
                            { RT::On(RelicHook::TURN_START,
                                     { CE::DamageAll(3) }) });

        case RelicId::MOLTEN_EGG:
            return Carried(id, "Molten Egg", RelicTier::UNCOMMON);

        case RelicId::MUMMIFIED_HAND:
            return Reacting(id, "Mummified Hand", RelicTier::UNCOMMON,
                            { RT::On(RelicHook::POWER_PLAYED,
                                     { CE::SetHandCost(0, true) }) });

        case RelicId::ORNAMENTAL_FAN:
            return Reacting(id, "Ornamental Fan", RelicTier::UNCOMMON,
                            { RT::EveryInTurn(RelicHook::ATTACK_PLAYED, 3,
                                        { CE::Block(4) }) });

        case RelicId::PANTOGRAPH:
            return Carried(id, "Pantograph", RelicTier::UNCOMMON);

        case RelicId::PEAR:
            return Carried(id, "Pear", RelicTier::UNCOMMON);

        case RelicId::QUESTION_CARD:
            return Carried(id, "Question Card", RelicTier::UNCOMMON);

        case RelicId::SHURIKEN:
            return Reacting(id, "Shuriken", RelicTier::UNCOMMON,
                            { RT::EveryInTurn(RelicHook::ATTACK_PLAYED, 3,
                                        { CE::ApplyPowerToSelf(
                                            PowerType::STRENGTH, 1) }) });

        case RelicId::SINGING_BOWL:
            return Carried(id, "Singing Bowl", RelicTier::UNCOMMON);

        case RelicId::STRIKE_DUMMY:
            // The battle adds the damage to cards named Strike.
            return Carried(id, "Strike Dummy", RelicTier::UNCOMMON);

        case RelicId::SUNDIAL:
            return Reacting(id, "Sundial", RelicTier::UNCOMMON,
                            { RT::Every(RelicHook::SHUFFLED, 3,
                                        { CE::Energy(2) }) });

        case RelicId::THE_COURIER:
            return Carried(id, "The Courier", RelicTier::UNCOMMON);

        case RelicId::TOXIC_EGG:
            return Carried(id, "Toxic Egg", RelicTier::UNCOMMON);

        case RelicId::WHITE_BEAST_STATUE:
            return Carried(id, "White Beast Statue", RelicTier::UNCOMMON);

        // Rare

        case RelicId::BIRD_FACED_URN:
            return Reacting(id, "Bird-Faced Urn", RelicTier::RARE,
                            { RT::On(RelicHook::POWER_PLAYED,
                                     { CE::Heal(2) }) });

        case RelicId::CALIPERS:
            // The battle keeps 15 of the block instead of all of it.
            return Carried(id, "Calipers", RelicTier::RARE);

        case RelicId::CAPTAINS_WHEEL:
            return Reacting(id, "Captain's Wheel", RelicTier::RARE,
                            { RT::OnTurn(3, { CE::Block(18) }) });

        case RelicId::CHAMPION_BELT:
            // Answering a Vulnerable is worked out by the battle.
            return Carried(id, "Champion Belt", RelicTier::RARE);

        case RelicId::CHARONS_ASHES:
            return Reacting(id, "Charon's Ashes", RelicTier::RARE,
                            { RT::On(RelicHook::CARD_EXHAUSTED,
                                     { CE::DamageAll(3) }) });

        case RelicId::DEAD_BRANCH:
            return Reacting(id, "Dead Branch", RelicTier::RARE,
                            { RT::On(RelicHook::CARD_EXHAUSTED,
                                     { CE::AddRandomCard(1) }) });

        case RelicId::DU_VU_DOLL:
            return Carried(id, "Du-Vu Doll", RelicTier::RARE);

        case RelicId::FOSSILIZED_HELIX:
            return Reacting(id, "Fossilized Helix", RelicTier::RARE,
                            { RT::On(RelicHook::BATTLE_START,
                                     { CE::ApplyPowerToSelf(PowerType::BUFFER,
                                                            1) }) });

        case RelicId::GAMBLING_CHIP:
            return Carried(id, "Gambling Chip", RelicTier::RARE);

        case RelicId::GINGER:
            // The battle refuses Weak while this is carried.
            return Carried(id, "Ginger", RelicTier::RARE);

        case RelicId::GIRYA:
            return Carried(id, "Girya", RelicTier::RARE);

        case RelicId::ICE_CREAM:
            // The battle carries the energy over instead of refilling.
            return Carried(id, "Ice Cream", RelicTier::RARE);

        case RelicId::INCENSE_BURNER:
            return Reacting(id, "Incense Burner", RelicTier::RARE,
                            { RT::Every(RelicHook::TURN_START, 6,
                                        { CE::ApplyPowerToSelf(
                                            PowerType::INTANGIBLE, 1) }) });

        case RelicId::LIZARD_TAIL:
            // The battle brings the player back once.
            return Carried(id, "Lizard Tail", RelicTier::RARE);

        case RelicId::MANGO:
            return Carried(id, "Mango", RelicTier::RARE);

        case RelicId::OLD_COIN:
            return Carried(id, "Old Coin", RelicTier::RARE);

        case RelicId::PEACE_PIPE:
            return Carried(id, "Peace Pipe", RelicTier::RARE);

        case RelicId::POCKETWATCH:
            // The battle counts the cards played and draws next turn.
            return Carried(id, "Pocketwatch", RelicTier::RARE);

        case RelicId::PRAYER_WHEEL:
            return Carried(id, "Prayer Wheel", RelicTier::RARE);

        case RelicId::SHOVEL:
            return Carried(id, "Shovel", RelicTier::RARE);

        case RelicId::STONE_CALENDAR:
            return Reacting(id, "Stone Calendar", RelicTier::RARE,
                            { RT::OnTurnEnd(7, { CE::DamageAll(52) }) });

        case RelicId::THREAD_AND_NEEDLE:
            // Plated Armor is not modelled, so this hands over block instead.
            return Reacting(id, "Thread and Needle", RelicTier::RARE,
                            { RT::On(RelicHook::BATTLE_START,
                                     { CE::Block(4) }) });

        case RelicId::TORII:
            // The battle turns small hits into 1.
            return Carried(id, "Torii", RelicTier::RARE);

        case RelicId::TUNGSTEN_ROD:
            // The battle takes 1 off every health loss.
            return Carried(id, "Tungsten Rod", RelicTier::RARE);

        case RelicId::TURNIP:
            // The battle refuses Frail while this is carried.
            return Carried(id, "Turnip", RelicTier::RARE);

        case RelicId::UNCEASING_TOP:
            // The battle draws whenever the hand runs out.
            return Carried(id, "Unceasing Top", RelicTier::RARE);

        case RelicId::WING_BOOTS:
            return Carried(id, "Wing Boots", RelicTier::RARE);

        // Boss

        case RelicId::ASTROLABE:
            return Carried(id, "Astrolabe", RelicTier::BOSS);

        case RelicId::BLACK_BLOOD:
            return Carried(id, "Black Blood", RelicTier::BOSS);

        case RelicId::BLACK_STAR:
            return Carried(id, "Black Star", RelicTier::BOSS);

        case RelicId::BUSTED_CROWN:
            return Carried(id, "Busted Crown", RelicTier::BOSS);

        case RelicId::CALLING_BELL:
            return Carried(id, "Calling Bell", RelicTier::BOSS);

        case RelicId::COFFEE_DRIPPER:
            return Carried(id, "Coffee Dripper", RelicTier::BOSS);

        case RelicId::CURSED_KEY:
            return Carried(id, "Cursed Key", RelicTier::BOSS);

        case RelicId::ECTOPLASM:
            return Carried(id, "Ectoplasm", RelicTier::BOSS);

        case RelicId::EMPTY_CAGE:
            return Carried(id, "Empty Cage", RelicTier::BOSS);

        case RelicId::FUSION_HAMMER:
            return Carried(id, "Fusion Hammer", RelicTier::BOSS);

        case RelicId::HOVERING_KITE:
            // The battle pays out on the first discard of each turn.
            return Carried(id, "Hovering Kite", RelicTier::BOSS);

        case RelicId::INSERTER:
            return Reacting(id, "Inserter", RelicTier::BOSS,
                            { RT::Every(RelicHook::TURN_START, 2,
                                        { CE::AddOrbSlots(1) }) });

        case RelicId::MARK_OF_PAIN:
            return Carried(id, "Mark of Pain", RelicTier::BOSS);

        case RelicId::NUCLEAR_BATTERY:
            return Reacting(id, "Nuclear Battery", RelicTier::BOSS,
                            { RT::On(RelicHook::BATTLE_START,
                                     { CE::ChannelOrb(OrbType::PLASMA) }) });

        case RelicId::PANDORAS_BOX:
            return Carried(id, "Pandora's Box", RelicTier::BOSS);

        case RelicId::PHILOSOPHERS_STONE:
            return Reacting(id, "Philosopher's Stone", RelicTier::BOSS,
                            { RT::On(RelicHook::BATTLE_START,
                                     { CE::ApplyPowerToAll(PowerType::STRENGTH,
                                                           1) }) });

        case RelicId::RUNIC_CUBE:
            return Reacting(id, "Runic Cube", RelicTier::BOSS,
                            { RT::On(RelicHook::HEALTH_LOST,
                                     { CE::Draw(1) }) });

        case RelicId::RUNIC_DOME:
            return Carried(id, "Runic Dome", RelicTier::BOSS);

        case RelicId::RUNIC_PYRAMID:
            // The battle keeps the whole hand while this is carried.
            return Carried(id, "Runic Pyramid", RelicTier::BOSS);

        case RelicId::SACRED_BARK:
            return Carried(id, "Sacred Bark", RelicTier::BOSS);

        case RelicId::SLAVERS_COLLAR:
            return Carried(id, "Slaver's Collar", RelicTier::BOSS);

        case RelicId::SNECKO_EYE:
            // The battle draws two more; the random costs are not modelled.
            return Carried(id, "Snecko Eye", RelicTier::BOSS);

        case RelicId::SOZU:
            return Carried(id, "Sozu", RelicTier::BOSS);

        case RelicId::TINY_HOUSE:
            return Carried(id, "Tiny House", RelicTier::BOSS);

        case RelicId::VELVET_CHOKER:
            // The battle stops at six cards a turn.
            return Carried(id, "Velvet Choker", RelicTier::BOSS);

        // Shop

        case RelicId::BRIMSTONE:
            return Reacting(id, "Brimstone", RelicTier::SHOP,
                            { RT::On(RelicHook::TURN_START,
                                     { CE::ApplyPowerToSelf(
                                           PowerType::STRENGTH, 2),
                                       CE::ApplyPowerToAll(PowerType::STRENGTH,
                                                           1) }) });

        case RelicId::CAULDRON:
            return Carried(id, "Cauldron", RelicTier::SHOP);

        case RelicId::CHEMICAL_X:
            // The battle counts two more for every X cost.
            return Carried(id, "Chemical X", RelicTier::SHOP);

        case RelicId::CLOCKWORK_SOUVENIR:
            return Reacting(id, "Clockwork Souvenir", RelicTier::SHOP,
                            { RT::On(RelicHook::BATTLE_START,
                                     { CE::ApplyPowerToSelf(
                                         PowerType::ARTIFACT, 1) }) });

        case RelicId::DOLLYS_MIRROR:
            return Carried(id, "Dolly's Mirror", RelicTier::SHOP);

        case RelicId::FROZEN_EYE:
            return Carried(id, "Frozen Eye", RelicTier::SHOP);

        case RelicId::HAND_DRILL:
            return Carried(id, "Hand Drill", RelicTier::SHOP);

        case RelicId::LEES_WAFFLE:
            return Carried(id, "Lee's Waffle", RelicTier::SHOP);

        case RelicId::MEDICAL_KIT:
            // The battle lets status cards be played while this is carried.
            return Carried(id, "Medical Kit", RelicTier::SHOP);

        case RelicId::MEMBERSHIP_CARD:
            return Carried(id, "Membership Card", RelicTier::SHOP);

        case RelicId::ORANGE_PELLETS:
            // The battle clears the debuffs once all three kinds are played.
            return Carried(id, "Orange Pellets", RelicTier::SHOP);

        case RelicId::ORRERY:
            return Carried(id, "Orrery", RelicTier::SHOP);

        case RelicId::PRISMATIC_SHARD:
            return Carried(id, "Prismatic Shard", RelicTier::SHOP);

        case RelicId::SLING_OF_COURAGE:
            // The real relic only helps against elites.
            return Reacting(id, "Sling of Courage", RelicTier::SHOP,
                            { RT::On(RelicHook::BATTLE_START,
                                     { CE::ApplyPowerToSelf(
                                         PowerType::STRENGTH, 2) }) });

        case RelicId::STRANGE_SPOON:
            return Carried(id, "Strange Spoon", RelicTier::SHOP);

        case RelicId::THE_ABACUS:
            return Reacting(id, "The Abacus", RelicTier::SHOP,
                            { RT::On(RelicHook::SHUFFLED,
                                     { CE::Block(6) }) });

        case RelicId::TOOLBOX:
            return Reacting(
                id, "Toolbox", RelicTier::SHOP,
                { RT::On(RelicHook::BATTLE_START,
                         { CE::AddRandomCard(1)
                               .FromPool(CardColor::COLORLESS) }) });

        // Event

        case RelicId::BLOODY_IDOL:
            return Carried(id, "Bloody Idol", RelicTier::EVENT);

        case RelicId::CULTIST_HEADPIECE:
            return Carried(id, "Cultist Headpiece", RelicTier::EVENT);

        case RelicId::ENCHIRIDION:
            return Reacting(id, "Enchiridion", RelicTier::EVENT,
                            { RT::On(RelicHook::BATTLE_START,
                                     { CE::AddRandomPower() }) });

        case RelicId::FACE_OF_CLERIC:
            return Carried(id, "Face of Cleric", RelicTier::EVENT);

        case RelicId::GOLDEN_IDOL:
            return Carried(id, "Golden Idol", RelicTier::EVENT);

        case RelicId::GREMLIN_VISAGE:
            return Reacting(id, "Gremlin Visage", RelicTier::EVENT,
                            { RT::On(RelicHook::BATTLE_START,
                                     { CE::ApplyPowerToSelf(PowerType::WEAK,
                                                            1) }) });

        case RelicId::MARK_OF_THE_BLOOM:
            return Carried(id, "Mark of the Bloom", RelicTier::EVENT);

        case RelicId::MUTAGENIC_STRENGTH:
            return Reacting(id, "Mutagenic Strength", RelicTier::EVENT,
                            { RT::On(RelicHook::BATTLE_START,
                                     { CE::ApplyPowerToSelf(
                                           PowerType::STRENGTH, 3),
                                       CE::ApplyPowerToSelf(
                                           PowerType::STRENGTH_DOWN, 3) }) });

        case RelicId::NLOTHS_GIFT:
            return Carried(id, "N'loth's Gift", RelicTier::EVENT);

        case RelicId::NLOTHS_HUNGRY_FACE:
            return Carried(id, "N'loth's Hungry Face", RelicTier::EVENT);

        case RelicId::NECRONOMICON:
            return Carried(id, "Necronomicon", RelicTier::EVENT);

        case RelicId::NEOWS_LAMENT:
            return Carried(id, "Neow's Lament", RelicTier::EVENT);

        case RelicId::NILRYS_CODEX:
            return Carried(id, "Nilry's Codex", RelicTier::EVENT);

        case RelicId::ODD_MUSHROOM:
            return Carried(id, "Odd Mushroom", RelicTier::EVENT);

        case RelicId::RED_MASK:
            return Reacting(id, "Red Mask", RelicTier::EVENT,
                            { RT::On(RelicHook::BATTLE_START,
                                     { CE::ApplyPowerToAll(PowerType::WEAK,
                                                           1) }) });

        case RelicId::SPIRIT_POOP:
            return Carried(id, "Spirit Poop", RelicTier::EVENT);

        case RelicId::SSSERPENT_HEAD:
            return Carried(id, "Ssserpent Head", RelicTier::EVENT);

        case RelicId::WARPED_TONGS:
            return Reacting(id, "Warped Tongs", RelicTier::EVENT,
                            { RT::On(RelicHook::TURN_START,
                                     { CE::UpgradeRandomHandCard() }) });

        case RelicId::INVALID:
            break;
    }

    return Relic();
}

const std::vector<RelicId>& RelicRegistry::GetAll()
{
    static const std::vector<RelicId> all = [] {
        std::vector<RelicId> built;

        for (int i = 1; i <= static_cast<int>(LAST_RELIC_ID); ++i)
        {
            const RelicId id = static_cast<RelicId>(i);

            if (Get(id).GetId() != RelicId::INVALID)
            {
                built.emplace_back(id);
            }
        }

        return built;
    }();

    return all;
}

std::vector<RelicId> RelicRegistry::GetPool(RelicTier tier)
{
    std::vector<RelicId> matching;

    for (const RelicId id : GetAll())
    {
        if (Get(id).GetTier() == tier)
        {
            matching.emplace_back(id);
        }
    }

    return matching;
}

RelicId RelicRegistry::GetStarterRelic(CardColor color)
{
    switch (color)
    {
        case CardColor::RED:
            return RelicId::BURNING_BLOOD;

        case CardColor::GREEN:
            return RelicId::RING_OF_THE_SNAKE;

        case CardColor::BLUE:
            return RelicId::CRACKED_CORE;

        default:
            return RelicId::INVALID;
    }
}

bool RelicRegistry::GivesExtraEnergy(RelicId id)
{
    // A slaver's collar is not on this list: it only pays out in the harder
    // fights, which the battle sees to.

    switch (id)
    {
        case RelicId::BUSTED_CROWN:
        case RelicId::COFFEE_DRIPPER:
        case RelicId::CURSED_KEY:
        case RelicId::ECTOPLASM:
        case RelicId::FUSION_HAMMER:
        case RelicId::MARK_OF_PAIN:
        case RelicId::PHILOSOPHERS_STONE:
        case RelicId::RUNIC_DOME:
        case RelicId::SOZU:
        case RelicId::VELVET_CHOKER:
            return true;

        default:
            return false;
    }
}

int RelicRegistry::BonusMaxHealth(RelicId id)
{
    switch (id)
    {
        case RelicId::STRAWBERRY:
            return 7;

        case RelicId::PEAR:
            return 10;

        case RelicId::MANGO:
            return 14;

        default:
            return 0;
    }
}
}  // namespace ConquerTheSpire
