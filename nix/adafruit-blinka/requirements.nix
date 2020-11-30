# generated using pypi2nix tool (version: 2.0.4)
# See more at: https://github.com/nix-community/pypi2nix
#
# COMMAND:
#   pypi2nix -e adafruit-blinka
#

{ pkgs ? import <nixpkgs> {},
  overrides ? ({ pkgs, python }: self: super: {})
}:

let

  inherit (pkgs) makeWrapper;
  inherit (pkgs.stdenv.lib) fix' extends inNixShell;

  pythonPackages =
  import "${toString pkgs.path}/pkgs/top-level/python-packages.nix" {
    inherit pkgs;
    inherit (pkgs) stdenv;
    python = pkgs.python3;
  };

  commonBuildInputs = [];
  commonDoCheck = false;

  withPackages = pkgs':
    let
      pkgs = builtins.removeAttrs pkgs' ["__unfix__"];
      interpreterWithPackages = selectPkgsFn: pythonPackages.buildPythonPackage {
        name = "python3-interpreter";
        buildInputs = [ makeWrapper ] ++ (selectPkgsFn pkgs);
        buildCommand = ''
          mkdir -p $out/bin
          ln -s ${pythonPackages.python.interpreter} \
              $out/bin/${pythonPackages.python.executable}
          for dep in ${builtins.concatStringsSep " "
              (selectPkgsFn pkgs)}; do
            if [ -d "$dep/bin" ]; then
              for prog in "$dep/bin/"*; do
                if [ -x "$prog" ] && [ -f "$prog" ]; then
                  ln -s $prog $out/bin/`basename $prog`
                fi
              done
            fi
          done
          for prog in "$out/bin/"*; do
            wrapProgram "$prog" --prefix PYTHONPATH : "$PYTHONPATH"
          done
          pushd $out/bin
          ln -s ${pythonPackages.python.executable} python
          ln -s ${pythonPackages.python.executable} \
              python3
          popd
        '';
        passthru.interpreter = pythonPackages.python;
      };

      interpreter = interpreterWithPackages builtins.attrValues;
    in {
      __old = pythonPackages;
      inherit interpreter;
      inherit interpreterWithPackages;
      mkDerivation = args: pythonPackages.buildPythonPackage (args // {
        nativeBuildInputs = (args.nativeBuildInputs or []) ++ args.buildInputs;
      });
      packages = pkgs;
      overrideDerivation = drv: f:
        pythonPackages.buildPythonPackage (
          drv.drvAttrs // f drv.drvAttrs // { meta = drv.meta; }
        );
      withPackages = pkgs'':
        withPackages (pkgs // pkgs'');
    };

  python = withPackages {};

  generated = self: {
    "adafruit-blinka" = python.mkDerivation {
      name = "adafruit-blinka-5.7.0";
      src = pkgs.fetchurl {
        url = "https://files.pythonhosted.org/packages/62/5f/1e3d370573c3a9b2656b32e655ee7e66fce795d5678bf180933654d293e4/Adafruit-Blinka-5.7.0.tar.gz";
        sha256 = "f2f34c793be58ae5dd9a50f7f728221bfd466a1fb6e78f4cd2243dcb079c63e5";
};
      doCheck = commonDoCheck;
      format = "setuptools";
      buildInputs = commonBuildInputs ++ [

      ];
      propagatedBuildInputs = [
        self."adafruit-platformdetect"
        self."adafruit-pureio"
        self."pyftdi"
      ];
      meta = with pkgs.stdenv.lib; {
        homepage = "https://github.com/adafruit/Adafruit_Blinka";
        license = licenses.mit;
        description = "CircuitPython APIs for non-CircuitPython versions of Python such as CPython on Linux and MicroPython.";
      };
    };

    "adafruit-platformdetect" = python.mkDerivation {
      name = "adafruit-platformdetect-2.21.0";
      src = pkgs.fetchurl {
        url = "https://files.pythonhosted.org/packages/ac/87/11e899ebd0bfccab9f3a7946d495ab426ec9d4eec9ea80deaae2c79b7232/Adafruit-PlatformDetect-2.21.0.tar.gz";
        sha256 = "2840cc5294bf2b6c6f6aff384e85c0e18124f038a91c2b3f08a398750de20c38";
};
      doCheck = commonDoCheck;
      format = "setuptools";
      buildInputs = commonBuildInputs ++ [

      ];
      propagatedBuildInputs = [ ];
      meta = with pkgs.stdenv.lib; {
        homepage = "https://github.com/adafruit/Adafruit_Python_PlatformDetect";
        license = licenses.mit;
        description = "Platform detection for use by libraries like Adafruit-Blinka.";
      };
    };

    "adafruit-pureio" = python.mkDerivation {
      name = "adafruit-pureio-1.1.7";
      src = pkgs.fetchurl {
        url = "https://files.pythonhosted.org/packages/90/6d/ee3b05a3016aefb446f14103e99021186aace51e9d75f5aee25e031f8e8d/Adafruit_PureIO-1.1.7.tar.gz";
        sha256 = "2d6522d9b333e60d67fad8c3169b0c6560016a5f5f8f571b1f9692db60e14eb4";
};
      doCheck = commonDoCheck;
      format = "setuptools";
      buildInputs = commonBuildInputs ++ [

      ];
      propagatedBuildInputs = [ ];
      meta = with pkgs.stdenv.lib; {
        homepage = "https://github.com/adafruit/Adafruit_Python_PureIO";
        license = licenses.mit;
        description = "Pure python (i.e. no native extensions) access to Linux IO    including I2C and SPI. Drop in replacement for smbus and spidev modules.";
      };
    };

    "pyftdi" = python.mkDerivation {
      name = "pyftdi-0.52.0";
      src = pkgs.fetchurl {
        url = "https://files.pythonhosted.org/packages/46/9d/78ef718e9616759fa249ba8e379d88014bd8e352beceb567042403b2be54/pyftdi-0.52.0.tar.gz";
        sha256 = "5f591cefc87c89177798b53c70914fadff54717433afc9ccb50213d053e87ee6";
};
      doCheck = commonDoCheck;
      format = "setuptools";
      buildInputs = commonBuildInputs ++ [

      ];
      propagatedBuildInputs = [
        self."pyserial"
        self."pyusb"
      ];
      meta = with pkgs.stdenv.lib; {
        homepage = "http://github.com/eblot/pyftdi";
        license = licenses.bsdOriginal;
        description = "FTDI device driver (pure Python)";
      };
    };

    "pyserial" = python.mkDerivation {
      name = "pyserial-3.4";
      src = pkgs.fetchurl {
        url = "https://files.pythonhosted.org/packages/cc/74/11b04703ec416717b247d789103277269d567db575d2fd88f25d9767fe3d/pyserial-3.4.tar.gz";
        sha256 = "6e2d401fdee0eab996cf734e67773a0143b932772ca8b42451440cfed942c627";
};
      doCheck = commonDoCheck;
      format = "setuptools";
      buildInputs = commonBuildInputs ++ [

      ];
      propagatedBuildInputs = [ ];
      meta = with pkgs.stdenv.lib; {
        homepage = "https://github.com/pyserial/pyserial";
        license = licenses.bsdOriginal;
        description = "Python Serial Port Extension";
      };
    };

    "pyusb" = python.mkDerivation {
      name = "pyusb-1.1.0";
      src = pkgs.fetchurl {
        url = "https://files.pythonhosted.org/packages/12/9b/8f5be839753667c39fe522162bea7f8121f28ba49c5ad1e5681681967c79/pyusb-1.1.0.tar.gz";
        sha256 = "d69ed64bff0e2102da11b3f49567256867853b861178689671a163d30865c298";
};
      doCheck = commonDoCheck;
      format = "pyproject";
      buildInputs = commonBuildInputs ++ [
        self."setuptools"
        self."setuptools-scm"
        self."wheel"
      ];
      propagatedBuildInputs = [ ];
      meta = with pkgs.stdenv.lib; {
        homepage = "https://pyusb.github.io/pyusb";
        license = licenses.bsdOriginal;
        description = "Python USB access module";
      };
    };

    "setuptools" = python.mkDerivation {
      name = "setuptools-50.3.2";
      src = pkgs.fetchurl {
        url = "https://files.pythonhosted.org/packages/a7/e0/30642b9c2df516506d40b563b0cbd080c49c6b3f11a70b4c7a670f13a78b/setuptools-50.3.2.zip";
        sha256 = "ed0519d27a243843b05d82a5e9d01b0b083d9934eaa3d02779a23da18077bd3c";
};
      doCheck = commonDoCheck;
      format = "setuptools";
      buildInputs = commonBuildInputs ++ [

      ];
      propagatedBuildInputs = [ ];
      meta = with pkgs.stdenv.lib; {
        homepage = "https://github.com/pypa/setuptools";
        license = licenses.mit;
        description = "Easily download, build, install, upgrade, and uninstall Python packages";
      };
    };

    "setuptools-scm" = python.mkDerivation {
      name = "setuptools-scm-4.1.2";
      src = pkgs.fetchurl {
        url = "https://files.pythonhosted.org/packages/cd/66/fa77e809b7cb1c2e14b48c7fc8a8cd657a27f4f9abb848df0c967b6e4e11/setuptools_scm-4.1.2.tar.gz";
        sha256 = "a8994582e716ec690f33fec70cca0f85bd23ec974e3f783233e4879090a7faa8";
};
      doCheck = commonDoCheck;
      format = "pyproject";
      buildInputs = commonBuildInputs ++ [
        self."setuptools"
        self."wheel"
      ];
      propagatedBuildInputs = [
        self."setuptools"
      ];
      meta = with pkgs.stdenv.lib; {
        homepage = "https://github.com/pypa/setuptools_scm/";
        license = licenses.mit;
        description = "the blessed package to manage your versions by scm tags";
      };
    };

    "wheel" = python.mkDerivation {
      name = "wheel-0.35.1";
      src = pkgs.fetchurl {
        url = "https://files.pythonhosted.org/packages/83/72/611c121b6bd15479cb62f1a425b2e3372e121b324228df28e64cc28b01c2/wheel-0.35.1.tar.gz";
        sha256 = "99a22d87add3f634ff917310a3d87e499f19e663413a52eb9232c447aa646c9f";
};
      doCheck = commonDoCheck;
      format = "setuptools";
      buildInputs = commonBuildInputs ++ [
        self."setuptools"
      ];
      propagatedBuildInputs = [ ];
      meta = with pkgs.stdenv.lib; {
        homepage = "https://github.com/pypa/wheel";
        license = licenses.mit;
        description = "A built-package format for Python";
      };
    };
  };
  localOverridesFile = ./requirements_override.nix;
  localOverrides = import localOverridesFile { inherit pkgs python; lib = pkgs.lib; };
  commonOverrides = [
        (let src = pkgs.fetchFromGitHub { owner = "nix-community"; repo = "pypi2nix-overrides"; rev = "90e891e83ffd9e55917c48d24624454620d112f0"; sha256 = "0cl1r3sxibgn1ks9xyf5n3rdawq4hlcw4n6xfhg3s1kknz54jp9y"; } ; in import "${src}/overrides.nix" { inherit pkgs python; })
  ];
  paramOverrides = [
    (overrides { inherit pkgs python; })
  ];
  allOverrides =
    (if (builtins.pathExists localOverridesFile)
     then [localOverrides] else [] ) ++ commonOverrides ++ paramOverrides;

in python.withPackages
   (fix' (pkgs.lib.fold
            extends
            generated
            allOverrides
         )
   )