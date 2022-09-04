# fzero
This kernel module is similar to /dev/zero but doesn't solely return zeros.

This module will create a character device at /dev/fzero. Initially, that device will solely read out values of 0xFF.
Additionally, the character device has an internal global buffer of 256 bytes that it will read out as a circular
buffer. That internal data can be set by writing to the character device.

To reset the device to its original state you can write a single byte of 0x00 to the device.
