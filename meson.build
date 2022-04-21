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
  ['nast.c', 'render.c', 'writable.c', 'strs.c'],
  # include_directories: incdir,
  dependencies: deps
)

executable(
  'test_writable',
  ['test_writable.c'],
  # include_directories: incdir,
)

executable(
  'test_strs',
  ['test_strs.c'],
)

executable(
  'raw_inputs',
  ['raw_inputs.c'],
)

executable(
  'read_tty_response',
  ['read_tty_response.c', 'strs.c'],
)

executable(
  'scrape_xtgettcap',
  ['scrape_xtgettcap.c'],
)

executable(
  'spysh',
  ['spysh.c'],
  dependencies: [cc.find_library('util')],
)

executable(
  'dumptermios',
  ['dumptermios.c'],
)
