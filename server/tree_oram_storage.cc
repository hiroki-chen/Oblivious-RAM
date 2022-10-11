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
#include "tree_oram_storage.h"

#include <spdlog/logger.h>

#include "base/oram_utils.h"

extern std::shared_ptr<spdlog::logger> logger;

namespace oram_impl {
TreeOramServerStorage::TreeOramServerStorage(uint32_t id, size_t capacity,
                                             size_t block_size,
                                             size_t bucket_size,
                                             const std::string& instance_hash)
    : BaseOramServerStorage(id, capacity, block_size, instance_hash,
                            OramStorageType::kTreeStorage),
      bucket_size_(bucket_size) {
  level_ = std::ceil(LOG_BASE(capacity_ + 1, 2)) - 1;

  DBG(logger, "level = {}, capacity = {}", level_, capacity);

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

OramStatus TreeOramServerStorage::ReadPath(uint32_t level, uint32_t path,
                                           p_oram_bucket_t* const out_bucket) {
  // The offset should be calculated by the level and path.
  const uint32_t offset = std::floor(path * 1. / POW2(level_ - level));
  const server_storage_tag_t tag = std::make_pair(level, offset);
  INFO(logger, "Read offset {} at level {} for path {}.", offset, level, path);

  auto iter = storage_.find(tag);
  if (iter == storage_.end()) {
    // Not found.
    return OramStatus(StatusCode::kObjectNotFound, "Cannot find the bucket.",
                      __func__);
  } else {
    for (auto& data : iter->second) {
      if (!data.empty()) {
        // Decompress the data.
        oram_block_t block;
        size_t size;

        OramStatus status = oram_utils::DataDecompress(
            reinterpret_cast<const uint8_t*>(data.data()), data.size(),
            reinterpret_cast<uint8_t*>(&block), &size);
        if (!status.ok()) {
          OramStatus ret = OramStatus(StatusCode::kInvalidOperation,
                                      "Cannot Read from the server", __func__);
          ret.Append(status);

          return ret;
        }

        out_bucket->emplace_back(block);

        // Clear the data.
        data.clear();
      }
    }

    return OramStatus::OK;
  }
}

OramStatus TreeOramServerStorage::WritePath(uint32_t level, uint32_t path,
                                            const p_oram_bucket_t& in_bucket) {
  const uint32_t offset = std::floor(path * 1. / POW2(level_ - level));

  return AccurateWritePath(level, offset, in_bucket, oram_impl::Type::kNormal);
}

OramStatus TreeOramServerStorage::AccurateWritePath(
    uint32_t level, uint32_t offset, const p_oram_bucket_t& in_bucket,
    oram_impl::Type type) {
  INFO(logger, "Write offset {} at level {}. ", offset, level);
  const server_storage_tag_t tag = std::make_pair(level, offset);

  auto iter = storage_.find(tag);
  uint8_t buf[DEFAULT_COMPRESSED_BUF_SIZE];

  if (type == oram_impl::Type::kInit) {
    for (size_t i = 0; i < in_bucket.size(); i++) {
      size_t compressed_size;
      OramStatus status = oram_utils::DataCompress(
          (uint8_t*)(&in_bucket[i]), ORAM_BLOCK_SIZE, buf, &compressed_size);

      if (!status.ok()) {
        OramStatus ret = OramStatus(StatusCode::kInvalidOperation,
                                    "Cannot write to the server", __func__);
        ret.Append(status);

        return ret;
      }

      iter->second.emplace_back(
          std::string(reinterpret_cast<const char*>(buf), compressed_size));
    }

    return OramStatus::OK;
  }

  if (iter == storage_.end() && type == oram_impl::Type::kNormal) {
    // Not found.
    return OramStatus(StatusCode::kObjectNotFound, "Cannot find the bucket.",
                      __func__);
  } else {
    iter->second.clear();

    for (size_t i = 0; i < in_bucket.size(); i++) {
      size_t compressed_size;
      OramStatus status = oram_utils::DataCompress(
          (uint8_t*)(&in_bucket[i]), ORAM_BLOCK_SIZE, buf, &compressed_size);
      if (!status.ok()) {
        OramStatus ret = OramStatus(StatusCode::kInvalidOperation,
                                    "Cannot write to the server", __func__);
        ret.Append(status);

        return ret;
      }

      iter->second.emplace_back(
          std::string(reinterpret_cast<const char*>(buf), compressed_size));
    }

    return OramStatus::OK;
  }
}

float TreeOramServerStorage::ReportStorage(void) const {
  // Calculate the overall size of the storage in Megabytes.
  uint64_t storage_size = 0;
  for (const auto& iter : storage_) {
    storage_size += iter.second.size() * sizeof(oram_block_t);
  }

  return storage_size * 1. / POW2(20);
}
}  // namespace oram_impl