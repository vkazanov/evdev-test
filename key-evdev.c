#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <linux/input.h>

#include <libevdev/libevdev.h>

#include "key-evdev.h"

struct key_evdev {
    struct libevdev *dev;
};

static const int key_map[KEY_CNT] = {
    [KEY_1] = KEY_1,
    [KEY_2] = KEY_2,
    [KEY_3] = KEY_3,
    [KEY_4] = KEY_C,

    [KEY_Q] = KEY_4,
    [KEY_W] = KEY_5,
    [KEY_E] = KEY_6,
    [KEY_R] = KEY_D,

    [KEY_A] = KEY_7,
    [KEY_S] = KEY_8,
    [KEY_D] = KEY_9,
    [KEY_F] = KEY_E,

    [KEY_Z] = KEY_A,
    [KEY_X] = KEY_0,
    [KEY_C] = KEY_B,
    [KEY_V] = KEY_F,
};

static const uint8_t key_value_map[] = {
    [KEY_0] = 0x0,
    [KEY_1] = 0x1,
    [KEY_2] = 0x2,
    [KEY_3] = 0x3,
    [KEY_4] = 0x4,
    [KEY_5] = 0x5,
    [KEY_6] = 0x6,
    [KEY_7] = 0x7,
    [KEY_8] = 0x8,
    [KEY_9] = 0x9,
    [KEY_A] = 0xA,
    [KEY_B] = 0xB,
    [KEY_C] = 0xC,
    [KEY_D] = 0xD,
    [KEY_E] = 0xE,
    [KEY_F] = 0xF,
};


static const uint8_t value_key_map[] = {
    [0x0] = KEY_0,
    [0x1] = KEY_1,
    [0x2] = KEY_2,
    [0x3] = KEY_3,
    [0x4] = KEY_4,
    [0x5] = KEY_5,
    [0x6] = KEY_6,
    [0x7] = KEY_7,
    [0x8] = KEY_8,
    [0x9] = KEY_9,
    [0xA] = KEY_A,
    [0xB] = KEY_B,
    [0xC] = KEY_C,
    [0xD] = KEY_D,
    [0xE] = KEY_E,
    [0xF] = KEY_F,
};

static bool is_suitable_device(struct libevdev *dev);
static void evdev_resync(key_evdev *ke);

int key_evdev_new(const char *path, key_evdev **ke_ptr)
{
    int rc = 1;
    int fd = 0;
    struct libevdev *dev = NULL;
    key_evdev *ke = NULL;

    fd = open(path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("Failed to open device");
        return rc;
    }

    rc = libevdev_new_from_fd(fd, &dev);
    if (rc < 0) {
        fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
        return rc;
    }

    if (!is_suitable_device(dev)) {
        fprintf(stderr, "This device does is not a suitable keyboard\n");
        goto err;
    }

    ke = calloc(1, sizeof(key_evdev));
    if (ke == NULL) {
        fprintf(stderr, "Calloc failure\n");
        goto err;
    }
    ke->dev = dev;

    *ke_ptr = ke;

    rc = 0;

    return rc;
err:
    libevdev_free(dev);
    return rc;
}

void key_evdev_free(key_evdev *ke)
{
    if (!ke)
        return;
    /* TODO: fre free the descriptor also as _free doesn't do that */
    libevdev_free(ke->dev);
    free(ke);
}

int key_evdev_wait_for_key(key_evdev *ke, int *key_pressed)
{
    int rc = -1;

    for (;;) {
        struct input_event ev;
        rc = libevdev_next_event(ke->dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

        if (rc == LIBEVDEV_READ_STATUS_SYNC) {
            evdev_resync(ke);
        } else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            /* Done waiting? */
            if (ev.type == EV_KEY && key_map[ev.code] != KEY_RESERVED) {
                *key_pressed = key_value_map[ev.code];
                return KEY_EVDEV_SUCCESS;
            }
            /* Just ignore non-interesting ones */
        } else if (rc == -EAGAIN) {
            /* No more events, let's keep waiting */
        } else {
            /* Error? propagate the error */
            return rc;
        }

    };
}

int key_evdev_is_key_pressed(key_evdev *ke, int key_to_check, bool *is_key_pressed)
{
    int rc = -1;

    for (;;) {
        struct input_event ev;
        rc = libevdev_next_event(ke->dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

        if (rc == LIBEVDEV_READ_STATUS_SYNC) {
            evdev_resync(ke);
        } else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            /* Just flush all the events and update the internal libevdev
             * state */
        } else if (rc == -EAGAIN) {
            /* No more events, so let's check keyboard state  */
            int value = libevdev_get_event_value(ke->dev, EV_KEY, value_key_map[key_to_check]);
            *is_key_pressed = (value != 0);
            return KEY_EVDEV_SUCCESS;
        } else {
            /* Error? propagate the error */
            return rc;
        }
    }

}

int key_evdev_flush(key_evdev *ke)
{
    int rc = -1;

    for (;;) {
        struct input_event ev;
        rc = libevdev_next_event(ke->dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

        if (rc == LIBEVDEV_READ_STATUS_SYNC) {
            evdev_resync(ke);
        } else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            /* Just flush events */
        } else if (rc == -EAGAIN) {
            /* No more events so we're done  */
            return KEY_EVDEV_SUCCESS;
        } else {
            /* Error? propagate the error */
            return rc;
        }
    }
}

static void evdev_resync(key_evdev *ke)
{
    int rc = -1;
    do {
        struct input_event ev;
        rc = libevdev_next_event(ke->dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
    } while (rc == LIBEVDEV_READ_STATUS_SYNC);
}

static bool is_suitable_device(struct libevdev *dev)
{
    if (!libevdev_has_event_type(dev, EV_KEY))
        return false;

    for (size_t i = 0; i < sizeof(key_map) / sizeof(key_map[0]); i++) {
        if (key_map[i] == KEY_RESERVED)
            continue;

        if (!libevdev_has_event_code(dev, EV_KEY, key_map[i]))
            return false;
    }

    return true;
}

static int print_event(struct input_event *ev)
{
    if (ev->type != EV_SYN)
        printf("Event: type %d (%s), code %d (%s), value %d\n",
               ev->type,
               libevdev_event_type_get_name(ev->type),
               ev->code,
               libevdev_event_code_get_name(ev->type, ev->code),
               ev->value);
    return 0;

}
