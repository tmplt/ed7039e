# ED7039E - *Project in Industrial Computer Systems/Electronical Systems & Control Technology*
This repository will eventually constitute the full model,
any and all simulations related to, and implementation of the project for the D7039E and E7032E courses taken at Luleå Technical University during the first semester of 2020-21.

The project is to model, simulate, and implement a robot that,
via communication over the [Arrowhead framework](https://www.arrowhead.eu/arrowheadframework),
automatically picks up a processed object at the end of a conveyor belt and moves it elsewhere.
The deadline for this project is December 1st, 2020.

Throughout the development of this project, the report under `report/` will continuously be updated.
The report can be build my running `./report.nix`; the path to the compiled report will be printed.
A symbolic link to it is also be created under `result/`.

## Building and project setup
The bootable image for the Raspberry Pi can be built by executing `./mmc-image.nix`.
A convenience script is available for flashing a target block device:
```bash
$ ./build.sh /dev/mmcblk0 # or your equivalent
```
