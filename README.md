# **pqcBitcoin**

This repository is the result of a collaborative effort between [QBlock](https://github.com/QBlockQ) & [Qbits](https://github.com/QbitsCode), working together to build a future-proof version of Bitcoin Core that can withstand the potential threats posed by quantum computers. The integration of Post-Quantum Cryptography (PQC) algorithms into Bitcoin Core is a key initiative in ensuring that Bitcoin remains secure in the advent of quantum computing.

Quantum computing represents a breakthrough in computational capabilities, but it also poses a significant risk to current cryptographic techniques, including the elliptic curve cryptography (ECC) widely used in Bitcoin today. These classical encryption methods could potentially be broken by powerful quantum computers, leading to vulnerabilities in blockchain technologies. To mitigate this, the integration of quantum-resistant algorithms into Bitcoin Core is imperative.

## **Overview**

Quantum computers poses a potential threat to current cryptographic methods, including those used in Bitcoin, like elliptic curve cryptography (ECC). This project investigates incorporating post-quantum cryptographic algorithms to secure Bitcoin transactions and wallets in the event of future quantum attacks.

The goal is to make Bitcoin Core quantum-resistant by adopting algorithms that remain secure even in a world with powerful quantum computers.

## **Features**

- **Integration of PQC Algorithms**: Implements quantum-safe cryptographic algorithms alongside existing Bitcoin protocols.
- **Quantum-Resistant Wallets**: Modify Bitcoin Core's wallet functionality to utilize PQC keys for enhanced security.
- **Backward Compatibility**: Maintain compatibility with Bitcoin's current cryptographic algorithms for users not yet ready to switch to PQC.

## **Current PQC Algorithms Implemented**

### Group 1: Digital Signature Algorithms
- **SPHINCS+**: A stateless hash-based signature scheme with minimal security assumptions.
- **CRYSTALS-Dilithium**: A lattice-based digital signature scheme.
- **FALCON**: A fast lattice-based digital signature scheme optimized for small signatures.
- **SQIsign**: An isogeny-based signature scheme.

### Group 2: Key Encapsulation Mechanisms (KEM)
- **Kyber**: A lattice-based key encapsulation mechanism (KEM) for public-key encryption.
- **FrodoKEM**: A key encapsulation mechanism based on the hardness of the learning with errors (LWE) problem.
- **NTRU**: A lattice-based public-key cryptosystem designed to be secure against quantum computers.

These algorithms are integrated into the Bitcoin codebase in a way that ensures both backward and forward compatibility with existing Bitcoin infrastructure. Group 1 algorithms handle digital signatures for transaction signing, while Group 2 algorithms provide secure key exchange mechanisms for encrypted communications between nodes and wallets.

## **Post-Quantum Cryptography Support**

This fork of Bitcoin Core implements post-quantum cryptography (PQC) to provide protection against quantum computer attacks while maintaining backward compatibility with the existing Bitcoin network.

### **Implemented PQC Features**

#### Key Management System
- HybridKey class for managing both classical and PQC keys
- Integration with Bitcoin's existing key management system
- Support for hybrid key generation and signing

#### Supported PQC Algorithms
##### Digital Signatures (Group 1)
- **SPHINCS+**: Stateless hash-based signatures
- **CRYSTALS-Dilithium**: Lattice-based signatures
- **FALCON**: Fast lattice-based signatures
- **SQIsign**: Isogeny-based signatures

##### Key Encapsulation (Group 2)
- **Kyber**: Lattice-based KEM
- **FrodoKEM**: LWE-based KEM
- **NTRU**: Lattice-based cryptosystem

#### Configuration Options
Enable PQC features using command-line arguments:
```bash
bitcoind -pqc=1 -pqcalgo=kyber,ntru -pqcsig=sphincs,dilithium -pqchybridsig=1
```

Available options:
- `-pqc=0|1`: Enable/disable all PQC features (default: 1)
- `-pqchybridkeys=0|1`: Enable/disable hybrid key generation (default: 1)
- `-pqchybridsig=0|1`: Enable/disable hybrid signatures (default: 1)
- `-pqcalgo=algo1,algo2,...`: Specify enabled KEM algorithms (default: kyber,frodo,ntru)
- `-pqcsig=sig1,sig2,...`: Specify enabled signature schemes (default: sphincs,dilithium,falcon,sqisign)

For detailed documentation on PQC features, see [doc/pqc.md](doc/pqc.md).

## **Installation**

To build and test the PQC-enabled Bitcoin Core:

### Build Requirements

* GCC 7+ or Clang 8+
* CMake 3.13+
* OpenSSL 1.1+
* Boost 1.70+
* Additional PQC-specific requirements:
  - PQCRYPTO-NIST library (for Kyber and NTRU)
  - FrodoKEM reference implementation

### Build Steps

1. Install dependencies:
```bash
# Ubuntu/Debian
sudo apt-get install build-essential libtool autotools-dev automake pkg-config bsdmainutils python3
sudo apt-get install libevent-dev libboost-dev libboost-system-dev libboost-filesystem-dev
sudo apt-get install libsqlite3-dev libminiupnpc-dev libnatpmp-dev libzmq3-dev
sudo apt-get install libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools

# Install PQC dependencies
git clone https://github.com/PQClean/PQClean.git
cd PQClean && make
sudo make install
```

2. Clone and build:
```bash
git clone https://github.com/QBlockQ/pqc-bitcoin.git
cd pqc-bitcoin
./autogen.sh
./configure --with-pqc
make
make check  # Run tests
```

3. Run with PQC features:
```bash
./src/bitcoind -pqc=1 -pqcalgo=kyber,ntru -pqchybridsig=1
```

## Run PQC Bitcoin Core

After building Bitcoin Core, you can run the PQC-enabled Bitcoin Core in regtest mode for testing
```bash
./src/bitcoind -regtest
```

## Testing PQC Bitcoin

The test framework ensures that the PQC algorithms integrate smoothly with Bitcoin Core’s existing features.
For detailed testing instructions, refer to the Bitcoin Test Suite.

## To run tests:
```bash
make check
```

## Validate PQC Key Generation: 

Test key generation using PQC algorithms

```bash
./src/bitcoin-cli pqc-keygen
```

## Developer note — macOS local setup

During local development I needed to install a few macOS packages via Homebrew to run `./autogen.sh`, `./configure` and perform a short smoke build. These are the packages I used and why:

- autoconf, automake, libtool, m4, pkg-config: the GNU autotools chain (provides `aclocal`, `autoreconf`, etc.) required by `./autogen.sh`.
- gettext: provides `xgettext`/`lupdate` used by translation-related build steps.
- boost (Homebrew): required C++ libraries/headers (Boost >= 1.73 required by the project).
- libevent: required by the build (libevent >= 2.1.8).

For a full, up-to-date macOS build guide (including optional dependencies), see `doc/build-osx.md`. The snippet below is one known-good configuration that worked locally.

Reproduce the exact steps I used (copy/paste):

```bash
# Install autotools, helper tools and runtime deps
brew install autoconf automake libtool m4 pkg-config gettext boost libevent

# Ensure Homebrew bins are on PATH
export PATH="$(brew --prefix)/bin:$PATH"

# If Boost was installed to a cellar path, point configure at it.
# The version can change over time; this uses the currently installed
# Homebrew Boost version dynamically.
BOOST_VERSION="$(brew list --versions boost | awk '{print $2}')"
export BOOST_ROOT="$(brew --cellar boost)/${BOOST_VERSION}"
export CPPFLAGS="-I${BOOST_ROOT}/include"
export LDFLAGS="-L${BOOST_ROOT}/lib"

# Regenerate build system and configure (non-Qt GUI build)
./autogen.sh
./configure --with-pqc --with-gui=no

# Short smoke build
make -j2
```

Historical note (debugging only, not required for normal builds)

- During early debugging, temporary Makefile stubs were created under `src/qt` and `src/test` to help `autoreconf` succeed when some auxiliary files were missing. These files were never meant to be part of the permanent repository and have since been removed. You do **not** need to create or keep any `src/qt/Makefile*` or `src/test/Makefile*` files for a normal build.

Decision note

- Keep `doc/build-osx.md` as the canonical source for macOS build instructions. This developer note is a lightweight, concrete example of one working setup. If you change dependencies or toolchain versions, prefer updating `doc/build-osx.md` and then, if helpful, refreshing this example.

## Contributions

We welcome contributions to make Bitcoin Core quantum-resistant. Feel free to fork this repository and submit pull requests.

For discussions and issues, please open an issue on the GitHub repository.

## License

This project is licensed under the MIT License. **Made with love by [QBlock](https://github.com/QBlockQ) & [Qbits](https://github.com/QbitsCode))** 💖

