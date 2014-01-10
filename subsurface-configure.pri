#
# Global settings
#
# Set some C constructs to be diagnosed as errors:
#  - calling implicit functions
#  - casting from integers to pointers or vice-versa without an explicit cast
# Also turn on C99 mode with GNU extensions
*-g++*: QMAKE_CFLAGS += -Werror=int-to-pointer-cast -Werror=pointer-to-int-cast -Werror=implicit-int 

# these warnings are in general just wrong and annoying - but should be
# turned on every once in a while in case they do show the occasional
# actual bug
# turns out the gcc 4.2 (as used on MacOS 10.6) doesn't have no-unused-result, yet
*-clang*: QMAKE_CFLAGS += -Wno-unused-result -Wno-pointer-sign -fno-strict-overflow
*-g++*: {
	system( g++ --version | grep -e "4\\.2\\." > /dev/null ) {
		QMAKE_CFLAGS += -Wno-pointer-sign -fno-strict-overflow
	} else {
		QMAKE_CFLAGS += -Wno-unused-result -Wno-pointer-sign -fno-strict-overflow
	}
}

*-clang*: QMAKE_CFLAGS += -Wno-format-security
*-g++*: QMAKE_CXXFLAGS += -fno-strict-overflow
!win32: !mac: {
*-g++*: QMAKE_CXXFLAGS += -Wno-maybe-uninitialized
*-g++*: QMAKE_CFLAGS += -Wno-maybe-uninitialized
}
mac: QMAKE_CLAGS += -mmacosx-version-min=10.5
mac: QMAKE_CXXLAGS += -mmacosx-version-min=10.5


!win32-msvc*: QMAKE_CFLAGS += -std=gnu99

# Don't turn warnings on (but don't suppress them either)
CONFIG -= warn_on warn_off

# Turn exceptions off
!win32-msvc*: QMAKE_CXXFLAGS += -fno-exceptions
CONFIG += exceptions_off

# Check if we have pkg-config
isEmpty(PKG_CONFIG):PKG_CONFIG=pkg-config
equals($$QMAKE_HOST.os, "Windows"):NUL=NUL
else:NUL=/dev/null
PKG_CONFIG_OUT = $$system($$PKG_CONFIG --version 2> $$NUL)
!isEmpty(PKG_CONFIG_OUT) {
	CONFIG += link_pkgconfig
} else {
	message("pkg-config not found, no detection performed. See README for details")
}

#
# Find libdivecomputer
#
!isEmpty(LIBDCDEVEL) {
	# find it next to our sources
	INCLUDEPATH += ../libdivecomputer/include
	LIBS += ../libdivecomputer/src/.libs/libdivecomputer.a
	LIBDC_LA = ../libdivecomputer/src/libdivecomputer.la
} else:!isEmpty(CROSS_PATH):exists($${CROSS_PATH}"/lib/libdivecomputer.a"):exists($${CROSS_PATH}"/lib/libusb-1.0.a") {
	LIBS += $${CROSS_PATH}"/lib/libdivecomputer.a" $${CROSS_PATH}"/lib/libusb-1.0.a"
} else:exists(/usr/local/lib/libdivecomputer.a) {
	LIBS += /usr/local/lib/libdivecomputer.a
	LIBDC_LA = /usr/local/lib/libdivecomputer.la
} else:exists(/usr/local/lib64/libdivecomputer.a) {
	LIBS += /usr/local/lib64/libdivecomputer.a
	LIBDC_LA = /usr/local/lib64/libdivecomputer.la
} else:link_pkgconfig {
	# find it via pkg-config, but we need to pass the --static flag,
	# so we can't use the PKGCONFIG variable.
	LIBS += $$system($$PKG_CONFIG --static --libs libdivecomputer)
	LIBDC_CFLAGS = $$system($$PKG_CONFIG --static --cflags libdivecomputer)
	QMAKE_CFLAGS += $$LIBDC_CFLAGS
	QMAKE_CXXFLAGS += $$LIBDC_CFLAGS
	unset(LIBDC_CFLAGS)
}

!isEmpty(LIBDC_LA):exists($$LIBDC_LA) {
	# Source the libtool .la file to get the dependent libs
	LIBS += $$system(". $$LIBDC_LA && echo \$dependency_libs")
	unset(LIBDC_LA)
}

#
# Find libxml2 and libxslt
#
# They come with shell scripts that contain the information we need, so we just
# run them. They also come with pkg-config files, but those are missing on
# Mac (where they are part of the XCode-supplied tools).
#
XML2_CFLAGS = $$system(xml2-config --cflags 2>$$NUL)
XSLT_CFLAGS = $$system(xslt-config --cflags 2>$$NUL)
XML2_LIBS = $$system(xml2-config --libs 2>$$NUL)
XSLT_LIBS = $$system(xslt-config --libs 2>$$NUL)

link_pkgconfig {
	isEmpty(XML2_CFLAGS)|isEmpty(XML2_LIBS) {
		XML2_CFLAGS = $$system($$PKG_CONFIG --cflags libxml2 2> $$NUL)
		XML2_LIBS = $$system($$PKG_CONFIG --libs libxml2 2> $$NUL)
	}
	isEmpty(XSLT_CFLAGS)|isEmpty(XSLT_LIBS) {
		XSLT_CFLAGS = $$system($$PKG_CONFIG --cflags libxslt 2> $$NUL)
		XSLT_LIBS = $$system($$PKG_CONFIG --libs libxslt 2> $$NUL)
	}
	isEmpty(XML2_CFLAGS)|isEmpty(XML2_LIBS): \
		error("Could not find libxml2. Did you forget to install it?")
	isEmpty(XSLT_CFLAGS)|isEmpty(XSLT_LIBS): \
		error("Could not find libxslt. Did you forget to install it?")
}

QMAKE_CFLAGS *= $$XML2_CFLAGS $$XSLT_CFLAGS
QMAKE_CXXFLAGS *= $$XML2_CFLAGS $$XSLT_CFLAGS
LIBS *= $$XML2_LIBS $$XSLT_LIBS

EXIV2_CFLAGS = $$system($$PKG_CONFIG --cflags exiv2 2> $$NUL)
EXIV2_LIBS = $$system($$PKG_CONFIG --libs exiv2 2> $$NUL)

QMAKE_CFLAGS *= $$EXIV2_CFLAGS
QMAKE_CXXFLAGS *= $$EXIV2_CFLAGS
LIBS *= $$EXIV2_LIBS -lexiv2

# Find other pkg-config-based projects
# We're searching for:
#  libzip
#  sqlite3
link_pkgconfig: PKGCONFIG += libzip sqlite3

# Add libiconv if needed
link_pkgconfig: packagesExist(libiconv): PKGCONFIG += libiconv

#
# Find libmarble
#
# Before Marble 4.9, the GeoDataTreeModel.h header wasn't installed
# Check if it's present by trying to compile
# ### FIXME: implement that
win32: CONFIG(debug, debug|release): LIBS += -lmarblewidgetd
else: LIBS += -lmarblewidget

#
# Platform-specific changes
#
win32 {
	LIBS += -lwsock32
	DEFINES -= UNICODE
}

#
# misc
#
!equals(V, 1): !equals(QMAKE_HOST.os, "Windows"): CONFIG += silent
MOC_DIR = .moc
UI_DIR = .uic
RCC_DIR = .rcc
OBJECTS_DIR = .obj
