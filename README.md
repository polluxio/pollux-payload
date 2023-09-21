<div align="center">
<img width="150" alt="Pollux Logo" src="./docs/images/pollux.jpg"><h1>Pollux</h1>
</div>


## Introduction
**Pollux is a Cloud orchestrator designed to distribute and manage complex algorithms, including NP-Hard and Complete problems, across multiple cloud-based machines.
With **Pollux, algorithms can seamlessly span the Cloud, ensuring synchronization and continuous communication among distributed components.
**Pollux has been designed in order to support multiple languages for Payload.

## Use cases
Currently, three **Pollux use cases can be found:
 - [Pollux payload example](): a simple test application deploying a configurable number of workers and showing
 - [Pollux PSO - Particle Swarm Optimization](): a [PSO](https://en.wikipedia.org/wiki/Particle_swarm_optimization) Pollux implementation.
 - [Pollux SAT](): a 
https://github.com/nuvulu/pollux-payload/assets/3635601/3fa89970-34cd-44c1-8359-9802c0719ff4

## Building and Installing
```bash
git submodule update --init --recursive
```

## Runtime environment
### Local docker
### Local
### Cloud
#### Qarnot
#### Azure

## Supported languages
Currently, only `C++` is supported. `Python` support could be easily added (some early tests have been done).

## Launcher
https://qarnot.com/documentation/sdk-python/installation.html



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
cd thirdparty
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
