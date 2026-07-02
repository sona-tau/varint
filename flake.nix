{
	description = "Declarations for the environment that this project will use.";

	# Flake inputs
	inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1";

	# Flake outputs
	outputs = inputs@{self, nixpkgs}:
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
			forEachSupportedSystem = f: inputs.nixpkgs.lib.genAttrs supportedSystems (system: f {
				pkgs = import inputs.nixpkgs { inherit system; };
			});
		in {

			packages = forEachSupportedSystem ({ pkgs }: {
				default = pkgs.stdenv.mkDerivation {
					pname = "varint";
					version = version;
					src = ./.;

					outputs = [
						"dev" # headers & static.a
						"out" # shared.so
					];

					configurePhase = ''
						runHook preConfigure
						cc -o nob nob.c
						runHook postConfigure
					'';

					buildPhase = ''
						runHook preBuild
						./nob build
						runHook postBuild
					'';

					installPhase = ''
						runHook preInstall
						mkdir -p $out/lib
						cp .build/libvarint.so $out/lib/

						mkdir -p $dev/include
						cp include/varint.h $dev/include

						mkdir -p $dev/lib
						cp .build/libvarint.a $dev/lib/
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

					/*
					meta = with nixpkgs.lib; {
						description = "A reproducible minimal varint implementation in C.";
						license = licenses.mit;
						platforms = systems;
					};
					*/
				};
			});

			devShells = forEachSupportedSystem ({ pkgs }: {
				default = pkgs.mkShell {
					# The Nix packages provided in the environment
					# Add any you need here
					packages = with pkgs; [
						gcc
						bear
						gdb
						gum
						universal-ctags
						cppcheck
						doxygen
						clang-tools  # provides clang-format and clangd
					] ++ lib.optionals stdenv.isLinux [ valgrind ];

					# Set any environment variables for your dev shell
					env = { };

					# Add any shell logic you want executed any time the environment is activated
					shellHook = ''
					'';
				};
			});
		};
}
