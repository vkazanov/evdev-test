#ifndef IO_EVDEV_H
#define IO_EVDEV_H

#include <stdbool.h>

enum key_evdev_status {
    KEY_EVDEV_SUCCESS,
};

typedef struct key_evdev key_evdev;

int key_evdev_new(const char *path, key_evdev **ke_ptr);

void key_evdev_free(key_evdev *ke);

int key_evdev_wait_for_key(key_evdev *ke, int *key_pressed);

int key_evdev_is_key_pressed(key_evdev *ke, int key_to_check, bool *is_key_pressed);

int key_evdev_flush(key_evdev *ke);

#endif /* IO_EVDEV_H */
