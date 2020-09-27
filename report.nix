#!/usr/bin/env nix-build
{ pkgs ? import nix/nixpkgs {} }:
with pkgs;

stdenv.mkDerivation {
  name = "ed7039e-report";
  # TODO: use git commit as version number
  buildInputs = [
    (texlive.combine {
      inherit (texlive)
        scheme-small
        latexmk
        
        kpfonts
        libertine
        inconsolata # TODO: fix or replace; it is not used
        amsmath
        tikzsymbols
        hyperref
        biblatex
        enumitem
        biber;
    })
  ];

  srcs = [
    report/master.tex
    report/ref.bib
    report/sections
  ];

  unpackPhase = ''
    for f in $srcs; do
      cp -r $f $(stripHash $f)
    done
  '';

  buildPhase = ''
    latexmk -shell-escape -xelatex -silent -quiet -interaction=nonstopmode
  '';

  installPhase = ''
    mkdir $out
    cp master.pdf $out/report.pdf
  '';

  meta = with lib; {
    description = "Compiled report of the ed7039e project";
    license = licenses.bsd3;
  };
}
