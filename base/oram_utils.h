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
#define PANIC(message)   \
  {                      \
    ASSERT_MSG(message); \
    exit(1);             \
  }

#define ASSEMBLE_HEADER(request, id, hash, version)      \
  {                                                      \
    request.mutable_header()->set_id(id);                \
    request.mutable_header()->set_instance_hash(         \
        std::string(reinterpret_cast<char*>(hash), 32)); \
    request.mutable_header()->set_version(version);      \
  }

namespace oram_utils {
std::string ReadKeyCrtFile(const std::string& path);

template <typename From, typename To>
To* TryCast(From* const from) {
  To* to = dynamic_cast<To*>(from);

  PANIC_IF(to == nullptr, "Polymorphism failed.");
  return to;
}

template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) noexcept {
  return static_cast<typename std::underlying_type<E>::type>(e);
}

template <typename T>
void PermuteBy(const std::vector<uint32_t>& perm, std::vector<T>& arr) {
  // A temporary buffer.
  std::vector<T> clone = arr;

  for (size_t i = 0; i < arr.size(); i++) {
    // Perm[i] = The i-th element is placed to perm[i]-th position.
    clone[perm[i]] = arr[i];
  }

  arr.clear();
  arr.assign(clone.begin(), clone.end());
}

template <typename... Args>
std::string StrCat(Args&&... args) {
  std::ostringstream oss;
  // Recursively concatenate the rest of the arguments using argument pack
  // expansion and perfect forwarding.
  (oss << ... << std::forward<Args>(args));
  return oss.str();
}

template <typename Fn>
oram_impl::OramStatus TryExec(Fn&& target_func) {
  try {
    // Run the function that probably throws exception.
    target_func();
  } catch (const std::exception& e) {
    // Cast this exception to status code.
    return oram_impl::OramStatus(oram_impl::StatusCode::kUnknownError,
                                 e.what());
  }

  // OK.
  return oram_impl::OramStatus::OK;
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

oram_impl::OramType StrToType(const std::string& type);

template <class T>
void PermuteBy(const std::vector<uint32_t>& perm,
               std::vector<oram_impl::oram_block_t>& arr);
}  // namespace oram_utils

#endif  // ORAM_IMPL_BASE_ORAM_UTILS_H_