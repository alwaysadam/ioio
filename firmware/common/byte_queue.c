/*
 * Copyright 2011 Ytai Ben-Tsvi. All rights reserved.
 *
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ARSHAN POURSOHI OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied.
 */
#include "byte_queue.h"

#include <assert.h>
#include <string.h>
#include "atomic.h"
#include "logging.h"

int queue_count = 0;
int queue_cycle = 0;

void ByteQueueOverflow() {
  // TODO: do something
  log_printf("Buffer overflow!");
}

void ByteQueuePushByte(BYTE_QUEUE* q, BYTE b) {
  if (q->size == q->capacity) {
    ByteQueueOverflow();
    return;
  }
  q->buf[q->write_cursor++] = b;
  if (q->write_cursor == q->capacity) {
    q->write_cursor = 0;
    queue_cycle++;
  }
  queue_count++;
  atomic16_add(&q->size, 1);
}

BYTE ByteQueuePullByte(BYTE_QUEUE* q) {
  BYTE ret;
  assert(q->size);
  ret = q->buf[q->read_cursor++];
  if (q->read_cursor == q->capacity) {
    q->read_cursor = 0;
  }
  queue_count--;
  atomic16_add(&q->size, -1);
  return ret;
}

void ByteQueuePullToBuffer(BYTE_QUEUE* q, void* buffer, int size) {
  const BYTE *data1, *data2;
  int size1, size2;
  assert(q->size >= size);
  ByteQueuePeekMax(q, size, &data1, &size1, &data2, &size2);
  if (size1) memcpy(buffer, data1, size1);
  if (size2) memcpy(((BYTE *) buffer) + size1, data2, size2);
  ByteQueuePull(q, size);
}


void ByteQueuePushBuffer(BYTE_QUEUE* q, const void* buf, int len) {
  if (!len) return;
  //log_printf("ByteQueue: %x   length = %d", buf, len);
  if (q->size + len > q->capacity) {
    ByteQueueOverflow();
    log_printf("PushBufferOverflow byte: %x  length = %d", &buf, len);
    log_printf("QUEUE DETAILS");
    log_printf("   read_cursor = %d", q->read_cursor);
    log_printf("   write_cursor = %d", q->write_cursor);
    log_printf("   total capacity = %d", q->capacity);
    log_printf("   current size = %d", q->size);
    log_printf("   indep. size measurement = %d", queue_count);
    log_printf("   number of queue cycles = %d", queue_cycle);
    log_printf("   new queue length = %d", len);
    log_printf("   necessary space = %d", q->size + len);
    while (1) { };
    return;
  }
  if (q->write_cursor + len <= q->capacity) {
    memcpy(q->buf + q->write_cursor, buf, len);
    q->write_cursor += len;
    if (q->write_cursor == q->capacity) {
      q->write_cursor -= q->capacity;
      queue_cycle++;
    }
  } else {
    int size_first = q->capacity - q->write_cursor;
    memcpy(q->buf + q->write_cursor, buf, size_first);
    memcpy(q->buf, ((const BYTE*) buf) + size_first, len - size_first);
    q->write_cursor += (len - q->capacity);
    queue_cycle++;
  }
  queue_count+=len;
  atomic16_add(&q->size, len);
}


/*
void ByteQueuePushBuffer(BYTE_QUEUE* q, const BYTE* buf, int len) {
  if (!len) return;
  if (q->size + len > q->capacity) {
    ByteQueueOverflow();
    return;
  }
  int i=0;
  while (len-- > 0) {
    ByteQueuePushByte(q, buf[i]);
    i++;
  }
}
*/

void ByteQueuePeek(BYTE_QUEUE* q, const BYTE** data, int* size) {
  *data = q->buf + q->read_cursor;
  if (!q->size) {
    *size = 0;
  } else if (q->write_cursor <= q->read_cursor) {
    *size = q->capacity - q->read_cursor;
  } else {
    *size = q->write_cursor - q->read_cursor;
  }
}

/*
void ByteQueuePeekAll(BYTE_QUEUE* q, const BYTE** data1, int* size1,
                      const BYTE** data2, int* size2) {
  *data1 = q->buf + q->read_cursor;
  if (q->write_cursor < q->read_cursor) {
    *size1 = q->capacity - q->read_cursor;
    *data2 = q->buf;
    *size2 = q->write_cursor;
  } else {
    *size1 = q->write_cursor - q->read_cursor;
    *size2 = 0;
  }
}
*/

void ByteQueuePeekMax(BYTE_QUEUE* q, int max_size, const BYTE** data1,
                      int* size1, const BYTE** data2, int* size2) {
  if (max_size > q->size) max_size = q->size;
  *data1 = q->buf + q->read_cursor;
  if (q->read_cursor + max_size > q->capacity) {
    *size1 = q->capacity - q->read_cursor;
    *data2 = q->buf;
    *size2 = max_size - *size1;
  } else {
    *size1 = max_size;
    *size2 = 0;
  }
}

void ByteQueuePull(BYTE_QUEUE* q, int size) {
  assert(size <= q->size);
  q->read_cursor += size;
  queue_count-=size;
  atomic16_add(&q->size, -size);
  if (q->read_cursor >= q->capacity) {
    q->read_cursor -= q->capacity;
  }
}
