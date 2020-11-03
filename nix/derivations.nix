{ pkgs, ... }:
with pkgs;

let
  src = builtins.filterSource (path: type:
    baseNameOf path != "build")
    ../src;
in rec {
  pythonLibs = rec {
    spidev = python3Packages.buildPythonPackage rec {
      name = "${pname}-${version}";
      pname = "spidev";
      version = "3.5";

      src = python3Packages.fetchPypi {
        inherit pname version;
        sha256 = "03cicc9kpi5khhq0bl4dcy8cjcl2j488mylp8sna47hnkwl5qzwa";
      };
    };

    brickpi3 = python3Packages.buildPythonPackage rec {
      name = "${pname}";
      pname = "brickpi3";

      src = (fetchFromGitHub {
        owner = "DexterInd";
        repo = "BrickPi3";
        rev = "365a69589b53fce58bf60e3ed0845b933e0f4c58";
        sha256 = "1accspaq2cci7ihkdryzgknm0rdcgkby961f943kimmpj325nr26";
      }) + "/Software/Python";

      nativeBuildInputs = [ spidev ];
      propagatedBuildInputs = [ spidev ];
    };

    lcmPythonDeps = stdenv.mkDerivation {
      inherit src;
      name = "lcm-python";
      buildInputs  = [ cmake lcm ];
      propagatedBuildInputs = [ lcm ];
      cmakeFlags = [ "-DINSTALL_PYTHON_DEPS=ON" ];
    };
  };

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

  systemNodes = {
    scripts = python3Packages.buildPythonPackage {
      inherit src;
      name = "system-nodes-python";
      propagatedBuildInputs = [ pythonLibs.lcmPythonDeps ];
      doCheck = false;
    };

    binaries = stdenv.mkDerivation {
      inherit src;
      name = "system-nodes-binary";
      buildInputs = [ cmake lcm ];
      cmakeFlags = [ "-DINSTALL_BINARY_NODES=ON" ];
    };
  };
}
