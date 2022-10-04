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
#include "square_root_oram_controller.h"

#include <spdlog/spdlog.h>

#include "oram.h"

extern std::shared_ptr<spdlog::logger> logger;

namespace oram_impl {

static std::vector<uint32_t> CreateVec(size_t size) {
  std::vector<uint32_t> ans(size, 0);

  for (size_t i = 0; i < size; i++) {
    ans[i] = i;
  }

  return ans;
}

OramStatus SquareRootOramController::ReadBlockFromShelter(
    oram_block_t* const data) {
  // TODO.
  return OramStatus::OK;
}

OramStatus SquareRootOramController::ReadBlockFromDummy(void) {
  return OramStatus::OK;
}

OramStatus SquareRootOramController::ReadBlockFromMain(uint32_t pos,
                                                       oram_block_t* data) {
  return OramStatus::OK;
}

OramStatus SquareRootOramController::WriteBlock(uint32_t position,
                                                oram_block_t* const data,
                                                bool write_to_cache) {
  grpc::ClientContext context;

  // Prepare the message.
  WriteSqrtMessage request;
  google::protobuf::Empty response;

  request.mutable_header()->set_id(id_);
  request.mutable_header()->set_instance_hash(
      std::string(reinterpret_cast<char*>(instance_hash_), 32));
  request.mutable_header()->set_version(GetVersion());

  // Serialize the the block into request type.
  std::string data_str;
  oram_utils::ConvertToString(data, &data_str);
  request.set_content(data_str);
  request.set_pos(position);
  request.set_write_to_cache(write_to_cache);

  grpc::Status status = stub_->WriteSqrtMemory(&context, request, &response);
  if (!status.ok()) {
    return OramStatus(StatusCode::kServerError, status.error_message());
  }

  return OramStatus::OK;
}

SquareRootOramController::SquareRootOramController(uint32_t id, bool standalone,
                                                   size_t block_num)
    : OramController(id, standalone, block_num, OramType::kSquareOram),
      sqrt_m_((size_t)std::ceil(std::sqrt(block_num))),
      next_dummy_(0ul),
      counter_(0ul) {
  // TODO: Should check m + \sqrt{m} = log_2(x), for some x.
}

OramStatus SquareRootOramController::PermuteOnFull(void) {
  OramStatus status = OramStatus::OK;
  // Increment the counter and check if we need to permute the oram again.
  // The permutation is done on the server side; we just need to update the
  // position map.
  if (counter_++ == sqrt_m_) {
    // First let us clear the counter.
    counter_ = 0;

    // Then we create a random permutation.
    std::vector<uint32_t> perm = std::move(CreateVec(block_num_ + sqrt_m_));
    status = oram_crypto::RandomPermutation(perm);
    if (!status.ok()) {
      return OramStatus(status.ErrorCode(),
                        oram_utils::StrCat("Permutation failed due to ",
                                           status.ErrorMessage()));
    } else {
      // Send the whole vector to the server.
      return DoPermute(std::move(oram_utils::PermToStr(perm)));
    }
  }

  // Nothing happens or the permutation is done correctly.
  return status;
}

OramStatus SquareRootOramController::DoPermute(const std::string& content) {
  grpc::ClientContext context;
  google::protobuf::Empty response;

  SqrtPermMessage request;
  request.mutable_header()->set_id(id_);
  request.mutable_header()->set_instance_hash(
      std::string(reinterpret_cast<char*>(instance_hash_), 32));
  request.mutable_header()->set_version(GetVersion());
  request.set_content(content);

  grpc::Status status = stub_->SqrtPermute(&context, request, &response);
  if (!status.ok()) {
    return OramStatus(StatusCode::kServerError, status.error_message());
  }

  return OramStatus::OK;
}

OramStatus SquareRootOramController::InternalAccess(Operation op_type,
                                                    uint32_t address,
                                                    oram_block_t* const data,
                                                    bool dummy) {
  PANIC_IF(op_type == Operation::kInvalid, "Invalid ORAM operation!");

  // Step 1: Randomly permute the contents of locations 1 through m + \sqrt{m}.
  // That is, select a permutation π over the integers 1 through m + \sqrt{m}
  // and relocate the contents of word i into word π(i) (later, we show how to
  // do this obliviously).

  // Step 2: Simulate \sqrt{m} memory accesses of the original RAM.
  // - If i is found in the shelter, then read next dummy.
  // - If is not found in the shelter, then read main memory using its permuted
  //   tag \pi(i) which is then stored in the shelter temporarily.

  // Step 3: After \sqrt{m} operations, re-shuffle the memory content and write
  // back.
  //
  // Note that in the original Ostrovsky's algorithm, the permutation is
  // realized by AKS sorting network:
  //  1. Give each memory location in [0, m + \sqrt{m}] a tag by a random
  //     oracle.
  //  2. Use sorting network to sort all the memory blocks by their tags.
  //
  // Because sorting network behaves identically with different values, the
  // algorithm is oblivious.
  //
  // Workflow:
  //    Server --> [ Client (Read & Update) -->
  //               Server stores the block in the shelter ] --> (Permute)
  // A full read-write operation is regarded as an atomic operation here.

  // Check the position map.
  if (position_map_.find(address) == position_map_.end()) {
    return OramStatus(StatusCode::kInvalidArgument,
                      "The requested block does not exist!");
  }

  const uint32_t position = position_map_[address];

  oram_block_t block;
  block.header.block_id = address;
  block.header.type = BlockType::kNormal;

  // Read from the server.
  // Step 1: Scan the cache (shelter) of the server.
  bool in_shelter = false;
  OramStatus status = ReadBlockFromShelter(&block);
  if (!status.ok()) {
    // Check if the error is NOT FOUND; if not, we return the error.
    if (StatusCode::kObjectNotFound != status.ErrorCode()) {
      return status;
    }

  } else {
    in_shelter = true;
  }

  // Step 2: If not found in the cache, read from main; else read next dummy.
  if (in_shelter) {
    // Can be permuted, but for convenience, we just "literally" read the next
    // dummy. If there is any error, we return it.
    status = ReadBlockFromDummy();
    if (!status.ok()) {
      return status;
    }

  } else {
    // If a block is not found in the shelter, then it MUST be in the main
    // memory even if randomly permuted. So we use the permuted posiiton to
    // index it.
    status = ReadBlockFromMain(position, &block);
    if (!status.ok()) {
      return status;
    }
  }

  // Check if the id matches.
  if (block.header.block_id != address) {
    return OramStatus(
        StatusCode::kUnknownError,
        oram_utils::StrCat("[InternalAccess] The requested id ", address,
                           " does not match with the fetched block ",
                           block.header.block_id, "!"));
  }

  // Update the block, if it needs.
  if (op_type == Operation::kRead) {
    // Copy the block to the user side.
    memcpy(reinterpret_cast<void*>(data), reinterpret_cast<void*>(&block),
           ORAM_BLOCK_SIZE);
  } else {
    // Call write.
    status = WriteBlock(position, data, in_shelter);
    // Propagate the error.
    if (!status.ok()) {
      return OramStatus(
          status.ErrorCode(),
          oram_utils::StrCat("[InternalAccess] Write back failed on ", address,
                             ". Error: ", status.ErrorMessage()));
    }
  }

  // Check if we need to permute the oram storage.
  if (!(status = PermuteOnFull()).ok()) {
    return OramStatus(
        status.ErrorCode(),
        oram_utils::StrCat("[InternalAccess] ", status.ErrorMessage()));
  }

  return OramStatus::OK;
}
}  // namespace oram_impl