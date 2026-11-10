#!/bin/sh
# The sandbox-escape regressions now drive the real runner engine (no hand-copied bwrap flags).
# Optional argument: a language (c|rust|go) or a substring of a case name.
cd "$(dirname "$0")/../.." || exit 2
exec node --conditions=react-server --import tsx scripts/security/sandbox-escape.mts "$@"
