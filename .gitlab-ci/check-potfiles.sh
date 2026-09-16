#!/bin/sh

cdirs="shell/ libdocument/ libview/ nautilus/ previewer/ thumbnailer/"
rsdirs="shell/src"
uidirs="shell/resources libview/ previewer"
desktopdirs="data/ libdocument/"

# find C files that contain gettext keywords
files=$(grep -lR --include='*.c' '\(gettext\|[^I_)]_\) \?(' $cdirs)

# find Rust files that contain gettext keywords
files="$files "$(grep -lR --include='*.rs' 'gettext\(_f\)\?(' $rsdirs/)

# find blp files that contain translatable string
files="$files "$(grep -lRi --include='*.blp' '_("' $uidirs)"

# find .desktop files
files="$files "$(find $desktopdirs -name '*.desktop*')

# filter out excluded files
if [ -f po/POTFILES.skip ]; then
  files=$(for f in $files; do ! grep -q ^$f po/POTFILES.skip && echo $f; done)
fi

# find those that aren't listed in POTFILES.in
missing=$(for f in $files; do ! grep -q ^$f po/POTFILES.in && echo $f; done)

if [ ${#missing} -eq 0 ]; then
  exit 0
fi

cat >&2 <<EOT

The following files are missing from po/POTFILES.po:

EOT

for f in $missing; do
  echo "  $f" >&2
done
echo >&2

exit 1
