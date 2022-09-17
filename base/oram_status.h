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
#ifndef ORAM_IMPL_BASE_ORAM_STATUS_H_
#define ORAM_IMPL_BASE_ORAM_STATUS_H_

#include <string>
#include <unordered_map>
#include <stdexcept>

namespace oram_impl {
enum class StatusCode : int {
  // StatusCode::kOk
  //
  // kOK does not indicate an error; this value is returned on success. It is
  // typical to check for this value before proceeding on any given call across
  // an API or ORAM boundary. To check this value, use the
  // `oram_impl::Status::ok()` member function rather than inspecting the raw
  // code.
  kOK = 0,

  // StatusCode::kInvalidArgument
  //
  // kInvalidArgument indicates the caller specified an invalid argument, such
  // as a malformed filename. Note that useof such errors should be narrowly
  // limited to indicate the invalid nature ofthe arguments themselves. Errors
  // with validly formed arguments that maycause errors with the state of the
  // receiving system should be denoted with `kFailedPrecondition` instead.
  kInvalidArgument = 1,

  // StatusCode::kInvalidOperation
  //
  // kInvalidOperation indicates an error typically occurring when the caller is
  // invoking a method that is NOT supported on a given type. For example, the
  // user cannot perform linear read when the remote server storage type is a
  // tree-like one.
  kInvalidOperation = 2,

  // StatusCode::kOutOfMemory
  //
  // kOutOfMemory indicates an error typically occurring when the system
  // cannot allocate memory upon invocation of `malloc`, `calloc`, or `realloc`
  // functions. When the system is OOM, any allocation will fail.
  kOutOfMemory = 3,

  // StatusCode::kFileNotFound
  //
  // kFileNotFound indicates an error when the file / path does not exist on the
  // target filesystem. Often this is related to misspelled path.
  kFileNotFound = 4,

  // StatusCode::kFileIOError
  //
  // Unlike `kFileNotFound`, this error indicates the file pointer is busy or
  // temporarily occupied by other processes and may cause data race, or the
  // file / path cannot be properly read due to corruption.
  kFileIOError = 5,

  // StatusCode::kOutOfRange
  //
  // kOutOfRange indicates the request contains some targets that exceed the
  // expected boundary. For example, if the ORAM only has three blocks but the
  // client is requesting the non-existing fourth block, then the controller
  // should report `kOutOfRange` error to the client.
  kOutOfRange = 6,

  // StatusCode::kServerError
  //
  // kServerError usually happens when the remote server reports an unexpetec
  // error by `grpc::Status`. We simply do not convert `grpc::Status` to
  // `oram_impl::StatuCode` because `grpc::Status` carries sufficient error
  // information and details for the client to figure out what happened so that
  // it can solve the problem, or it can let the cloud administrator to fix the
  // server error.
  kServerError = 7,

  // StatusCode::kObjectNotFound
  //
  // kObjectNotFound indicates an error that will occur if the desired target is
  // not found. This is slightly different from `StatusCode::kOutOfRange`.
  kObjectNotFound = 8,

  // StatusCode::kUnkownError
  //
  // The error is peculiar so that we cannot figure out what is happening. If
  // the program crashes in a totally unexpected way, this error should be
  // reported to the client. Since this error carries less information and can
  // be hard to deal with, the error reporter should avoid using this error.
  kUnknownError = 9,

  // StatusCode::kVersionMismatch
  //
  // Currently this error is reserved only when the client ORAM version
  // mismatches with that of the remote storage. In future, we may develop more
  // APIs that have different versions, and this errir will indicate that the
  // request to the API cannot be fulfilled due to version mismatch: e.g., the
  // client cannot use APIs with v1 when it is constructing an object with v2.
  kVersionMismatch = 10,

  // StatusCode::kUnimplemented
  //
  // kUnimplemented indicates an unimplemented interface.
  kUnimplemented = 11,
};

static const std::unordered_map<StatusCode, std::string> kErrorList = {
    {StatusCode::kOK, "OK"},
    {StatusCode::kInvalidArgument, "Invalid argument"},
    {StatusCode::kInvalidOperation, "Invalid operation"},
    {StatusCode::kOutOfMemory, "Out of memory"},
    {StatusCode::kFileNotFound, "File not found"},
    {StatusCode::kFileIOError, "File IO error"},
    {StatusCode::kOutOfRange, "Out of range"},
    {StatusCode::kServerError, "Server error"},
    {StatusCode::kObjectNotFound, "The object is not found"},
    {StatusCode::kUnknownError, "Unknown error"},
    {StatusCode::kVersionMismatch, "Version mismatch"},
};

class OramStatus final {
  StatusCode code_;
  std::string message_;

 public:
  // Constructors

  // This default constructor creates an OK status with no message or payload.
  // Avoid this constructor and prefer explicit construction of an OK status.
  OramStatus() : code_(StatusCode::kOK), message_("") {}

  // Creates a status in the canonical error space with the specified
  // `oram_impl::StatusCode` and an error message for error details.
  OramStatus(StatusCode code, const std::string& msg)
      : code_(code), message_(msg) {}

  // Pre-defined constants for convenience.
  static const OramStatus& OK;

  // Updates the current `OramStatus` type if `status->ok()` returns true.
  // If the existing status already contains a non-OK error, this update has no
  // effect and preserves the current data.
  void Update(const StatusCode& new_status);

  // Returns true if the current status matches `OramStatus::OK`.
  // If the functions returns false, then the user may need to check the error
  // message.
  bool ok(void) const { return code_ == StatusCode::kOK; }

  StatusCode ErrorCode(void) const { return code_; }

  std::string ErrorMessage(void) const { return message_; }

  std::string ToString(void) const { return kErrorList.at(code_); }
};

inline void OramStatus::Update(const StatusCode& new_status) {
  // Only OK status is allowed to transmute.
  if (code_ == StatusCode::kOK) {
    code_ = new_status;
  }
}
}  // namespace oram_impl

#endif  // ORAM_IMPL_BASE_ORAM_STATUS_H_