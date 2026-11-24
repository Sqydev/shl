shl_token:
	meson setup build/shl_token --reconfigure -Dlib=shl_token
	meson compile -C build/shl_token
	just clangd-link build/shl_token

shl_vector:
	meson setup build/shl_vector --reconfigure -Dlib=shl_vector
	meson compile -C build/shl_vector
	just clangd-link build/shl_vector

shl_string:
	meson setup build/shl_string --reconfigure -Dlib=shl_string
	meson compile -C build/shl_string
	just clangd-link build/shl_string

shl_class:
	meson setup build/shl_class --reconfigure -Dlib=shl_class
	meson compile -C build/shl_class
	just clangd-link build/shl_class

shl_cordor:
	meson setup build/shl_cordor --reconfigure -Dlib=shl_cordor
	meson compile -C build/shl_cordor
	just clangd-link build/shl_cordor

clangd-link DIR='':
	[ -z "{{DIR}}" ] || ln -sfn "{{DIR}}/compile_commands.json" compile_commands.json

clean:
	rm -rf build/ compile_commands.json compiled/ core .cache/

license-check:
	bash scripts/check_license.sh

license-fix:
	bash scripts/fix_license.sh
