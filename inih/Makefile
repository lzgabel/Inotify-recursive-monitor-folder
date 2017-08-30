OUT = libinih.so
OBJ = ini.c ini.h
INCLUDES = -I.
INCLUDE_ARG = ini.h
INCLUDE_DIR = /usr/include/
INSTALL_DIR = /usr/local/lib/
CCFLAGS = -g -O2 -fPIC -shared -Wall 
CC = gcc

default: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(INCLUDES) $(CCFLAGS)  -c $< -o $@

install:
	cp $(INCLUDE_ARG) $(INCLUDE_DIR)
	cp $(OUT) $(INSTALL_DIR)

uninstall:
	-rm -f $(INSTALL_DIR)$(OUT)
	-rm -f $(INCLUDE_DIR)$(INCLUDE_ARG) 
.PHONY : clean
clean:
	-rm -f $(OUT)
