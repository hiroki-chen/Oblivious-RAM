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
#ifndef ORAM_IMPL_BASE_ORAM_UTILS_H_
#define ORAM_IMPL_BASE_ORAM_UTILS_H_

#include <cassert>
#include <sstream>
#include <string>

#include "oram_crypto.h"
#include "oram_defs.h"
#include "oram_status.h"
#include "ods_objects.h"

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
    const oram_impl::p_oram_bucket_t& bucket);

oram_impl::p_oram_bucket_t DeserializeFromStringVector(
    const std::vector<std::string>& data);

oram_impl::p_oram_bucket_t SampleRandomBucket(size_t size, size_t tree_size,
                                              size_t initial_offset);

void SafeFree(void* ptr);

void SafeFreeAll(size_t ptr_num, ...);

void ConvertToBlock(const std::string& data,
                    oram_impl::oram_block_t* const block);

void ConvertToString(const oram_impl::oram_block_t* const block,
                     std::string* const data);

void CheckStatus(const oram_impl::OramStatus& status,
                 const std::string& reason);

void PadStash(oram_impl::p_oram_stash_t* const stash, const size_t bucket_size);

void PrintStash(const oram_impl::p_oram_stash_t& stash);

void PrintOramTree(const oram_impl::server_tree_storage_t& storage);

void EncryptBlock(oram_impl::oram_block_t* const block,
                  oram_crypto::Cryptor* const cryptor);

void DecryptBlock(oram_impl::oram_block_t* const block,
                  oram_crypto::Cryptor* const cryptor);

size_t DataCompress(const uint8_t* data, size_t data_size, uint8_t* const out);

size_t DataDecompress(const uint8_t* data, size_t data_size,
                      uint8_t* const out);

std::string TypeToName(oram_impl::OramType oram_type);

std::vector<std::string> split(const std::string& str, char delim);
}  // namespace oram_utils

#endif  // ORAM_IMPL_BASE_ORAM_UTILS_H_