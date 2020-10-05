{ pkgs, lib, ... }:

{
  # `sd-image-aarch64.nix` configures the UART pins for the Linux
  # console, but we need them for the MDEK1001. We don't use the
  # Linux console anyway, as we use SSH instead. Free the pins up.
  #boot.kernelParams = lib.mkForce [ "cma=32M" ];

  environment.systemPackages = with pkgs; [
    minicom # for debugging/prototyping purposes
  ];

  # hardware.deviceTree = {
  #   overlays = [{
  #     name = "uart";
  #     dtsFile = ./dtso/uart.dts;
  #   }];
  # };
}
