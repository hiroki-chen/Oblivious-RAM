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
#include "oram_storage.h"

#include <cmath>

#include <spdlog/logger.h>

#include "base/oram_utils.h"

extern std::shared_ptr<spdlog::logger> logger;

namespace partition_oram {
OramServerStorage::OramServerStorage(uint32_t id, size_t capacity,
                                     size_t bucket_size)
    : id_(id), capacity_(capacity), bucket_size_(bucket_size) {
  level_ = std::ceil(LOG_BASE(capacity_ + 1, 2)) - 1;

  logger->debug("level = {}, capacity = {}", level_, capacity);

  for (uint32_t i = 0; i <= level_; i++) {
    // Initialize dummy blocks into the storage.
    const uint32_t cur_size = POW2(i);

    for (uint32_t j = 0; j < cur_size; j++) {
      const server_storage_tag_t tag = std::make_pair(i, j);
      server_storage_data data;
      storage_[tag] = data;
    }
  }
}

Status OramServerStorage::ReadPath(uint32_t level, uint32_t path,
                                   p_oram_bucket_t* const out_bucket) {
  // The offset should be calculated by the level and path.
  const uint32_t offset = std::floor(path * 1. / POW2(level_ - level));
  const server_storage_tag_t tag = std::make_pair(level, offset);
  logger->info("Read offset {} at level {} for path {}.", offset, level, path);

  auto iter = storage_.find(tag);
  if (iter == storage_.end()) {
    logger->error("OramServerStorage::ReadPath: Cannot find the bucket.");
    // Not found.
    return Status::kObjectNotFound;
  } else {
    std::for_each(iter->second.begin(), iter->second.end(),
                  [&out_bucket](const std::string& data) {
                    // Decompress the data.
                    oram_block_t block;
                    oram_utils::DataDecompress(
                        reinterpret_cast<const uint8_t*>(data.data()),
                        data.size(), reinterpret_cast<uint8_t*>(&block));

                    out_bucket->emplace_back(block);
                  });

    return Status::kOK;
  }
}

Status OramServerStorage::WritePath(uint32_t level, uint32_t path,
                                    const p_oram_bucket_t& in_bucket) {
  const uint32_t offset = std::floor(path * 1. / POW2(level_ - level));

  return AccurateWritePath(level, offset, in_bucket,
                           partition_oram::Type::kNormal);
}

Status OramServerStorage::AccurateWritePath(uint32_t level, uint32_t offset,
                                            const p_oram_bucket_t& in_bucket,
                                            partition_oram::Type type) {
  logger->info("Write offset {} at level {}. ", offset, level);
  const server_storage_tag_t tag = std::make_pair(level, offset);

  auto iter = storage_.find(tag);
  uint8_t buf[DEFAULT_COMPRESSED_BUF_SIZE];

  if (type == partition_oram::Type::kInit) {
    for (size_t i = 0; i < in_bucket.size(); i++) {
      const size_t compressed_size = oram_utils::DataCompress(
          (uint8_t*)(&in_bucket[i]), ORAM_BLOCK_SIZE, buf);

      iter->second.emplace_back(
          std::string(reinterpret_cast<const char*>(buf), compressed_size));
    }

    return Status::kOK;
  }

  if (iter == storage_.end() && type == partition_oram::Type::kNormal) {
    logger->error("OramServerStorage::WritePath: Cannot find the bucket.");
    // Not found.
    return Status::kObjectNotFound;
  } else {
    return Status::kOK;
    iter->second.clear();

    for (size_t i = 0; i < in_bucket.size(); i++) {
      const size_t compressed_size = oram_utils::DataCompress(
          (uint8_t*)(&in_bucket[i]), ORAM_BLOCK_SIZE, buf);

      iter->second.emplace_back(
          std::string(reinterpret_cast<const char*>(buf), compressed_size));
    }

    return Status::kOK;
  }
}

float OramServerStorage::ReportStorage(void) const {
  // Calculate the overall size of the storage in Megabytes.
  uint64_t storage_size = 0;
  for (const auto& iter : storage_) {
    storage_size += iter.second.size() * sizeof(oram_block_t);
  }

  return storage_size * 1. / POW2(20);
}
}  // namespace partition_oram