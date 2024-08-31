cpp_args = ['-Wall', '-Wextra', '-O0']

link_args = []

inc = include_directories('.')
chat_lib = static_library(
  'chat_lib',
  [
    'gui/io.cpp',
    'gui/gui.cpp',
    'log.cpp',
    'audio/lib.cpp',
    'audio/player.cpp',
    'audio/recorder.cpp',
    'audio/dsp.cpp',
    'audio/codec.cpp',
    'audio/netbuf.cpp',
    'tls/net.cpp',
  ],
  dependencies: chat_deps,
  cpp_args: cpp_args,
  link_args: link_args,
  include_directories: inc,
)
chat_lib_dep = declare_dependency(
  include_directories: inc,
  link_with: chat_lib,
  dependencies: chat_deps,
  link_args: link_args,
)
