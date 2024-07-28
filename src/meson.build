inc = include_directories('.')
cpp_args = ['-Wall', '-Wextra']

chat_deps = [dependency('gl'), dependency('glfw3'), utfcpp_dep]

# for use in tests
chat_lib = static_library('chat_lib', [
    'gui/io.cpp',
    'gui/gui.cpp',
    'log.cpp'
  ],
  dependencies : chat_deps,
  cpp_args : cpp_args,
  include_directories : inc,
)
chat_lib_dep = declare_dependency(
  include_directories: inc,
  link_with: chat_lib
)

executable(
  'chat',
  ['main.cpp'],
  dependencies: chat_deps + chat_lib_dep,
)

audio_lib = static_library(
  'audio_lib',
  [
    'audio/lib.cpp',
    'audio/player.cpp',
    'audio/recorder.cpp',
    'audio/rnnoise_dsp.cpp',
    'audio/volume_dsp.cpp',
    'audio/codec.cpp',
  ],
  dependencies: [portaudiocpp_dep, rnnoise_dep, opus_dep],
  include_directories: 'audio/',
  cpp_args: cpp_args,
)

audio_dep = declare_dependency(
  include_directories: 'audio/',
  link_with: audio_lib,
  dependencies: [portaudiocpp_dep, rnnoise_dep, opus_dep],
)

executable(
  'echo',
  ['audio/examples/echo.cpp'],
  dependencies: [audio_dep],
)

executable(
  'echo_opus',
  ['audio/examples/echo_opus.cpp'],
  dependencies: [audio_dep],
)
