project('chat', 'cpp')

cmake = import('cmake')

rnnoise_dep = dependency('rnnoise', required : false)
if not rnnoise_dep.found()
  rnnoise_dep = subproject('rnnoise').get_variable('rnnoise_dep')
endif

chat_deps = [
  cmake.subproject('GSL').dependency('GSL'),
  dependency('gl'),
  dependency('glfw3'),
  dependency('portaudiocpp'),
  dependency('opus'),
  dependency('boost'),
  subproject('portaudio').get_variable('portaudiocpp_dep'),
  rnnoise_dep,
]


if target_machine.system() == 'windows'
    add_project_arguments('-DCHAT_BUILD_WINDOWS', language : ['c', 'cpp'])
    chat_deps += meson.get_compiler('cpp').find_library('ws2_32', required: true)
endif

subdir('src')

executable(
  'chat',
  ['src/main.cpp'],
  dependencies: chat_lib_dep,
)

executable(
  'echo',
  ['src/audio/examples/echo.cpp'],
  dependencies: chat_lib_dep,
)

executable(
  'echo_opus',
  ['src/audio/examples/echo_opus.cpp'],
  dependencies: chat_lib_dep,
)

executable(
  'audio_demo',
  ['src/audio/examples/demo.cpp'],
  dependencies: chat_lib_dep,
)

executable(
  'audio_demo_buf',
  ['src/audio/examples/demo_buf.cpp'],
  dependencies: chat_lib_dep,
)


executable(
  'audio_demo_serv',
  ['src/audio/examples/demo_serv.cpp'],
  dependencies: chat_lib_dep,
)

subdir('test')
