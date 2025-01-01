project('chat', 'cpp')

cmake = import('cmake')

rnnoise_dep = dependency('rnnoise', required: false)
if not rnnoise_dep.found()
  rnnoise_dep = subproject('rnnoise').get_variable('rnnoise_dep')
endif

chat_deps = [
  cmake.subproject('GSL').dependency('GSL'),
  dependency('gl'),
  dependency('glfw3'),
  dependency('boost'),
  dependency('opus'),
  dependency('speexdsp'),
  dependency('portaudio-2.0'),
  rnnoise_dep,
]

if not meson.get_compiler('cpp').check_header('glm/glm.hpp')
  error('require glm library')
endif

if target_machine.system() == 'windows'
  chat_deps += meson.get_compiler('cpp').find_library('ws2_32', required: true)
endif

add_project_arguments(
  '-DCHAT_BUILD_TARGET_' + target_machine.system().to_upper(),
  language: ['c', 'cpp'],
)

subdir('src')

executable(
  'chat',
  ['src/main.cpp'],
  dependencies: chat_lib_dep,
)

executable(
  'aud_echo',
  ['src/audio/examples/echo.cpp'],
  dependencies: chat_lib_dep,
)

subdir('test')