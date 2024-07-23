project('chat', 'cpp')

cmake = import('cmake')

imgui_proj = subproject('imgui')
imgui_headers_dep = imgui_proj.get_variable('imgui_headers_dep')
imgui_dep = imgui_proj.get_variable('imgui_dep')

utfcpp_proj = cmake.subproject('utfcpp')
utfcpp_dep = utfcpp_proj.dependency('utf8cpp')

subdir('src')
subdir('test')
