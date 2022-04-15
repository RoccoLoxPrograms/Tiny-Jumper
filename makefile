# ----------------------------
# Makefile Options
# ----------------------------

NAME = TNYJUMPR
ICON = icon.png
DESCRIPTION = "A tiny platforming game"
COMPRESSED = YES
ARCHIVED = YES

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
