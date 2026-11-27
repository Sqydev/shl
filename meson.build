project(
	'shl',
	'c',
	version: '1.0.0',
	meson_version: '>=1.1.0',
	default_options: [
		'buildtype=debugoptimized',
		'b_sanitize=address,undefined',
		'c_std=c99',
		'warning_level=3',
	],
)

cc = meson.get_compiler('c')

add_project_arguments(
	cc.get_supported_arguments(
		'-DDEBUG=1',
		'-D_FORTIFY_SOURCE=3',
		'-fasynchronous-unwind-tables',
		'-fno-sanitize-recover=all',
	),
	language: 'c',
)

fs = import('fs')

lib = get_option('lib')
if lib == ''
	error('No library selected. Pass -Dlib=<name>')
endif

if lib.contains('/') or lib.contains('..')
	error('Invalid library name "@0@" (expected a simple name from src/).'.format(lib))
endif

src_dir = meson.current_source_dir() / 'src'
tester_dir = meson.current_source_dir() / 'tester' / lib
tester_main = tester_dir / 'main.c'

if not fs.is_dir(tester_dir)
	error('No tester found for library "@0@" (expected the directory @1@).'.format(lib, tester_dir))
endif

if not fs.exists(tester_main)
	error('Tester for library "@0@" has no main.c (expected @1@).'.format(lib, tester_main))
endif

lib_header = src_dir / (lib + '.h')
lib_source = src_dir / (lib + '.c')

if not fs.exists(lib_header) and not fs.exists(lib_source)
	error('Library "@0@" not found in @1@ (expected @2@ or @3@).'.format(lib, src_dir, lib_header, lib_source))
endif

include = include_directories('src')

lib_dep = declare_dependency(include_directories: include)
if fs.exists(lib_source)
	lib_dep = declare_dependency(link_with: static_library('shl_' + lib, lib_source, include_directories: include), include_directories: include)
endif

tester = executable(
	lib + '_tester',
	tester_main,
	dependencies: lib_dep,
	c_args: ['-D' + lib.to_upper() + '_IMPLEMENTATION'],
	install: false,
)

test(lib + '_test', tester)

cpp_tester_main = tester_dir / 'main.cpp'
if fs.exists(cpp_tester_main)
	add_languages('cpp', native: false)

	cpp = meson.get_compiler('cpp')

	cpp_tester = executable(
		lib + '_cpp_tester',
		cpp_tester_main,
		dependencies: lib_dep,
		cpp_args: cpp.get_supported_arguments('-fno-sanitize-recover=all'),
		install: false,
	)

	test(lib + '_cpp_test', cpp_tester)
endif
