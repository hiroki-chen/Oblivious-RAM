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
#ifndef PARTITION_ORAM_BASE_ORAM_DEFS_H_
#define PARTITION_ORAM_BASE_ORAM_DEFS_H_

#include <utility>
#include <vector>
#include <unordered_map>

#include <absl/container/flat_hash_map.h>

#define DEFAULT_ORAM_DATA_SIZE 512
#define DEFAULT_COMPRESSED_BUF_SIZE 8192

#define ORAM_BLOCK_SIZE sizeof(partition_oram::oram_block_t)

#if !defined(POW2)
#define POW2(x) (1 << (x))
#endif

#if !defined(LOG_BASE)
#define LOG_BASE(x, base) (log(x) / log(base))
#endif

namespace partition_oram {
enum class Status {
  kOK = 0,
  kInvalidArgument = 1,
  kInvalidOperation = 2,
  kOutOfMemory = 3,
  kFileNotFound = 4,
  kFileIOError = 5,
  kOutOfRange = 6,
  kServerError = 7,
  kObjectNotFound = 8,
  kUnknownError = 9,
};

enum class Operation {
  kRead = 0,
  kWrite = 1,
  kInvalid = 2,
};

enum class EvictType {
  kEvictSeq = 0,
  kEvictRand = 1,
};

enum class BlockType {
  kDummy = 0,
  kNormal = 1,
  kInvalid = 2,
};

// The header containing metadata.
typedef struct _oram_block_header_t {
  uint32_t block_id;
  BlockType type;
  uint8_t iv[12];
  uint8_t mac_tag[16];
} oram_block_header_t;

// The block for ORAM storage.
typedef struct _oram_block_t {
  oram_block_header_t header;

  uint8_t data[DEFAULT_ORAM_DATA_SIZE];
} oram_block_t;

// Constants.
static const std::unordered_map<Status, std::string> kErrorList = {
    {Status::kOK, "OK"},
    {Status::kInvalidArgument, "Invalid argument"},
    {Status::kInvalidOperation, "Invalid operation"},
    {Status::kOutOfMemory, "Out of memory"},
    {Status::kFileNotFound, "File not found"},
    {Status::kFileIOError, "File IO error"},
    {Status::kOutOfRange, "Out of range"},
    {Status::kServerError, "Server error"},
    {Status::kObjectNotFound, "The object is not found"},
    {Status::kUnknownError, "Unknown error"}};

// This factor can also be used to control the size of the Path ORAM to prevent
// storage overflow.
static const float kPartitionAdjustmentFactor = 1.;

static const uint32_t kMaximumOramStorageNum = 1e5;

// Alias for Path ORAM.
using p_oram_bucket_t = std::vector<oram_block_t>;
using p_oram_stash_t = std::vector<oram_block_t>;
using p_oram_path_t = std::vector<p_oram_bucket_t>;
using p_oram_position_t = std::unordered_map<uint32_t, uint32_t>;
// Alias for Partition ORAM.
using pp_oram_slot_t = std::vector<std::vector<oram_block_t>>;
// Alias for server storage.
using server_storage_data = std::vector<std::string>;
using server_storage_tag_t = std::pair<uint32_t, uint32_t>;
using server_storage_t =
    absl::flat_hash_map<server_storage_tag_t, server_storage_data>;

struct BlockEqual {
 private:
  uint32_t block_id_;

 public:
  explicit BlockEqual(uint32_t block_id) : block_id_(block_id) {}
  inline bool operator()(const oram_block_t& block) const {
    // Dummy blocks cannot be accidentally read out.
    return block.header.block_id == block_id_ &&
           block.header.type == BlockType::kNormal;
  }
};
}  // namespace partition_oram

#endif  // PARTITION_ORAM_BASE_ORAM_DEFS_H_
