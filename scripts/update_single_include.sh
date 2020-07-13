#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd $(dirname "${DIR}")

python3 third_party/amalgamate/amalgamate.py -c scripts/amalgamate_config.json -s include -v yes
