/*
 * Copyright (C) 2011-2015  Maclab, DIBRIS, Universita di Genova <info@cyskin.com>
 * Authored by Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * The research leading to these results has received funding from
 * the European Commission's Seventh Framework Programme (FP7) under
 * Grant Agreement n. 231500 (ROBOSKIN).
 *
 * This file is part of Skinware.
 *
 * Skinware is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Skinware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Skinware.  If not, see <http://www.gnu.org/licenses/>.
 */

#define URT_LOG_PREFIX "skin: "
#include <skin_base.h>
#include <skin_sensor.h>
#include <skin_module.h>
#include <skin_patch.h>
#include "internal.h"
#include "user_internal.h"

#define DEFINE_SET_HOOK_FUNC(object, event)			\
void (skin_set_##object##_##event##_hook)(struct skin *skin,	\
		skin_hook_##object hook, void *user_data, ...)	\
{								\
	skin->object##_##event##_hook = hook;			\
	skin->object##_##event##_user_data = user_data;		\
}								\
URT_EXPORT_SYMBOL(skin_set_##object##_##event##_hook);

DEFINE_SET_HOOK_FUNC(writer, init)
DEFINE_SET_HOOK_FUNC(writer, clean)
DEFINE_SET_HOOK_FUNC(reader, init)
DEFINE_SET_HOOK_FUNC(reader, clean)
DEFINE_SET_HOOK_FUNC(driver, init)
DEFINE_SET_HOOK_FUNC(driver, clean)
DEFINE_SET_HOOK_FUNC(user, init)
DEFINE_SET_HOOK_FUNC(user, clean)
DEFINE_SET_HOOK_FUNC(sensor, init)
DEFINE_SET_HOOK_FUNC(sensor, clean)
DEFINE_SET_HOOK_FUNC(module, init)
DEFINE_SET_HOOK_FUNC(module, clean)
DEFINE_SET_HOOK_FUNC(patch, init)
DEFINE_SET_HOOK_FUNC(patch, clean)
