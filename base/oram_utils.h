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
#ifndef PARTITION_ORAM_BASE_ORAM_UTILS_H_
#define PARTITION_ORAM_BASE_ORAM_UTILS_H_

#include <cassert>
#include <string>
#include <sstream>

#include "oram_defs.h"
#include "oram_crypto.h"

#define ASSERT_MSG(x) !(std::cerr << "Assertion failed: " << x << std::endl)
#define PANIC_IF(cond, message) assert(!(cond) || ASSERT_MSG(message))

namespace oram_utils {
std::string ReadKeyCrtFile(const std::string& path);

template <typename... Args>
std::string StrCat(const std::string& s, Args&&... args) {
  std::ostringstream oss;
  oss << s;
  // Recursively concatenate the rest of the arguments using argument pack
  // expansion and perfect forwarding.
  (oss << ... << std::forward<Args>(args));
  return oss.str();
}

std::vector<std::string> ReadDataFromFile(const std::string& path);

std::vector<std::string> SerializeToStringVector(
    const partition_oram::p_oram_bucket_t& bucket);

partition_oram::p_oram_bucket_t DeserializeFromStringVector(
    const std::vector<std::string>& data);

partition_oram::p_oram_bucket_t SampleRandomBucket(size_t size,
                                                   size_t tree_size,
                                                   size_t initial_offset);

void SafeFree(void* ptr);

void SafeFreeAll(size_t ptr_num, ...);

void ConvertToBlock(const std::string& data,
                    partition_oram::oram_block_t* const block);

void ConvertToString(const partition_oram::oram_block_t* const block,
                     std::string* const data);

void CheckStatus(partition_oram::Status status, const std::string& reason);

void PadStash(partition_oram::p_oram_stash_t* const stash,
              const size_t bucket_size);

void PrintStash(const partition_oram::p_oram_stash_t& stash);

void PrintOramTree(const partition_oram::server_storage_t& storage);

void EncryptBlock(partition_oram::oram_block_t* const block,
                  oram_crypto::Cryptor* const cryptor);

void DecryptBlock(partition_oram::oram_block_t* const block,
                  oram_crypto::Cryptor* const cryptor);

size_t DataCompress(const uint8_t* data, size_t data_size, uint8_t* const out);

size_t DataDecompress(const uint8_t* data, size_t data_size, uint8_t* const out);
}  // namespace oram_utils

#endif  // PARTITION_ORAM_BASE_ORAM_UTILS_H_