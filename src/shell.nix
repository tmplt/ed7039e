{ pkgs ? import <nixpkgs> {}, ... }:
with pkgs;

let
  derivations = callPackage ../nix/derivations.nix {};
in mkShell {
  buildInputs = with derivations; [
    lcm
    (lib.attrValues systemNodes)

    (python3.buildEnv.override {
      extraLibs = (with python3Packages; [
        numpy
      ])
      ++ (lib.attrValues derivations.pythonLibs)
      ++ (builtins.attrValues (import ../nix/adafruit-blinka/requirements.nix { inherit pkgs; }).packages);
    })
  ];
}
