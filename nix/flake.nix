{
  description = "QMK and Mejiro31 firmware development environment";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

  outputs = { nixpkgs, ... }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forEachSystem = nixpkgs.lib.genAttrs systems;
    in {
      devShells = forEachSystem (system:
        let
          pkgs = import nixpkgs { inherit system; };
          python = pkgs.python3.withPackages (ps: [ ps.pyelftools ps.unicorn ]);
        in {
          default = pkgs.mkShell {
            packages = [
              pkgs.qmk
              pkgs.gcc-arm-embedded
              pkgs.gnumake
              pkgs.git
              pkgs.just
              pkgs.rustup
              python
            ];
            MEJIRO_QMK_NIX = "1";
          };
        });
    };
}
