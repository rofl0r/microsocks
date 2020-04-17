#!/bin/sh

# Dist Clean
make distclean >/dev/null 2>&1 

# Autotools Directories
rm -rf config 
rm -rf src/.deps
rm -rf autom4te.cache

# Build Output
rm -f src/*.o src/microsocks 

# Generated Files
find -type f -name Makefile.in -delete
find -name Makefile -delete
rm -f config.log config.status configure aclocal.m4

# Dist tarball if it was created
rm -f microsocks-*.tar.gz
rm -rf microsocks-*
