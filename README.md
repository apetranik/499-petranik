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

- `$ mkdir aliya_petranik_499_chirp && cd aliya_petranik_499_chirp`

#### Initialize VM

`$ vagrant init ubuntu/xenial`

- This could create a Vagrantfile in your current directory
- Check that your Vagrantfile says

```
config.vm.box = "ubuntu/xenial64"
```

#### Installing a Box

    $ vagrant box add ubuntu/xenial64

#### Starting up the VM and SSHing in

```
$ vagrant up
$ vagrant ssh`
```

## Configuring VM for Project

#### Install make

    $ sudo apt install make

#### Install gflags and glog

```
$ sudo apt-get install libgflags-dev
$ sudo apt install libgoogle-glog-dev
```

## Getting the Project

```
$ sudo apt install git
$ git clone https://github.com/apetranik/499-petranik.git
$ cd 499-petranik
$ make clean && make
```

Open minimum 4 shells for testing all functionality

- Go to folder with Vagrantfile and use, `vagrant ssh`

## Running Project

_Both backend and service must be running to use chirp CLI_

#### Start backend:

    $ ./backend_server

- Note: closing this will resete key value store

#### Start service layer:

    $ ./service

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
