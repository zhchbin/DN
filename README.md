Distributed Ninja
=======

[![Build Status](https://travis-ci.org/zhchbin/DN.svg?branch=master)](https://travis-ci.org/zhchbin/DN)

[Ninja][ninja] is a small build system with a focus on speed, it is born from [@martine][martine]'s work on the `Chromium` browser project. And DN, which is short for __Distributed Ninja__, can finish the time consuming compilation job with several machines.

*NOTE: currently this project is only a proof of concept. It just work, but the performance didn't tested yet.*

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
