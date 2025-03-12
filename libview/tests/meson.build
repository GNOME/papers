SRCDIR = meson.current_source_dir()

cflags = [
  '-DG_LOG_DOMAIN="PapersViewTests"',
  '-DPAPERS_COMPILATION',
  '-DSRCDIR="@0@"'.format(SRCDIR)
]

test_executable = executable(
  'test-pps-jobs',
  'test-pps-jobs.c',
  include_directories: [top_inc],
  dependencies: libppsview_dep,
  c_args: cflags
)

test('pps-jobs-test', test_executable, env: backend_env)
