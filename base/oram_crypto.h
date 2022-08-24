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
#ifndef ORAM_IMPL_BASE_ORAM_CRYPTO_H_
#define ORAM_IMPL_BASE_ORAM_CRYPTO_H_

#include <sodium.h>

#include <memory>
#include <string>
#include <utility>

#include "oram_defs.h"

#define ORAM_CRYPTO_KEY_SIZE crypto_aead_aes256gcm_KEYBYTES
#define ORAM_CRYPTO_RANDOM_SIZE crypto_aead_aes256gcm_NPUBBYTES

#define ull unsigned long long

namespace oram_crypto {
class Cryptor {
  // The symmetric keys are valid only after key negotiation.
  uint8_t session_key_rx_[ORAM_CRYPTO_KEY_SIZE];
  uint8_t session_key_tx_[ORAM_CRYPTO_KEY_SIZE];
  uint8_t random_val_[ORAM_CRYPTO_RANDOM_SIZE];

  // The public key and the secret key.
  uint8_t public_key_[crypto_kx_PUBLICKEYBYTES];
  uint8_t secret_key_[crypto_kx_SECRETKEYBYTES];

  bool is_initialized = false;
  bool is_setup = false;

  Cryptor();

  void CryptoPrelogue(void);

 public:
  // Use Fisher-Yates shuffle to generate a random permutation.
  template <typename Tp>
  static oram_impl::OramStatus RandomShuffle(std::vector<Tp>& array) {
    if (array.empty()) {
      return oram_impl::OramStatus::kInvalidArgument;
    }

    for (size_t i = array.size() - 1; i > 0; --i) {
      uint32_t j;
      if (Cryptor::UniformRandom(0, i, &j) != oram_impl::OramStatus::kOK) {
        return oram_impl::OramStatus::kUnknownError;
      }
      std::swap(array[i], array[j]);
    }

    return oram_impl::OramStatus::kOK;
  }

  static std::shared_ptr<Cryptor> GetInstance(void);

  static oram_impl::OramStatus UniformRandom(uint32_t min, uint32_t max,
                                             uint32_t* const out);
  static oram_impl::OramStatus RandomBytes(uint8_t* const out, size_t size);

  oram_impl::OramStatus Encrypt(const uint8_t* message, size_t length,
                                uint8_t* const iv, std::string* const out);
  oram_impl::OramStatus Decrypt(const uint8_t* message, size_t length,
                                const uint8_t* iv, std::string* const out);
  oram_impl::OramStatus Digest(const uint8_t* message, size_t length,
                               std::string* const out);
  oram_impl::OramStatus SampleKeyPair(void);
  oram_impl::OramStatus SampleSessionKey(const std::string& peer_pk, bool type);

  std::pair<std::string, std::string> GetKeyPair(void);
  std::pair<std::string, std::string> GetSessionKeyPair(void);

  void NoNeedForSessionKey(void);

  virtual ~Cryptor();
};
}  // namespace oram_crypto

#endif  // ORAM_IMPL_BASE_ORAM_CRYPTO_H_