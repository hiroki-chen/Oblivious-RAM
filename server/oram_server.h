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
#include <unordered_map>

#include "base/oram_crypto.h"
#include "base/oram_utils.h"
#include "oram_storage.h"
#include "protos/messages.grpc.pb.h"
#include "protos/messages.pb.h"

extern std::shared_ptr<spdlog::logger> logger;

namespace oram_impl {
class OramService final : public oram_server::Service {
 private:
  friend class ServerRunner;

  std::shared_ptr<oram_crypto::Cryptor> cryptor_;
  std::unordered_map<uint32_t, std::unique_ptr<BaseOramServerStorage>>
      storages_;

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

  grpc::Status LoadSqrtOram(grpc::ServerContext* context,
                            const LoadSqrtOramRequest* request,
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
                               const WriteSqrtMessage* request,
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
  uint32_t port_;
  std::shared_ptr<grpc::ServerCredentials> creds_;

  bool is_initialized;

 public:
  ServerRunner(const std::string& address, uint32_t port,
               const std::string& key_path, const std::string& crt_path);

  void Run(void);
};

template <typename T>
grpc::Status CheckStorage(BaseOramServerStorage* const storage,
                          const std::string& instance_hash,
                          OramStorageType type, T*& dst) {
  // Try cast.
  dst = dynamic_cast<T*>(storage);

  // Nullptr means that this target type of T is not a derived class.
  // In this case, we need to directly panic because this is triggered by
  // programming error rather than runtime error.
  PANIC_IF(dst == nullptr,
           "Trying to cast to a non-derived class of `BaseOramServerStorage`.");

  if (type != dst->GetOramStorageType()) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, oram_type_mismatch_err);
  } else if (dst->GetInstanceHash() != instance_hash) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, oram_hash_mismatch_err);
  }

  return grpc::Status::OK;
}

}  // namespace oram_impl

#endif  // ORAM_IMPL_SERVER_ORAM_SERVER_H_