/*
 *  Copyright (c) 2012 Peter Brinkmann (peter.brinkmann@gmail.com)
 *
 *  For information on usage and redistribution, and for a DISCLAIMER OF ALL
 *  WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "ringbuffer.h"

#include <stdlib.h>
#include <string.h>

ring_buffer *rb_create(int size) {
  if (size & 0xff) return NULL;  // size must be a multiple of 256
  ring_buffer *buffer = malloc(sizeof(ring_buffer));
  if (!buffer) return NULL;
  buffer->buf_ptr = calloc(size, sizeof(char));
  if (!buffer->buf_ptr) {
    free(buffer);
    return NULL;
  }
  buffer->size = size;
  buffer->write_idx = 0;
  buffer->read_idx = 0;
  return buffer;
}

void rb_free(ring_buffer *buffer) {
  free(buffer->buf_ptr);
  free(buffer);
}

int rb_available_to_write(ring_buffer *buffer) {
  // Note: The largest possible result is buffer->size - 1 because
  // we adopt the convention that read_idx == write_idx means that the
  // buffer is empty.
  return buffer
    ? (buffer->size + buffer->read_idx - buffer->write_idx - 1) % buffer->size
    : 0;
}

int rb_available_to_read(ring_buffer *buffer) {
  return buffer
    ? (buffer->size + buffer->write_idx - buffer->read_idx) % buffer->size : 0;
}

int rb_write_to_buffer(ring_buffer *buffer, const char *src, int len) {
  if (len > rb_available_to_write(buffer)) return -1;
  int write_idx = buffer->write_idx;
  if (write_idx + len <= buffer->size) {
    memcpy(buffer->buf_ptr + write_idx, src, len);
  } else {
    int d = buffer->size - write_idx;
    memcpy(buffer->buf_ptr + write_idx, src, d);
    memcpy(buffer->buf_ptr, src + d, len - d);
  }
  __sync_synchronize();  // memory barrier
  __sync_val_compare_and_swap(&(buffer->write_idx), buffer->write_idx,
       (write_idx + len) % buffer->size);
  return 0; 
}

int rb_read_from_buffer(ring_buffer *buffer, char *dest, int len) {
  if (len > rb_available_to_read(buffer)) return -1;
  __sync_synchronize();  // memory barrier
  int read_idx = buffer->read_idx;
  if (read_idx + len <= buffer->size) {
    memcpy(dest, buffer->buf_ptr + read_idx, len);
  } else {
    int d = buffer->size - read_idx;
    memcpy(dest, buffer->buf_ptr + read_idx, d);
    memcpy(dest + d, buffer->buf_ptr, len - d);
  }
  __sync_val_compare_and_swap(&(buffer->read_idx), buffer->read_idx,
       (read_idx + len) % buffer->size);
  return 0; 
}
