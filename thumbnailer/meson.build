input = 'papers.thumbnailer'

configure_file(
  input: input + '.in',
  output: input,
  configuration: mime_types_conf,
  install: true,
  install_dir: join_paths(pps_datadir, 'thumbnailers'),
)

thumbnailer_sources = files(
  'papers-thumbnailer.c',
)

thumbnailer_deps = [
  libppsdocument_dep,
]

executable(
  'papers-thumbnailer',
  sources: thumbnailer_sources,
  include_directories: top_inc,
  dependencies: thumbnailer_deps,
  link_args: common_ldflags,
  install: true,
)
