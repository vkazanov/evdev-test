CFLAGS = -g -Wall -Wextra $(shell pkg-config --cflags libevdev)
LDFLAGS =  $(shell pkg-config --libs libevdev)

evdev-test: main.c key-evdev.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
clean:
	rm -rf evdev-test

.PHONY: clean
