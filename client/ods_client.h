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
#ifndef ORAM_IMPL_CLIENT_ODS_CLIENT_H_
#define ORAM_IMPL_CLIENT_ODS_CLIENT_H_

#include <memory>

#include <grpc++/grpc++.h>

#include "core/odict_controller.h"
#include "protos/messages.grpc.pb.h"
#include "protos/messages.pb.h"

using namespace oram_impl;
using namespace ods;

class Client {
  std::shared_ptr<oram_server::Stub> stub_;

 public:
  Client(const std::string address, const std::string port,
         const std::string crt_path, size_t odict_size,
         size_t client_cache_max_size, uint32_t block_num,
         uint32_t bucket_size);

  std::unique_ptr<OdictController> controller_;
};

#endif  // ORAM_IMPL_CLIENT_ODS_CLIENT_H_