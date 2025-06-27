input = 'papers.thumbnailer'

configure_file(
  input: input + '.in',
  output: input,
  configuration: mime_types_conf,
  install: true,
  install_dir: join_paths(pps_datadir, 'thumbnailers'),
)

cargo = find_program('cargo', required: true)

thumbnailer_root = meson.current_source_dir()
cargo_options = ['--manifest-path', thumbnailer_root / 'Cargo.toml']
cargo_options += ['--target-dir', meson.current_build_dir()]

if rust_target == 'release'
  cargo_options += [ '--release' ]
endif

cargo_env = cargo_common_env

cargo_build = custom_target(
  'cargo-build',
  build_by_default: true,
  build_always_stale: true,
  output: 'src',
  console: true,
  env: cargo_env,
  command: [
    cargo,
    'build',
    cargo_options,
  ],
)

copy = custom_target(
  'cp-binary',
  depends: cargo_build,
  build_by_default: true,
  build_always_stale: true,
  install: true,
  install_dir: get_option('bindir'),
  output: meson.project_name() + '-thumbnailer',
  command: [
    'cp',
    meson.current_build_dir() / rust_target / meson.project_name()
    + '-thumbnailer',
    '@OUTPUT@',
  ],
)

pps_library_path = '@0@/libview:@0@/libdocument:@0@/shell'.format(build_root)
test_env = backend_env
test_env.set('DYLD_LIBRARY_PATH', pps_library_path)
test_env.set('LD_LIBRARY_PATH', pps_library_path)
test_env.set('G_DEBUG', 'fatal-criticals')

meson_env = environment()
meson_env.prepend('PATH', meson.current_build_dir())
meson.add_devenv(meson_env)

if get_option('tests')
  test(
    'thumbnailer',
    files(thumbnailer_root / 'thumbnailer-test.sh'),
    args: [meson.project_source_root() / 'libview' / 'tests' / 'utf16le-annot.pdf'],
    env: test_env,
    workdir: meson.current_build_dir(),
    depends: [copy],
  )
endif
