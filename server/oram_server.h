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
#ifndef PARTITION_ORAM_SERVER_ORAM_SERVER_H_
#define PARTITION_ORAM_SERVER_ORAM_SERVER_H_

#include <string>
#include <memory>
#include <vector>

#include <grpc++/grpc++.h>
#include <spdlog/spdlog.h>

#include "oram_storage.h"
#include "base/oram_crypto.h"
#include "protos/messages.grpc.pb.h"
#include "protos/messages.pb.h"

extern std::shared_ptr<spdlog::logger> logger;

namespace partition_oram {
class PartitionORAMService final : public server::Service {
 private:
  friend class ServerRunner;

  std::shared_ptr<oram_crypto::Cryptor> cryptor_;
  std::vector<std::unique_ptr<OramServerStorage>> storages_;

 public:
  grpc::Status InitOram(grpc::ServerContext* context,
                        const InitOramRequest* request,
                        google::protobuf::Empty* empty) override;

  grpc::Status PrintOramTree(grpc::ServerContext* context,
                             const PrintOramTreeRequest* request,
                             google::protobuf::Empty* response) override;

  grpc::Status ReadPath(grpc::ServerContext* context,
                        const ReadPathRequest* request,
                        ReadPathResponse* response) override;

  grpc::Status WritePath(grpc::ServerContext* context,
                         const WritePathRequest* request,
                         WritePathResponse* response) override;

  grpc::Status CloseConnection(grpc::ServerContext* context,
                               const google::protobuf::Empty* request,
                               google::protobuf::Empty* response) override;

  grpc::Status KeyExchange(grpc::ServerContext* context,
                           const KeyExchangeRequest* request,
                           KeyExchangeResponse* response) override;

  grpc::Status SendHello(grpc::ServerContext* context,
                         const HelloMessage* request,
                         google::protobuf::Empty* empty) override;

  grpc::Status ReportServerInformation(
      grpc::ServerContext* context, const google::protobuf::Empty* request,
      google::protobuf::Empty* response) override;

  grpc::Status ResetServer(grpc::ServerContext* context,
                           const google::protobuf::Empty* request,
                           google::protobuf::Empty* response) override;
};

class ServerRunner {
 private:
  std::unique_ptr<PartitionORAMService> service_;

  // Networking configurations.
  std::string address_;
  std::string port_;
  std::shared_ptr<grpc::ServerCredentials> creds_;

  bool is_initialized;

 public:
  ServerRunner(const std::string& address, const std::string& port,
               const std::string& key_path, const std::string& crt_path);

  void Run(void);
};
}  // namespace partition_oram

#endif  // PARTITION_ORAM_SERVER_ORAM_SERVER_H_