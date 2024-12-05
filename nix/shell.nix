{ pkgs }:

pkgs.clang19Stdenv.mkDerivation (rec {
  name = "shell";
  buildInputs = with pkgs; [
    catch2_3
    cmake
    cmake-language-server
    ninja
  ];
})
