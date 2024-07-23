inc = include_directories('.')
cpp_args = ['-Wall', '-Wextra']

# for use in tests
chat_lib = static_library('chat_lib', [
    # 'gui/gui.cpp',
    'gui/text.cpp',
    'utils/unicode.cpp'
  ],
  dependencies : [imgui_dep, utfcpp_dep],
  cpp_args : cpp_args,
  include_directories : inc,
)
chat_dep = declare_dependency(
  include_directories: inc,
  link_with: chat_lib
)

executable('chat', [
    'main.cpp'
  ],
  dependencies: [chat_dep, imgui_dep, utfcpp_dep],
  link_args : ['-lGL', '-lglfw', '-lfreetype']
)
