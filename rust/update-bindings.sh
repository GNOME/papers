#!/bin/bash

if [ -n "${MESON_SOURCE_ROOT}" ]; then
	cd "${MESON_SOURCE_ROOT}/rust"
fi

export PATH="$PATH:${MESON_SOURCE_ROOT}/rust/gir/target/release"

if ! command -v gir &> /dev/null
then
	echo "command gir could not be found in PATH"
	exit 1
fi

for g in ${@:1}; do
	cp "$g" pps-girs
done

for d in papers-document papers-view; do
	pushd $d > /dev/null
	pushd sys > /dev/null
	gir -o .
	popd &> /dev/null
	gir -o .
	popd > /dev/null
done
