if enable_docs
  docs_dir = pps_datadir / 'doc'

  toml_conf = configuration_data()
  toml_conf.set('version', meson.project_version())
  gidocgen = find_program('gi-docgen', required: true)

  subdir('reference/libdocument')
  subdir('reference/libview')
endif

if enable_user_doc
  sources = [
    'annotation-properties.page',
    'annotations-delete.page',
    'annotations-disabled.page',
    'annotations-navigate.page',
    'annotations.page',
    'annotations-save.page',
    'bookmarks.page',
    'bug-filing.page',
    'commandline.page',
    'convertSVG.page',
    'develop.page',
    'documentation.page',
    'duplex-10pages.page',
    'duplex-11pages.page',
    'duplex-12pages.page',
    'duplex-13pages.page',
    'duplex-14pages.page',
    'duplex-15pages.page',
    'duplex-16pages.page',
    'duplex-3pages.page',
    'duplex-4pages.page',
    'duplex-5pages.page',
    'duplex-6pages.page',
    'duplex-7pages.page',
    'duplex-8pages.page',
    'duplex-9pages.page',
    'duplex-npages.page',
    'editing.page',
    'finding.page',
    'formats.page',
    'forms.page',
    'forms-saving.page',
    'headerbar.page',
    'index.page',
    'introduction.page',
    'invert-colors.page',
    'license.page',
    'legal.xml',
    'legal-unported.xml',
    'movingaround.page',
    'noprint.page',
    'openerror.page',
    'opening.page',
    'password.page',
    'presentations.page',
    'print-2sided.page',
    'print-booklet.page',
    'print-differentsize.page',
    'printing.page',
    'print-order.page',
    'print-pagescaling.page',
    'print-select.page',
    'reload.page',
    'singlesided-13-16pages.page',
    'singlesided-17-20pages.page',
    'singlesided-3-4pages.page',
    'singlesided-5-8pages.page',
    'singlesided-9-12pages.page',
    'singlesided-npages.page',
    'textselection.page',
    'translate.page',
  ]

  media = [
    'figures/add-text-annotation.png',
    'figures/annotations-nav-to-page.png',
    'figures/emblem-system-symbolic.svg',
    'figures/org.gnome.Papers.svg',
    'figures/search-not-found.png',
    'figures/zoom.png',
  ]

  gnome.yelp(
    pps_name,
    sources: sources,
    media: media,
    symlink_media: true,
  )
endif
