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

  lcmPythonDeps = stdenv.mkDerivation {
    name = "lcm-python";
    src = ../src;
    buildInputs  = [ cmake lcm ];
    propagatedBuildInputs = [ lcm ];
    cmakeFlags = [ "-DINSTALL_PYTHON_DEPS=ON" ];
  };

  nodes = {
    scripts = python3Packages.buildPythonPackage {
      name = "dwm_recv.py";
      src = ../src;
      propagatedBuildInputs = [ lcmPythonDeps ];
    };

    binaries = stdenv.mkDerivation {
      name = "dwm_send";
      src = ../src;
      buildInputs = [ cmake lcm ];
      cmakeFlags = [ "-DINSTALL_BINARY_NODES=ON" ];
    };
  };
in {
  # Required for the LCM UDP multicast provider
  networking.firewall.allowedUDPPorts = [ 7667 ];

  environment.systemPackages = with nodes; [ binaries scripts ];
}
