setup LIB:
	if [ -d "build/{{LIB}}" ]; then meson setup "build/{{LIB}}" --reconfigure -Dlib={{LIB}}; else meson setup "build/{{LIB}}" -Dlib={{LIB}}; fi

shl_token:
	just setup shl_token
	meson compile -C build/shl_token
	just clangd-link build/shl_token

shl_vector:
	just setup shl_vector
	meson compile -C build/shl_vector
	just clangd-link build/shl_vector

shl_string:
	just setup shl_string
	meson compile -C build/shl_string
	just clangd-link build/shl_string

shl_cordor:
	just setup shl_cordor
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
