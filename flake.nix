{
  description = "Declarations for the environment that this project will use.";

  # Flake inputs
  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";

  # Flake outputs
  outputs =
    inputs@{ self, nixpkgs }:
    let
      version = "373K";
      # The systems supported for this flake
      supportedSystems = [
        "x86_64-linux" # 64-bit Intel/AMD Linux
        "aarch64-linux" # 64-bit ARM Linux
        "x86_64-darwin" # 64-bit Intel macOS
        "aarch64-darwin" # 64-bit ARM macOS
      ];

      # Helper to provide system-specific attributes
      forEachSupportedSystem =
        f:
        inputs.nixpkgs.lib.genAttrs supportedSystems (
          system:
          f {
            pkgs = import inputs.nixpkgs { inherit system; };
          }
        );
    in
    {
      checks = forEachSupportedSystem (
        { pkgs }: let
			system = pkgs.stdenv.hostPlatform.system;
		in {
          build-test = self.packages.${system}.default;
        });

      packages = forEachSupportedSystem (
        { pkgs }: {
          default = pkgs.stdenv.mkDerivation {
            pname = "varint";
            version = version;
            src = ./.;
            __contentAddressed = true;
            doCheck = true;

            meta = with nixpkgs.lib; {
              description = "A reproducible minimal varint implementation in C.";
              homepage = "https://tangled.org/stau.space/varint";
              license = licenses.mit;
              platforms = platforms.all;
            };

            outputs = [
              "dev" # headers & static.a
              "out" # shared.so
              "doc" # Doxygen files
            ];

            configurePhase = ''
              runHook preConfigure
              cc -o nob nob.c
              runHook postConfigure
            '';

            buildPhase = ''
              runHook preBuild
              ./nob build
              ./nob docs || true
              runHook postBuild
            '';

            checkPhase = ''
              runHook preCheck
              ./nob test
              runHook postCheckCheck
            '';

            installPhase = ''
              runHook preInstall

              mkdir -p $out/lib
              cp .build/libvarint.so $out/lib/

              mkdir -p $dev/include
              cp include/varint.h $dev/include

              mkdir -p $dev/lib
              cp .build/libvarint.a $dev/lib/

              mkdir -p $doc/share/doc/varint
              if [ -d .build/docs/html/ ]; then
                cp -r .build/docs/html $doc/share/doc/varint/html
              fi

              cp README.md  $doc/share/doc/varint/ 2>/dev/null || true
              cp CHANGELOG.md  $doc/share/doc/varint/ 2>/dev/null || true

              if [ -d .build/docs/man ]; then
                mkdir -p $doc/share/man/man3
                cp .build/docs/man/man3/*.3 $doc/share/man/man3/ 2>/dev/null || true
              fi

              runHook postInstall
            '';

            postInstall = ''
              mkdir -p $dev/lib/pkgconfig
              cat > $dev/lib/pkgconfig/varint.pc <<EOF
              prefix=$out
              exec_prefix=''${prefix}
              libdir=''${exec_prefix}/lib
              includedir=$dev/include

              Name: varint
              Description: A reproducible minimal varint implementation in C.
              Version: ${version}
              Cflags: -I''${includedir}
              Libs: -L''${libdir} -lvarint
              EOF
            '';
          };
        }
      );

      devShells = forEachSupportedSystem (
        { pkgs }: {
          default = pkgs.mkShell {
            # The Nix packages provided in the environment
            # Add any you need here
            packages =
              with pkgs;
              [
                gcc
                bear
                gdb
                gum
                universal-ctags
                cppcheck
                doxygen
                clang-tools # provides clang-format and clangd
                nixfmt
              ]
              ++ lib.optionals stdenv.isLinux [ valgrind ];

            # Set any environment variables for your dev shell
            env = { };

            # Add any shell logic you want executed any time the environment is activated
            # shellHook = "          ";
          };
        }
      );
    };
}
