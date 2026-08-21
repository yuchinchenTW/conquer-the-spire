// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_RELIC_REGISTRY_HPP
#define CONQUER_THE_SPIRE_RELIC_REGISTRY_HPP

#include <conquer-the-spire/Enums/RelicId.hpp>
#include <conquer-the-spire/Models/Relic.hpp>

#include <vector>

namespace ConquerTheSpire
{
//!
//! \brief RelicRegistry class.
//!
//! Builds relics from their id, the same way the card registry builds cards.
//!
class RelicRegistry
{
 public:
    //! Returns the relic \p id, or an invalid relic when there is no such
    //! thing.
    static Relic Get(RelicId id);

    //! Returns every id the registry knows.
    static const std::vector<RelicId>& GetAll();

    //! Returns every id of \p tier.
    static std::vector<RelicId> GetPool(RelicTier tier);

    //! Returns the relic a character starts a run with.
    static RelicId GetStarterRelic(CardColor color);

    //! Returns true when carrying \p id refills one more energy each turn.
    static bool GivesExtraEnergy(RelicId id);

    //! Returns the maximum health \p id hands over when it is picked up.
    static int BonusMaxHealth(RelicId id);
};
}  // namespace ConquerTheSpire

#endif  // CONQUER_THE_SPIRE_RELIC_REGISTRY_HPP
