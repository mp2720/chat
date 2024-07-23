gtest = dependency('gtest', main : true, required : false)
if not gtest.found()
  error('dependency gtest not found')
endif

tests = [
  'example'
]

foreach t : tests
  test('gtest test ' + t, executable(
    t.underscorify(), t + '.cpp',
    dependencies: [gtest, chat_dep, utfcpp_dep]
  ))
endforeach
