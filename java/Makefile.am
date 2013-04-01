#
# Java automake file for frag_find
#

# We don't want to always build this; right now it's just here for demo.


TESTS	= frag_find_test.sh

bin_PROGRAMS = frag_find.jar$(EXEEXT)


SOURCES    = AFTimer.java FragFind.java GetHashes.java ImageFile.java Jdbc12.java MD5.java NSRLBloom.java\
		Sha1.java SpeedHash.java
CLASSES    = AFTimer.class FragFind.class GetHashes.class ImageFile.class Jdbc12.class MD5.class NSRLBloom.class\
		Sha1.class SpeedHash.class

frag_find_jar_SOURCES = $(SOURCES)

CLEANFILES = frag_find.jar 

EXTRA_DIST = mymanifest frag_find_test.sh $(SOURCES) frag_find.jar

# We need to tell autoconf to clean up. Perhaps we wouldn't need to do that if we were using gcj

CONFIG_CLEAN_FILES = *.class $(CLASSES)


# -------------------------------------------------

TEST_FILE1 = /corp/images/nps/nps-2009-casper-rw/ubnist1.casper-rw.gen3.raw

# -------------------------------------------------

#all: frag_find.jar

frag_find.jar$(EXEEXT): $(CLASSES)
	jar cfvm frag_find.jar ${srcdir}/mymanifest $^ 

clean:
	rm -f *~ *.class 

JAVAC = javac
SUFFIXES = .java
JAVAC_ARGS = -d ${builddir} -sourcepath ${srcdir}:${builddir}:. -cp ${srcdir}:${builddir}:.

.java.class:
	$(JAVAC) $(JAVAC_ARGS) $^

