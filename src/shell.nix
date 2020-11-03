{ pkgs ? import <nixpkgs> {}, ... }:
with pkgs;

let
  derivations = callPackage ../nix/derivations.nix {};
in mkShell {
  buildInputs = with derivations; [
    lcm
    (lib.attrValues systemNodes)

    (python3.buildEnv.override {
      extraLibs = lib.attrValues derivations.pythonLibs;
    })
  ];
}
