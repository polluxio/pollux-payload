# polluxapp
Pollux application example
## Introduction
polluxapp provides examples of applications using the Pollux tool. 
## Pollux installation
## Pollux testing

## Pollux application writing
Two langages are currently supported: C++ and Python. For each of them a sketch application is provided.
Those application are provided with 

### C++
Ubuntu dependencies:
```bash
sudo apt-get install cmake
sudo apt-get install g++
sudo apt-get install libboost-dev
sudo apt-get install protobuf-compiler-grpc
sudo apt-get install libgrpc++-dev
```
Copy relevant files:
```bash
#create a thirdparty dir
mkdir thirdparty
git submodule add https://github.com/gabime/spdlog
#checkout latest release
cd spdlog
git checkout ad0e89c
cd ..
git submodule add https://github.com/p-ranav/argparse
cd argparse
git checkout 997da92
```

### Python

## Pollux modes
### Fully asynchronous
Netlist Logic Optimization is a fully asynchronous application example.
### Asynchronous with 
Heuristics such as partitioning or simulated annealing are good candidates for this mode.
