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
#ifndef ORAM_IMPL_PARSE_ORAM_PARSE_H_
#define ORAM_IMPL_PARSE_ORAM_PARSE_H_

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
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
  // A logger for parser.
  std::shared_ptr<spdlog::logger> logger_;

  bool ignore_command_line_args_;

 protected:
  oram_impl::OramStatus DoParse(const YAML::const_iterator& cur_iter,
                                oram_impl::OramConfig& config);
  oram_impl::OramStatus DoCommandLine(oram_impl::OramConfig& config);

 public:
  // Parse the file.
  oram_impl::OramStatus Parse(oram_impl::OramConfig& config);
  // Or from the command line.
  oram_impl::OramStatus FromCommandLine(int argc, char** argv,
                                        oram_impl::OramConfig& config);

  bool IgnoreCommandLineArgs(void) { return ignore_command_line_args_; }

  YamlParser();
};

}  // namespace oram_parse

#endif  // ORAM_IMPL_PARSE_ORAM_PARSER_H_