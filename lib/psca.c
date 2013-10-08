/*
 * Pool Stack C Allocator
 *
 * Copyright (C) Scott Kroll 2013
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#include "psca.h"

#ifdef PSCA_USE_CONFIG_H
#include "psca_config.h"
#endif /* PSCA_USE_CONFIG_H */

#define PSCA_ALLOC_METHOD_MALLOC

#define PSCA_POOL_P(_p) ((struct psca_pool *)(_p))

struct psca_block {
	struct psca_block *prev;
	size_t             size;
};

typedef struct psca_block psca_block_t;

struct psca_frame {
	struct psca_block *blocks;
	struct psca_frame *prev;
	uint8_t           *next;
	size_t             free;
};

typedef struct psca_frame psca_frame_t;

#define PSCA_FRAME_OVERHEAD (sizeof(psca_frame_t))
#define PSCA_BLOCK_OVERHEAD (sizeof(psca_block_t))

#define PSCA_BLOCK_START(_block) ((void *)((uintptr_t)(_block) + PSCA_BLOCK_OVERHEAD))

/* Block allocation and deallocation functions */

/* psca_block_t * psca_block_alloc(void *pool, size_t size)
 *
 * This function should allocate memory for a block. It is allowed to
 * allocate more than the requested size (in the case of alignment, page
 * boundary requirements, etc), but NEVER less. The caller of this function
 * should depend on the size member of the psca_block_t structure to
 * determine the size of the block. If the memory cannot be allocated,
 * then this function should return NULL.
 */

/* void psca_block_free(void *pool, psca_block_t *block)
 *
 * This function should deallocate the specified block. There is no
 * return value.
 */
#if defined(PSCA_ALLOC_METHOD_MALLOC)

/* malloc implementation */

static inline psca_block_t *
psca_block_alloc(void *pool, size_t size)
{
	size_t internal_size = size + PSCA_BLOCK_OVERHEAD;

	psca_block_t *block = malloc(internal_size);

	if (block == NULL) {
		return NULL;
	}

	block->size = size;

	return block;
}

static inline void
psca_block_free(void *pool, psca_block_t *block)
{
	free((void *)block);
}

#elif defined(PSCA_ALLOC_METHOD_MMAP)

/* mmap implementation */

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif /* MAP_ANONYMOUS */

static inline psca_block_t *
psca_block_alloc(void *pool, size_t size)
{
	size_t page_size = (size_t)getpagesize();
	size_t internal_size = size + PSCA_BLOCK_OVERHEAD;
	size_t rounded_size = ((internal_size + page_size) / page_size) * page_size;
	psca_block_t *block = mmap(NULL, rounded_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);

	if (block == MAP_FAILED) {
		return NULL;
	}

	block->size = rounded_size - PSCA_BLOCK_OVERHEAD;

	return block;
}

static inline void
psca_block_free(void *pool, psca_block_t *block)
{
	size_t map_size = block->size + PSCA_BLOCK_OVERHEAD;
	munmap((void *)block, map_size);
}

#else

/* no implementation defined */

#endif

/* adds a to a chain of blocks */
static inline psca_block_t *
psca_block_add(void *pool,
               psca_block_t *prev,
               size_t size)
{
	psca_block_t *block = psca_block_alloc(pool, size);

	if (block == NULL) {
		return NULL;
	}

	block->prev = prev;

	return block;
}

static inline psca_block_t *
psca_block_remove(void *pool,
                  psca_block_t *block)
{
	psca_block_t *prev = block->prev;

	psca_block_free(pool, block);

	return prev;
}

static inline psca_frame_t *
psca_frame_add(void *pool,
               psca_frame_t *prev)
{
	psca_frame_t *frame;

	if ((prev == NULL) || (prev->free < PSCA_FRAME_OVERHEAD)) {
		/* either this is the first frame in the pool, or there is not enough
		 * room in the previous frame to store the new frame */
		psca_block_t *block = psca_block_add(pool, NULL, PSCA_DEFAULT_BLOCK_SIZE);

		if (block == NULL) {
			return NULL;
		}

		frame = PSCA_BLOCK_START(block);

		frame->next = (uint8_t *)((uintptr_t)frame + PSCA_FRAME_OVERHEAD);
		frame->free = block->size - PSCA_FRAME_OVERHEAD;
		frame->blocks = block;
	} else {
		frame = (psca_frame_t *)prev->next;

		frame->next = prev->next + PSCA_FRAME_OVERHEAD;
		frame->free = prev->free - PSCA_FRAME_OVERHEAD;
		frame->blocks = NULL;
	}

	frame->prev = prev;

	return frame;
}

static inline psca_frame_t *
psca_frame_remove(void *pool,
                  psca_frame_t *frame)
{
	psca_frame_t *prev = frame->prev;
	psca_block_t *block = frame->blocks;

	while (block) {
		block = psca_block_remove(pool, block);
	}

	return prev;
}

const void *
psca_frame_push(const void *_pool)
{
	struct psca_pool *pool = PSCA_POOL_P(_pool);

	pool->frames = psca_frame_add(pool, pool->frames);

	return (void *)pool->frames;
}

const void *
psca_frame_pop(const void *_pool)
{
	struct psca_pool *pool = PSCA_POOL_P(_pool);

	psca_frame_t *frame = pool->frames;

	pool->frames = psca_frame_remove(pool, frame);

	return (void *)frame;
}

void *
psca_malloc(const void *_pool, size_t size)
{
	struct psca_pool *pool = PSCA_POOL_P(_pool);
	void *ptr;

	psca_frame_t *frame = pool->frames;

	if (frame->free < size) {
		size_t alloc_size = size;
		psca_block_t *blocks_head;

		if (alloc_size < PSCA_DEFAULT_BLOCK_SIZE) {
			alloc_size = PSCA_DEFAULT_BLOCK_SIZE;
		} else {
			alloc_size *= PSCA_BIG_ALLOC_MULTIPLIER;
		}

		blocks_head = psca_block_add(pool, frame->blocks, alloc_size);

		if (blocks_head == NULL) {
			return NULL;
		}

		frame->blocks = blocks_head;
		frame->next = PSCA_BLOCK_START(blocks_head);
		frame->free = blocks_head->size;
	}

	ptr = frame->next;

	frame->next += size;
	frame->free -= size;

	return ptr;
}

