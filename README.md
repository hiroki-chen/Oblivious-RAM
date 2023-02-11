# Implementation of different Oblivious RAMs

This repository was created initially for evluating the performance of Partition-based Oblivious RAM proposed in **Towards Practical Oblivious RAM** on NDSS but now is making efforts to implement some common oblivious RAMs in the area of academic research.

Currently, we consider implmenting the following oblivious RAMs.

* (Implemented) Linear ORAM
* (Implemented) [(Modified) Basic square root ORAM](https://dl.acm.org/doi/pdf/10.1145/28395.28416)*
* (Implemented) [Path ORAM](https://eprint.iacr.org/2013/280.pdf)
* (Implemented) [Partition-based ORAM](https://www.ndss-symposium.org/wp-content/uploads/2017/09/04_4.pdf)
* (Implemented) [Path ORAM-based Oblivious dictionary](https://eprint.iacr.org/2014/185.pdf)
* (TODO) [Cuckoo-hashing-based ORAM](https://arxiv.org/pdf/1007.1259v1.pdf)

In future development, the project may be migrated to an equivalent Rust version with enclave support.

\* Note that the format-preserving-encryption may be buggy. It sometimes can cause `SIGSEGV`.

User guide is [here](https://oblivious-ram.gitbook.io/doc/).

## Prerequisites

To correctly build the repo, one must install the following libraries, and the compiler needs to support C++17. All the libraries are compiled in C++17, and you can pass it to `cmake` by

```shell
cmake -DCMAKE_CXX_STANDARD=17 <arguments>
```

* Google's `gRPC` library for remote process call. Follow the instructions in the `gRPC` GitHub repository and build it from source.
  
  Do NOT simply install the pre-built library via package managers like `apt` or `yum` because most of the time they offer a relatively deprecated version of `gRPC` library, and the dependency of `abseil` may also problemsome.
  
* Google's `abseil` library for some advanced tools for C++ (If you build gRPC from source, then `libabseil` is automatically installed on your computer); remember to build shared libraries.

* `spdlog` for logging. You also need to build it from source.

* `Libsodium` for cryptographic algorithms.

* `yaml-cpp` for config file parsing.

* `liblz4` for compression. (Can be installed via `sudo apt install liblz4-dev`)

## Build

Moved to user guide.

## Run the binaries

Moved to user guide.

## Providing the ORAM with Configuration Details

Moved to user guide.

## About the secret key negotiation

In fact, there is no need to negotiate a session key with the server, here the purpose of doing so is solely for the convenience of debugging and illustration of how to use `libsodium`. A session key will allow the server to decrypt the block on the cloud, and we can check if there is anything wrong.

## Claim

This project was launched as a personal research project and has nothing to do with the authors that originally proposed the ORAM constructions. Furthermore, the code quality and robustness are not guaranteed. Please refer to the licence for further information.
