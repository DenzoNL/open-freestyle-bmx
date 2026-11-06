{
  description = "opendmfbmx - Dave Mirra Freestyle BMX reimplementation";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "aarch64-darwin"
        "x86_64-linux"
        "aarch64-linux"
      ];
      forAll = f: nixpkgs.lib.genAttrs systems (s: f nixpkgs.legacyPackages.${s});
    in
    {
      devShells = forAll (
        pkgs:
        let
          llvm = pkgs.llvmPackages_21;
          python = pkgs.python3.withPackages (p: [
            p.unicorn
            p.pefile
            p.capstone
            p.pytest
          ]);
          crossEnv = {
            # Unwrapped clang for the freestanding i686-pc-windows-msvc hook build; the
            # nixpkgs wrapper injects host-only flags.
            OPENDMFBMX_CROSS_CLANG = "${llvm.clang-unwrapped}/bin/clang";
            OPENDMFBMX_LLD_LINK = "${llvm.lld}/bin/lld-link";
          };
          core = [
            pkgs.cmake
            pkgs.ninja
            llvm.lld
            llvm.llvm
            llvm.clang-tools
            python
          ];
        in
        {
          # Lean shell for CI: hook build, lint, differential tests.
          ci = pkgs.mkShellNoCC ({ packages = core; } // crossEnv);

          default = pkgs.mkShell (
            {
              packages =
                core
                ++ [
                  pkgs.cppcheck
                  pkgs.sdl3
                ]
                ++ pkgs.lib.optional (pkgs.stdenv.hostPlatform.isDarwin || pkgs.stdenv.hostPlatform.isx86_64) pkgs.ghidra
                # The macOS / Linux native build (docs/PORTING.md "X5 part 1"): SPIR-V -> MSL in tools/shaders.sh.
                ++ [ pkgs.spirv-cross ]
                # The Linux native build (docs/PORTING.md "X4 (Linux x64)", the `linux` preset): a wrapped clang
                # for the host (the shell's stdenv compiler is gcc there) and patchelf for the tarball.
                ++ pkgs.lib.optionals pkgs.stdenv.hostPlatform.isLinux [
                  llvm.clang
                  pkgs.patchelf
                ];
              # find_package(SDL3) of the macOS / Linux native build (the `macos` / `linux` presets).
              SDL3_ROOT = "${pkgs.sdl3.dev}";
            }
            // crossEnv
          );
        }
      );
    };
}
