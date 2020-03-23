# Include subdirs
SUBDIRS = src . systemd

# Keep support files
EXTRA_DIST = clean.sh \
             autogen.sh \
             LICENSE \
             README.md

# Handle Systemd
DISTCHECK_CONFIGURE_FLAGS = \
        --with-systemdsystemunitdir=$$dc_install_base/$(systemdsystemunitdir)
