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
#include "oram_status.h"

#include "oram_utils.h"

namespace oram_impl {
const OramStatus& OramStatus::OK = OramStatus();

std::ostream& operator<<(std::ostream& os, OramStatus code) {
  return os << code.EmitString();
}

std::string OramStatus::EmitString(void) const {
  // In general, the error information is printed in the following format.
  // [error_type]: additional message; caused by
  // ----> [error_type]: additional message; caused by
  // ...
  std::string ans;

  // First, get the information from itself.
  if (error_code_ != StatusCode::kOK) {
    ans.append(oram_utils::StrCat("\n\t[", kErrorList.at(error_code_), "@",
                                  location_, "]: ", error_message_));
  }

  for (auto iter = nested_status_.crbegin(); iter != nested_status_.crend();
       iter++) {
    ans.append("; caused by\n\t");
    ans.append(oram_utils::StrCat("[", kErrorList.at(iter->error_code_), "@",
                                  iter->location_,
                                  "]: ", iter->error_message_));
  }

  return ans;
}
}  // namespace oram_impl