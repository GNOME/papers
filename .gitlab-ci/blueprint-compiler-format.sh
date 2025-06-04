#!/bin/sh

set -e

exitval=0

formattable="$(find libview/ previewer/ shell/ -name "*.blp")"
unformatted="$(for f in $formattable; do blueprint-compiler format --no-diff $f 2>&1 >/dev/null || echo "$f"; done)"

if [ ${#unformatted} -ne 0 ]; then
  exitval=1

  echo >&2 "The following files are not correctly formatted:"
  for f in "${unformatted}"; do
    echo "$f" >&2
  done
  echo >&2

  echo "The files above were not correctly formated."
  echo "Please run 'blueprint-compiler format --fix' to fix them."
fi

exit $exitval
