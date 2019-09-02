NULL =

SUBDIRS = \
	helpers \
	libkbdfile \
	libkeymap \
	$(NULL)

AUTOM4TE = $(SHELL) $(top_srcdir)/config/missing --run autom4te
AUTOTEST = $(AUTOM4TE) --language=autotest

KEYWORDS ?=

ATCONFIG  = $(abs_builddir)/atconfig
ATPACKAGE = $(abs_builddir)/atpackage
TESTSUITE = $(abs_builddir)/testsuite

EXTRA_DIST = \
	data          \
	libkbdfile.at \
	libkeymap.at  \
	testsuite.at  \
	$(NULL)

CLEANFILES = \
	$(ATCONFIG)      \
	$(ATPACKAGE)     \
	$(TESTSUITE)     \
	$(TESTSUITE).log \
	$(NULL)

all: $(TESTSUITE)

# The `:;' works around a Bash 3.2 bug when the output is not writable.
$(ATPACKAGE): $(top_srcdir)/configure.ac
	:;{ \
	    echo '# Signature of the current package.';                       \
	    echo 'm4_define([AT_PACKAGE_NAME],      [$(PACKAGE_NAME)])';      \
	    echo 'm4_define([AT_PACKAGE_TARNAME],   [$(PACKAGE_TARNAME)])';   \
	    echo 'm4_define([AT_PACKAGE_VERSION],   [$(PACKAGE_VERSION)])';   \
	    echo 'm4_define([AT_PACKAGE_STRING],    [$(PACKAGE_STRING)])';    \
	    echo 'm4_define([AT_PACKAGE_BUGREPORT], [$(PACKAGE_BUGREPORT)])'; \
	    echo 'm4_define([AT_PACKAGE_URL],       [$(PACKAGE_URL)])';       \
	} >'$(ATPACKAGE)'

$(ATCONFIG): $(top_builddir)/config.status
	cd $(top_builddir) && \
	$(SHELL) ./config.status tests/atconfig

check-local: $(ATCONFIG) $(TESTSUITE)
	$(SHELL) '$(TESTSUITE)' -k '$(KEYWORDS)' $(TESTSUITEFLAGS)

installcheck-local: $(ATCONFIG) $(TESTSUITE)
	$(SHELL) '$(TESTSUITE)' -k '$(KEYWORDS)' AUTOTEST_PATH='$(bindir)' $(TESTSUITEFLAGS)

$(TESTSUITE): $(ATPACKAGE) $(ATCONFIG)
	$(AUTOTEST) -I '$(srcdir)' -o - testsuite.at > $@
