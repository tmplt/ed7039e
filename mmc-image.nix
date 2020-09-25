# TODO: remove cruft we do not need: nixos-install, ZFS, etc.
# TODO: add a document header to this file, explaining that this is a modified installer env.

{ pkgs, lib, config, ... }:

{
  imports = [
    <nixpkgs/nixos/modules/installer/cd-dvd/sd-image-aarch64.nix>
    ./modules/brickpi3.nix
  ];

  # We can only flash an uncompressed image.
  # Additionally, compression whilst emulating the platform takes a looong time.
  sdImage.compressImage = false;

  # Latest release of major 5 doesn't always play ball with the hardware.
  # Relase 4.19 is stable and "battle-tested".
  # See <https://github.com/NixOS/nixpkgs/issues/82455>.
  boot.kernelPackages = pkgs.linuxPackages_4_19;

  networking.hostName = "ed7039e";

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
  systemd.services.wpa_supplicant.wantedBy = lib.mkForce [ "default.target" ];

  # Ensure a correct system clock.
  services.timesyncd.enable = true;
  time.timeZone = "Europe/Stockholm";

  # Enables us to inspect core dumps in a centralized manner (incl. timestamps)
  systemd.coredump.enable = true;

  # Automatically log in as root, require no passphrase.
  users.users.root.initialHashedPassword = "";
  services.mingetty.autologinUser = lib.mkForce "root";

  # Automatically start SSH server after boot, and establish a reverse proxy
  # with a known bastion. This allows us to access the system from any system
  # with Internet access (and allows this system to live behind NAT:ed networks).
  services.openssh = {
    enable = true;
    permitRootLogin = "yes";
  };
  systemd.services.sshd.wantedBy = lib.mkForce [ "multi-user.target" ];
  environment.etc."id_rsa" = {
    source = ./id_rsa;
    user = "nixos";
    mode = "0600";
  };
  users.extraUsers.root.openssh.authorizedKeys.keys = lib.attrValues (import ./ssh-keys.nix);
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

  # Image minification
  # documentation.enable = lib.mkForce false;
  # documentation.nixos.enable = lib.mkForce false;
  environment.noXlibs = lib.mkForce true;
  services.xserver.enable = false;
  services.xserver.desktopManager.xterm.enable = lib.mkForce false;
  services.udisks2.enable = lib.mkForce false;
  security.polkit.enable = lib.mkForce false;
  boot.supportedFilesystems = lib.mkForce [ "vfat" ];
  i18n.supportedLocales = lib.mkForce [ (config.i18n.defaultLocale + "/UTF-8") ];
}
