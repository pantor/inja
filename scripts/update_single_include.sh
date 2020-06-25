#!/usr/bin/env sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
SOURCE_ROOT=$(dirname "${DIR}")

echo "Move to Source Root: ${SOURCE_ROOT}"
cd ${SOURCE_ROOT}

python3 third_party/amalgamate/amalgamate.py -c scripts/amalgamate_config.json -s include -v yes
