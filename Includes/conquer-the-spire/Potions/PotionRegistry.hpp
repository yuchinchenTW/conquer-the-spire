// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_POTION_REGISTRY_HPP
#define CONQUER_THE_SPIRE_POTION_REGISTRY_HPP

#include <conquer-the-spire/Models/Potion.hpp>

#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief PotionRegistry class.
//!
//! Builds potions from their id, the same way the card and relic registries
//! build theirs.
//!
class PotionRegistry
{
 public:
    //! Returns the potion \p id, or an invalid potion when there is no such
    //! thing.
    static Potion Get(PotionId id);

    //! Returns every id the registry knows.
    static const std::vector<PotionId>& GetAll();

    //! Returns every id the character can find.
    static std::vector<PotionId> GetAll(CardColor character);

    //! Returns every id of \p rarity.
    static std::vector<PotionId> GetPool(PotionRarity rarity);

    //! Returns every id of \p rarity the character can find.
    static std::vector<PotionId> GetPool(PotionRarity rarity,
                                         CardColor character);
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_POTION_REGISTRY_HPP
