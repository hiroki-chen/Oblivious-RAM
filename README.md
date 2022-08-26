# Implementation of different Oblivious RAMs

This repository was created initially for evluating the performance of Partition-based Oblivious RAM proposed in **Towards Practical Oblivious RAM** on NDSS but now is making efforts to implement some common oblivious RAMs in the area of academic research.

Currently, we consider implmenting the following oblivious RAMs.

* (Implemented) Linear ORAM
* (Ongoing) [Basic sqaure root ORAM](https://dl.acm.org/doi/pdf/10.1145/28395.28416)
* (Implemented) [Path ORAM](https://eprint.iacr.org/2013/280.pdf)
* (Implemented) [Partition-based ORAM](https://www.ndss-symposium.org/wp-content/uploads/2017/09/04_4.pdf)
* (TODO) [Path ORAM-based Oblivious dictionary](https://eprint.iacr.org/2014/185.pdf)
* (TODO) [Cuckoo-hashing-based ORAM](https://arxiv.org/pdf/1007.1259v1.pdf)

In future development, the project may be migrated to an equivalent Rust version with enclave support.

## Prerequisites:

To correctly build the repo, one must install the following libraries, and the compiler needs to support C++17. All the libraries are compiled in C++17, and you can pass it to `cmake` by

```shell
$ cmake -DCMAKE_CXX_STANDARD=17 <arguments>
```

* Google's `gRPC` library for remote process call. Follow the instructions in the `gRPC` GitHub repository and build it from source.
  
  Do NOT simply install the pre-built library via package managers like `apt` or `yum` because most of the time they offer a relatively deprecated version of `gRPC` library, and the dependency of `abseil` may also problemsome.
  
* Google's `abseil` library for some advanced tools for C++ (If you build gRPC from source, then `libabseil` is automatically installed on your computer); remember to build shared libraries.

* `spdlog` for logging. You also need to build it from source.

* `Libsodium` for cryptographic algorithms.

* `liblz4` for compression. (Can be installed via `sudo apt install liblz4-dev`)

## Build

Clone the repo first.

```shell
$ git clone --recursive https://github.com/hiroki-chen/Oblivious-RAM.git
```

Build `spdlog`.

```shell
$ cd Oblivious-RAM/spdlog
$ mkdir build && cd build
$ cmake .. -DCMAKE_CXX_STANDARD=CXX17 && make -j && sudo make install
```

Build `libsodium`.

```shell
$ cd Oblivious-RAM/spdlog
$ ./configure && make -j && sudo make install
```

Build `gRPC` and its dependencies.

```shell
$ cd ~ && git clone -b v1.49.x https://github.com/grpc/grpc.git
$ cd grpc
$ git submodule update --init
$ mkdir -p ./cmake/build && cd build
$ cmake .. -DCMAKE_CXX_STANDARD=CXX17 -DgRPC_INSTALL=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=<installation/path>
$ make -j && sudo make install
```

Finally, build the ORAM.

```shell
$ mkdir build && cd build
$ cmake .. && make -j
```

## Run the binaries.

All the compiled binaries are located under `./build/bin` directory. Sample usage of the binaries is given as follows.

```shell
$ ./bin/server --address 0.0.0.0 --port 1234 --key_path ../key/sslcred.key --crt_path ../key/sslcred.crt --log_level 2
$ ./bin/client --address localhost --port 1234 --crt_path ../key/sslcred.crt --block_num 65535 --bucket_size 4
```

If you are using domain other than `localhost`. You may need to re-generate the SSL certificate and key by `openssl`.

## Path ORAM Example

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
  logger->error("Unexpected error: {}", kErrorList.at(status));
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

# About the secret key negotiation.
In fact, there is no need to negotiate a session key with the server, here the purpose of doing so is solely for the convenience of debugging and illustration of how to use `libsodium`. A session key will allow the server to decrypt the block on the cloud, and we can check if there is anything wrong.

# Claim
This project was intiated as a personal research project and has nothing to do with the authors that originally proposed the ORAM constructions. Furthermore, the code quality and robustness are not guaranteed. Please refer to the licence for further information.
