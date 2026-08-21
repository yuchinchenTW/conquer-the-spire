// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CONQUER_THE_SPIRE_CARD_BUILDERS_HPP
#define CONQUER_THE_SPIRE_CARD_BUILDERS_HPP

#include <conquer-the-spire/Enums/CardId.hpp>
#include <conquer-the-spire/Models/Card.hpp>

namespace ConquerTheSpire::Detail
{
//! Builds an Ironclad card, or an invalid card when \p id is not one. The
//! registry is the way to reach these; they are split by colour only to keep
//! the files a readable size.
Card MakeIroncladCard(CardId id, int upgradeCount);

//! Builds a Silent card, or an invalid card when \p id is not one.
Card MakeSilentCard(CardId id, int upgradeCount);

//! Builds a Defect card, or an invalid card when \p id is not one.
Card MakeDefectCard(CardId id, int upgradeCount);

//! Builds a colourless card, or an invalid card when \p id is not one.
Card MakeColorlessCard(CardId id, int upgradeCount);

//! Builds a status card, or an invalid card when \p id is not one.
Card MakeStatusCard(CardId id, int upgradeCount);

//! Builds a curse card, or an invalid card when \p id is not one.
Card MakeCurseCard(CardId id, int upgradeCount);
}  // namespace ConquerTheSpire::Detail

#endif  // CONQUER_THE_SPIRE_CARD_BUILDERS_HPP
