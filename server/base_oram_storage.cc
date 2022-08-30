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
#include "base_oram_storage.h"

namespace oram_impl {
// Simply does nothing. We need a pure virtual destructor mainly because we do
// not want an instance of BaseOramServerStorage to be created accidentally.
BaseOramServerStorage::~BaseOramServerStorage() {}
}  // namespace oram_impl