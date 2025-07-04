cargo = find_program('cargo', required: true)

cargo_options = [ '--manifest-path', shell_rs_root / 'Cargo.toml' ]
cargo_options += [ '--target-dir', meson.current_build_dir() ]

if rust_target == 'release'
  cargo_options += [ '--release' ]
endif

if enable_keyring
  cargo_options += [ '--features', 'with-keyring' ]
endif

if enable_spellcheck
  cargo_options += ['--features', 'spell-check']
endif

cargo_env = cargo_common_env

cargo_env.set('PAPERS_RESOURCES_FILE', papers_resources.full_path())

papers_resources_env = environment()
papers_resources_env.set('PAPERS_RESOURCES_FILE', papers_resources.full_path())
meson.add_devenv(papers_resources_env)


cargo_build = custom_target(
  'cargo-build',
  build_by_default: true,
  build_always_stale: true,
  output: 'src',
  console: true,
  depends: [papers_resources],
  env: cargo_env,
  command: [
    cargo, 'build',
    cargo_options,
  ]
)

custom_target(
  'cp-binary',
  depends: cargo_build,
  build_by_default: true,
  build_always_stale: true,
  install: true,
  install_dir: get_option('bindir'),
  output: meson.project_name(),
  command: [
    'cp',
    meson.current_build_dir() / rust_target / meson.project_name(),
    '@OUTPUT@',
  ],
)

custom_target(
  'cargo-doc',
  build_by_default: false,
  build_always_stale: true,
  output: 'doc',
  console: true,
  env: cargo_env,
  command: [
    cargo, 'doc',
    cargo_options + ['--document-private-items', '--all-features', '--workspace'],
  ],
)

pps_library_path = '@0@/libview:@0@/libdocument:@0@/shell'.format(build_root)
test_env = cargo_env
test_env.set('DYLD_LIBRARY_PATH', pps_library_path)
test_env.set('LD_LIBRARY_PATH', pps_library_path)

cargo_clippy = find_program('cargo-clippy', required: false)
if cargo_clippy.found()
  run_target(
    'cargo-clippy',
    command: [
      cargo,
      'clippy',
      cargo_options,
      '--no-deps',
      '--',
      '-D',
      'warnings',
    ],
    depends: cargo_build,
    env: cargo_env,
  )
endif

test(
  'cargo-test',
  cargo,
  args: [
    'test',
    cargo_options,
    '--',
    '--test-threads=1',
  ],
  depends: cargo_build,
  env: test_env,
  is_parallel: false,
  timeout: 1800,
)

meson_env = environment()
meson_env.prepend('PATH', meson.current_build_dir())
meson.add_devenv(meson_env)
