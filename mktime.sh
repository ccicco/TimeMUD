#!/bin/sh
# mktime.sh - create the TimeMUD repo skeleton
# Settled layout 2026-11-18: five top folders, 13 categories, data mirrors code.
# Idempotent: safe to re-run; existing folders and files are left untouched.
# POSIX sh: works with sh, dash, or bash.

ROOT="${1:-$HOME/TimeMUD}"

# The 13 settled categories, in design-doc order
CATEGORIES="comm session commands world obj entities magic combat
skills economy events persist script"

# Runtime/generated dirs (gitignored)
GENERATED="build log"

echo "Creating TimeMUD skeleton at $ROOT ..."

mkdir -p "$ROOT"

# code/: the 13 categories + include/ + tests/ (harness is a client of the headless core)
for c in $CATEGORIES; do
  mkdir -p "$ROOT/code/$c"
done
mkdir -p "$ROOT/code/include" "$ROOT/code/tests"

# data/: mirrors the categories that carry content
for c in $CATEGORIES; do
  mkdir -p "$ROOT/data/$c"
done

# doc/, tools/, and the generated dirs
mkdir -p "$ROOT/doc" "$ROOT/tools"
for g in $GENERATED; do
  mkdir -p "$ROOT/$g"
done

echo "Done. Tree:"
if command -v tree >/dev/null 2>&1; then
  tree -L 2 --dirsfirst "$ROOT"
else
  find "$ROOT" -type d | sort | sed "s|$ROOT|.|;s|[^/]*/|  |g"
fi
