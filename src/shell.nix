{ pkgs ? import <nixpkgs> {}, ... }:
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

  pythonNodeDeps = stdenv.mkDerivation rec {
    name = "lcm-python";
    src = ./.;
    buildInputs = [ cmake lcm ];
    cmakeFlags = [ "-DINSTALL_PYTHON_DEPS=ON" ];
  };
in mkShell {
  buildInputs = [ lcm pythonNodeDeps ];
}
