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
#include "ods_client.h"

#include <spdlog/spdlog.h>

extern std::shared_ptr<spdlog::logger> logger;


Client::Client(const std::string address, const std::string port,
               const std::string crt_path, size_t odict_size,
               size_t client_cache_max_size, uint32_t x, uint32_t block_num,
               uint32_t bucket_size) {
  // We temporarily set it to a standalone one for compatibility because we do
  // not want to really remove the item when reading something.
  const std::shared_ptr<PathOramController> controller =
      std::make_shared<PathOramController>(0, block_num, bucket_size, true);

  const std::string addr = oram_utils::StrCat(address, ":", port);

  logger->info("Client started, and the address is given as {}:{}.",
               address, port);

  // Configure the SSL connection.
  const std::string crt_file = oram_utils::ReadKeyCrtFile(crt_path);
  grpc::SslCredentialsOptions ssl_opts;
  ssl_opts.pem_root_certs = crt_file;
  std::shared_ptr<grpc::ChannelCredentials> ssl_creds =
      grpc::SslCredentials(ssl_opts);

  stub_ =
      std::move(oram_server::NewStub(grpc::CreateChannel(addr, ssl_creds)));
  controller->SetStub(stub_);
  
  auto cryptor = oram_crypto::Cryptor::GetInstance();
  cryptor->NoNeedForSessionKey();

  // Initialize the ORAM controller.
  // Assume block_num is some power of 2.
  controller->InitOram();
  controller->FillWithData(
      oram_utils::SampleRandomBucket(block_num, block_num, 0ul));

  controller_ = std::make_unique<OdictController>(
      odict_size, client_cache_max_size, x, controller);
  controller_->SetStub(stub_);
}