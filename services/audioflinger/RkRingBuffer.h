/*
 * Copyright (C) 2024 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_RK_RING_BUFFER_H__
#define ANDROID_RK_RING_BUFFER_H__

#include <pthread.h>
#include <utils/Errors.h>

namespace android {

typedef struct ring_buffer {
    pthread_mutex_t lock;
    uint8_t *start_addr; //start address
    uint8_t *r_addr;     //read address
    uint8_t *w_addr;     //write address
    size_t total_size;   //total size of the ringbuffer
    bool isLastWriteOp;  //last operation is write
}ring_buffer_t;

class RkRingBuffer {
public:
    static status_t ring_buffer_init(struct ring_buffer *rbuffer, size_t size);
    static size_t ring_buffer_write(struct ring_buffer *rbuffer, uint8_t* data, size_t size);
    static size_t ring_buffer_read(struct ring_buffer *rbuffer, uint8_t* buffer, size_t size);
    static void ring_buffer_release(struct ring_buffer *rbuffer);
    static void ring_buffer_reset(struct ring_buffer *rbuffer);
    static size_t get_buffer_read_space(struct ring_buffer *rbuffer);
    static size_t get_buffer_write_space(struct ring_buffer *rbuffer);

private:
};

} //ANDROID_RK_RING_BUFFER_H__
#endif