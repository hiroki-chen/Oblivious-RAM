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
#include <cmath>

#include "odict_controller.h"
#include "base/oram_utils.h"

#include <spdlog/logger.h>

using namespace oram_impl;
using namespace ods;

extern std::shared_ptr<spdlog::logger> logger;

static inline uint32_t ComputePadVal(uint32_t x) {
  return 3 * 1.44 * std::log(x);
}

TreeNode* OdictController::Balance(uint32_t root_id) {
  TreeNode* const root = new TreeNode(root_id);
  oram_utils::CheckStatus(OdsAccess(OdsOperation::kRead, root),
                          "OdsAccess R failed");

  const uint32_t left_height = root->left_height_;
  const uint32_t right_height = root->right_height_;

  if (left_height - right_height > 1) {
    TreeNode* const left = new TreeNode(root->left_id_);
    oram_utils::CheckStatus(OdsAccess(OdsOperation::kRead, left),
                            "OdsAccess R failed");

    delete root;
    // Case 1. LL. Single right rotation.
    if (left->left_height_ >= left->right_height_) {
      delete left;
      return RightRotate(root_id);
    } else {
      delete left;
      // Case 2. LR. double rotation.
      return LeftRightRotate(root_id);
    }
  } else if (right_height - left_height > 1) {
    TreeNode* const right = new TreeNode(root->right_id_);
    oram_utils::CheckStatus(OdsAccess(OdsOperation::kRead, right),
                            "OdsAccess R failed");

    delete root;
    // Case 3. RR. Single left rotation.
    if (right->right_height_ >= right->left_height_) {
      delete right;
      return LeftRotate(root_id);
    } else {
      // Case 4. RL.
      delete right;
      return RightLeftRotate(root_id);
    }
  }

  return root;
}

TreeNode* OdictController::LeftRotate(uint32_t root_id) {
  logger->debug("Doing left rotation.");

  // Read the root node and the right node.
  TreeNode* const root = new TreeNode(root_id);
  oram_utils::CheckStatus(OdsAccess(OdsOperation::kRead, root),
                          "OdsAccess R failed");
  TreeNode* const right = new TreeNode(root->right_id_);
  oram_utils::CheckStatus(OdsAccess(OdsOperation::kRead, right),
                          "OdsAccess R failed");

  // Adjust pointers.
  root->right_id_ = right->left_id_;
  root->children_pos_[1].pos_tag_ = right->children_pos_[0].pos_tag_;
  root->right_height_ = right->left_height_;
  oram_utils::CheckStatus(OdsAccess(OdsOperation::kWrite, root),
                          "OdsAccess W failed");

  right->left_height_ = HEIGHT(root);
  right->left_id_ = root_id;
  right->children_pos_[0].pos_tag_ = root->pos_tag_;
  oram_utils::CheckStatus(OdsAccess(OdsOperation::kWrite, right),
                          "OdsAccess failed");

  delete root;
  return right;
}

TreeNode* OdictController::RightRotate(uint32_t root_id) {
  logger->debug("Doing right rotation.");

  // Read the root node and the right node.
  TreeNode* const root = new TreeNode(root_id);
  oram_utils::CheckStatus(OdsAccess(OdsOperation::kRead, root),
                          "OdsAccess R failed");
  TreeNode* const left = new TreeNode(root->left_id_);
  oram_utils::CheckStatus(OdsAccess(OdsOperation::kRead, left),
                          "OdsAccess R failed");

  // Adjust pointers.
  root->left_id_ = left->right_id_;
  root->children_pos_[0].pos_tag_ = left->children_pos_[1].pos_tag_;
  root->left_height_ = left->right_height_;
  oram_utils::CheckStatus(OdsAccess(OdsOperation::kWrite, root),
                          "OdsAccess W failed");

  left->right_height_ = HEIGHT(root);
  left->right_id_ = root_id;
  left->children_pos_[1].pos_tag_ = root->pos_tag_;
  oram_utils::CheckStatus(OdsAccess(OdsOperation::kWrite, left),
                          "OdsAccess failed");

  delete root;
  return left;
}

TreeNode* OdictController::LeftRightRotate(uint32_t root_id) {
  logger->debug("Doing left right rotation.");

  TreeNode* const root = new TreeNode(root_id);
  oram_utils::CheckStatus(OdsAccess(OdsOperation::kRead, root),
                          "OdsAccess R failed");
  TreeNode* const left = new TreeNode(root->left_id_);

  root->left_id_ = left->id_;
  root->left_height_ = HEIGHT(left);
  oram_utils::CheckStatus(OdsAccess(OdsOperation::kWrite, root),
                          "OdsAccess W failed");

  delete root;
  delete left;
  return RightRotate(root_id);
}

TreeNode* OdictController::RightLeftRotate(uint32_t root_id) {
  logger->debug("Doing right left rotation.");

  TreeNode* const root = new TreeNode(root_id);
  oram_utils::CheckStatus(OdsAccess(OdsOperation::kRead, root),
                          "OdsAccess R failed");
  TreeNode* const right = new TreeNode(root->right_id_);

  root->right_id_ = right->id_;
  root->right_height_ = HEIGHT(right);
  oram_utils::CheckStatus(OdsAccess(OdsOperation::kWrite, root),
                          "OdsAccess W failed");

  delete root;
  delete right;
  return RightRotate(root_id);
}

OramStatus OdictController::CacheHelper(uint32_t id, TreeNode* const node) {
  // ODS does not need the position map to access the oblivious RAM because its
  // internal structure already fulfills this.
  uint32_t pos = (id == root_id_ ? root_pos_ : ods_cache_->FindPosById(id));

  oram_block_t block;
  block.header.block_id = id;
  OramStatus status = OramStatus::kOK;
  status = oram_contrller_->AccessDirect(Operation::kRead, id, pos, &block);

  // Error occurs here.
  if (status != OramStatus::kOK) {
    return status;
  }
  // Decrypt the block.
  oram_utils::DecryptBlock(&block, oram_crypto::Cryptor::GetInstance().get());

  // Deserialize the node.
  oram_utils::TreeNodeDeserialize(
      std::string(reinterpret_cast<char*>(block.data), sizeof(TreeNode)), node);
  return status;
}

OramStatus OdictController::OdsAccessRead(TreeNode* const node) {
  const uint32_t block_address = node->id_;
  logger->info("Read node {} from the oblivious dictionary.", block_address);

  read_count_++;
  // Try get the tree node from the cache.
  TreeNode* const cache_node = ods_cache_->Get(block_address);
  if (cache_node == nullptr) {
    // Not found.
    // Read the node from the oblivious ram.
    logger->debug("[-] Cache miss. Read from ORAM.");

    // We prevent creating raw pointers; instead we use RFII.
    TreeNode node;
    oram_utils::CheckStatus(CacheHelper(block_address, &node),
                            oram_utils::StrCat("OdsAccessRead", oram_read_err));
  } else {
    logger->debug("[+] Cache hit.");
    memcpy(node, cache_node, sizeof(TreeNode));
  }

  return OramStatus::kOK;
}

OramStatus OdictController::OdsAccessWrite(TreeNode* const node) {
  const uint32_t block_address = node->id_;
  logger->info("Write node {} to the oblivious dictionary.", block_address);

  write_count_++;
  // Try get the tree node from the cache.
  TreeNode* const cache_node = ods_cache_->Get(block_address);
  if (cache_node == nullptr) {
    // FIXME: Is this really needed?
    TreeNode node;
    oram_utils::CheckStatus(CacheHelper(block_address, &node),
                            oram_utils::StrCat("OdsAccessRead", oram_read_err));
  }

  // We always write to the cache.
  ods_cache_->Put(block_address, node);
  return OramStatus::kOK;
}

OramStatus OdictController::OdsAccessRemove(TreeNode* const node) {
  const uint32_t block_address = node->id_;
  logger->info("Remove node {} from the oblivious dictionary.", block_address);

  TreeNode* const cache_node = ods_cache_->Get(block_address);
  if (cache_node == nullptr) {
    // For an ORAM, read can be seen as read and remove. If the client reads the
    // target block, then it will be removed from the storage. So we just read
    // the block from the ORAM but do not add it to the local cache.
    TreeNode node;
    oram_utils::CheckStatus(CacheHelper(block_address, &node),
                            oram_utils::StrCat("OdsAccessRead", oram_read_err));
  } else {
    // Remove the node from the cache.
    ods_cache_->Pop();
  }

  return OramStatus::kOK;
}

OramStatus OdictController::OdsAccessInsert(TreeNode* const node) {
  const uint32_t block_address = node->id_;
  logger->info("Insert node {} to the oblivious dictionary.", block_address);

  ods_cache_->Put(block_address, node);

  return OramStatus::kOK;
}

OramStatus OdictController::OdsAccess(OdsOperation op_type,
                                      TreeNode* const node) {
  PANIC_IF(op_type == OdsOperation::kInvalid,
           "Invalid Operation to oblivious dictionary!");

  switch (op_type) {
    case OdsOperation::kRead:
      return OdsAccessRead(node);
    case OdsOperation::kWrite:
      return OdsAccessWrite(node);
    case OdsOperation::kRemove:
      return OdsAccessRemove(node);
    case OdsOperation::kInsert:
      return OdsAccessInsert(node);
  }

  return OramStatus::kOK;
}

OramStatus OdictController::OdsFinalize(size_t pad_val) {
  // Update rootPos based on rootId / generate new position tags.
  root_pos_ = ods_cache_->UpdatePos(root_id_);

  // Pad the read operation. This is simple because we can read arbitrary
  // blocks based on our preference.
  PANIC_IF(pad_val < read_count_, pad_val_err);
  for (size_t i = pad_val - read_count_; i <= pad_val; i++) {
    oram_block_t block;
    block.header.block_id = 0;
    block.header.type = BlockType::kNormal;

    OramStatus status = oram_contrller_->Access(Operation::kRead, 0, &block);
    oram_utils::CheckStatus(status,
                            oram_utils::StrCat("OdsFinalize", oram_read_err));
  }

  // Evict the cache.
  while (!ods_cache_->IsEmpty()) {
    TreeNode* const node = ods_cache_->Get();
    logger->debug("[+] Evicting {}", node->id_);

    // Need to serialize the node body.
    const std::string serialized_body = oram_utils::TreeNodeSerialize(node);
    // Construct the oram block.
    oram_block_t block;
    const uint32_t id = node->id_;
    const uint32_t pos = node->pos_tag_;

    block.header.block_id = id;
    block.header.type = BlockType::kNormal;
    memcpy(block.data, serialized_body.data(), serialized_body.size());

    // Encrypt the block.
    oram_utils::EncryptBlock(&block, oram_crypto::Cryptor::GetInstance().get());
    OramStatus status =
        oram_contrller_->AccessDirect(Operation::kWrite, id, pos, &block);

    // Finally pop the element.
    ods_cache_->Pop();
  }

  // Pad write operation.
  PANIC_IF(pad_val < write_count_, pad_val_err);
  for (size_t i = pad_val - write_count_; i <= pad_val; i++) {
    oram_block_t block;
    block.header.block_id = 0;
    block.header.type = BlockType::kNormal;

    OramStatus status = oram_contrller_->Access(Operation::kWrite, 0, &block);
    oram_utils::CheckStatus(status,
                            oram_utils::StrCat("OdsFinalize", oram_write_err));
  }

  write_count_ = read_count_ = 0ul;
  logger->debug("[-] OdsFinalize finished.");

  return OramStatus::kOK;
}

OramStatus OdictController::InitOds(void) {
  OramStatus status = OramStatus::kOK;

  // Create a dummy for padding operations in future.
  uint8_t* const dummy_data = (uint8_t*)(malloc(sizeof(TreeNode)));
  status = oram_crypto::Cryptor::RandomBytes(dummy_data, sizeof(TreeNode));

  if (status != OramStatus::kOK) {
    return status;
  }

  oram_block_t block;
  block.header.block_id = 0ul;
  block.header.type = BlockType::kNormal;
  memcpy(block.data, dummy_data, sizeof(TreeNode));

  status = oram_contrller_->Access(Operation::kRead, 0ul, &block);
  if (status != OramStatus::kOK) {
    return status;
  }

  oram_utils::SafeFree(dummy_data);
  return status;
}

OdictController::OdictController(
    size_t odict_size, size_t client_cache_max_size, uint32_t x,
    const std::shared_ptr<PathOramController>& oram_controller)
    : x_(x), node_count_(1ul), oram_contrller_(oram_controller) {
  // Initialize the cache.
  logger->info("[+] Using {} as backbone ORAM for Odict controller.",
               oram_controller->GetName());
  ods_cache_ =
      std::make_unique<OdsCache>(client_cache_max_size, oram_controller);

  // Abort if it is not initialized.
  if (!oram_controller->IsInitialized()) {
    logger->error(
        "[-] You cannot use oblivious dictionary when the backbone ORAM "
        "controller is not properly initialized!");
    abort();
  }

  OramStatus status = InitOds();
  if (status != OramStatus::kOK) {
    logger->error("[-] Initialize ODS controller failed: {}",
                  kErrorList.at(status));
    abort();
  }
}

TreeNode* OdictController::InternalFind(uint32_t key, uint32_t root_id) {
  if (root_id == 0) {
    return nullptr;
  }

  TreeNode* root = new TreeNode();
  oram_utils::CheckStatus(OdsAccess(OdsOperation::kRead, root),
                          "ReadOram failed");
  if (root->kv_pair_.first == key) {
    return root;
  }
  return root->kv_pair_.first < key ? InternalFind(key, root->right_id_)
                                    : InternalFind(key, root->left_id_);
}

TreeNode* OdictController::InternalInsert(TreeNode* node, uint32_t root_id) {
  // If there is no root node yet, assign the current node as the root node.
  if (root_id == 0) {
    oram_utils::CheckStatus(OdsAccess(OdsOperation::kInsert, node),
                            "OdsAccess I failed.");
    return node;
  }

  // Else read the current root node.
  TreeNode* root = new TreeNode();
  oram_utils::CheckStatus(OdsAccess(OdsOperation::kRead, root),
                          "ReadOram R failed");

  if (root->kv_pair_.first < node->kv_pair_.first) {
    TreeNode* const cur = InternalInsert(node, root->right_id_);
    root->right_id_ = cur->id_;
    root->right_height_ = HEIGHT(cur);
  } else if (root->kv_pair_.first > node->kv_pair_.first) {
    TreeNode* const cur = InternalInsert(node, root->left_id_);
    root->left_id_ = cur->id_;
    root->left_height_ = HEIGHT(cur);
  }

  oram_utils::CheckStatus(OdsAccess(OdsOperation::kWrite, root),
                          "ReadOram W failed");
                          
  delete root;
  return Balance(root_id);
}

OramStatus OdictController::InternalRemove(uint32_t key, uint32_t root_id) {
  return OramStatus::kOK;
}

TreeNode* OdictController::Find(uint32_t key) {
  // Each operation on the ODS consists of three phase:
  // start, access,and finalize.
  OdsStart();
  TreeNode* const node = InternalFind(key, root_id_);
  oram_utils::CheckStatus(OdsFinalize(ComputePadVal(node_count_)),
                          oram_utils::StrCat("Find: ", "OdsFinalize error!"));

  return node;
}

TreeNode* OdictController::Insert(TreeNode* node) {
  TreeNode* ans = InternalInsert(node, root_id_);

  if (root_id_ == 0) {
    root_id_ = ans->id_;
  }

  return ans;
}

OramStatus OdictController::Remove(uint32_t key) {
  return InternalRemove(key, root_id_);
}