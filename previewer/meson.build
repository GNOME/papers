previewer_sources = files(
  'pps-previewer-window.c',
  'pps-previewer-window.h',
  'pps-page-selector.c',
  'pps-page-selector.h',
  'pps-previewer.c',
)

previewer_sources += gnome.compile_resources(
  'pps-previewer-resources',
  'previewer.gresource.xml',
  source_dir: data_dir,
  c_name: 'pps_previewer',
)

previewer_deps = [
  libppsdocument_dep,
  libppsview_dep,
  libaw_dep
]

if enable_gtk_unix_print
  previewer_deps += gtk_unix_print_dep
endif

executable(
  'papers-previewer',
  previewer_sources,
  include_directories: top_inc,
  dependencies: previewer_deps,
  link_args: common_ldflags,
  install: true,
)
