// Copyright (c) 2019 Chris Ohk

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <conquer-the-spire/Monsters/MonsterLibrary.hpp>

namespace ConquerTheSpire::Monsters
{
Monster JawWorm()
{
    return Monster("Jaw Worm", 42,
                   { MonsterMove::Attack("Chomp", 11),
                     MonsterMove::AttackAndDefend("Thrash", 7, 5),
                     MonsterMove::Buff("Bellow", PowerType::STRENGTH, 3, 6) });
}

Monster Cultist()
{
    // Incantation only happens once, so the script stops on Dark Strike.
    return Monster("Cultist", 50,
                   { MonsterMove::Buff("Incantation", PowerType::STRENGTH, 3),
                     MonsterMove::Attack("Dark Strike", 6) },
                   false);
}

Monster RedLouse()
{
    return Monster("Red Louse", 11, { MonsterMove::Attack("Bite", 6) });
}

Monster AcidSlimeS()
{
    return Monster("Acid Slime (S)", 8,
                   { MonsterMove::Attack("Tackle", 3),
                     MonsterMove::Debuff("Lick", PowerType::WEAK, 1) });
}

Monster TrainingDummy(int health)
{
    return Monster("Training Dummy", health, {});
}
}  // namespace ConquerTheSpire::Monsters
