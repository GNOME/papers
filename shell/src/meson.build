configure_file(
  input: 'config.rs.in',
  output: 'config.rs',
  configuration: config_h,
)

build_root = meson.project_build_root()
source_root = meson.current_source_dir()

cargo = find_program('cargo', required: true)

cargo_options = [ '--manifest-path', shell_rs_root / 'Cargo.toml' ]
cargo_options += [ '--target-dir', meson.current_build_dir() ]

if get_option('optimization') == '0'
  rust_target = 'debug'
  rust_profile = 'dev'
else
  cargo_options += [ '--release' ]
  rust_target = 'release'
  rust_profile = 'release'
endif

# Setup the debuginfo option from the meson build system
cargo_options += ['--config', 'profile.@0@.debug=@1@'.format(rust_profile, get_option('debug'))]

if enable_keyring
  cargo_options += [ '--features', 'with-keyring' ]
endif

if enable_spellcheck
  cargo_options += ['--features', 'spell-check']
endif

cargo_env = environment()
# it seems that flatpak can't access network during install stage.
cargo_env.set('CARGO_HOME', build_root / 'cargo-home')
cargo_env.set('CODEGEN_BUILD_DIR', meson.current_build_dir())
# override library lookup, we should not use the system library.
cargo_env.set('SYSTEM_DEPS_PAPERS_VIEW_4_0_NO_PKG_CONFIG', '1')
cargo_env.set('SYSTEM_DEPS_PAPERS_VIEW_4_0_SEARCH_NATIVE', build_root / 'libview')
cargo_env.set('SYSTEM_DEPS_PAPERS_VIEW_4_0_LIB', 'ppsview-4.0')
cargo_env.set('SYSTEM_DEPS_PAPERS_DOCUMENT_4_0_NO_PKG_CONFIG', '1')
cargo_env.set('SYSTEM_DEPS_PAPERS_DOCUMENT_4_0_SEARCH_NATIVE', build_root / 'libdocument')
cargo_env.set('SYSTEM_DEPS_PAPERS_DOCUMENT_4_0_LIB', 'ppsdocument-4.0')

# If the typename is internal, then we have built libadwaita from the subproject and we nee to tell cargo to look into that
if libaw_dep.type_name() == 'internal'
  cargo_env.set('SYSTEM_DEPS_LIBADWAITA_1_NO_PKG_CONFIG', '1')
  cargo_env.set('SYSTEM_DEPS_LIBADWAITA_1_SEARCH_NATIVE', build_root / 'subprojects/adwaita/src')
  cargo_env.set('SYSTEM_DEPS_LIBADWAITA_1_LIB', 'adwaita-1')
endif

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

meson.add_devenv(cargo_env)
meson_env = environment()
meson_env.prepend('PATH', meson.current_build_dir())
meson.add_devenv(meson_env)
