APPLICATION = monica-blimp

RIOTBASE ?= ../RIOT

BOARD = samr21-xpro

# mandatory modules
USEMODULE += fmt
USEMODULE += isrpipe
USEMODULE += isrpipe_read_timeout
USEMODULE += xtimer
# networking modules
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_ipv6_default
# optional modules, e.g. for debugging
USEMODULE += gnrc_icmpv6_echo
USEMODULE += gcoap

CFLAGS += -DGCOAP_REQ_WAITING_MAX=8
CFLAGS += -DGCOAP_NON_TIMEOUT=1000000
CFLAGS += -L$(APPDIR)/../strato3/
INCLUDES += -I$(APPDIR)/../strato3/

SRC += $(APPDIR)/../strato3/strato3.c

DEVELHELP=1

include $(RIOTBASE)/Makefile.include
