
> formerly known as _cpp_bearchain_ project.


## Building from source

### Get the source code

Git and GitHub are used to maintain the source code. Clone the repository by:

```shell
git clone --recursive https://github.com/BaerChain/cpp-lsac.git
cd brcd
```

The `--recursive` option is important. It orders git to clone additional
submodules to build the project.
If you missed `--recursive` option, you are able to correct your mistake with command
`git submodule update --init`.

### Install CMake

CMake is used to control the build configuration of the project. Latest version of CMake is required
(at the time of writing [3.9.3 is the minimum](CMakeLists.txt#L5)).
We strongly recommend you to install CMake by downloading and unpacking the binary
distribution  of the latest version available on the
[**CMake download page**](https://cmake.org/download/).

The CMake package available in your operating system can also be installed
and used if it meets the minimum version requirement.

> **Alternative method**
>
> The repository contains the
[scripts/install_cmake.sh](scripts/install_cmake.sh) script that downloads
> a fixed version of CMake and unpacks it to the given directory prefix.
> Example usage: `scripts/install_cmake.sh --prefix /usr/local`.

### Install dependencies (Windows)

We provide prebuilt dependencies to build the project. Download them
with the [scripts\install_deps.bat](scripts/install_deps.bat) script.

```shell
scripts\install_deps.bat
```

### Build

Configure the project build with the following command to create the
`build` directory with the configuration.

```shell
mkdir build; cd build  # Create a build directory.
cmake ..               # Configure the project.
cmake --build .        # Build all default targets.
```

On **Windows** Visual Studio 2015 is required. You should generate Visual Studio
solution file (.sln) for 64-bit architecture by adding
`-G "Visual Studio 14 2015 Win64"` argument to the CMake configure command.
After configuration is completed, the `brcd.sln` can be found in the
`build` directory.

```shell
cmake .. -G "Visual Studio 14 2015 Win64"
```
On **Ubuntu 18.04** 
```
    sudo apt-get install -y 
                        libjsonrpccpp-dev 
                        libjsonrpccpp-tools 
                        librocksdb5.8 
                        libscrypt-dev 
                        libmicrohttpd-dev
```
#### Common Issues Building on Windows
##### LINK : fatal error LNK1158: cannot run 'rc.exe'
Rc.exe is the [Microsoft Resource Compiler](https://docs.microsoft.com/en-us/windows/desktop/menurc/resource-compiler). It's distributed with the [Windows SDK](https://developer.microsoft.com/en-US/windows/downloads/windows-10-sdk) and is required for generating the Visual Studio solution file. It can be found in the following directory: ```%ProgramFiles(x86)%\Windows Kits\<OS major version>\bin\<OS full version>\<arch>\```

If you hit this error, adding the directory to your path (and launching a new command prompt) should fix the issue.


## Usage
*Note: The following is the output of ```brcd.exe -h [--help]```*

```
NAME:
   brcd 1.4.0
USAGE:
   brcd [options]

WALLET USAGE:
   account list                                List all keys available in wallet
   account new                                 Create a new key and add it to wallet
   account update [<uuid>|<address> , ... ]    Decrypt and re-encrypt keys
   account import [<uuid>|<file>|<secret-hex>] Import keys from given source and place in wallet
   wallet import <file>                        Import a presale wallet

CLIENT MODE (default):
  --mainnet                               Use the main network protocol
  --ropsten                               Use the Ropsten testnet
  --private <name>                        Use a private chain
  --test                                  Testing mode; disable PoW and provide test rpc interface
  --config <file>                         Configure specialised blockchain using given JSON information

  -o [ --mode ] <full/peer>               Start a full node or a peer node (default: full)

  --ipc                                   Enable IPC server (default: on)
  --ipcpath <path>                        Set .ipc socket path (default: data directory)
  --no-ipc                                Disable IPC server
  --admin <password>                      Specify admin session key for JSON-RPC (default: auto-generated and printed at start-up)
  -K [ --kill ]                           Kill the blockchain first
  -R [ --rebuild ]                        Rebuild the blockchain from the existing database
  --rescue                                Attempt to rescue a corrupt database

  --import-presale <file>                 Import a pre-sale key; you'll need to specify the password to this key
  -s [ --import-secret ] <secret>         Import a secret key into the key store
  -S [ --import-session-secret ] <secret> Import a secret session into the key store
  --master <password>                     Give the master password for the key store; use --master "" to show a prompt
  --password <password>                   Give a password for a private key

CLIENT TRANSACTING:
  --ask <wei>            Set the minimum ask gas price under which no transaction will be mined (default: 20000000000)
  --bid <wei>            Set the bid gas price to pay for transactions (default: 20000000000)
  --unsafe-transactions  Allow all transactions to proceed without verification; EXTREMELY UNSAFE

CLIENT MINING:
  -a [ --address ] <addr>         Set the author (mining payout) address (default: auto)
  -m [ --mining ] <on/off/number> Enable mining; optionally for a specified number of blocks (default: off)
  --extra-data arg                Set extra data for the sealed blocks

CLIENT NETWORKING:
  -x [ --peers ] <number>         Attempt to connect to a given number of peers (default: 11)
  --peer-stretch <number>         Give the accepted connection multiplier (default: 7)
  --public-ip <ip>                Force advertised public IP to the given IP (default: auto)
  --listen-ip <ip>(:<port>)       Listen on the given IP for incoming connections (default: 0.0.0.0)
  --listen <port>                 Listen on the given port for incoming connections (default: 30303)
  -r [ --remote ] <host>(:<port>) Connect to the given remote host (default: none)
  --port <port>                   Connect to the given remote port (default: 30303)
  --network-id <n>                Only connect to other hosts with this network id
  --peerset <list>                Space delimited list of peers; element format: type:publickey@ipAddress[:port]
                                          Types:
                                          default     Attempt connection when no other peers are available and pinning is disabled
                                          required    Keep connected at all times

  --no-discovery                  Disable node discovery; implies --no-bootstrap
  --pin                           Only accept or connect to trusted peers

BENCHMARKING MODE:
  -M,--benchmark               Benchmark for mining and exit
  --benchmark-warmup <seconds> Set the duration of warmup for the benchmark tests (default: 3)
  --benchmark-trial <seconds>  Set the duration for each trial for the benchmark tests (default: 3)
  --benchmark-trials <n>       Set the number of trials for the benchmark tests (default: 5)

MINING CONFIGURATION:
  -C,--cpu                   When mining, use the CPU
  -t, --mining-threads <n>   Limit number of CPU/GPU miners to n (default: use everything available on selected platform)
  --current-block            Let the miner know the current block number at configuration time. Will help determine DAG size and required GPU memory
  --disable-submit-hashrate  When mining, don't submit hashrate to node

IMPORT/EXPORT MODES:
  -I [ --import ] <file>      Import blocks from file
  -E [ --export ] <file>      Export blocks to file
  --from <n>                  Export only from block n; n may be a decimal, a '0x' prefixed hash, or 'latest'
  --to <n>                    Export only to block n (inclusive); n may be a decimal, a '0x' prefixed hash, or 'latest'
  --only <n>                  Equivalent to --export-from n --export-to n
  --format <binary/hex/human> Set export format
  --dont-check                Prevent checking some block aspects. Faster importing, but to apply only when the data is known to be valid
  --download-snapshot <path>  Download Parity Warp Sync snapshot data to the specified path
  --import-snapshot <path>    Import blockchain and state data from the Parity Warp Sync snapshot

VM OPTIONS:
  --vm <name>|<path> (=legacy) Select VM implementation. Available options are: interpreter, legacy.

LOGGING OPTIONS:
  -v [ --log-verbosity ] <0 - 4>        Set the log verbosity from 0 to 4 (default: 2).
  --log-channels <channel_list>         Space-separated list of the log channels to show (default: show all channels).
  --log-exclude-channels <channel_list> Space-separated list of the log channels to hide.

GENERAL OPTIONS:
  -V [ --version ]        Show the version and exit
  -h [ --help ]           Show this help message and exit
```


## Testing
Details on how to run and debug the tests can be found


## Documentation


## License

