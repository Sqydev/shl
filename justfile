default:
	just build

shl_token:
	meson setup build/debug --reconfigure -Dvtwm_debug=true
	meson compile -C build/debug
	just clangd-link build/debug

clangd-link DIR='build/release':
	ln -sfn {{DIR}}/compile_commands.json compile_commands.json

clean:
	rm -rf build/ compile_commands.json compiled/ core .cache/

license-check:
	bash scripts/check_license.sh

license-fix:
	bash scripts/fix_license.sh
