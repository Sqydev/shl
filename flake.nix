{
  	description = "shl";

	inputs = {
		nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
	};

	outputs = { self, nixpkgs }:
		let
			systems = [ "x86_64-linux" "aarch64-linux" ];

			forAllSystems = nixpkgs.lib.genAttrs systems;

			pkgsFor = system: nixpkgs.legacyPackages.${system};
		in
		{
			devShells = forAllSystems (system:
				let
					pkgs = pkgsFor system;

				in
				{
					default = pkgs.mkShell {
						packages = with pkgs; [
							gcc
							clang-tools
              				meson
							ninja
							just
							pkg-config
							fish
						];

						shellHook = "exec fish";

						C_INCLUDE_PATH = envIncludes;
						CPLUS_INCLUDE_PATH = envIncludes;
						CPATH = envIncludes;
					};
				});
		};
}
