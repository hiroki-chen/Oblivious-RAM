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
static std::vector<oram_block_t> Padded(const std::vector<oram_block_t>& data,
                                        size_t num) {
  std::vector<oram_block_t> ans = data;

  for (size_t i = data.size(); i != num; i++) {
    // Nonsense data.
    oram_block_t block;
    block.header.type = BlockType::kInvalid;
    block.header.block_id = i;
    ans.emplace_back(block);
  }

  return ans;
}

static std::vector<uint32_t> CreateVec(size_t size) {
  std::vector<uint32_t> ans;

  for (size_t i = 0; i < size; i++) {
    ans.emplace_back(i);
  }

  return ans;
}

void SquareRootOramController::UpdatePosition(
    const std::vector<uint32_t>& perm) {
  for (size_t i = 0; i < perm.size(); i++) {
    if (position_map_.find(i) == position_map_.end()) {
      // Initial state.
      // The element with index i is placed on index perm[i].
      position_map_[i] = perm[i];
    } else {
      // Two levels of permutation.
      position_map_[i] = perm[position_map_[i]];
    }
  }
}

OramStatus SquareRootOramController::InitOram(void) {
  grpc::ClientContext context;
  InitSqrtOramRequest request;
  google::protobuf::Empty empty;

  ASSEMBLE_HEADER(request, id_, instance_hash_, GetVersion());
  request.set_block_size(ORAM_BLOCK_SIZE);
  request.set_capacity(block_num_);
  request.set_squared_m(sqrt_m_);

  grpc::Status status = stub_->InitSqrtOram(&context, request, &empty);
  if (!status.ok()) {
    return OramStatus(StatusCode::kServerError, status.error_message(),
                      __func__);
  }

  return OramStatus::OK;
}

OramStatus SquareRootOramController::FillWithData(
    const std::vector<oram_block_t>& data) {
  // Initially, the first m words of the oblivious RAM contain the contents of
  // the words of the original RAM.
  // Pad the data size to m + \sqrt{m}. This is a must.
  std::vector<oram_block_t> padded_data = Padded(data, block_num_ + sqrt_m_);

  grpc::ClientContext context;
  LoadSqrtOramRequest request;
  google::protobuf::Empty response;

  ASSEMBLE_HEADER(request, id_, instance_hash_, GetVersion());

  // Get the permutation vector.
  std::vector<uint32_t> perm = std::move(CreateVec(padded_data.size()));
  oram_crypto::RandomPermutation(perm);
  UpdatePosition(perm);

  // First we need to make sure that original array is sorted.
  std::sort(padded_data.begin(), padded_data.end(),
            [](const oram_block_t& lhs, const oram_block_t& rhs) {
              return lhs.header.block_id <= rhs.header.block_id;
            });
  // Make sure the data vector is correctly ordered.
  oram_utils::PermuteBy(perm, padded_data);

  // Randomly permute the contents of locations 1 through m + \sqrt{m}. That is,
  // select a permutation π over the integers 1 through m + \sqrt{m} and
  // relocate the contents of word i into word pi(i).
  for (size_t i = 0; i < padded_data.size(); i++) {
    // Load the Square Root ORAM with permutation.
    oram_block_t block = padded_data[i];

    DBG(logger, "Perm: {}, {}; visiting block {}", i, perm[i],
        block.header.block_id);

    std::string block_str;
    oram_utils::EncryptBlock(&block, cryptor_.get());
    oram_utils::ConvertToString(&block, &block_str);

    // Add to request.
    request.add_contents(block_str);
  }

  grpc::Status status = stub_->LoadSqrtOram(&context, request, &response);
  if (!status.ok()) {
    return OramStatus(StatusCode::kServerError, status.error_message(),
                      __func__);
  }

  is_initialized_ = true;

  return OramStatus::OK;
}

OramStatus SquareRootOramController::ReadShelter(oram_block_t* const data) {
  bool found = false;
  // Scan the whole shelter.
  for (size_t i = 0; i < sqrt_m_; i++) {
    grpc::ClientContext context;

    ReadSqrtRequest request;
    SqrtMessage response;

    ASSEMBLE_HEADER(request, id_, instance_hash_, GetVersion());
    request.set_read_from(kShelter);
    request.set_tag(i);

    grpc::Status status = stub_->ReadSqrtMemory(&context, request, &response);
    if (!status.ok()) {
      return OramStatus(StatusCode::kServerError, status.error_message(),
                        __func__);
    }

    // Need to check if the block is empty.
    if (!response.content().empty()) {
      oram_block_t block;
      // Deserialze the received block.
      oram_utils::ConvertToBlock(response.content(), &block);
      // Decrypt the block.
      oram_utils::DecryptBlock(&block, cryptor_.get());
      // Check if the block id matches.
      if (block.header.type == BlockType::kNormal &&
          block.header.block_id == data->header.block_id) {
        memcpy(reinterpret_cast<void*>(data), reinterpret_cast<void*>(&block),
               ORAM_BLOCK_SIZE);
        found = true;
      }
    }
  }

  // Object not found.
  return found ? OramStatus::OK
               : OramStatus(StatusCode::kObjectNotFound,
                            oram_utils::StrCat(
                                "The target block ", data->header.block_id,
                                " is not found in the shelter."));
}

OramStatus SquareRootOramController::ReadBlock(uint32_t from, uint32_t pos,
                                               oram_block_t* const data) {
  // We invoke a separate function to handle `ReadShelter` request.
  if (from == kShelter) {
    return ReadShelter(data);
  }

  grpc::ClientContext context;
  // Prepare the message.
  ReadSqrtRequest request;
  SqrtMessage response;

  ASSEMBLE_HEADER(request, id_, instance_hash_, GetVersion());

  request.set_read_from(from);
  request.set_tag(pos);

  grpc::Status status = stub_->ReadSqrtMemory(&context, request, &response);
  if (!status.ok()) {
    return OramStatus(StatusCode::kServerError, status.error_message(),
                      __func__);
  }

  // Not a dummy read.
  if (from != kDummy) {
    if (!response.content().empty()) {
      // Deserialze the received block.
      oram_utils::ConvertToBlock(response.content(), data);
      // Decrypt the block.
      oram_utils::DecryptBlock(data, cryptor_.get());
    } else {
      // Not found. Should panic?
      return OramStatus(
          StatusCode::kObjectNotFound,
          "The target block cannot be found both in shelter and main memory.");
    }
  }

  return OramStatus::OK;
}

OramStatus SquareRootOramController::WriteBlock(uint32_t position,
                                                oram_block_t* const data) {
  grpc::ClientContext context;

  // Prepare the message.
  WriteSqrtMessage request;
  google::protobuf::Empty response;

  ASSEMBLE_HEADER(request, id_, instance_hash_, GetVersion());

  // Serialize the the block into request type.
  std::string data_str;
  // Encrypt the block.
  oram_utils::EncryptBlock(data, cryptor_.get());
  oram_utils::ConvertToString(data, &data_str);
  request.set_content(data_str);
  request.set_pos(position);

  grpc::Status status = stub_->WriteSqrtMemory(&context, request, &response);
  if (!status.ok()) {
    return OramStatus(StatusCode::kServerError, status.error_message(),
                      __func__);
  }

  return OramStatus::OK;
}

SquareRootOramController::SquareRootOramController(uint32_t id, bool standalone,
                                                   size_t block_num)
    : OramController(id, standalone, block_num, OramType::kSquareOram),
      sqrt_m_((size_t)std::ceil(std::sqrt(block_num))),
      next_dummy_(0ul),
      counter_(0ul) {
  // Check if the input is valid according to the current implementation of FPE.
  PANIC_IF(
      ((sqrt_m_ + block_num) & (sqrt_m_ + block_num - 1)) != 0,
      "Currently, m + \\sqrt{m} should be some power of 2, where m denotes "
      "the block number.");
}

OramStatus SquareRootOramController::PermuteOnFull(void) {
  OramStatus status = OramStatus::OK;
  // Increment the counter and check if we need to permute the oram again.
  // The permutation is done on the server side; we just need to update the
  // position map.
  if (++counter_ == sqrt_m_) {
    DBG(logger, "Performing Permutation!");

    // First let us clear the counter.
    counter_ = 0ul;
    // Also clear next_dummy_.
    next_dummy_ = 0ul;

    // Then we create a random permutation.
    std::vector<uint32_t> perm = std::move(CreateVec(block_num_ + sqrt_m_));
    status = oram_crypto::RandomPermutation(perm);
    if (!status.ok()) {
      return status.Append(OramStatus(
          StatusCode::kInvalidOperation,
          "Sqaure Root Oram Controller cannot permute the memory", __func__));
    } else {
      // Send the whole vector to the server.
      return DoPermute(perm);
    }
  }

  // Nothing happens or the permutation is done correctly.
  return status;
}

OramStatus SquareRootOramController::DoPermute(
    const std::vector<uint32_t>& perm) {
  grpc::ClientContext context;
  google::protobuf::Empty response;

  SqrtPermMessage request;
  ASSEMBLE_HEADER(request, id_, instance_hash_, GetVersion());

  for (size_t i = 0; i < perm.size(); i++) {
    request.add_perms(perm[i]);
  }

  grpc::Status status = stub_->SqrtPermute(&context, request, &response);
  if (!status.ok()) {
    return OramStatus(StatusCode::kServerError, status.error_message(),
                      __func__);
  }

  // Flush position map.
  // This update should be done at the tag level.
  // So the internal logic is a little bit different.
  // address -> [tag -> tag] (We are doing this)
  UpdatePosition(perm);

  return OramStatus::OK;
}

OramStatus SquareRootOramController::InternalAccess(Operation op_type,
                                                    uint32_t address,
                                                    oram_block_t* const data,
                                                    bool dummy) {
  if (!is_initialized_) {
    return OramStatus(StatusCode::kInvalidOperation,
                      "Cannot access ORAM before it is initialized."
                      " You may need to call `InitOram()` and `FillWithData()` "
                      "method first.",
                      __func__);
  }

  PANIC_IF(op_type == Operation::kInvalid, "Invalid ORAM operation!");

  // Step 1: Randomly permute the contents of locations 1 through m + \sqrt{m}.
  // That is, select a permutation π over the integers 1 through m + \sqrt{m}
  // and relocate the contents of word i into word π(i) (later, we show how to
  // do this obliviously).

  // Step 2: Simulate \sqrt{m} memory accesses of the original RAM.
  // - If i is found in the shelter,fthen read next dummy.
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
                      "The requested block does not exist!", __func__);
  }

  const uint32_t position = position_map_[address];

  DBG(logger, "The position for {} is {}", address, position);

  oram_block_t block;
  block.header.block_id = address;
  block.header.type = BlockType::kNormal;

  // Read from the server.
  // Step 1: Scan the cache (shelter) of the server.
  bool in_shelter = false;
  OramStatus status = ReadBlock(kShelter, position, &block);
  if (!status.ok()) {
    // Check if the error is NOT FOUND; if not, we return the error.
    if (StatusCode::kObjectNotFound != status.error_code()) {
      return status.Append(OramStatus(
          StatusCode::kInvalidOperation,
          "Square Root Oram Controller cannot read the block from the shelter",
          __func__));
    }

  } else {
    in_shelter = true;
  }

  // Step 2: If not found in the cache, read from main; else read next dummy.
  if (in_shelter) {
    // Can be permuted, but for convenience, we just "literally" read the next
    // dummy. If there is any error, we return it.
    status = ReadBlock(kDummy, next_dummy_++, nullptr);
    if (!status.ok()) {
      return status.Append(
          OramStatus(StatusCode::kInvalidOperation,
                     "Square Root Oram Controller cannot read the "
                     "block from the dummy area",
                     __func__));
    }

  } else {
    // If a block is not found in the shelter, then it MUST be in the main
    // memory even if randomly permuted. So we use the permuted posiiton to
    // index it.
    status = ReadBlock(kMainMemory, position, &block);
    if (!status.ok()) {
      return status.Append(
          OramStatus(StatusCode::kInvalidOperation,
                     "Square Root Oram Controller cannot read the "
                     "block from the main memory",
                     __func__));
    }
  }

  // Check if the id matches.
  if (block.header.type == BlockType::kNormal &&
      block.header.block_id != address) {
    return OramStatus(
        StatusCode::kUnknownError,
        oram_utils::StrCat("The requested id ", address,
                           " does not match with the fetched block ",
                           block.header.block_id),
        __func__);
  }

  // Update the block, if it needs.
  if (op_type == Operation::kRead) {
    // Copy the block to the user side.
    memcpy(reinterpret_cast<void*>(data), reinterpret_cast<void*>(&block),
           ORAM_BLOCK_SIZE);
    // Immediately write it back.
    status = WriteBlock(position, data);
  } else {
    // Simply Call write.
    status = WriteBlock(position, data);
    // Propagate the error.
    if (!status.ok()) {
      return status.Append(OramStatus(StatusCode::kInvalidOperation,
                                      "Cannot write back the block", __func__));
    }
  }

  // Check if we need to permute the oram storage.
  if (!(status = PermuteOnFull()).ok()) {
    return status.Append(
        OramStatus(StatusCode::kUnknownError,
                   "Permutation failed or internal logic error!", __func__));
  }

  return OramStatus::OK;
}
}  // namespace oram_impl