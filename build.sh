#!/usr/local/bin/bash
set -o physical -o errtrace -o errexit -o nounset -o pipefail
shopt -s failglob inherit_errexit

exec gcc set-added-date-from-modified.c -o set-added-date-from-modified
