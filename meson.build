project('chat', 'cpp')

cmake = import('cmake')

utfcpp_proj = cmake.subproject('utfcpp')
utfcpp_dep = utfcpp_proj.dependency('utf8cpp')

gsl_proj = cmake.subproject('GSL')
gsl_dep = gsl_proj.dependency('GSL')

rnnoise_dep = dependency('rnnoise', required : false)
if not rnnoise_dep.found()
  rnnoise_dep = subproject('rnnoise').get_variable('rnnoise_dep')
endif

subdir('src')
subdir('test')
