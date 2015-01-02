#
#  Makefile for tal - trailer aligner
#  Thomas Jensen <tsjensen@stud.informatik.uni-erlangen.de>
#  Created May 03, 1997 (Saturday, 18:24h)
#

BIN	= .
SRC	= .

# cc is assumed to be the standard compiler. If it isn't for you,
# uncomment. On some platforms, we compile with gcc by default.
##CC	= gcc

all:
	$(MAKE) $(OS)_tal
aix41:
	$(MAKE) aix41_tal
osf1:
	$(MAKE) osf1_tal
hpux9:
	$(MAKE) hpux9_tal
hpux10:
	$(MAKE) hpux10_tal
irix5:
	$(MAKE) irix5_tal
irix6:
	$(MAKE) irix6_tal
linux:
	$(MAKE) linux_tal
sunos4:
	$(MAKE) sunos4_tal
sunos5:
	$(MAKE) sunos5_tal

aix41_tal:
	$(MAKE) $(BIN)/tal CFLAGS="-qlanglvl=ansi -O"
osf1_tal:
	$(MAKE) $(BIN)/tal CC=gcc CFLAGS="-ansi -O -Wall"
hpux9_tal:
	$(MAKE) $(BIN)/tal CFLAGS="-Aa -O -Wall -D_INCLUDE_XOPEN_SOURCE -D_INCLUDE_POSIX_SOURCE"
hpux10_tal:
	$(MAKE) $(BIN)/tal CFLAGS="-Ae -O -Wall"
irix5_tal:
	$(MAKE) $(BIN)/tal CFLAGS="-Aa -O"
irix6_tal:
	$(MAKE) $(BIN)/tal CFLAGS="-Aa -O"
linux_tal:
	$(MAKE) $(BIN)/tal CC=gcc CFLAGS="-ansi -O"
sunos5_tal:
	$(MAKE) $(BIN)/tal CC=gcc CFLAGS="-ansi -O"
sunos4_tal:
	$(MAKE) $(BIN)/tal CC=gcc CFLAGS="-ansi -O"


$(BIN)/tal: $(SRC)/tal.c
	$(CC) $(CFLAGS) -o $(BIN)/tal $(SRC)/tal.c
$(SRC)/tal.c:


_tal:
	@echo "Environment variable OS not set."
	@echo "Please specify one of the following platforms:"
	@echo "aix41, hpux9, hpux10, irix5, linux, osf1, sunos4, or sunos5."
	@echo "Example: make linux"

clean:
	-rm -f $(BIN)/tal
love:
	@echo Not in front of the kids, honey!


# EOF after next line
# vim:noet:nosta:ts=8
