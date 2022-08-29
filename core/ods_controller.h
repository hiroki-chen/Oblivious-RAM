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
#ifndef ORAM_IMPL_CORE_ODS_CONTROLLER_H_
#define ORAM_IMPL_CORE_ODS_CONTROLLER_H_

#include <memory>

#include "ods_cache.h"
#include "base/ods_defs.h"
#include "base/ods_objects.h"
#include "base/oram_defs.h"
#include "path_oram_controller.h"
#include "protos/messages.grpc.pb.h"

namespace oram_impl::ods {
class OdictController {
  std::shared_ptr<PathOramController> oram_contrller_;
  std::unique_ptr<OdsCache> ods_cache_;

  uint32_t root_id_;
  uint32_t root_pos_;
  uint32_t x_;  // A parameter for padding.
  size_t node_count_;

  // Some counters for padding.
  uint32_t read_count_;
  uint32_t write_count_;

  OramStatus CacheHelper(uint32_t id, TreeNode* const node);

  OramStatus InitOds(void);

  OramStatus OdsAccessRead(TreeNode* const node);
  OramStatus OdsAccessWrite(TreeNode* const node);
  OramStatus OdsAccessInsert(TreeNode* const node);
  OramStatus OdsAccessRemove(TreeNode* const node);

  void OdsStart(void) { ods_cache_->Clear(); }
  OramStatus OdsAccess(OdsOperation op_type, TreeNode* const node);
  OramStatus OdsFinalize(size_t pad_val);

  // Helper functions for AVL tree operations.
  // Because public interfaces are arkward if they are implemented recursively,
  // so we write them as wrapper functions that invoke the internal recursive
  // functions.
  TreeNode* InternalFind(uint32_t key, uint32_t root_id);
  TreeNode* InternalInsert(TreeNode* node, uint32_t root_id);
  OramStatus InternalRemove(uint32_t key, uint32_t root_id);

  // AVL Tree balance functions.
  TreeNode* Balance(uint32_t root_id);
  TreeNode* LeftRotate(uint32_t root_id);
  TreeNode* RightRotate(uint32_t root_id);
  TreeNode* LeftRightRotate(uint32_t root_id);
  TreeNode* RightLeftRotate(uint32_t root_id);

 public:
  TreeNode* Find(uint32_t key);
  TreeNode* Insert(TreeNode* node);
  OramStatus Remove(uint32_t key);

  void SetStub(const std::shared_ptr<oram_server::Stub>& stub) {
    oram_contrller_->SetStub(stub);
  }

  // To construct the controller for oblivious dictionary, the user needs to
  // first create an instance of the underlying oblivious RAM controller. We
  // currently designate PathOram as the backbone ORAM.
  OdictController(size_t odict_size, size_t client_cache_max_size, uint32_t x,
                  const std::shared_ptr<PathOramController>& oram_controller);
};
}  // namespace oram_impl::ods

#endif  // ORAM_IMPL_CORE_ODS_CONTROLLER_H_