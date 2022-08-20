# Implementation of the Partition ORAM

This is the reference implementation of the paper appearing on NDSS symposium: **Towards Practical Oblivious RAM. E. Stefanov, E. Shi, and D. Song.** We are using PathORAM as the Blackbox ORAM.

The code is constructed upon some important libraries:

* Google's `gRPC` library for remote process call (strongly recommended that the library is built from source).
* Google's `abseil` library for some advanced tools for C++ (If you build gRPC from source, then libabseil is automatically installed on your computer).
* `spdlog` for logging.
* `Libsodium` for cryptographic tools.

Important: all the source files must be compiled in C++17! Otherwise you will encounter `undefined reference` error (when linking against abseil-cpp). To enable CXX17, pass the following argument when you invoke `cmake`.

```shell
$ cmake -DCMAKE_CXX_STANDARD=17 <arguments>
```

# Run the binaries.

All the compiled binaries locate under `./build/bin` directory. Sample usage of the binaries is given as follows.

```shell
$ ./bin/server --address 0.0.0.0 --port 1234 --key_path ../key/server.key --crt_path ../key/server.crt --log_level 1
$ ./bin/client --address 127.0.0.1 --port 1234 --crt_path ../key/server.crt --block_num 65535 --bucket_size 4
```

# Use the PathORAM only.

You can run a PathORAM controller instance directly by

```cpp
using namespace partition_oram;

// Create the instance
const std::unique_ptr<PathOramController> path_oram_controller = 
	std::make_unique<PathOramController>(id, block_num, bucket_size));
	
// Create the gRPC stub and pass it to the controller.
path_oram_controller->SetStub(stub_);

// Initialize the oram.
Status status = path_oram_controller->InitOram();
if (status != Status::kOK) {
	logger->error("Unexpected error: {}", kErrorList[status]);
}

// Fill data. Assume you have created a vector of oram_block_t.
status = path_oram_controller->FillWithData(data);

// Read some data.
oram_block_t block;
status = path_oram_controller->ReadData(Operation::kRead, 0, &block, false);
```

For further interface reference please check the corresponding header files.

# Test automatically.

You can run the test by the following command:

```sh
chmod -x ./test.sh;
cd ./build;
../test.sh;
```

The output will be sent to the file in the `./build` directory with the current timestamp.
