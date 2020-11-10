{ pkgs, lib, python }:

self: super: {
  "adafruit-platformdetect" = python.overrideDerivation super."adafruit-platformdetect" (old: {
    buildInputs = old.buildInputs ++ [ self."setuptools-scm" ];
  });

  "adafruit-pureio" = python.overrideDerivation super."adafruit-pureio" (old: {
    buildInputs = old.buildInputs ++ [ self."setuptools-scm" ];
  });

  "adafruit-blinka" = python.overrideDerivation super."adafruit-blinka" (old: {
    buildInputs = old.buildInputs ++ [ self."setuptools-scm" ];
  });

  # Ensure pyusb knows where to find libusb.
  # TODO: test by overriding with pkgs.pythonPackages3.pyusb instead.
  "pyftdi" = python.overrideDerivation super."pyftdi" (old: {
    patchPhase = ''
      ${pkgs.gnused}/bin/sed -i 's/> 80/> 999/' setup.py
      ${pkgs.gnused}/bin/sed -i \
          '680s/.*/            backend = mod.get_backend\(find_library=lambda x: "${lib.replaceStrings ["/"] ["\\/"] "${pkgs.libusb}"}\/lib\/libusb-1.0.so"\)/' \
          pyftdi/usbtools.py
    '';
  });
}
