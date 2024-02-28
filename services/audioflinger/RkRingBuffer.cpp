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

#define LOG_TAG "RkRingBuffer"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <cutils/log.h>
#include "RkRingBuffer.h"

namespace android {

static size_t get_write_space(uint8_t *w_ptr, uint8_t *r_ptr,
        size_t total_size, bool isLastWriteOp)
{
    size_t size = 0;

    if (w_ptr > r_ptr) {
        size = total_size + r_ptr - w_ptr;
    } else if (w_ptr < r_ptr) {
        size = r_ptr - w_ptr;
    } else if (!isLastWriteOp) {
        size = total_size;
    }

    return size;
}

static size_t get_read_space(uint8_t *w_ptr, uint8_t *r_ptr,
        size_t total_size, bool isLastWriteOp) {
    size_t size = 0;

    if (w_ptr > r_ptr) {
        size = w_ptr - r_ptr;
    } else if (w_ptr < r_ptr) {
        size = total_size + w_ptr - r_ptr;
    } else if (isLastWriteOp) {
        size = total_size;
    }

    return size;
}

static void write_to_buffer(uint8_t *w_ptr, uint8_t *src, size_t size,
    uint8_t *start_addr, size_t total_size)
{
    size_t left = start_addr + total_size - w_ptr;

    if (left >= size) {
        memcpy(w_ptr, src, size);
    } else {
        memcpy(w_ptr, src, left);
        memcpy(start_addr, src + left, size - left);
    }
}

static void read_from_buffer(uint8_t *r_ptr, uint8_t *dst, size_t size,
    uint8_t *start_addr, size_t total_size)
{
    size_t left = start_addr + total_size - r_ptr;

    if (left >= size) {
        memcpy(dst, r_ptr, size);
    } else {
        memcpy(dst, r_ptr, left);
        memcpy(dst + left, start_addr, size - left);
    }
}

static inline void* update_ptr(uint8_t *current_ptr, size_t size,
        uint8_t *start_addr, size_t total_size)
{
    current_ptr += size;

    if (current_ptr >= start_addr + total_size) {
        current_ptr -= total_size;
    }

    return current_ptr;
}

status_t RkRingBuffer::ring_buffer_init(struct ring_buffer *rbuffer, size_t size)
{
    struct ring_buffer *buf = rbuffer;

    pthread_mutex_lock(&buf->lock);

    buf->total_size = size;
    buf->start_addr = (uint8_t *)malloc(size);
    if (buf->start_addr == nullptr) {
        ALOGE("ring_buffer_init malloc error!");
        pthread_mutex_unlock(&buf->lock);
        return NO_MEMORY;
    }

    memset(buf->start_addr, 0, size);
    buf->r_addr = buf->w_addr = buf->start_addr;
    pthread_mutex_unlock(&buf->lock);

    return OK;
}

size_t RkRingBuffer::ring_buffer_write(struct ring_buffer *rbuffer, uint8_t* data, size_t size)
{
    struct ring_buffer *buf = rbuffer;
    size_t left_space, w_size;

    pthread_mutex_lock(&buf->lock);

    if (buf->start_addr == nullptr || buf->r_addr == nullptr || buf->w_addr == nullptr
            || buf->total_size == 0) {
        ALOGE("%s bad input para!", __func__);
        pthread_mutex_unlock(&buf->lock);
        return 0;
    }

    left_space = get_write_space(buf->w_addr, buf->r_addr, buf->total_size, buf->isLastWriteOp);
    if (left_space < size) {
        w_size = left_space;
    } else {
        w_size = size;
    }

    write_to_buffer(buf->w_addr, data, w_size, buf->start_addr, buf->total_size);
    buf->w_addr = (uint8_t *)update_ptr(buf->w_addr, w_size, buf->start_addr, buf->total_size);
    if (w_size > 0)
        buf->isLastWriteOp = true;

    pthread_mutex_unlock(&buf->lock);

    return w_size;
}

size_t RkRingBuffer::ring_buffer_read(struct ring_buffer *rbuffer, uint8_t *buffer, size_t size)
{
    struct ring_buffer *buf = rbuffer;
    size_t left_space, r_size;

    pthread_mutex_lock(&buf->lock);

    if (buf->start_addr == nullptr || buf->r_addr == nullptr || buf->w_addr == nullptr
            || buf->total_size == 0) {
        ALOGE("%s bad input para!", __func__);
        pthread_mutex_unlock(&buf->lock);
        return 0;
    }

    left_space = get_read_space(buf->w_addr, buf->r_addr, buf->total_size, buf->isLastWriteOp);
    if (left_space < size) {
        r_size = left_space;
    } else {
        r_size = size;
    }

    read_from_buffer(buf->r_addr, buffer, r_size, buf->start_addr, buf->total_size);
    buf->r_addr = (uint8_t *)update_ptr(buf->r_addr, r_size, buf->start_addr, buf->total_size);
    if (r_size > 0)
        buf->isLastWriteOp = false;
    pthread_mutex_unlock(&buf->lock);

    return r_size;
}

void RkRingBuffer::ring_buffer_release(struct ring_buffer *rbuffer)
{
    struct ring_buffer *buf = rbuffer;

    pthread_mutex_lock(&buf->lock);

    if (buf->start_addr != nullptr) {
        free(buf->start_addr);
        buf->start_addr = nullptr;
    }

    buf->r_addr = nullptr;
    buf->w_addr = nullptr;
    buf->total_size = 0;
    buf->isLastWriteOp = false;

    pthread_mutex_unlock(&buf->lock);
}

void RkRingBuffer::ring_buffer_reset(struct ring_buffer *rbuffer)
{
    struct ring_buffer *buf = rbuffer;

    pthread_mutex_lock(&buf->lock);
    memset(buf->start_addr, 0, buf->total_size);
    buf->r_addr = buf->w_addr = buf->start_addr;
    buf->isLastWriteOp = false;
    pthread_mutex_unlock(&buf->lock);
}

size_t RkRingBuffer::get_buffer_read_space(struct ring_buffer *rbuffer)
{
    struct ring_buffer *buf = rbuffer;
    size_t left_space = 0;

    pthread_mutex_lock(&buf->lock);

    if (buf->start_addr == nullptr || buf->r_addr == nullptr || buf->w_addr == nullptr
            || buf->total_size == 0) {
        ALOGE("%s bad input para!", __func__);
        pthread_mutex_unlock(&buf->lock);
        return 0;
    }

    left_space = get_read_space(buf->w_addr, buf->r_addr, buf->total_size, buf->isLastWriteOp);
    pthread_mutex_unlock(&buf->lock);

    return left_space;
}

size_t RkRingBuffer::get_buffer_write_space(struct ring_buffer *rbuffer)
{
    struct ring_buffer *buf = rbuffer;
    size_t left_space = 0;

    pthread_mutex_lock(&buf->lock);

    if (buf->start_addr == nullptr || buf->r_addr == nullptr || buf->w_addr == nullptr
            || buf->total_size == 0) {
        ALOGE("%s bad input para!", __func__);
        pthread_mutex_unlock(&buf->lock);
        return 0;
    }

    left_space = get_write_space(buf->w_addr, buf->r_addr, buf->total_size, buf->isLastWriteOp);
    pthread_mutex_unlock(&buf->lock);

    return left_space;
}

}