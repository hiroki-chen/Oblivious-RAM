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
#ifndef ORAM_IMPL_PARSE_ORAM_PARSER_H_
#define ORAM_IMPL_PARSE_ORAM_PARSER_H_

#include <yaml-cpp/yaml.h>

#include "base/oram_config.h"
#include "base/oram_status.h"

namespace oram_parse {
// By default, this class will try to read the configuration file (in yaml) from
// "./config.yaml"; user could override this by providing the `ORAM_CONFIG_PATH`
// environment variable. If the target file is not found, then it will generate
// a default config object for the ORAM controller.
//
// The Yaml config file is case-sensitive.
class YamlParser {
 protected:
  oram_impl::OramConfig EmitDefault(void);

 public:
  // Parse the file.
  oram_impl::OramStatus Parse(oram_impl::OramConfig& config);
};

}  // namespace oram_parse

#endif  // ORAM_IMPL_PARSE_ORAM_PARSER_H_