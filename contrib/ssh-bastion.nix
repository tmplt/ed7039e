# TODO: document
# TODO: mention that this file is written with NixOS 20.03 (Markhor) in mind.
# TODO: write a config for `ssh -o StrictHostKeyChecking=no -p 21013 nixos@tmplt.dev`, or force a SSH host fingerprint

let
  cfg = import ../config.nix;
in
{ pkgs, ... }: {
  boot.binfmt.emulatedSystems = [ "aarch64-linux" ];

  services.openssh.extraConfig = ''
    StreamLocalBindUnlink yes
  '';

  users.extraUsers.bastion = {
    description = "proxy user for ed7039e";
    openssh.authorizedKeys.keyFiles = [ ../id_rsa.pub ];

    # Don't allow to the user to login or do anything.
    # We don't need a shell to create the socket in `ssh-port-forward.service`.
    shell = "${pkgs.shadow}/bin/nologin";
  };

  # Export socket via socat(1)
  networking.firewall.allowedTCPPorts = [ cfg.bastion.sshPort ];
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
