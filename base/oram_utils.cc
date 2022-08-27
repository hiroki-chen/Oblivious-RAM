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
#include "oram_utils.h"

#include <lz4.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>

#include <cstdarg>
#include <cstring>
#include <fstream>
#include <sstream>

#include "oram_crypto.h"

extern std::shared_ptr<spdlog::logger> logger;

namespace oram_utils {
std::string ReadKeyCrtFile(const std::string& path) {
  std::ifstream file(path, std::ifstream::in);
  std::ostringstream oss;

  if (file.good()) {
    oss << file.rdbuf();
    file.close();
  } else {
    logger->error("Failed to read key file: {}", path);
    return "";
  }

  return oss.str();
}

std::vector<std::string> ReadDataFromFile(const std::string& path) {
  std::ifstream file(path, std::ifstream::in);
  std::vector<std::string> data;

  if (file.good()) {
    std::string line;
    while (std::getline(file, line)) {
      data.emplace_back(line);
    }
    file.close();
  } else {
    logger->error("Failed to read data file: {}", path);
  }
  return data;
}

void SafeFree(void* ptr) {
  if (ptr != nullptr) {
    free(ptr);
  } else {
    logger->error("Failed to free nullptr");
  }
}

void SafeFreeAll(size_t ptr_num, ...) {
  va_list ap;
  va_start(ap, ptr_num);
  for (size_t i = 0; i < ptr_num; ++i) {
    void* ptr = va_arg(ap, void*);
    SafeFree(ptr);
  }
  va_end(ap);
}

void ConvertToBlock(const std::string& data,
                    oram_impl::oram_block_t* const block) {
  PANIC_IF(data.size() != ORAM_BLOCK_SIZE, "Invalid data size");

  memcpy(block, data.data(), ORAM_BLOCK_SIZE);
}

void ConvertToString(const oram_impl::oram_block_t* const block,
                     std::string* const data) {
  data->resize(ORAM_BLOCK_SIZE);
  memcpy(data->data(), (void*)block, ORAM_BLOCK_SIZE);
}

void CheckStatus(oram_impl::OramStatus status, const std::string& reason) {
  if (status != oram_impl::OramStatus::kOK) {
    logger->error("{}: {}", oram_impl::kErrorList.at(status), reason);
    abort();
  }
}

void PadStash(oram_impl::p_oram_stash_t* const stash,
              const size_t bucket_size) {
  const size_t stash_size = stash->size();
  if (stash_size < bucket_size) {
    for (size_t i = stash_size; i < bucket_size; ++i) {
      oram_impl::oram_block_t dummy;

      if (oram_crypto::Cryptor::RandomBytes((uint8_t*)(&dummy),
                                            ORAM_BLOCK_SIZE) !=
          oram_impl::OramStatus::kOK) {
        logger->error("Failed to generate random bytes");
        abort();
      }

      stash->emplace_back(dummy);
    }
  }
}

oram_impl::p_oram_bucket_t SampleRandomBucket(size_t size, size_t tree_size,
                                              size_t initial_offset) {
  size >>= 1;
  oram_impl::p_oram_bucket_t bucket;

  for (size_t i = 0; i < tree_size; ++i) {
    oram_impl::oram_block_t block;
    block.header.block_id = i + initial_offset;
    block.header.type =
        i < size ? oram_impl::BlockType::kNormal : oram_impl::BlockType::kDummy;
    block.data[0] = i + initial_offset;

    if (oram_crypto::Cryptor::RandomBytes(block.data + 1,
                                          DEFAULT_ORAM_DATA_SIZE - 1) !=
        oram_impl::OramStatus::kOK) {
      logger->error("Failed to generate random bytes");
      abort();
    }

    bucket.emplace_back(block);
  }

  // Do a shuffle.
  CheckStatus(
      oram_crypto::Cryptor::RandomShuffle<oram_impl::oram_block_t>(bucket),
      "Random shuffle failed due to internal error.");

  return bucket;
}

std::vector<std::string> SerializeToStringVector(
    const oram_impl::p_oram_bucket_t& bucket) {
  std::vector<std::string> ans;

  for (size_t i = 0; i < bucket.size(); ++i) {
    std::string data;
    ConvertToString(&bucket[i], &data);
    ans.emplace_back(data);
  }

  return ans;
}

oram_impl::p_oram_bucket_t DeserializeFromStringVector(
    const std::vector<std::string>& data) {
  oram_impl::p_oram_bucket_t ans;

  for (size_t i = 0; i < data.size(); ++i) {
    oram_impl::oram_block_t block;
    ConvertToBlock(data[i], &block);
    ans.emplace_back(block);
  }

  return ans;
}

void PrintStash(const oram_impl::p_oram_stash_t& stash) {
  logger->debug("Stash:");

  for (size_t i = 0; i < stash.size(); ++i) {
    logger->debug("Block {}: type : {}, data: {}", stash[i].header.block_id,
                  (int)stash[i].header.type, stash[i].data[0]);
  }
}

void PrintOramTree(const oram_impl::server_tree_storage_t& storage) {
  logger->debug("The size of the ORAM tree is {}", storage.size());

  for (auto iter = storage.begin(); iter != storage.end(); ++iter) {
    logger->debug("Tag {}, {}: ", iter->first.first, iter->first.second);

    for (const auto& block : iter->second) {
      // Decompress the storage.
      oram_impl::oram_block_t decompressed_block;
      DataDecompress(reinterpret_cast<const uint8_t*>(block.data()),
                     block.size(),
                     reinterpret_cast<uint8_t*>(&decompressed_block));

      logger->debug("id: {}, type: {}", decompressed_block.header.block_id,
                    (int)decompressed_block.header.type);
    }
  }
}

void EncryptBlock(oram_impl::oram_block_t* const block,
                  oram_crypto::Cryptor* const cryptor) {
  // First let us generate the iv.
  oram_impl::OramStatus status =
      cryptor->RandomBytes(block->header.iv, ORAM_CRYPTO_RANDOM_SIZE);
  CheckStatus(status, "Failed to generate iv!");

  // Second prepare the buffer. The buffer includes part of the header.
  std::string enc;
  status =
      cryptor->Encrypt((uint8_t*)(&block->header) + DEFAULT_ORAM_ENCSKIP_SIZE,
                       DEFAULT_ORAM_DATA_SIZE + DEFAULT_ORAM_METADATA_SIZE,
                       block->header.iv, &enc);
  CheckStatus(status, "Failed to encrypt data!");

  // Third, let us split the mac tag to the header.
  memcpy(block->header.mac_tag,
         enc.data() + enc.size() - crypto_aead_aes256gcm_ABYTES,
         crypto_aead_aes256gcm_ABYTES);

  // Fourth, let us copy the encrypted data to the block.
  memcpy((uint8_t*)(&block->header) + DEFAULT_ORAM_ENCSKIP_SIZE, enc.data(),
         enc.size() - crypto_aead_aes256gcm_ABYTES);
}

void DecryptBlock(oram_impl::oram_block_t* const block,
                  oram_crypto::Cryptor* const cryptor) {
  // First, let us prepare the buffer.
  uint8_t* enc_data =
      (uint8_t*)malloc(DEFAULT_ORAM_DATA_SIZE + DEFAULT_ORAM_METADATA_SIZE +
                       crypto_aead_aes256gcm_ABYTES);

  // Second, let us copy the encrypted data to the buffer.
  memcpy(enc_data, (uint8_t*)(&block->header) + DEFAULT_ORAM_ENCSKIP_SIZE,
         DEFAULT_ORAM_DATA_SIZE + DEFAULT_ORAM_METADATA_SIZE);

  // Third, let us copy the mac tag to the buffer.
  memcpy(enc_data + DEFAULT_ORAM_DATA_SIZE + DEFAULT_ORAM_METADATA_SIZE,
         block->header.mac_tag, crypto_aead_aes256gcm_ABYTES);

  // Fourth, let us decrypt the data.
  std::string dec;
  oram_impl::OramStatus status =
      cryptor->Decrypt(enc_data,
                       DEFAULT_ORAM_DATA_SIZE + DEFAULT_ORAM_METADATA_SIZE +
                           crypto_aead_aes256gcm_ABYTES,
                       block->header.iv, &dec);
  CheckStatus(status, "Failed to decrypt data!");

  // Fifth, let us copy the decrypted data to the block.
  memcpy((uint8_t*)(&block->header) + DEFAULT_ORAM_ENCSKIP_SIZE, dec.data(),
         dec.size());

  SafeFree(enc_data);
}

size_t DataCompress(const uint8_t* data, size_t data_size, uint8_t* const out) {
  // Compress the source std::string with lz4 compression libarary.
  // The compressed data will be stored in the destination buffer.
  // The destination will be pre-allocated by the correct size.
  const size_t max_allowed_size = LZ4_compressBound(data_size);
  size_t compressed_size = LZ4_compress_default(
      reinterpret_cast<const char*>(data), reinterpret_cast<char*>(out),
      data_size, max_allowed_size);

  if (compressed_size == 0) {
    logger->error("Failed to compress data!");
    abort();
  }

  return compressed_size;
}

size_t DataDecompress(const uint8_t* data, size_t data_size,
                      uint8_t* const out) {
  // Decompress the source std::string with lz4 compression
  // library.
  const size_t max_allowed_size = data_size * 2;
  size_t decompressed_size = LZ4_decompress_safe(
      reinterpret_cast<const char*>(data), reinterpret_cast<char*>(out),
      data_size, max_allowed_size);

  if (decompressed_size == 0) {
    logger->error("Failed to decompress data!");
    abort();
  }

  return decompressed_size;
}

std::string TypeToName(oram_impl::OramType oram_type) {
  switch (oram_type) {
    case oram_impl::OramType::kLinearOram:
      return "LinearOram";
    case oram_impl::OramType::kSquareOram:
      return "SquareOram";
    case oram_impl::OramType::kPathOram:
      return "PathOram";
    case oram_impl::OramType::kPartitionOram:
      return "PartitionOram";
    case oram_impl::OramType::kCuckooOram:
      return "CuckooOram";

    default:
      return "InvalidOram";
  }
}

std::vector<std::string> split(const std::string& str, char delim) {
  std::vector<std::string> result;
  std::stringstream ss(str);
  std::string item;

  while (getline(ss, item, delim)) {
    result.emplace_back(item);
  }

  return result;
}

// A very simple function that converts all the fields of node into string and
// concatenate them by underscore.
std::string TreeNodeSerialize(oram_impl::ods::TreeNode* const node) {
  std::string ans;

  ans.append(std::to_string(node->id_) + "_");
  ans.append(std::to_string(node->pos_tag_) + "_");
  ans.append(std::to_string(node->old_tag_) + "_");
  ans.append(std::to_string(node->kv_pair_.first) + "_");
  ans.append(node->kv_pair_.second + "_");

  for (size_t i = 0; i < 2; i++) {
    ans.append(std::to_string(node->children_pos_[i].id_) + "_");
    ans.append(std::to_string(node->children_pos_[i].pos_tag_) + "_");
  }

  ans.append(std::to_string(node->left_id_) + "_");
  ans.append(std::to_string(node->right_id_) + "_");
  ans.append(std::to_string(node->left_height_) + "_");
  ans.append(std::to_string(node->right_height_));

  return ans;
}

void TreeNodeDeserialize(const std::string& str,
                         oram_impl::ods::TreeNode* const out_node) {
  const std::vector<std::string> data = split(str, '_');

  out_node->id_ = std::stoul(data[0]);
  out_node->pos_tag_ = std::stoul(data[1]);
  out_node->old_tag_ = std::stoul(data[2]);
  out_node->kv_pair_.first = std::stoul(data[3]);
  out_node->kv_pair_.second = (data[4]);
  out_node->children_pos_[0].id_ = std::stoul(data[5]);
  out_node->children_pos_[0].pos_tag_ = std::stoul(data[6]);
  out_node->children_pos_[1].id_ = std::stoul(data[7]);
  out_node->children_pos_[1].pos_tag_ = std::stoul(data[8]);
  out_node->left_id_ = std::stoul(data[9]);
  out_node->right_id_ = std::stoul(data[10]);
  out_node->left_height_ = std::stoul(data[11]);
  out_node->right_height_ = std::stoul(data[12]);
}
}  // namespace oram_utils
