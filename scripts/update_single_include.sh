#!/usr/bin/env sh

if [ "${MESON_SOURCE_ROOT}" != "" ]
then
    cd ${MESON_SOURCE_ROOT}
fi

python3 amalgamate/amalgamate.py -c amalgamate/config.json -s include -v yes
