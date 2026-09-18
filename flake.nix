{
  description = "ZaitC - an MPD client made using C";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            gcc
            gnumake
            libmpdclient
            notcurses
            pkg-config
          ];

          LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath (
            with pkgs;
            [
              libmpdclient
              notcurses
            ]
          );
        };
        packages.default = pkgs.stdenv.mkDerivation {
          name = "zaitc";
          pname = "zaitc";
          version = "1.0.0";
          src = ./.;
          buildInputs = with pkgs; [
            libmpdclient
            notcurses
          ];
          nativeBuildInputs = with pkgs; [
            pkg-config
          ];
          installFlags = [ "PREFIX=$(out)" ];
        };
      }
    );
}
