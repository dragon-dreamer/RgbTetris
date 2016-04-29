// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include "static_class.h"

///Something like classic "Space invaders" game. You shoot differently shaped space
///invaders and don't allow them to land on your shooting platform. They can shoot you,
///too! You have 5 lives. There're also some bosses in the game, which have
///a lot of lives and shoot you. These bosses don't fall down, but they're
///very strong and have some weak points which you can shoot. You can also
///damage a boss shooting anywhere at it, but in this case there's only small
///probability that your bullet will pass boss armor.
class space_invaders : static_class
{
public:
	static void run();
};
