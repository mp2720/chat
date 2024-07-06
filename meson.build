project('chat', 'cpp')

cmake = import('cmake')

imgui_proj = subproject('imgui')
imgui_dep = imgui_proj.get_variable('imgui_dep')

utfcpp_proj = cmake.subproject('utfcpp')
utfcpp_dep = utfcpp_proj.dependency('utf8cpp')

executable('chat', [
    'src/main.cpp',
    'src/gui/text.cpp',
    'src/utils/unicode.cpp'
  ],
  dependencies : [imgui_dep, utfcpp_dep],
  cpp_args : ['-Wall', '-Wextra'],
  include_directories : ['src/'],
  link_args : ['-lGL', '-lglfw', '-lfreetype'],
)
