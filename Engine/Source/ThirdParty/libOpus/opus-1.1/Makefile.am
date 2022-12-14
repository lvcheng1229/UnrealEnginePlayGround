# Provide the full test output for failed tests when using the parallel
# test suite (which is enabled by default with automake 1.13+).
export VERBOSE = yes

AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libopus.la

DIST_SUBDIRS = doc

AM_LDFLAGS =
AM_CFLAGS =
AM_CXXFLAGS =
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/celt -I$(top_srcdir)/silk \
              -I$(top_srcdir)/silk/float -I$(top_srcdir)/silk/fixed

include celt_sources.mk
include silk_sources.mk
include opus_sources.mk

if FIXED_POINT
SILK_SOURCES += $(SILK_SOURCES_FIXED)
else
SILK_SOURCES += $(SILK_SOURCES_FLOAT)
endif

if DISABLE_FLOAT_API
else
OPUS_SOURCES += $(OPUS_SOURCES_FLOAT)
endif

if CLANG_ARCH_X86_64
AM_CPPFLAGS += -arch x86_64
AM_CFLAGS += -arch x86_64
AM_CXXFLAGS += -arch x86_64
AM_LDFLAGS += -arch x86_64
endif

if CLANG_ARCH_ARM64
AM_CPPFLAGS += -arch arm64
AM_CFLAGS += -arch arm64
AM_CXXFLAGS += -arch arm64
AM_LDFLAGS += -arch arm64
endif

if CPU_ARM
CELT_SOURCES += $(CELT_SOURCES_ARM)
SILK_SOURCES += $(SILK_SOURCES_ARM)

if OPUS_ARM_EXTERNAL_ASM
nodist_libopus_la_SOURCES = $(CELT_SOURCES_ARM_ASM:.s=-gnu.S)
BUILT_SOURCES = $(CELT_SOURCES_ARM_ASM:.s=-gnu.S) \
 $(CELT_AM_SOURCES_ARM_ASM:.s.in=.s) \
 $(CELT_AM_SOURCES_ARM_ASM:.s.in=-gnu.S)
endif
endif

CLEANFILES = $(CELT_SOURCES_ARM_ASM:.s=-gnu.S) \
 $(CELT_AM_SOURCES_ARM_ASM:.s.in=-gnu.S)

include celt_headers.mk
include silk_headers.mk
include opus_headers.mk

libopus_la_SOURCES = $(CELT_SOURCES) $(SILK_SOURCES) $(OPUS_SOURCES)
libopus_la_LDFLAGS = -no-undefined -version-info @OPUS_LT_CURRENT@:@OPUS_LT_REVISION@:@OPUS_LT_AGE@
libopus_la_LIBADD = $(LIBM)

pkginclude_HEADERS = include/opus.h include/opus_multistream.h include/opus_types.h include/opus_defines.h

noinst_HEADERS = $(OPUS_HEAD) $(SILK_HEAD) $(CELT_HEAD)

if EXTRA_PROGRAMS
noinst_PROGRAMS = opus_demo repacketizer_demo opus_compare tests/test_opus_api tests/test_opus_encode tests/test_opus_decode tests/test_opus_padding celt/tests/test_unit_cwrs32 celt/tests/test_unit_dft celt/tests/test_unit_entropy celt/tests/test_unit_laplace celt/tests/test_unit_mathops celt/tests/test_unit_mdct celt/tests/test_unit_rotation celt/tests/test_unit_types

TESTS = celt/tests/test_unit_types celt/tests/test_unit_mathops celt/tests/test_unit_entropy celt/tests/test_unit_laplace celt/tests/test_unit_dft celt/tests/test_unit_mdct celt/tests/test_unit_rotation celt/tests/test_unit_cwrs32 tests/test_opus_api tests/test_opus_decode tests/test_opus_encode tests/test_opus_padding

opus_demo_SOURCES = src/opus_demo.c

opus_demo_LDADD = libopus.la $(LIBM)

repacketizer_demo_SOURCES = src/repacketizer_demo.c

repacketizer_demo_LDADD = libopus.la $(LIBM)

opus_compare_SOURCES = src/opus_compare.c
opus_compare_LDADD = $(LIBM)

tests_test_opus_api_SOURCES = tests/test_opus_api.c tests/test_opus_common.h
tests_test_opus_api_LDADD = libopus.la $(LIBM)

tests_test_opus_encode_SOURCES = tests/test_opus_encode.c tests/test_opus_common.h
tests_test_opus_encode_LDADD = libopus.la $(LIBM)

tests_test_opus_decode_SOURCES = tests/test_opus_decode.c tests/test_opus_common.h
tests_test_opus_decode_LDADD = libopus.la $(LIBM)

tests_test_opus_padding_SOURCES = tests/test_opus_padding.c tests/test_opus_common.h
tests_test_opus_padding_LDADD = libopus.la $(LIBM)

celt_tests_test_unit_cwrs32_SOURCES = celt/tests/test_unit_cwrs32.c
celt_tests_test_unit_cwrs32_LDADD = $(LIBM)

celt_tests_test_unit_dft_SOURCES = celt/tests/test_unit_dft.c
celt_tests_test_unit_dft_LDADD = $(LIBM)

celt_tests_test_unit_entropy_SOURCES = celt/tests/test_unit_entropy.c
celt_tests_test_unit_entropy_LDADD = $(LIBM)

celt_tests_test_unit_laplace_SOURCES = celt/tests/test_unit_laplace.c
celt_tests_test_unit_laplace_LDADD = $(LIBM)

celt_tests_test_unit_mathops_SOURCES = celt/tests/test_unit_mathops.c
celt_tests_test_unit_mathops_LDADD = $(LIBM)

celt_tests_test_unit_mdct_SOURCES = celt/tests/test_unit_mdct.c
celt_tests_test_unit_mdct_LDADD = $(LIBM)

celt_tests_test_unit_rotation_SOURCES = celt/tests/test_unit_rotation.c
celt_tests_test_unit_rotation_LDADD = $(LIBM)

celt_tests_test_unit_types_SOURCES = celt/tests/test_unit_types.c
celt_tests_test_unit_types_LDADD = $(LIBM)
endif

if CUSTOM_MODES
pkginclude_HEADERS += include/opus_custom.h
if EXTRA_PROGRAMS
noinst_PROGRAMS += opus_custom_demo
opus_custom_demo_SOURCES = celt/opus_custom_demo.c
opus_custom_demo_LDADD = libopus.la $(LIBM)
endif
endif

EXTRA_DIST = version.mk \
             opus.pc.in \
             opus-uninstalled.pc.in \
             opus.m4 \
             Makefile.unix \
             tests/run_vectors.sh \
             celt/arm/arm2gnu.pl \
             celt/arm/celt_pitch_xcorr_arm.s \
             win32/VS2010/silk_float.vcxproj \
             win32/VS2010/celt.vcxproj.filters \
             win32/VS2010/opus.vcxproj \
             win32/VS2010/silk_common.vcxproj.filters \
             win32/VS2010/silk_float.vcxproj.filters \
             win32/VS2010/test_opus_encode.vcxproj.filters \
             win32/VS2010/silk_common.vcxproj \
             win32/VS2010/test_opus_encode.vcxproj \
             win32/VS2010/opus_demo.vcxproj \
             win32/VS2010/test_opus_api.vcxproj.filters \
             win32/VS2010/test_opus_api.vcxproj \
             win32/VS2010/test_opus_decode.vcxproj.filters \
             win32/VS2010/silk_fixed.vcxproj.filters \
             win32/VS2010/opus_demo.vcxproj.filters \
             win32/VS2010/silk_fixed.vcxproj \
             win32/VS2010/opus.vcxproj.filters \
             win32/VS2010/test_opus_decode.vcxproj \
             win32/VS2010/celt.vcxproj \
             win32/VS2010/opus.sln \
             win32/genversion.bat \
             win32/config.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = opus.pc

m4datadir = $(datadir)/aclocal
m4data_DATA = opus.m4

# Targets to build and install just the library without the docs
opus check-opus install-opus: export NO_DOXYGEN = 1

opus: all
check-opus: check
install-opus: install


# Or just the docs
docs:
	( cd doc && $(MAKE) $(AM_MAKEFLAGS) )

install-docs:
	( cd doc && $(MAKE) $(AM_MAKEFLAGS) install )


# Or everything (by default)
all-local:
	@[ -n "$(NO_DOXYGEN)" ] || ( cd doc && $(MAKE) $(AM_MAKEFLAGS) )

install-data-local:
	@[ -n "$(NO_DOXYGEN)" ] || ( cd doc && $(MAKE) $(AM_MAKEFLAGS) install )

clean-local:
	-( cd doc && $(MAKE) $(AM_MAKEFLAGS) clean )

uninstall-local:
	( cd doc && $(MAKE) $(AM_MAKEFLAGS) uninstall )


# We check this every time make is run, with configure.ac being touched to
# trigger an update of the build system files if update_version changes the
# current PACKAGE_VERSION (or if package_version was modified manually by a
# user with either AUTO_UPDATE=no or no update_version script present - the
# latter being the normal case for tarball releases).
#
# We can't just add the package_version file to CONFIGURE_DEPENDENCIES since
# simply running autoconf will not actually regenerate configure for us when
# the content of that file changes (due to autoconf dependency checking not
# knowing about that without us creating yet another file for it to include).
#
# The MAKECMDGOALS check is a gnu-make'ism, but will degrade 'gracefully' for
# makes that don't support it.  The only loss of functionality is not forcing
# an update of package_version for `make dist` if AUTO_UPDATE=no, but that is
# unlikely to be a real problem for any real user.
$(top_srcdir)/configure.ac: force
	@case "$(MAKECMDGOALS)" in \
	    dist-hook)                             exit 0       ;; \
	    dist-* | dist | distcheck | distclean) _arg=release ;; \
	esac; \
	if ! $(top_srcdir)/update_version $$_arg 2> /dev/null; then \
	    if [ ! -e $(top_srcdir)/package_version ]; then \
		echo 'PACKAGE_VERSION="unknown"' > $(top_srcdir)/package_version; \
	    fi; \
	    . $(top_srcdir)/package_version || exit 1; \
	    [ "$(PACKAGE_VERSION)" != "$$PACKAGE_VERSION" ] || exit 0; \
	fi; \
	touch $@

force:

# Create a minimal package_version file when make dist is run.
dist-hook:
	echo 'PACKAGE_VERSION="$(PACKAGE_VERSION)"' > $(top_distdir)/package_version


.PHONY: opus check-opus install-opus docs install-docs

# automake doesn't do dependency tracking for asm files, that I can tell
$(CELT_SOURCES_ARM_ASM:%.s=%-gnu.S): celt/arm/armopts-gnu.S
$(CELT_SOURCES_ARM_ASM:%.s=%-gnu.S): $(top_srcdir)/celt/arm/arm2gnu.pl

# convert ARM asm to GNU as format
%-gnu.S: $(top_srcdir)/%.s
	$(top_srcdir)/celt/arm/arm2gnu.pl < $< > $@
# For autoconf-modified sources (e.g., armopts.s)
%-gnu.S: %.s
	$(top_srcdir)/celt/arm/arm2gnu.pl < $< > $@
