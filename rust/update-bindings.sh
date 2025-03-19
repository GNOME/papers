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

for d in papers-document papers-view; do
	pushd $d > /dev/null
	pushd sys > /dev/null
	gir -d ${MESON_SOURCE_ROOT}/rust/gir-files -d `dirname $1` -d `dirname $2` -o .
	popd &> /dev/null
	gir -d ${MESON_SOURCE_ROOT}/rust/gir-files -d `dirname $1` -d `dirname $2` -o .
	popd > /dev/null
done
