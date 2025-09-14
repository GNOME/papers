backends_common_incs = [top_inc]
backends_common_deps = [libppsdocument_dep]
backends_common_cflags = [
  '-DPAPERS_COMPILATION',
]

foreach backend, backend_mime_types: backends
  backend_name = backend + 'document'
  backend_id = 'org.gnome.Papers.@0@Document'.format(backend.substring(0, 1).to_upper() + backend.substring(1))

  # Set default values, let each backend configure its own
  backend_sources = []
  backend_incs = backends_common_incs
  backend_deps = backends_common_deps
  backend_cflags = backends_common_cflags
  subdir(backend)

  shared_module(
    backend_name,
    sources: backend_sources,
    include_directories: backend_incs,
    dependencies: backend_deps,
    c_args: backend_cflags,
    gnu_symbol_visibility: 'hidden',
    install: true,
    install_dir: pps_backendsdir,
  )

  backend_mime_types_conf = configuration_data()
  backend_mime_types_conf.set(backend.to_upper() + '_MIME_TYPES', ';'.join(backend_mime_types))

  metainfo_mime_types = ''
  foreach type: backend_mime_types
    metainfo_mime_types += '\t<mediatype>@0@</mediatype>\n'.format(type)
  endforeach

  backend_mime_types_conf.set('METAINFO_@0@_MIME_TYPES'.format(backend.to_upper()), metainfo_mime_types)

  backend_desc = backend_id + '.papers-backend'

  backend_desc_in = configure_file(
    input: '@0@/@1@.desktop.in.in'.format(backend, backend_desc),
    output: backend_desc + '.desktop.in',
    configuration: backend_mime_types_conf,
  )

  i18n.merge_file(
    type: 'desktop',
    input: backend_desc_in,
    output: backend_desc,
    po_dir: po_dir,
    install: true,
    install_dir: pps_backendsdir,
    args: '--keyword=TypeDescription',
  )

  backend_metainfo = '@0@.metainfo.xml'.format(backend_id)
  i18n.merge_file(
    input: configure_file(
      input: '@0@/@1@.in.in'.format(backend, backend_metainfo),
      output: backend_metainfo + '.in',
      configuration: backend_mime_types_conf,
    ),
    output: backend_metainfo,
    po_dir: po_dir,
    install: true,
    install_dir: join_paths(pps_datadir, 'metainfo'),
  )

  if get_option('tests')
    if (appstreamcli.found())
      test('validate-@0@-metainfo'.format(backend_id),
           appstreamcli,
           args: ['validate', '--no-net', '--explain', backend_metainfo],
           workdir: meson.current_build_dir()
          )
    endif
  endif
endforeach

backend_env = environment()
backend_env.set('PPS_BACKENDS_DIR', meson.current_build_dir())
meson.add_devenv(backend_env)
