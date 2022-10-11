/*
 Copyright (c) 2022 Haobin Chen

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef ORAM_IMPL_CORE_ORAM_H_
#define ORAM_IMPL_CORE_ORAM_H_

#include <string>

#include "linear_oram_controller.h"
#include "odict_controller.h"
#include "ods_cache.h"
#include "oram_controller.h"
#include "partition_oram_controller.h"
#include "path_oram_controller.h"
#include "square_root_oram_controller.h"

namespace oram_impl {
static const uint8_t major_version = 1;
static const uint8_t minor_version = 2;
static const uint8_t patch_version = 2;

std::string GetVersion(void);
}  // namespace oram_impl

#endif  // ORAM_IMPL_CORE_ORAM_H_