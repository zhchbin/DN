Distributed Ninja
=======

[![Build Status](https://travis-ci.org/zhchbin/DN.svg?branch=master)](https://travis-ci.org/zhchbin/DN)

DN, which is short for __Distributed Ninja__, aims to finish the time consuming compilation job with several machines. Unlike [distcc][distcc] or [icecream][icecream], DN is based on [ninja][ninja], and supports platforms: `Linux` and `Windows`. Ninja is a small build system with a focus on speed, it is born from [@martine][martine]'s work on the `Chromium` browser project.

![](http://ww4.sinaimg.cn/large/7184df6bgw1epjm0z1b21j20bh0a0js3.jpg) 

*NOTE: currently this project is only a proof of concept.*

## Usage

* Master

    ```bash
    $ dn --working_dir=/path/to/your/project
    ```

* Slave

    ```bash
    $ dn --working_dir=/path/to/your/project --master=<master_ip>
    ```

## Build

1. Clone the repository and submodule.

    ```bash
    $ git clone https://github.com/zhchbin/DN.git
    $ cd DN && git submodule update --init
    ```

2. Install `gyp`.

    ```bash
    $ git clone https://github.com/svn2github/gyp
    $ cd gyp
    $ echo export PATH='$PATH':`pwd` >> ~/.bashrc && source ~/.bashrc
    ```bash

3. Install `ninja`.

    ```bash
    $ sudo apt-get install ninja-build
    $ echo export GYP_GENERATORS=ninja >> ~/.bashrc && source ~/.bashrc
    ```

4. Generate ninja build files and build

    ```bash
    $ sudo apt-get install libcurl4-openssl-dev
    $ gyp --depth=. -I base/src/build/common.gypi
    $ ninja -C out/Debug
    ```

## What I have done?

0. Pull out useful low-level ("base") routines routines from the Chromium open-source project at http://www.chromium.org, see [chromium-base][chromium-base].
1. RPC componment based on `Google/protobuf`, see `src/net` and `src/rpc`. Usage demo can be found [here][rpc-demo].
2. Integrate `ninja` code into the our thread model, and dispatch compilation command from master to slave.

## License

`DN`'s code in this repo uses the BSD license, see our `LICENSE` file.

[ninja]: http://martine.github.com/ninja/
[martine]: https://github.com/martine
[chromium-base]: https://github.com/zhchbin/chromium-base
[rpc-demo]: https://github.com/zhchbin/DN/tree/master/src/rpc/example
[distcc]: https://code.google.com/p/distcc/
[icecream]: https://github.com/icecc/icecream
