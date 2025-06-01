#!/bin/sh
# Run this to generate all the initial makefiles, etc.

# Fail on error
set -e

# Check for required tools
autoreconf --version >/dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`autoreconf\` installed to."
  echo "Download the appropriate package for your distribution,"
  echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
  exit 1
}

# Clean up any previous autotools files if present
rm -f config.guess config.sub install-sh missing mkinstalldirs ltmain.sh
rm -rf autom4te.cache

# Generate the build system
echo "Running autoreconf --force --install --verbose..."
autoreconf --force --install --verbose

echo
echo "Disaster Party build system generated."
echo "Now you can run './configure' and 'make'."

