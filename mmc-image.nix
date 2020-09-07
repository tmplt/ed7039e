# TODO: pin nixpkgs, probably via nixpkgs submodule, or at least current stable
# TODO: remove cruft we do not need: nixos-install, ZFS, etc.
# TODO: enable interaction via UART.
# TODO: add a document header to this file, explaining that this is a modified installer env.
# TODO: change hostname, replace nixos user with root

{ pkgs, lib, ... }: {
  imports = [
    <nixpkgs/nixos/modules/installer/cd-dvd/sd-image-aarch64.nix>
  ];

  # We can only flash an uncompressed image.
  # Additionally, compression whilst emulating the platform takes a looong time.
  sdImage.compressImage = false;

  # Latest release of major 5 doesn't always play ball with the hardware.
  # Relase 4.19 is stable and "battle-tested".
  # See <https://github.com/NixOS/nixpkgs/issues/82455>.
  boot.kernelPackages = pkgs.linuxPackages_4_19;

  # Automatically connect to eduroam via wlan0 for remote access.
  networking.wireless = let es = (import ./local-secrets.nix).eduroam; in {
    enable = true;
    interfaces = [ "wlan0" ];
    networks."eduroam".auth = ''
      key_mgmt=WPA-EAP
      eap=PEAP
      proto=RSN
      identity="${es.identity}"
      password="${es.password}"
      phase2="auth-MSCHAPV2"
    '';
  };
  systemd.services.wpa_supplicant.wantedBy = lib.mkOverride 10 [ "default.target" ];

  # Ensure a correct system clock.
  services.timesyncd.enable = true;
  time.timeZone = "Europe/Stockholm";

  # Enables us to inspect core dumps in a centralized manner (incl. timestamps)
  systemd.coredump.enable = true;

  services.sshd.enabled = true;
  # sshd has an empty `wantedBy` in the installer derivation, but we need it;
  # override the override.
  systemd.services.sshd.wantedBy = lib.mkOverride 40 [ "multi-user.target" ];
  environment.etc."id_rsa" = {
    source = ./id_rsa;
    user = "nixos";
    mode = "0600";
  };
  users.extraUsers.nixos.openssh.authorizedKeys.keys = with (import ./ssh-keys.nix); [
    tmplt
  ];
  systemd.services.ssh-port-forward = {
    description = "forwarding reverse SSH connection to a known bastion";
    wantedBy = [ "multi-user.target" ];
    after = [ "network.target" "nss-lookup.target" ];
    serviceConfig = let bastion = (import ./config.nix).bastion; in {
      ExecStart = ''
        ${pkgs.openssh}/bin/ssh -o StrictHostKeyChecking=no \
          -TNR ${bastion.socketPath}:localhost:22 ${bastion.user}@${bastion.host} \
          -i /etc/id_rsa
      '';
      
      StandardError = "journal";
      Type = "simple";

      # Upon exit, try to establish a new connection every 5s.
      Restart = "always";
      RestartSec = "5s";
      StartLimitIntervalSec = "0";
    };
  };
}
