project('pcsx2_ipc_c', 'cpp', default_options : ['cpp_std=c++17',
  'buildtype=release'])

winsock = dependency('ws2_32', required : false)
thread_dep = dependency('threads')
src = ['src/client.cpp', 'src/pcsx2_ipc.h']
executable('client', src, dependencies : [thread_dep, winsock])



catch2 = dependency('catch2', required : false)
test_src = ['src/tests.cpp']
if catch2.found()
  # TODO: in the future if we need to add threads to the API test cases might
  # run into an infinite loop, i have absolutely no clue why that happens but
  # including thread_dep here just leads to it. so don't include that future me.
  e = executable('tests', test_src, dependencies : [catch2, winsock], cpp_args :
    '-DTESTS')
  test('tests', e)
endif


cleaner = find_program('clang-format', required : false)
if cleaner.found()
  run_command(cleaner, '-i', src)
  run_command(cleaner, '-i', test_src)
endif
