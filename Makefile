# Makefile
#
# Created on: 13.11.2009
#     Author: Andrea Palmatè
#
#

CC = ppc-amigaos-gcc

DEBUG = -O0 -gstabs -DDEBUG
RELEASE = -O3 -ffast-math
CFLAGS = $(RELEASE) $(DEBUG) -I. -Iinclude -Wall -DCATCOMP_NUMBERS -DCATCOMP_STRINGS
LDFLAGS = -use-dynld -Wl,-export-dynamic
LIBS = -L../iODBCGui -liodbc -liodbcinst -ldl -lpthread -lunix -lauto -lraauto 
EXENAME = "SQL Manager"

SRC = SQLManager_glue.c SQLManager_connect.c SQLManager_utils.c SQLManager_cat.c SQLManager.c 
        
OBJ = $(SRC:.c=.o)

all: SQLManager_cat.h SQLManager

SQLManager_cat.h: SQLManager.ct SQLManager.cd
	@echo Making Catalog code...
#	@catcomp DESCRIPTOR SQLManager.cd CFILE SQLManager_cat.h NOBLOCK NOCODE

SQLManager: $(OBJ) SQLManager_cat.h SQLManager.h
	$(CC) $(LDFLAGS) $(DEBUG) $(CFLAGS) -o $(EXENAME) $(OBJ) $(LIBS)
	echo "Done"

clean:
	rm -f $(OBJ) $(EXENAME)
    
