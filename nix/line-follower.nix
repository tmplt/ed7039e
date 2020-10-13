{ pkgs, lib, ... }:

let
  colorzero = pkgs.python3Packages.buildPythonPackage rec {
    name = "${pname}-${version}";
    pname = "colorzero";
    version = "1.1";

    src = pkgs.python3Packages.fetchPypi {
      inherit pname version;
      sha256 = "16a532mgfwyr9l400g6cbhw82m6cdkz9mniw1ml5b1axkc8lgfmc";
    };
  };

  rpigpio = pkgs.python3Packages.buildPythonPackage rec {
    name = "${pname}-${version}";
    pname = "RPi.GPIO";
    version = "0.7.0";

    src = pkgs.python3Packages.fetchPypi {
      inherit pname version;
      sha256 = "0gvxp0nfm2ph89f2j2zjv9vl10m0hy0w2rpn617pcrjl41nbq93l";
    };

    doCheck = false; # tests can only be run on an RPi
  };
  
  gpiozero = pkgs.python3Packages.buildPythonPackage rec {
    name = "${pname}-${version}";
    pname = "gpiozero";
    version = "1.5.1";

    src = pkgs.python3Packages.fetchPypi {
      inherit pname version;
      sha256 = "0k9f8azglr9wa30a0zrl8qn0a66jj0r8z5h0z7cgz4z7wv28s6mf";
    };

    nativeBuildInputs = [ colorzero rpigpio ];
    propagatedBuildInputs = nativeBuildInputs;

    doCheck = false; # tests can only be run on an RPi
  };
in
{
  # XXX: can we install gpiozero here externally from brickpi.nix?
  environment.systemPackages = [
    (pkgs.python3.buildEnv.override (old: {
      extraLibs = old.extraLibs ++ [ gpiozero ];
    }))
  ];
}
