# CS499 - Chirp Project Phase 1

## Aliya Petranik

petranik@usc.edu

---

## Setup the VM

#### Download Vagrant VM

- Download Vagrant VM:
  https://www.vagrantup.com/downloads.html

#### Verify your installation with

```
$ vagrant

Usage: vagrant [options] <command> [<args>]

  -v, --version                    Print the version and exit.
  -h, --help                       Print this help.

# ...
```

#### Project setup

Create a new directory for this project

    $ mkdir aliya_petranik_499_chirp && cd aliya_petranik_499_chirp

#### Initialize VM

    $ vagrant init generic/ubuntu1804

- This could create a Vagrantfile in your current directory
- Check that your Vagrantfile says

```
config.vm.box = "generic/ubuntu1804"
```

#### Installing a Box

    vagrant box add generic/ubuntu1804

#### Starting up the VM and SSHing in

```
vagrant up
vagrant ssh
```

## Configuring VM for Project

#### Install make

```
sudo apt-get -y update
sudo apt-get -y install build-essential autoconf libtool pkg-config
sudo apt-get -y install clang libc++dev
sudo apt-get install clang-6.0
sudo apt-get -y install make cmake
```

#### Install GRPC

```
git clone -b $(curl -L https://grpc.io/release) https://github.com/grpc/grpc
cd grpc
git submodule update --init
make && sudo make install
```

_(this takes a while)_

```
cd third_party/protobuf
make && sudo make install
cd ~
```

#### Install gflags and glog and gtest

```
sudo apt-get -y install libgflags-dev
sudo apt-get -y install libgoogle-glog-dev
sudo apt-get -y install libgtest-dev
sudo apt-get install cmake cd /usr/src/gtest sudo cmake CMakeLists.txt sudo make sudo cp *.a /usr/lib
```

## Getting the Project

```
sudo apt install git
git clone https://github.com/apetranik/499-petranik.git
cd 499-petranik
git checkout FinalPR-phase1
git pull origin FinalPR-phase1
make clean && make
```

Open minimum 4 shells for testing all functionality

- Go to folder with Vagrantfile and use, `vagrant ssh` as you did the first time

## Running Project

_Both backend and service must be running to use chirp CLI_

#### Start backend:

    $ ./run_server

- Note: closing this will resete key value store

#### Start service layer:

    $ ./run_service

- Note: if you quit the service layer, the backend key-value store will still be saved

## Chirp Client command reference

- All commands start with `./chirp`...
- You can not perform more than one of these commands at once

  ### **Register**

      $ ./chirp --register <username>

  (_user must be registered to perform any other commands_)

  ### **Chirp**

      $ ./chirp --user <username> --chirp <chirp text>

  ### **Reply**

      $ ./chirp --user <username> --chirp <chirp text> --reply <reply chirp id>

  ### **Follow**

      $ ./chirp --user <username> --follow <username_to_follow>

  ### **Read**

      $ ./chirp --read <chirp_id>

  ### **Monitor**

      $ ./chirp --user <username> --monitor

#### Run Tests:

    $ ./key_value_store_unit_tests
    $ ./service_layer_unit_tests
