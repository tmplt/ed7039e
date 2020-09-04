# ED7039E - *Project in Industrial Computer Systems/Electronical Systems & Control Technology*
This repository will eventually constitute the full model,
any and all simulations related to, and implementation of the project for the D7039E and E7032E courses taken at Luleå Technical University during the first semester of 2020-21.

The project is to model, simulate, and implement a robot that,
via communication over the [Arrowhead framework](https://www.arrowhead.eu/arrowheadframework),
automatically picks up a processed object at the end of a conveyor belt and moves it elsewhere.
The deadline for this project is December 1st, 2020.

Throughout the development of this project, the report under `report/` will continuously be updated.

## Building and project setup
The boot image of the Raspberry Pi 3 can be built via
```bash
$ # TODO: explain how to setup aarch64-linux emulation for cross-compilation
$ # TODO: explain how to write a local-secrets.nix
$ ./build.sh /dev/mmcblk0 # or your equivalent
```
