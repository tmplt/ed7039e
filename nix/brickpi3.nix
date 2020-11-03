{ pkgs, lib, ... }:

{
  # Linux is not aware of the SPI peripheral on the Raspberry Pi.
  # This device tree overlay describes the peripheral, and makes
  # sure that the appropriate drivers are loaded on boot.
  hardware.deviceTree = {
    enable = true;
    filter = "*rpi*.dtb";
    overlays = [{
      name = "spi";
      dtsFile = ./dtso/spi.dts;
    }];
  };
}
