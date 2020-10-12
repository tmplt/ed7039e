{ pkgs, lib, ... }:

let
  smbus-cffi = pkgs.python3Packages.buildPythonPackage rec {
    name = "${pname}-${version}";
    pname = "smbus-cffi";
    version = "0.5.1";

    src = pkgs.python3Packages.fetchPypi {
      inherit pname version;
      sha256 = "1s5xsvd6i1z44dz5kz924vqzh6ybnn8323gncdl5h0gwmfm9ahgv";
    };

    nativeBuildInputs = [ pkgs.python3Packages.cffi ];
    propagatedBuildInputs = [ pkgs.python3Packages.cffi ];
  };

  wiringpi = pkgs.python3Packages.buildPythonPackage rec {
    name = "${pname}-${version}";
    pname = "wiringpi";
    version = "2.60.0";

    src = pkgs.python3Packages.fetchPypi {
      inherit pname version;
      sha256 = "0kc8hjfbnr1qsll8nrw9dwcmf1qfx4p2ya909di7yswmi4lg3n1y";
    };
  };

  spidev = pkgs.python3Packages.buildPythonPackage rec {
    name = "${pname}-${version}";
    pname = "spidev";
    version = "3.5";

    src = pkgs.python3Packages.fetchPypi {
      inherit pname version;
      sha256 = "03cicc9kpi5khhq0bl4dcy8cjcl2j488mylp8sna47hnkwl5qzwa";
    };
  };

  brickpi3 = pkgs.python3Packages.buildPythonPackage rec {
    name = "${pname}";
    pname = "brickpi3";

    src = (pkgs.fetchFromGitHub {
      owner = "DexterInd";
      repo = "BrickPi3";
      rev = "365a69589b53fce58bf60e3ed0845b933e0f4c58";
      sha256 = "1accspaq2cci7ihkdryzgknm0rdcgkby961f943kimmpj325nr26";
    }) + "/Software/Python";

    nativeBuildInputs = [ spidev ];
    propagatedBuildInputs = [ spidev ];
  };

  rfrToolsMisc = pkgs.python3Packages.buildPythonPackage rec {
    name = "${pname}";
    pname = "Dexter_AutoDetection_and_I2C_Mutex";

    src = (pkgs.fetchFromGitHub {
      owner = "DexterInd";
      repo = "RFR_Tools";
      rev = "22068c24767beeedd1dc8417f0e2b46b0bc73150";
      sha256 = "0lmqv0kiiyh540r869xrf3ijwl29272c3mwcqgjglqdn3y8h9hfp";
    }) + "/miscellaneous";

    nativeBuildInputs = [
      smbus-cffi
      pkgs.python3Packages.pyserial
      pkgs.python3Packages.python-periphery
      wiringpi
    ];
  };

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
  environment.systemPackages = [
    (pkgs.python3.buildEnv.override {
      extraLibs = with pkgs.python3Packages; [
        brickpi3
        gpiozero

        # Not required, but we may need them later.
        wiringpi
        rfrToolsMisc
      ];
    })
  ];
}
