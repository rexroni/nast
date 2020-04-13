project('nast', 'c')

cc = meson.get_compiler('c')

deps = [
  dependency('gtk+-3.0'),
  dependency('fontconfig'),
  cc.find_library('m', required: false),
  cc.find_library('util'),
  cc.find_library('X11'),
  cc.find_library('Xft'),
]

executable(
  'nast',
  ['nast.c', 'x.c', 'render.c', 'writable.c'],
  # include_directories: incdir,
  dependencies: deps
)

executable(
  'test_writable',
  ['test_writable.c'],
  # include_directories: incdir,
)
