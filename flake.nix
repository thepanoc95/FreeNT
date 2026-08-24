{
  description = "FreeBSD Nix flake with MinGW 64-bit and 32-bit (i686) cross-compilation toolchains";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachSystem [ "x86_64-freebsd" "aarch64-freebsd" ] (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          config = {
            platform = "freebsd";
          };
        };

        pkgsCrossMingw64 = import nixpkgs {
          inherit system;
          crossSystem = {
            config = "x86_64-w64-mingw32";
          };
          config = {
            platform = "freebsd";
          };
        };

        pkgsCrossMingw32 = import nixpkgs {
          inherit system;
          crossSystem = {
            config = "i686-w64-mingw32";
          };
          config = {
            platform = "freebsd";
          };
        };
      in
      {
        packages = {
          mingw64 = pkgsCrossMingw64;
          mingw32 = pkgsCrossMingw32;
        };

        devShells = {
          default = pkgs.mkShell {
            buildInputs = with pkgs; [
              gnumake
            ];
          };

          mingw64 = pkgs.mkShell {
            buildInputs = with pkgsCrossMingw64; [
              stdenv.cc
              gcc
              cmake
              pkg-config
              libtool
              mingw64-binutils
              mingw64-gcc
            ];
          };

          mingw32 = pkgs.mkShell {
            buildInputs = with pkgsCrossMingw32; [
              stdenv.cc
              gcc
              cmake
              pkg-config
              mingw32-binutils
              mingw32-gcc
            ];
          };
        };

        apps = {
          mingw64-gcc = {
            type = "app";
            program = "${pkgsCrossMingw64.gcc}/bin/x86_64-w64-mingw32-gcc";
          };
          mingw32-gcc = {
            type = "app";
            program = "${pkgsCrossMingw32.gcc}/bin/i686-w64-mingw32-gcc";
          };
        };
      }
    );
}
