# Reset implicit rules as if using -r
.SUFFIXES:
# Reset implicit variables as if using -R
$(foreach var,$(filter-out .% MAKE% SUFFIXES,$(.VARIABLES)),\
  $(if $(findstring $(origin $(var)),default),\
    $(if $(filter undefine,$(.FEATURES)),\
      $(eval undefine $(var)),\
      $(eval $(var)=))))

# Build options
BUILTINS=1
PRAGMA_FP_CONTRACT=0
SIMD=1
OPENMP=1
DEBUG=0
PROFILE=0
SAVE_ASM=0
WINDOWS=0

# VARIABLES
PNAME=jpeg2png
CFLAGS+=-std=c11 -pedantic
CFLAGS+=-msse2 -mfpmath=sse
CFLAGS+=-g
WARN_FLAGS+=-Wall -Wextra -Winline -Wshadow
NO_WARN_FLAGS+=-w
SRCS=src
RM=rm -f
ifeq ($(CC),)
CC=$(HOST)gcc
endif
ifeq ($(WINDRES),)
WINDRES=$(HOST)windres
endif
LIBS+=-ljpeg -lpng -lm -lz
OBJS+=$(SRCS)/jpeg2png.o $(SRCS)/utils.o $(SRCS)/jpeg.o $(SRCS)/png.o $(SRCS)/box.o $(SRCS)/compute.o $(SRCS)/logger.o $(SRCS)/progressbar.o $(SRCS)/fp_exceptions.o $(SRCS)/gopt/gopt.o $(SRCS)/ooura/dct.o
HOST=
EXE=

ifeq ($(BUILTINS),1)
CFLAGS+=-DBUILTIN_UNREACHABLE -DBUILTIN_ASSUME_ALIGNED -DATTRIBUTE_UNUSED
endif

ifeq ($(PRAGMA_FP_CONTRACT),1)
CFLAGS+=-DPRAGMA_FP_CONTRACT
else # not supported by gcc
CFLAGS+=-ffp-contract=off
endif

ifeq ($(SIMD),1)
CFLAGS+=-DUSE_SIMD
endif

ifeq ($(OPENMP),1)
BFLAGS+=-fopenmp
endif

ifeq ($(DEBUG),1)
CFLAGS+=-Og -DDEBUG
else
CFLAGS+=-O3 -DNDEBUG
endif

ifeq ($(PROFILE),1)
BFLAGS+=-pg
endif

ifeq ($(WINDOWS),1)
HOST=i686-w64-mingw32-
EXE=.exe
LDFLAGS+=-static -s
CFLAGS+=-mstackrealign # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=48659
RES+=icon.rc.o
else
CFLAGS+=-D_POSIX_C_SOURCE=200112
endif

ifeq ($(SAVE_ASM),1)
CFLAGS+=-save-temps -masm=intel -fverbose-asm
endif

CFLAGS+=$(BFLAGS)
LDFLAGS+=$(BFLAGS)

# RULES
.PHONY: clean all install uninstall
all: jpeg2png$(EXE)

$(PNAME)$(EXE): $(OBJS) $(RES)
	$(CC) $(OBJS) $(RES) -o $@ $(LDFLAGS) $(LIBS)

-include $(OBJS:.o=.d)

$(SRCS)/gopt/gopt.o: $(SRCS)/gopt/gopt.c $(SRCS)/gopt/gopt.h
	$(CC) $< -c -o $@ $(CFLAGS) $(NO_WARN_FLAGS)

$(SRCS)/%.o: $(SRCS)/%.c
	$(CC) -MP -MMD $< -c -o $@ $(CFLAGS) $(WARN_FLAGS)

%.rc.o: %.rc Makefile
	$(WINDRES) $< $@

clean:
	$(RM) $(OBJS) $(PNAME)$(EXE)
	git clean -Xf

install: all
	install -Dm755 jpeg2png "$(DESTDIR)"/usr/bin/jpeg2png

uninstall:
	rm "$(DESTDIR)"/usr/bin/jpeg2png
