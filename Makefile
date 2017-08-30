SUB_DIR = inih
OUT = monitor_file_systemd
OBJ = monitor_file_systemd.c
CONF = monitor_file_systemd.conf
SUB_OUT = libinih.so
INCLUDES = -I.
INSTALL_DIR = /usr/sbin/monitor_file_system/
CCFLAGS = -g -O2
CC = gcc

all: $(SUB_OUT) $(OUT)

$(SUB_OUT):
	cd $(SUB_DIR) && $(MAKE) && $(MAKE) install

$(OUT):
	$(CC) $(CCFLAGS) -o $(OUT) $(OBJ) -linih

install:
	mkdir $(INSTALL_DIR) && cp $(OUT) $(CONF) $(INSTALL_DIR)

uninstall:
	cd $(SUB_DIR) && $(MAKE) uninstall
	-rm -rf $(INSTALL_DIR)

.PHONY : clean
clean:
	-rm -f $(OUT)
	cd $(SUB_DIR) && $(MAKE) clean
