<div align="center">
<img width="150" alt="Pollux Logo" src="./docs/images/pollux-logo.jpg">
</div>

---

# Pollux

**Pollux** is a Message Passing Cloud orchestrator designed to: 

- distribute and manage complex algorithms, including NP-Hard and Complete problems, across multiple cloud-based machines.
- span algorithms on the Cloud, ensuring synchronization and continuous communication among distributed components.
- support different payload codes.
- support different payload multiple programming languages.

![Pollux Schema](./docs/images/pollux-general.png)

:information_desk_person: If you have any questions, send us a [mail](mailto:christophe.alexandre@getpollux.io).

:star: If you find Pollux interesting, and would like to stay up-to-date, consider starring this repo to help spread the word.

---

## Motivation

In today's world many computational problems from network routing to semiconductors to AI are becoming more and more complex.
We believe that the next path for optimization of such complex grand scale computation is to leverage the massive parallelization the cloud can offer. 
We believe that this step was not fully taken as there is no real cloud suitable framework for this kind of computation and most of the ones which were used for this purpose were actually meant for supercomputers and not the cloud.
Our goal is to provide such a framework that will be both designed for cloud architecture and will also be simple to use with a clear interface for engineers from any field to use in order to parallelize any algorithm without being cloud experts.
Our goal is not to treat the cloud as a super computer, it is not. Our goal is to virtualize a super computer on the cloud and finally open these capabilities to a wider audience. . 


![Pollux PSO](./docs/images/pollux-PSO.gif)

<div align="right">[ <a href="#introduction">↑ Back to top ↑</a> ]</div>

---

## Use Cases

Currently, the following **Pollux** use cases can be found:

- [Pollux payload example](https://github.com/polluxio/pollux-payload/blob/main/src/c%2B%2B/examples/test): a simple test application deploying a configurable number of workers and showing
- [Pollux PSO - Particle Swarm Optimization](https://github.com/polluxio/pollux-payload/tree/main/src/c%2B%2B/examples/pso): a [PSO](https://en.wikipedia.org/wiki/Particle_swarm_optimization) Pollux implementation.
- [Pollux SAT](https://github.com/polluxio/pollux-sat): a [SAT](https://en.wikipedia.org/wiki/Boolean_satisfiability_problem) Cloud application using a mix of divide&conquer and multi-threaded portfolios techniques. More details on the project page.

<div align="right">[ <a href="#introduction">↑ Back to top ↑</a> ]</div>

---

## Architecture

**Pollux** is both:

- a distributed Payload API (C++ for the moment, more to come in the future)
- and a Cloud orchestrator
The overall architecture can be summarized in picture below. 
![Pollux architecture](./docs/images/pollux-architecture.png)

<div align="right">[ <a href="#introduction">↑ Back to top ↑</a> ]</div>

---

## Building and Installing

### Getting sources

```bash
# First clone the repository
git clone https://github.com/polluxio/pollux-payload
# go inside it and init submodules
cd pollux-payload
git submodule update --init --recursive
```
At this point, you are good to test pollux, locally if you have docker installed or on the Cloud if you have an account.

## Running

Running modes are managed through the **Pollux** [launcher](#pollux-launcher) '-m' option. Following modes are supported:

### Local
If you have **Docker** installed on your system, we suggest to directly use the [local docker](#local-docker) mode.

To use this, mode **Pollux** binaries need to be installed locally. We provide binaries compiled for Ubuntu in pollux-payload release.

If you face any compatibility issue or you need binaries for another Linux version,
please contact [us](mailto:christophe.alexandre@getpollux.io). 
Once you have downloaded them (`pollux` and `zebulon`).
Untar the archive in a directory and set a $POLLUX_INSTALL environment variable pointing to that directory.

```bash
mkdir pollux_install
cd pollux_install
tar xvzf pollux.tar.gz
export POLLUX_INSTALL pollux_install
```

Once this is done, launch in local mode:

```bash
# Following command will launch 4 workers
python3 <pollux_payload_sources>/src/launcher/pollux.py -p <path_to_payload> -m local -n 4
```

<div align="right">[ <a href="#introduction">↑ Back to top ↑</a> ]</div>

---

### Local docker
This is the easiest mode to use to get started and the best step to prepare for Cloud deployment.

Use the pollux test [image](https://hub.docker.com/repository/docker/polluxio/pollux-payload-examples/general) to quickly test pollux.
For this purpose, use the following pollux launcher command:

```bash
# Following command will launch 4 workers
# -c : docker image containing the payload to run as a container.
# -p : path to payload in payload container to run (usually payloads are located in /root)
python3 <pollux-payload-sources>/pollux.py -m local_docker -c polluxio/pollux-payload-examples -p /root/pollux-payload-test -n 4 
```

Following command will launch the **PSO** example using 8 workers:

```bash
python3 <pollux-payload-sources>~/pollux.py -m local_docker -c polluxio/pollux-payload-examples -p /root/pollux-payload-pso -n 8 
```

<div align="right">[ <a href="#introduction">↑ Back to top ↑</a> ]</div>

---

### Cloud - Qarnot

[Qarnot](https://qarnot.com) is a Cloud provider proposing a unique approach to cloud computing by utilizing the wasted heat generated by computer servers to heat buildings. 

**Qarnot** offers free trial credits in a couple of clicks. To try deployment on **Qarnot**, first create an account on this [page](https://tasq.qarnot.com/login/).

Then retrieve your access token, save it locally ('qarnot.conf) and then launch using following command:

```bash
#Launching on Qarnot with 3 workers
python3 <pollux-payload-sources>/pollux.py -m qarnot -c polluxio/pollux-payload-examples -p /root/pollux-payload-pso -n 3 
```

<div align="right">[ <a href="#introduction">↑ Back to top ↑</a> ]</div>

---

## Pollux Launcher

To launch Pollux applications, a launcher `Python` script is provided [here](https://github.com/polluxio/pollux-payload/blob/main/src/launcher/pollux.py).

Most convenient way is to create a `Python` virtual environment. With Python 3:

```bash
apt-get install python3-venv
pip3 install virtualenv
```

Then in a project directory, create an environment:

```bash
python3 -m venv pollux
```

Then activate it:

```bash
. pollux/bin/activate
```

Now that the environment has been created and launched, you can install the following `Python` libraries:

```bash
pip install pyyaml
#to use local Docker pollux mode
pip install docker
#To use Qarnot Pollux mode 
pip install qarnot
```

<div align="right">[ <a href="#table-of-contents">↑ Back to top ↑</a> ]</div>

---

## Write your own Pollux payload

### Supported Payload languages

Currently, only `C++` is supported. `Python` support for instance could be easily added (some early tests have been done).

### C++

First install following dependencies, On Ubuntu for instance:

```bash
sudo apt-get install cmake
sudo apt-get install g++
sudo apt-get install protobuf-compiler-grpc
sudo apt-get install libgrpc++-dev
```

Copy relevant files:

```bash
#create a thirdparty dir
mkdir thirdparty
cd thirdparty
git submodule add https://github.com/polluxio/pollux-payload
```

### Dockerfile

```Dockerfile
```
