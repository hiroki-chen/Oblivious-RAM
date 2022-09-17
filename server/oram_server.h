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
#ifndef ORAM_IMPL_SERVER_ORAM_SERVER_H_
#define ORAM_IMPL_SERVER_ORAM_SERVER_H_

#include <grpc++/grpc++.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <vector>

#include "base/oram_crypto.h"
#include "oram_storage.h"
#include "protos/messages.grpc.pb.h"
#include "protos/messages.pb.h"

extern std::shared_ptr<spdlog::logger> logger;

namespace oram_impl {
class OramService final : public oram_server::Service {
 private:
  friend class ServerRunner;

  std::shared_ptr<oram_crypto::Cryptor> cryptor_;
  std::vector<std::unique_ptr<BaseOramServerStorage>> storages_;

  grpc::Status CheckInitRequest(uint32_t id);
  grpc::Status CheckIdValid(uint32_t id);

 public:
  grpc::Status InitTreeOram(grpc::ServerContext* context,
                            const InitTreeOramRequest* request,
                            google::protobuf::Empty* empty) override;

  grpc::Status InitFlatOram(grpc::ServerContext* context,
                            const InitFlatOramRequest* request,
                            google::protobuf::Empty* empty) override;

  grpc::Status InitSqrtOram(grpc::ServerContext* context,
                            const InitSqrtOramRequest* request,
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

  grpc::Status ReadFlatMemory(grpc::ServerContext* context,
                              const ReadFlatRequest* request,
                              FlatVectorMessage* response) override;

  grpc::Status WriteFlatMemory(grpc::ServerContext* context,
                               const FlatVectorMessage* request,
                               google::protobuf::Empty* empty) override;

  grpc::Status ReadSqrtMemory(grpc::ServerContext* context,
                              const ReadSqrtRequest* request,
                              SqrtMessage* response) override;

  grpc::Status WriteSqrtMemory(grpc::ServerContext* context,
                               const SqrtMessage* request,
                               google::protobuf::Empty* empty) override;

  grpc::Status SqrtPermute(grpc::ServerContext* context,
                           const SqrtPermMessage* message,
                           google::protobuf::Empty* empty) override;

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
  std::unique_ptr<OramService> service_;

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
}  // namespace oram_impl

#endif  // ORAM_IMPL_SERVER_ORAM_SERVER_H_