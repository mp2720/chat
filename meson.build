project('chat', 'cpp')

cmake = import('cmake')

utfcpp_proj = cmake.subproject('utfcpp')
utfcpp_dep = utfcpp_proj.dependency('utf8cpp')

rnnoise_dep = dependency('rnnoise', required : false)
if not rnnoise_dep.found()
  rnnoise_dep = subproject('rnnoise').get_variable('rnnoise_dep')
endif

portaudiocpp_dep = dependency('portaudiocpp')

opus_dep = dependency('opus')

subdir('src')
subdir('test')
