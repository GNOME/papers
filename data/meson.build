subdir('icons')

# Desktop file
desktops = [[pps_namespace + '.desktop', app_id + '.desktop']]

if enable_previewer
  desktops += [[pps_namespace + '-previewer.desktop', app_id + '-previewer.desktop']]
endif

foreach desktop: desktops
  desktop_in = desktop[0]
  desktop_out = desktop[1]
  i18n.merge_file(
    type: 'desktop',
    input: configure_file(
      input: desktop_in + '.in.in',
      output: desktop_in + '.in',
      configuration: mime_types_conf,
    ),
    output: desktop_out,
    po_dir: po_dir,
    install: true,
    install_dir: join_paths(pps_datadir, 'applications'),
  )
endforeach

# GSettings schema
gschema_file = pps_namespace + '.gschema.xml'
install_data(
  gschema_file,
  install_dir: join_paths(pps_datadir, 'glib-2.0', 'schemas'),
)

gnome.compile_schemas(
  depend_files: gschema_file,
)

# man file
man_pages = ['papers.1', 'papers-previewer.1', 'papers-thumbnailer.1']
foreach man_page: man_pages
  install_data(
    man_page,
    install_dir: join_paths(pps_mandir, 'man1'),
  )
endforeach

# Metainfo file
metainfo_conf = configuration_data()
metainfo_conf.set('APP_ID', app_id)
metainfo_file = i18n.merge_file(
  input: configure_file(
    input: files('@0@.metainfo.xml.in.in'.format(pps_namespace)),
    output: '@0@.metainfo.xml.in'.format(pps_namespace),
    configuration: metainfo_conf,
  ),
  output: '@0@.metainfo.xml'.format(app_id),
  po_dir: po_dir,
  install: true,
  install_dir: join_paths(pps_datadir, 'metainfo'),
)

# Validate Appdata
if get_option('tests')
  if (appstreamcli.found())
    test('validate-appdata',
         appstreamcli,
         args: ['validate', '--no-net', '--strict', '--pedantic', '--explain', metainfo_file],
         workdir: meson.current_build_dir()
        )
  endif
endif
