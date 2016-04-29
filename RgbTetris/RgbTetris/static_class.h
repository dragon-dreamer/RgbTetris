// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0

#pragma once

///Base helper class for static classes
class static_class
{
public:
	static_class() = delete;
	static_class(const static_class&) = delete;
	static_class& operator=(const static_class&) = delete;
};
