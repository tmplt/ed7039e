{ pkgs, lib, ... }:
with pkgs;

let
  lcm = stdenv.mkDerivation rec {
    name = "lcm";
    version = "1.4.0";
    
    src = fetchFromGitHub {
      owner = "lcm-proj";
      repo = "lcm";
      rev = "v${version}";
      sha256 = "04hw6qrfhbm14wwhvqzjr42maramy1r40gb3c11lxf69dvdgjg5s";
    };

    nativeBuildInputs = [ cmake glib python3 ];
    propagatedBuildInputs = [ python3 ];
  };

  nodes = stdenv.mkDerivation {
    name = "ed7039e-nodes";
    src = ../src;

    buildInputs  = [ cmake lcm ];
    propagatedBuildInputs = [ python3 lcm ];
  };
in {
  # Required for the LCM UDP multicast provider
  networking.firewall.allowedUDPPorts = [ 7667 ];

  environment.systemPackages = [ nodes lcm ];
}
