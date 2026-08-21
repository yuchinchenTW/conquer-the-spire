// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_REWARD_GENERATOR_HPP
#define CONQUER_THE_SPIRE_REWARD_GENERATOR_HPP

#include <conquer-the-spire/Enums/MonsterEnums.hpp>
#include <conquer-the-spire/Models/Player.hpp>
#include <conquer-the-spire/Rewards/Reward.hpp>

#include <istream>
#include <map>
#include <random>
#include <string>
#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief RewardGenerator class.
//!
//! Rolls what a fight or a chest hands over, and keeps the books a run needs
//! between them: which relics are still to be found, how likely the next
//! potion is, and how far the rare card chance has crept up.
//!
class RewardGenerator
{
 public:
    //! How many cards a reward normally offers.
    static constexpr int CARD_CHOICES = 3;

    //! What the potion chance starts an act at.
    static constexpr int BASE_POTION_CHANCE = 40;

    //! How far below the listed chance a rare card starts, and where it goes
    //! back to once a rare has been rolled.
    static constexpr int RARE_CARD_HANDICAP = 5;

    RewardGenerator() = default;

    //! Sets up the pools for a run with \p character, shuffled with \p rng.
    RewardGenerator(CardColor character, std::mt19937& rng);

    //! Starts act \p act. The potion chance goes back to where an act
    //! starts it, and what a boss hands over depends on which act it fell
    //! in: the first two hand over cards and potions, the last two do not.
    void BeginAct(int act);

    //! Returns the act the rewards are being rolled for.
    int GetAct() const;

    //! Rolls what a fight of \p type hands over to \p player.
    std::vector<Reward> ForCombat(MonsterType type, const Player& player,
                                  std::mt19937& rng);

    //! Rolls what a chest of \p size holds for \p player.
    std::vector<Reward> ForChest(ChestSize size, const Player& player,
                                 std::mt19937& rng);

    //! Rolls how big the next chest is: small half the time, medium a third
    //! of the time, large the rest.
    static ChestSize RollChestSize(std::mt19937& rng);

    //! Takes the next relic of \p tier out of the run's pool, which is what
    //! a shop stocking its shelf does, so that nothing turns up twice.
    RelicId TakeRelic(RelicTier tier, const Player& player,
                      std::mt19937& rng);

    //! Rolls which potion turns up, by the same weights a fight uses.
    PotionId TakePotion(std::mt19937& rng) const;

    //! Rolls a relic tier by the plain weights: half common, a third
    //! uncommon, the rest rare.
    RelicTier RollTier(std::mt19937& rng) const;

    //! Writes the books out as one line, and reads them back.
    std::string Serialize() const;
    bool Load(std::istream& in);

    //! Returns how likely the next potion is, out of a hundred.
    int GetPotionChance() const;

    //! Returns how many relics of \p tier are still to be found.
    std::size_t CountRemaining(RelicTier tier) const;

 private:
    //! Takes the next relic of \p tier out of the pool, skipping the ones
    //! \p player already carries. Falls back to another tier when one runs
    //! dry.
    RelicId DrawRelic(RelicTier tier, const Player& player,
                      std::mt19937& rng);

    //! Rolls a tier the way a plain relic reward does: half common, a third
    //! uncommon, the rest rare.
    RelicTier RollRelicTier(std::mt19937& rng) const;

    //! Rolls the cards a fight of \p type offers, never the same one twice.
    //! With \p rareOnly the pick is all rare cards, which is what a boss
    //! hands over.
    std::vector<CardId> RollCardChoices(MonsterType type,
                                        const Player& player,
                                        std::mt19937& rng,
                                        bool rareOnly = false);

    //! Rolls one card of the rarity the weights land on, or a rare one when
    //! \p rareOnly is set.
    CardId RollCard(MonsterType type, const Player& player, std::mt19937& rng,
                    bool rareOnly = false);

    //! Puts a potion on \p rewards if one drops, and moves the chance of the
    //! next one either way.
    void AddPotionReward(const Player& player, std::mt19937& rng,
                         std::vector<Reward>& rewards);

    //! Rolls which potion turns up: most often a common one.
    PotionId RollPotion(std::mt19937& rng) const;

    CardColor m_character = CardColor::INVALID;
    int m_act = 1;
    std::map<RelicTier, std::vector<RelicId>> m_relicPools;
    int m_potionChance = BASE_POTION_CHANCE;

    //! How much is taken off the rare card chance. It shrinks with every
    //! common rolled and goes back up once a rare turns up.
    int m_rareHandicap = RARE_CARD_HANDICAP;

    //! How many more chests hand over a second relic, which is what a
    //! Matryoshka promises, and whether it has been wound up yet.
    int m_extraChestRelics = 0;
    bool m_matryoshkaStarted = false;

    //! Whether the empty chest a N'loth's Hungry Face brings has been had.
    bool m_hungryFaceSpent = false;
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_REWARD_GENERATOR_HPP
