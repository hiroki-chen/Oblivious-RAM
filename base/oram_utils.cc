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

#include <algorithm>
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
    ERRS(logger, "Failed to read key file: {}", path);
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
    ERRS(logger, "Failed to read data file: {}", path);
  }
  return data;
}

void SafeFree(void* ptr) {
  if (ptr != nullptr) {
    free(ptr);
  } else {
    ERRS(logger, "Double free detected.");
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
  memcpy(reinterpret_cast<void*>(data->data()),
         reinterpret_cast<const void*>(block), ORAM_BLOCK_SIZE);
}

void CheckStatus(const oram_impl::OramStatus& status,
                 const std::string& reason) {
  if (!status.ok()) {
    ERRS(logger, "{}: {}", status.error_message(), reason);
    abort();
  }
}

void PadStash(oram_impl::p_oram_stash_t* const stash,
              const size_t bucket_size) {
  const size_t stash_size = stash->size();
  if (stash_size < bucket_size) {
    for (size_t i = stash_size; i < bucket_size; ++i) {
      oram_impl::oram_block_t dummy;

      if (!oram_crypto::RandomBytes((uint8_t*)(&dummy), ORAM_BLOCK_SIZE).ok()) {
        ERRS(logger, "Failed to generate random bytes");
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

    if (!oram_crypto::RandomBytes(block.data + 1, DEFAULT_ORAM_DATA_SIZE - 1)
             .ok()) {
      PANIC("Failed to generate random bytes");
    }

    bucket.emplace_back(block);
  }

  // Do a shuffle.
  CheckStatus(oram_crypto::RandomShuffle<oram_impl::oram_block_t>(bucket),
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
  DBG(logger, "Stash:");

  for (size_t i = 0; i < stash.size(); ++i) {
    DBG(logger, "Block {}: type : {}, data: {}", stash[i].header.block_id,
        (int)stash[i].header.type, stash[i].data[0]);
  }
}

void PrintOramTree(const oram_impl::server_tree_storage_t& storage) {
  DBG(logger, "The size of the ORAM tree is {}", storage.size());

  for (auto iter = storage.begin(); iter != storage.end(); ++iter) {
    DBG(logger, "Tag {}, {}: ", iter->first.first, iter->first.second);

    for (const auto& block : iter->second) {
      // Decompress the storage.
      size_t size;
      oram_impl::oram_block_t decompressed_block;
      oram_impl::OramStatus status = DataDecompress(
          reinterpret_cast<const uint8_t*>(block.data()), block.size(),
          reinterpret_cast<uint8_t*>(&decompressed_block), &size);

      if (!status.ok()) {
        ERRS(logger, status.EmitString());
      } else {
        DBG(logger, "id: {}, type: {}", decompressed_block.header.block_id,
            (int)decompressed_block.header.type);
      }
    }
  }
}

oram_impl::OramStatus EncryptBlock(oram_impl::oram_block_t* const block,
                                   oram_crypto::Cryptor* const cryptor) {
  // First let us generate the iv.
  oram_impl::OramStatus status =
      oram_crypto::RandomBytes(block->header.iv, ORAM_CRYPTO_RANDOM_SIZE);
  CheckStatus(status, "Failed to generate iv!");

  // Second prepare the buffer. The buffer includes part of the header.
  std::string enc;
  status =
      cryptor->Encrypt((uint8_t*)(&block->header) + DEFAULT_ORAM_ENCSKIP_SIZE,
                       DEFAULT_ORAM_DATA_SIZE + DEFAULT_ORAM_METADATA_SIZE,
                       block->header.iv, &enc);
  if (!status.ok()) {
    oram_impl::OramStatus ret = oram_impl::OramStatus(
        oram_impl::StatusCode::kInvalidArgument,
        "Encryption failed (check if the data size is correct?)", __func__);
    ret.Append(status);

    return ret;
  }

  // Third, let us split the mac tag to the header.
  memcpy(block->header.mac_tag,
         enc.data() + enc.size() - crypto_aead_aes256gcm_ABYTES,
         crypto_aead_aes256gcm_ABYTES);

  // Fourth, let us copy the encrypted data to the block.
  memcpy((uint8_t*)(&block->header) + DEFAULT_ORAM_ENCSKIP_SIZE, enc.data(),
         enc.size() - crypto_aead_aes256gcm_ABYTES);

  return oram_impl::OramStatus::OK;
}

oram_impl::OramStatus DecryptBlock(oram_impl::oram_block_t* const block,
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
  if (!status.ok()) {
    oram_impl::OramStatus ret = oram_impl::OramStatus(
        oram_impl::StatusCode::kInvalidArgument,
        "Decryption failed due to corrupted ciphertext", __func__);
    ret.Append(status);

    return ret;
  }

  // Fifth, let us copy the decrypted data to the block.
  memcpy((uint8_t*)(&block->header) + DEFAULT_ORAM_ENCSKIP_SIZE, dec.data(),
         dec.size());

  SafeFree(enc_data);

  return oram_impl::OramStatus::OK;
}

oram_impl::OramStatus DataCompress(const uint8_t* data, size_t data_size,
                                   uint8_t* const out,
                                   size_t* const compressed_size) {
  // Compress the source std::string with lz4 compression libarary.
  // The compressed data will be stored in the destination buffer.
  // The destination will be pre-allocated by the correct size.
  const size_t max_allowed_size = LZ4_compressBound(data_size);
  const size_t res = LZ4_compress_default(reinterpret_cast<const char*>(data),
                                          reinterpret_cast<char*>(out),
                                          data_size, max_allowed_size);

  if (res == 0) {
    return oram_impl::OramStatus(oram_impl::StatusCode::kExternalError,
                                 "lz4 returned a zero compressed size",
                                 __func__);
  }

  *compressed_size = res;
  return oram_impl::OramStatus::OK;
}

oram_impl::OramStatus DataDecompress(const uint8_t* data, size_t data_size,
                                     uint8_t* const out,
                                     size_t* const decompressed_size) {
  // Decompress the source std::string with lz4 compression
  // library.
  const size_t max_allowed_size = data_size * 2;
  const size_t res = LZ4_decompress_safe(reinterpret_cast<const char*>(data),
                                         reinterpret_cast<char*>(out),
                                         data_size, max_allowed_size);

  if (res == 0) {
    return oram_impl::OramStatus(
        oram_impl::StatusCode::kExternalError,
        "lz4 returned a zero decompressed size (the input may be corrupted)",
        __func__);
  }

  *decompressed_size = res;
  return oram_impl::OramStatus::OK;
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

oram_impl::OramType StrToType(const std::string& type) {
  if (type == "PathOram") {
    return oram_impl::OramType::kPathOram;
  } else if (type == "SquareRootOram") {
    return oram_impl::OramType::kSquareOram;
  } else if (type == "LinearOram") {
    return oram_impl::OramType::kLinearOram;
  } else if (type == "PartitionOram") {
    return oram_impl::OramType::kPartitionOram;
  } else if (type == "CuckooOram") {
    return oram_impl::OramType::kCuckooOram;
  } else if (type == "ODS") {
    return oram_impl::OramType::kOds;
  } else {
    return oram_impl::OramType::kInvalid;
  }
}
}  // namespace oram_utils
