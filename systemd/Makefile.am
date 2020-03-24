EXTRA_DIST = microsocks.service.in microsocks

if HAVE_SYSTEMD
   # systemd unit file
   systemdsystemunit_DATA = microsocks.service

   # systemd options file
   sysconfigdir = @sysconfdir@/sysconfig
   sysconfig_DATA = microsocks
endif
