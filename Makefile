ifneq ("$(wildcard ./debug.mk)","")
include ./debug.mk
else ifneq ("$(wildcard ./libspicboard/debug.mk)","")
include ./libspicboard/debug.mk
else
LIBSPICBOARDDIR ?= /proj/i4spic/pub/libspicboard
include $(LIBSPICBOARDDIR)/debug.mk
endif

default: doom-nano.elf

doom-nano.elf: doom-nano.c entities.c input.c types.c constants.c display_wrap.c
	$(CC) -Os $(CFLAGS) -o $@ $^ $(LDFLAGS)
	$(SIZE) $@

installer.elf: installer.c level.h
	$(CC) -Os $(CFLAGS) -o $@ $^ $(LDFLAGS)
	$(SIZE) $@

