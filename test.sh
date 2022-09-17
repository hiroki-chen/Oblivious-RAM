#!/bin/bash
# Copyright (c) 2022 Haobin Chen
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

MAGENTA='\033[1;35m'
NC='\033[0m' # No Color

printf "${MAGENTA}[+] Begin testing the Partition ORAM by different numbers of blocks...${NC}\n"

# set -x;

file="$(date).log";
touch "$file";

# Check if the environment variable is set.
if [[ -z ${https_proxy} ]]; then
    echo "Unsetting the proxy.";
    unset ${https_proxy};
fi

for ((i=6; i<=21; i++)); do
    block=$(echo "$((2 ** ${i}))");
    printf "${MAGENTA}    Testing block number: ${block}...${NC}\n";
    
    # Start the server in the background.
    ./bin/server --log_level=2 > ./log-server.log &
    
    # Wait for the server to start.
    sleep 1;
    
    ./bin/client --block_num=${block} --log_level=2 > ./log-client.log;
    client_pid=$!;
    
    wait ${client_pid};
    
    # Extract the running time by ms.
    running_time_client=$(grep "Time elapsed per block:" ./log-client.log | awk '{ for (i=1;i<=NF;i++) { if ($i == "us.") { print $(i-1) } } }');
    echo "[+] Running time: ${running_time_client} us for block number: ${block}." >> "./${file}";
    
    # Extract the server storage by MB.
    size=$(grep "The total storage size is" ./log-server.log | awk '{ for (i=1;i<=NF;i++) { if ($i == "MB.") { print $(i-1) } } }');
    echo "[+] Server storage: ${size} MB for block number: ${block}." >> "./${file}";
    
    # Extract the initialization time by us.
    init_time=$(grep "The Partition Oram Controller is initialized. Elapsed time is" ./log-client.log | awk '{ for (i=1;i<=NF;i++) { if ($i == "us.") { print $(i-1) } } }');
    echo "[+] Initialization time: ${init_time} us for block number: ${block}." >> "./${file}";
    
    # # Extract the server latency by ms.
    # runnint_time_server=$(grep "Elapsed time when reading a path:" ./log-server.log | awk '{ for (i=1;i<=NF;i++) { if ($i == "us.") { print $(i-1) } } }');
    # echo "[+] Server query time: ${runnint_time_server} us for block number: ${block}." >> "./${file}";
    
    echo >> "./${file}";
done

printf "${MAGENTA}[+] Successfully tested the Partition ORAM. Goodbye.${NC}\n";