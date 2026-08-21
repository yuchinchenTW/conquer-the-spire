// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Rewards/Reward.hpp>

#include <utility>

namespace ConquerTheSpire
{
Reward Reward::Gold(int amount)
{
    Reward reward;
    reward.kind = RewardKind::GOLD;
    reward.amount = amount;

    return reward;
}

Reward Reward::CardChoice(std::vector<CardId> cards)
{
    Reward reward;
    reward.kind = RewardKind::CARD_CHOICE;
    reward.cards = std::move(cards);

    return reward;
}

Reward Reward::RelicChoice(std::vector<RelicId> relics)
{
    Reward reward;
    reward.kind = RewardKind::RELIC_CHOICE;
    reward.relics = std::move(relics);

    return reward;
}

Reward Reward::Potion(PotionId id)
{
    Reward reward;
    reward.kind = RewardKind::POTION;
    reward.potion = id;

    return reward;
}

Reward Reward::MaxHealth(int amount)
{
    Reward reward;
    reward.kind = RewardKind::MAX_HEALTH;
    reward.amount = amount;

    return reward;
}

Reward Reward::Curse(CardId id)
{
    Reward reward;
    reward.kind = RewardKind::CURSE;
    reward.cards.emplace_back(id);

    return reward;
}
}  // namespace ConquerTheSpire
