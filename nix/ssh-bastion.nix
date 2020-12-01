# This expression applies to your including NixOS system all services
# the Raspberry Pi expects on the host configured in ./config.nix.
# While these services may also be deployed on a non-NixOS system, no
# list of instuctions or scripts are readily available in this repo
# that explains the process or automates the task.
#
# Note: this expression has been written with the intention to work on
# NixOS 20.03 (Markhor). It may need to be altered to work on later
# releases.

let
  cfg = import ./config.nix;
in
{ pkgs, ... }: {
  # Cross-compiling a whole NixOS system is a resource-heavy task,
  # especially if the Linux kernel needs to be rebuilt. This option
  # allows us to build on a faster system and later download the
  # generated image.
  boot.binfmt.emulatedSystems = [ "aarch64-linux" ];

  # We want to be able to remotely access our headless system without
  # much hassle. The below set of attributes creates a noop user (a
  # user that cannot to anything of significance) on the host with
  # which the robot uses to establish a reverse SSH proxy from inside
  # a NAT network — in our case, eduroam. This allows us to reach the
  # Raspberry Pi from anywhere on the networks, because this bastion
  # host can readily be connected to.

  # Ensure dangling socket files are removed when a new proxy is
  # established from the robot’s side.
  services.openssh.extraConfig = ''
    StreamLocalBindUnlink yes
  '';
  networking.firewall.allowedTCPPorts = [ cfg.bastion.sshPort ];
  users.extraUsers.bastion = {
    description = "proxy user for ed7039e";
    openssh.authorizedKeys.keyFiles = [ ../id_rsa.pub ];

    # Don't allow to the user to login or do anything.
    # We don't need a shell to create the socket in `ssh-port-forward.service`.
    shell = "${pkgs.shadow}/bin/nologin";
  };
  # Expose the socket file to a static post.
  systemd.services.expose-ed7039e-socket = {
    description = "export of ed7039e's domain socket to port ${toString cfg.bastion.sshPort}";
    wantedBy = [ "multi-user.target" ];
    after = [ "network.target" "nss-lookup.target" ];
    serviceConfig = {
      ExecStart = ''
        ${pkgs.socat}/bin/socat -dd TCP4-LISTEN:${toString cfg.bastion.sshPort},fork \
           UNIX-CONNECT:${cfg.bastion.socketPath}
      '';

      StandardError = "journal";
      Type = "simple";
      Restart = "always";
      RestartSec = "5s";
      StartLimitIntervalSec = "0";
    };
  };
}
