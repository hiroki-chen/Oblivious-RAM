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
#include "oram_crypto.h"

#include <bitset>
#include <sstream>

#include <spdlog/spdlog.h>
#include <fpe.h>

#include "oram_utils.h"

extern std::shared_ptr<spdlog::logger> logger;

namespace oram_crypto {
Cryptor::Cryptor() {
  PANIC_IF(sodium_init() == -1, "Failed to initialize sodium.");

  // Initialize the random number.
  randombytes_buf(random_val_, sizeof(ORAM_CRYPTO_RANDOM_SIZE));
  is_initialized = true;
  INFO(logger, "Cryptor initialized.");
}

Cryptor::~Cryptor() {
  // Do nothing.
}

std::shared_ptr<Cryptor> Cryptor::GetInstance(void) {
  static std::shared_ptr<Cryptor> instance =
      std::shared_ptr<Cryptor>(new Cryptor());
  return instance;
}

void Cryptor::CryptoPrelogue(void) {
  PANIC_IF(!is_initialized, "Cryptor is not initialized.");
  PANIC_IF(crypto_aead_aes256gcm_is_available() == 0,
           "AES-256-GCM is not available on this CPU.");
}

oram_impl::OramStatus RandomPermutation(std::vector<uint32_t>& array) {
  // Pick up a PRP key.
  oram_impl::OramStatus status = oram_impl::OramStatus::OK;
  uint8_t prp_key[ORAM_PRP_KEY_SIZE];
  memset(prp_key, 0, ORAM_PRP_KEY_SIZE);
  randombytes_buf(prp_key, ORAM_PRP_KEY_SIZE);

  const size_t size = array.size();
  if (size == 0) {
    return oram_impl::OramStatus(oram_impl::StatusCode::kInvalidArgument,
                                 "The input array cannot be empty", __func__);
  }

  // Calculate the highest order.
  const uint32_t ord = (uint32_t)(std::log10(size) / std::log10(ORAM_RADIX));

  // Generate FPE key. By default, the radix is 2, which is set by `ORAM_RADIX`.
  FPE_KEY* fpe_key =
      FPE_ff3_1_create_key(reinterpret_cast<char*>(prp_key), "", ORAM_RADIX);

  // Start permutation. Prepare a buffer.
  std::vector<uint32_t> perm_buf(size);
  // The array size should be some power of 2 to ensure that there is round-up.
  for (size_t i = 0; i < size; i++) {
    std::bitset<sizeof(uint32_t)* 8> bit = array[i];
    std::string bin = bit.to_string();
    // Truncate the binary string.
    bin = bin.substr(bin.size() - ord);

    std::string out(bin.size(), '0');
    FPE_ff3_encrypt(bin.data(), out.data(), fpe_key);

    // Convert to uint32_t.
    bit = 0;
    std::stringstream ss(out);
    ss >> bit;
    perm_buf[i] = bit.to_ulong();
  }

  // Permute the array.
  for (size_t i = 0; i < size; i++) {
    std::swap(array[i], array[perm_buf[i]]);
  }

  FPE_ff3_delete_key(fpe_key);
  return oram_impl::OramStatus::OK;
}

// The encryption algorithm also uses AES-GCM mode to encrypt the message.
oram_impl::OramStatus Cryptor::Encrypt(const uint8_t* message, size_t length,
                                       uint8_t* const iv,
                                       std::string* const out) {
  CryptoPrelogue();
  PANIC_IF(!is_setup, "Cryptor is not yet correctly set up.");

  // Fill in the IV.
  out->resize(crypto_aead_aes256gcm_ABYTES + length);
  unsigned long long ciphertext_len;

  int ret = crypto_aead_aes256gcm_encrypt(
      (uint8_t*)out->data(), &ciphertext_len, message, length, nullptr, 0, NULL,
      iv, session_key_rx_);

  oram_impl::OramStatus err = oram_impl::OramStatus(
      oram_impl::StatusCode::kUnknownError,
      "Libsodium cannot encrypt the block! Maybe the buffer "
      "is truncated or corrupted.");

  return ret == 0 ? oram_impl::OramStatus::OK : err;
}

oram_impl::OramStatus Cryptor::Decrypt(const uint8_t* message, size_t length,
                                       const uint8_t* iv,
                                       std::string* const out) {
  CryptoPrelogue();
  PANIC_IF(!is_setup, "Cryptor is not yet correctly set up.");

  if (length < crypto_aead_aes256gcm_ABYTES) {
    return oram_impl::OramStatus(oram_impl::StatusCode::kInvalidArgument,
                                 "The length of the message is too short",
                                 __func__);
  }

  // The message consists of the GCM MAC tag, the nonce, ant the
  // ciphertext itself, so it is easily for us to dertemine the length of the
  // plaintext because length of ciphertext = length of plaintext.
  size_t message_len = length - crypto_aead_aes256gcm_ABYTES;
  uint8_t* const decrypted = (uint8_t*)malloc(message_len);

  int ret = crypto_aead_aes256gcm_decrypt(decrypted, (ull*)&message_len,
                                          nullptr, message, length, nullptr, 0,
                                          iv, session_key_rx_);
  *out = std::string((char*)decrypted, message_len);

  oram_impl::OramStatus err = oram_impl::OramStatus(
      oram_impl::StatusCode::kUnknownError,
      "Libsodium cannot decrypt the block! Maybe the buffer "
      "is truncated or corrupted.");

  // Free the memory.
  oram_utils::SafeFree(decrypted);
  return ret == 0 ? oram_impl::OramStatus::OK : err;
}

oram_impl::OramStatus Cryptor::Digest(const uint8_t* message, size_t length,
                                      std::string* const out) {
  uint8_t* const digest = (uint8_t*)malloc(crypto_hash_sha256_BYTES);
  uint8_t* const message_with_nonce =
      (uint8_t*)malloc(length + ORAM_CRYPTO_RANDOM_SIZE);
  // Concatenate the raw message with the random number.
  memcpy(message_with_nonce, message, length);
  memcpy(message_with_nonce + length, random_val_, ORAM_CRYPTO_RANDOM_SIZE);
  // Digest the message using SHA-256.
  int ret = crypto_hash_sha256(digest, message_with_nonce,
                               length + ORAM_CRYPTO_RANDOM_SIZE);
  *out = std::string((char*)digest, crypto_hash_sha256_BYTES);

  oram_impl::OramStatus err = oram_impl::OramStatus(
      oram_impl::StatusCode::kUnknownError,
      "Libsodium cannot digest the block! Maybe the buffer "
      "is truncated or corrupted.");

  // Free the memory.
  oram_utils::SafeFreeAll(2, digest, message_with_nonce);
  return ret == 0 ? oram_impl::OramStatus::OK : err;
}

oram_impl::OramStatus Cryptor::SampleKeyPair(void) {
  CryptoPrelogue();

  oram_impl::OramStatus err = oram_impl::OramStatus(
      oram_impl::StatusCode::kUnknownError,
      "Libsodium cannot sample the key pair! Maybe the buffer "
      "is truncated or corrupted.");

  // Generate a key pair.
  int ret = crypto_kx_keypair(public_key_, secret_key_);
  return ret == 0 ? oram_impl::OramStatus::OK : err;
}

oram_impl::OramStatus Cryptor::SampleSessionKey(const std::string& peer_pk,
                                                bool type) {
  CryptoPrelogue();

  // Check the length of the peer's public key.
  if (peer_pk.length() != crypto_kx_PUBLICKEYBYTES) {
    return oram_impl::OramStatus(
        oram_impl::StatusCode::kInvalidArgument,
        "The length of the peer's public key is not correct.");
  }

  // Generate a session key. Prerequisite after this point: the peer's public
  // key must be known. Compute two shared keys using the peer's public key
  // and the cryptor's secret key. session_key_rx_ will be used by the client to
  // receive data from the server, session_key_tx will be used by the client to
  // send data to the server.
  int ret = -1;
  if (type == 0) {
    ret = crypto_kx_client_session_keys(session_key_rx_, session_key_tx_,
                                        public_key_, secret_key_,
                                        (uint8_t*)peer_pk.c_str());
  } else {
    ret = crypto_kx_server_session_keys(session_key_rx_, session_key_tx_,
                                        public_key_, secret_key_,
                                        (uint8_t*)peer_pk.c_str());
  }

  if (ret == 0) {
    is_setup = true;
    return oram_impl::OramStatus::OK;
  } else {
    return oram_impl::OramStatus(
        oram_impl::StatusCode::kUnknownError,
        "Libsodium cannot sample the session key! Maybe the buffer "
        "is truncated or corrupted.");
    ;
  }
}

std::pair<std::string, std::string> Cryptor::GetKeyPair(void) {
  PANIC_IF(!is_initialized, "Cryptor is not initialized.");

  std::pair<std::string, std::string> key_pair;
  key_pair.first = std::string((char*)public_key_, crypto_kx_PUBLICKEYBYTES);
  key_pair.second = std::string((char*)secret_key_, crypto_kx_SECRETKEYBYTES);
  return key_pair;
}

std::pair<std::string, std::string> Cryptor::GetSessionKeyPair(void) {
  std::pair<std::string, std::string> key_pair;
  key_pair.first =
      std::string((char*)session_key_rx_, crypto_kx_SESSIONKEYBYTES);
  key_pair.second =
      std::string((char*)session_key_tx_, crypto_kx_SESSIONKEYBYTES);
  return key_pair;
}

oram_impl::OramStatus UniformRandom(uint32_t min, uint32_t max,
                                    uint32_t* const out) {
  if (min > max) {
    return oram_impl::OramStatus(
        oram_impl::StatusCode::kInvalidArgument,
        "The minimum value is greater than the maximum value");
  }

  // @ref Chromium's base/rand_util.cc for the implementation.
  uint32_t range = max - min + 1;
  uint32_t max_acceptable_value =
      (std::numeric_limits<uint32_t>::max() / range) * range - 1;
  // We sample a random number and them map it to the range [min, max]
  // (inclusive) in a uniform way by scaling.
  uint32_t value;

  do {
    // Use a strong RNG to generate a random number.
    // This is important because we want this function to be pseudorandom
    // and cannot be predicted by any adversary.
    value = randombytes_random();
  } while (value > max_acceptable_value);

  value = value % range + min;
  *out = value;
  return oram_impl::OramStatus::OK;
}

oram_impl::OramStatus RandomBytes(uint8_t* const out, size_t length) {
  if (length == 0) {
    return oram_impl::OramStatus(oram_impl::StatusCode::kInvalidArgument,
                                 "The length of the output buffer is zero",
                                 __func__);
  }

  randombytes_buf(out, length);
  return oram_impl::OramStatus::OK;
}

void Cryptor::NoNeedForSessionKey(void) {
  // There is no need to sample a session key with the server for debugging.
  is_setup = true;

  // Sample a random ephermal key.
  oram_impl::OramStatus status =
      RandomBytes(session_key_rx_, ORAM_CRYPTO_KEY_SIZE);
  oram_utils::CheckStatus(status, "Cannot sample ephermeral key.");
}
}  // namespace oram_crypto