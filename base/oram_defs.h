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
#ifndef ORAM_IMPL_BASE_ORAM_DEFS_H_
#define ORAM_IMPL_BASE_ORAM_DEFS_H_

#include <absl/container/flat_hash_map.h>

#include <ostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#define DEFAULT_ORAM_METADATA_SIZE sizeof(oram_impl::BlockType)
#define DEFAULT_ORAM_DATA_SIZE 512
#define DEFAULT_COMPRESSED_BUF_SIZE 8192
#define DEFAULT_ORAM_ENCSKIP_SIZE                                  \
  crypto_aead_aes256gcm_NPUBBYTES + crypto_aead_aes256gcm_ABYTES + \
      sizeof(uint32_t)

#define ORAM_BLOCK_SIZE sizeof(oram_impl::oram_block_t)
#define UNIMPLEMENTED_FUNC crash(__PRETTY_FUNCTION__, " not implemented yet")

#if !defined(POW2)
#define POW2(x) (1 << (x))
#endif

#if !defined(LOG_BASE)
#define LOG_BASE(x, base) (log(x) / log(base))
#endif

namespace oram_impl {
enum class OramStatus {
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
  kVersionMismatch = 10,
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

enum class OramType {
  kLinearOram = 0,
  kSquareOram = 1,
  kPathOram = 2,
  kPartitionOram = 3,
  kInvalidStorage = 4,
};

enum class OramStorageType {
  kFlatStorage = 0,
  kTreeStorage = 1,
  kLayeredStorage = 2,
  kInvalidStorage = 3,
};

// The header containing metadata.
typedef struct _oram_block_header_t {
  // No need to encrypt (can be seen by the server).
  uint8_t iv[12];
  uint8_t mac_tag[16];
  uint32_t block_id;

  // Encrypted fields only accessible to client.
  BlockType type;
} oram_block_header_t;

// The block for ORAM storage.
typedef struct _oram_block_t {
  oram_block_header_t header;

  uint8_t data[DEFAULT_ORAM_DATA_SIZE];
} oram_block_t;

// Constants.
static const std::unordered_map<OramStatus, std::string> kErrorList = {
    {OramStatus::kOK, "OK"},
    {OramStatus::kInvalidArgument, "Invalid argument"},
    {OramStatus::kInvalidOperation, "Invalid operation"},
    {OramStatus::kOutOfMemory, "Out of memory"},
    {OramStatus::kFileNotFound, "File not found"},
    {OramStatus::kFileIOError, "File IO error"},
    {OramStatus::kOutOfRange, "Out of range"},
    {OramStatus::kServerError, "Server error"},
    {OramStatus::kObjectNotFound, "The object is not found"},
    {OramStatus::kUnknownError, "Unknown error"},
    {OramStatus::kVersionMismatch, "Version mismatch"},
};

static const std::string oram_type_mismatch_err =
    "The remote storage cannot match the given ORAM type.";
static const std::string oram_size_mismatch_err =
    "The remote storage size cannot match the given ORAM size.";

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
using server_tree_storage_t =
    absl::flat_hash_map<server_storage_tag_t, server_storage_data>;
using server_flat_storage_t = std::string;

struct BlockEqual {
 private:
  uint32_t block_id_;

 public:
  explicit BlockEqual(uint32_t block_id) : block_id_(block_id) {}
  inline bool operator()(const oram_block_t &block) const {
    // Dummy blocks cannot be accidentally read out.
    return block.header.block_id == block_id_ &&
           block.header.type == BlockType::kNormal;
  }
};

static void crash_t(std::ostringstream &oss) {
  oss << std::endl;
  throw std::runtime_error(oss.str());
}

template <typename Arg, typename... Rest>
void crash_t(std::ostringstream &oss, Arg arg, Rest... rest) {
  oss << arg;
  crash_t(oss, rest...);
}

template <typename... Args>
void crash(Args... args) {
  std::ostringstream oss;
  crash_t(oss, args...);
}
}  // namespace oram_impl

#endif  // ORAM_IMPL_BASE_ORAM_DEFS_H_
