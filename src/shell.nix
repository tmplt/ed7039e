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

  zcm = stdenv.mkDerivation rec {
    name = "zcm";
    version = "1.0.3";

    src = fetchFromGitHub {
      owner = "ZeroCM";
      repo = "zcm";
      rev = "v${version}";
      sha256 = "1ywp3djdwwbfmy6rsaa1s74bvgackvnm7dmh3qq222vj8fy4z4xh";
    };

    # TODO: patch upstream
    patchPhase = ''
      patchShebangs scripts/prepend-embed-guards.sh
    '';

    wafConfigureFlags = [
      "--use-python" "--python=${python3}/bin/python"
      # "--pythondir=$out/lib"
      "--use-zmq"
      "--use-ipc"
      "--use-udpm"
    ];

    nativeBuildInputs = [
      wafHook
      python3Packages.cython
      pkg-config
      zeromq
      ncurses # Required for testing pyembed (for some reason)
    ];
  };

  pythonNodeDeps = stdenv.mkDerivation rec {
    name = "lcm-python";
    src = ./.;
    buildInputs = [ cmake lcm ];
    cmakeFlags = [ "-DINSTALL_PYTHON_DEPS=ON" ];
  };
in

mkShell {
  # buildInputs = [ lcm pythonNodeDeps ];
  buildInputs = [
    zcm
    lcm

    # (python3.buildEnv.override {
    #   extraLibs = [ zcm ];
    # })
  ];
}
