#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdlib.h>

#include "key-evdev.h"

int main(int argc, char *argv[])
{
    (void) argc;

    int rc = 1;
    key_evdev * ke = NULL;

    if (argc < 2)
        goto out;

    const char *path = argv[1];

    rc = key_evdev_new(path, &ke);
    if (rc != KEY_EVDEV_SUCCESS)
        goto out;

    {
        int key_pressed = -1;
        rc = key_evdev_wait_for_key(ke, &key_pressed);
        if (rc != KEY_EVDEV_SUCCESS){
            fprintf(stderr, "Failed to handle events\n");
            exit(EXIT_FAILURE);
        }
        printf("%d finally pressed\n", key_pressed);
    }

    printf("sleep...");
    sleep(1);

    {
        uint8_t key_to_check = 0x1;
        bool is_key_pressed = false;

        rc = key_evdev_is_key_pressed(ke, key_to_check, &is_key_pressed);
        if (rc != KEY_EVDEV_SUCCESS){
            fprintf(stderr, "Failed to check for a key press\n");
            exit(EXIT_FAILURE);
        }
        printf("%d pressed? %s\n", key_to_check, is_key_pressed ? "yes" : "no");
    }

    printf("sleep...");
    sleep(1);

    {
        rc = key_evdev_flush(ke);
        if (rc != KEY_EVDEV_SUCCESS){
            fprintf(stderr, "Failed to flush\n");
            exit(EXIT_FAILURE);
        }
    }

    rc = 0;
out:
    key_evdev_free(ke);

    return rc;
}
