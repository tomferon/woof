{ pkgs }:

pkgs.clang19Stdenv.mkDerivation {
  name = "shell";
  buildInputs = with pkgs; [
    boost186
    catch2_3
    cmake
    cmake-language-server
    ninja
    nlohmann_json
  ];
}
