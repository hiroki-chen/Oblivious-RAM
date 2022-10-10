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

#define DEFAULT_ORAM_METADATA_SIZE sizeof(oram_impl::BlockType) + sizeof(size_t)
#define DEFAULT_ORAM_DATA_SIZE 512
#define DEFAULT_COMPRESSED_BUF_SIZE 8192
#define DEFAULT_ORAM_ENCSKIP_SIZE                                  \
  crypto_aead_aes256gcm_NPUBBYTES + crypto_aead_aes256gcm_ABYTES + \
      sizeof(uint32_t)

#define ORAM_BLOCK_SIZE sizeof(oram_impl::oram_block_t)
#define CRASH(msg) crash(__PRETTY_FUNCTION__, msg)

#if !defined(POW2)
#define POW2(x) (1 << (x))
#endif

#if !defined(LOG_BASE)
#define LOG_BASE(x, base) (log(x) / log(base))
#endif

namespace oram_impl {
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
  kCuckooOram = 4,
  kOds = 5,
  kInvalid = 6,
};

enum class OramStorageType {
  kFlatStorage = 0,
  kSqrtStorage = 1,
  kTreeStorage = 2,
  kLayeredStorage = 3,
  kInvalidStorage = 4,
};

// The header containing metadata.
typedef struct _oram_block_header_t {
  // No need to encrypt (can be seen by the server).
  uint8_t iv[12];
  uint8_t mac_tag[16];
  uint32_t block_id;

  // Encrypted fields only accessible to client.
  BlockType type;
  size_t data_len;
} oram_block_header_t;

// The block for ORAM storage.
typedef struct _oram_block_t {
  oram_block_header_t header;

  uint8_t data[DEFAULT_ORAM_DATA_SIZE];
} oram_block_t;

static const std::string oram_type_mismatch_err =
    "The remote storage cannot match the given ORAM type.";
static const std::string oram_hash_mismatch_err =
    "The remote storage cannot match the given ORAM hash.";
static const std::string oram_size_mismatch_err =
    "The remote storage size cannot match the given ORAM size.";
static const std::string oram_block_id_mismatch_err = "Block id mismatch!";
static const std::string oram_read_err =
    "Cannot read from ORAM! Please check if there is any error.";
static const std::string oram_write_err =
    "Cannot write to ORAM! Please check if there is any error.";

// This factor can also be used to control the size of the Path ORAM to prevent
// storage overflow.
static const float kPartitionAdjustmentFactor = 1.;

static const uint32_t kMaximumOramStorageNum = 1e5;

static const uint32_t kInvalidMask = 0xFFFFFFFF;

// Alias for SQRT ORam.
using sqrt_oram_storage_t = std::vector<oram_block_t>;

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
using server_sqrt_storage_t = std::vector<std::string>;
using server_sqrt_shelter_t = std::vector<std::pair<uint32_t, std::string>>;

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
