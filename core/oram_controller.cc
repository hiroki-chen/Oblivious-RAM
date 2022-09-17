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
#include "oram_controller.h"

#include <spdlog/logger.h>

extern std::shared_ptr<spdlog::logger> logger;

namespace oram_impl {
OramController::OramController(uint32_t id, bool standalone, size_t block_num,
                               OramType oram_type)
    : id_(id),
      standalone_(standalone),
      block_num_(block_num),
      oram_type_(oram_type),
      is_initialized_(false) {
  cryptor_ = oram_crypto::Cryptor::GetInstance();

  // Intialize the hash of this instance.
  OramStatus status;
  if (!(status = oram_crypto::RandomBytes(instance_hash_, 32ul)).ok()) {
    logger->error(
        "[-] OramController failed to generated random instance id : {}",
        status.ErrorMessage());
    abort();
  }
}

}  // namespace oram_impl
