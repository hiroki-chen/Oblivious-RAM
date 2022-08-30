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
  kOutOfMemory = 3,
  kFileNotFound = 4,
  kFileIOError = 5,
  kOutOfRange = 6,
  kServerError = 7,
  kObjectNotFound = 8,
  kUnknownError = 9,
  kVersionMismatch = 10,
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
  void Update(const StatusCode& new_status) { code_ = new_status; }

  // Returns true if the current status matches `OramStatus::OK`.
  // If the functions returns false, then the user may need to check the error
  // message.
  bool ok(void) const { return code_ == StatusCode::kOK; }

  StatusCode ErrorCode(void) const { return code_; }

  std::string ErrorMessage(void) const { return message_; }

  std::string ToString(void) const { return kErrorList.at(code_); }
};
}  // namespace oram_impl

#endif  // ORAM_IMPL_BASE_ORAM_STATUS_H_