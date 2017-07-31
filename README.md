# NoirSocks Cli

Command line interface program of NoirSocks

## Installation

### Linux/MacOSX

After cloning NoirSocks-Cli's repository, you must do :

     $ git submodule init

to checkout NoirSocks-Core.

Make sure you have boost & openssl & yaml-cpp installed (both header file and library), then just type:

    # default CXX is clang++
    $ make CXX=g++

Everything should be fine.

### Windows

Also make sure that boost & openssl & yaml-cpp is installed.
Then just put everything into your VisualStudio project and click Compile button.

## How to use

First, you need to wirte your own *config.yaml* (see config/config_example.yaml).
Then :

    $ ./noirsocks-cli your_config_file