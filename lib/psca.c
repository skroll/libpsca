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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "psca.h"

#define PSCA_POOL_DEFAULT_BLOCK_SIZE    (64 * 1024)
#define PSCA_POOL_DEFAULT_GROWTH_FACTOR (2)

/*
 * A block in the system is an allocated chunk of memory. It can be used
 * by many frames, but will only have one owning frame. Once the owning
 * frame is removed from the stack, the block will be deallocated. Ideally,
 * a block is allocated such that multiple frames may exist in a block,
 * so that deallocations are nothing more than moving a pointer to a
 * previous spot in a block.
 */
struct psca_block {
	struct psca_block *prev;
	size_t             size;
};

typedef struct psca_block psca_block_t;

/*
 * A frame is a state in the allocation stack that points to a location
 * in a block. A frame can own blocks, and when the frame is removed from
 * the stack, all the blocks it owns are also deallocated. A frame can also
 * point to a piece of memory that is in a block of a "parent" frame.
 * All psca_malloc calls are actually just moving a pointer in the top-most
 * frame.
 *
 * Instead of allocating a frame using malloc, the frame structure is stored
 * inside of a block. This is to prevent any non-managed memory allocations
 * from occurring. This does mean that there is some overhead due to
 * allocations, but the positives outweigh the negatives.
 */
struct psca_frame {
	struct psca_block *blocks;
	struct psca_frame *prev;
	uint8_t           *next;
	size_t             free;
};

typedef struct psca_frame psca_frame_t;

/*
 * A pool is nothing more than a stack of frames (implemented as a linked
 * list) that stores some information about how memory should be allocated.
 * It is exposed to the user as an opaque pointer.
 */
struct psca_pool {
	struct psca_frame *frames;
	psca_alloc_func_t  alloc_func;
	psca_free_func_t   free_func;
	size_t             block_size;
	int                growth_factor;
	void              *context;
};

typedef struct psca_pool psca_pool_t;

/* adds a block to a chain */
static inline psca_block_t *
psca_block_add(psca_pool_t   *pool, /* in: the pool that owns the block */
               psca_block_t  *prev, /* in: previous block in the frame */
               size_t         size)
{
	psca_block_t *block;
	size += sizeof(psca_block_t);

	block = pool->alloc_func(&size, pool->context);

	if (block == NULL) {
		return NULL;
	}

	/* we requested more than is actually usable by the user */
	block->size = size - sizeof(psca_block_t);
	block->prev = prev;

	return block;
}

#define PSCA_BLOCK_START(_p) (void *)((uintptr_t)(_p) + sizeof(psca_block_t))
#define PSCA_POOL_P(_p) ((psca_pool_t *)(_p))
#define PSCA_FRAME_OVERHEAD (sizeof(psca_frame_t))

const void *
psca_push(psca_t p)
{
	psca_pool_t *pool = PSCA_POOL_P(p);
	psca_frame_t *prev = pool->frames;
	psca_frame_t *frame;

	if ((prev == NULL) || (prev->free < PSCA_FRAME_OVERHEAD)) {
		/* either this is the first frame in the pool, or there is not enough
		 * room in the previous frame to store the new frame */
		psca_block_t *block = psca_block_add(pool, NULL, pool->block_size);

		if (block == NULL) {
			return NULL;
		}

		frame = PSCA_BLOCK_START(block);
		frame->next = (uint8_t *)((uintptr_t)frame + PSCA_FRAME_OVERHEAD);
		frame->free = block->size - PSCA_FRAME_OVERHEAD;
		frame->blocks = block;
	} else {
		/* there was enough room in the previous frame's block, so create
		 * the frame there. */
		frame = (psca_frame_t *)prev->next;

		frame->next = prev->next + PSCA_FRAME_OVERHEAD;
		frame->free = prev->free - PSCA_FRAME_OVERHEAD;
		frame->blocks = NULL;
	}

	frame->prev = prev;
	pool->frames = frame;

	return (void *)frame;
}

const void *
psca_pop(psca_t p)
{
	psca_pool_t *pool = PSCA_POOL_P(p);
	psca_frame_t *frame = pool->frames;
	psca_block_t *block;

	pool->frames = frame->prev;

	block = frame->blocks;

	/* destroy all the blocks the frame owns */
	while (block) {
		psca_block_t *prev = block->prev;

		pool->free_func(block, pool->context);

		block = prev;
	}

	return (void *)frame;
}

void *
psca_malloc(psca_t  p,
            size_t  size)
{
	psca_pool_t *pool = PSCA_POOL_P(p);
	void *ptr;

	psca_frame_t *frame = pool->frames;

	if (frame->free < size) {
		size_t alloc_size = size;
		psca_block_t *blocks_head;

		if (alloc_size < pool->block_size) {
			alloc_size = pool->block_size;
		} else {
			alloc_size *= pool->growth_factor;
		}

		blocks_head = psca_block_add(pool, frame->blocks, alloc_size);

		if (blocks_head == NULL) {
			return NULL;
		}

		frame->next = PSCA_BLOCK_START(blocks_head);
		frame->blocks = blocks_head;
		frame->free = blocks_head->size;
	}

	ptr = frame->next;

	frame->next += size;
	frame->free -= size;

	return ptr;
}

/* default implementation of memory allocation */
static void *
psca_default_alloc(size_t *size,
                   void   *context)
{
	size_t sz = *size;

	void *block = malloc(sz);

	if (block == NULL) {
		return NULL;
	}

	*size = sz;

	return block;
}

/* default implementation of memory deallocation */
static void
psca_default_free(void   *block,
                  void   *context)
{
	free((void *)block);
}

psca_t
psca_new(void)
{
	psca_pool_t *pool = malloc(sizeof(psca_pool_t));
	memset(pool, 0, sizeof(psca_pool_t));

	pool->alloc_func = psca_default_alloc;
	pool->free_func = psca_default_free;
	pool->block_size = PSCA_POOL_DEFAULT_BLOCK_SIZE;
	pool->growth_factor = PSCA_POOL_DEFAULT_GROWTH_FACTOR;

	return pool;
}

int
psca_destroy(psca_t p)
{
	free((void *)p);

	return 0;
}

void
psca_set_funcs(psca_t             p,
               psca_alloc_func_t  alloc_func,
               psca_free_func_t   free_func,
               void              *context)
{
	psca_pool_t *pool = PSCA_POOL_P(p);

	pool->alloc_func = alloc_func;
	pool->free_func = free_func;
	pool->context = context;
}

void
psca_set_block_size(psca_t p,
                    size_t value)
{
	psca_pool_t *pool = PSCA_POOL_P(p);

	pool->block_size = value;
}

void
psca_set_growth_factor(psca_t p,
                       int    value)
{
	psca_pool_t *pool = PSCA_POOL_P(p);

	pool->growth_factor = value;
}

int
psca_version_major(void)
{
	return PSCA_VERSION_MAJOR;
}

int
psca_version_minor(void)
{
	return PSCA_VERSION_MINOR;
}

int
psca_version_patch(void)
{
	return PSCA_VERSION_PATCH;
}

