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
#ifndef PARTITION_ORAM_CLIENT_ORAM_CLIENT_H_
#define PARTITION_ORAM_CLIENT_ORAM_CLIENT_H_

#include <string>
#include <memory>

#include <grpc++/grpc++.h>

#include "oram_controller.h"

#include "protos/messages.grpc.pb.h"
#include "protos/messages.pb.h"
#include "base/oram_crypto.h"

namespace partition_oram {
class Client {
  std::string server_address_;
  std::string server_port_;
  std::string crt_path_;

  uint32_t bucket_size_;
  uint32_t block_num_;

  std::shared_ptr<server::Stub> stub_;

  std::unique_ptr<OramController> controller_;

  std::shared_ptr<oram_crypto::Cryptor> cryptor_;

 public:
  Client(const std::string& server_address, const std::string& server_port,
         const std::string& crt_path, uint32_t bucket_size, uint32_t block_num)
      : server_address_(server_address),
        server_port_(server_port),
        crt_path_(crt_path),
        bucket_size_(bucket_size),
        block_num_(block_num) {}

  void Run(void);

  int StartKeyExchange(void);
  int SendHello(void);
  int CloseConnection(void);
  int InitOram(void);
  int TestOram(void);
  int ResetServer(void);

  void Reset(uint32_t block_num) {
    block_num_ = block_num;

    controller_->Reset(block_num);
  }

  virtual ~Client() {}
};
}  // namespace partition_oram

#endif  // PARTITION_ORAM_CLIENT_ORAM_CLIENT_H_