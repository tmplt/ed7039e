# TODO: document

let
  vars = import ../common.nix;
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
  networking.firewall.allowedTCPPorts = [ vars.sshPort ];
  systemd.services.expose-ed7039e-socket = {
    description = "export of ed7039e's domain socket to port ${toString vars.sshPort}";
    wantedBy = [ "multi-user.target" ];
    after = [ "network.target" "nss-lookup.target" ];
    serviceConfig = {
      ExecStart = ''
        ${pkgs.socat}/bin/socat -dd TCP4-LISTEN:${toString vars.sshPort},fork \
           UNIX-CONNECT:${vars.socketPath}
      '';

      StandardError = "journal";
      Type = "simple";
      Restart = "always";
      RestartSec = "5s";
      StartLimitIntervalSec = "0";
    };
  };
}
