inc = include_directories('.')

cpp_args = ['-Wall', '-Wextra', '-Og', '-g']
link_args = ['-rdynamic']

chat_deps = [
  dependency('gl'),
  dependency('glfw3'),
  dependency('sdl2'),
  dependency('portaudiocpp'),
  dependency('opus'),
  dependency('boost'),
  utfcpp_dep,
  rnnoise_dep,
]

# for use in tests
chat_lib = static_library(
  'chat_lib',
  [
    'gui/io.cpp',
    'gui/gui.cpp',
    'log.cpp',
    'err.cpp',
    'audio/lib.cpp',
    'audio/player.cpp',
    'audio/recorder.cpp',
    'audio/dsp.cpp',
    'audio/rtp_jitter.cpp',
    'audio/rtp_sender.cpp',
    'audio/codec.cpp',
    'audio/bufsrc.cpp',
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

executable(
  'chat',
  ['main.cpp'],
  dependencies: chat_lib_dep,
)

executable(
  'echo',
  ['audio/examples/echo.cpp'],
  dependencies: chat_lib_dep,
)

executable(
  'echo_opus',
  ['audio/examples/echo_opus.cpp'],
  dependencies: chat_lib_dep,
)

executable(
  'demo',
  ['audio/examples/demo.cpp'],
  dependencies: chat_lib_dep,
)

executable(
  'demo_serv',
  ['audio/examples/demo_serv.cpp'],
  dependencies: dependency('boost'),
)