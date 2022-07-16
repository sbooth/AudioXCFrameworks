AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = CMakeLists.txt libmpc.kdevelop docs win32

if MPC_CHAP
  MPCCHAP_DIR = mpcchap
endif
if MPC_GAIN
  MPCGAIN_DIR = mpcgain
endif

SUBDIRS = \
include \
libmpcdec \
libmpcenc \
libmpcpsy \
libwavformat \
mpcenc \
mpc2sv8 \
mpccut \
mpcdec \
wavcmp \
$(MPCCHAP_DIR) \
$(MPCGAIN_DIR)

