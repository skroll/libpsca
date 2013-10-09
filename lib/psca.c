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

#define PSCA_POOL_DEFAULT_BLOCK_SIZE (64 * 1024)
#define PSCA_POOL_DEFAULT_MULTIPLIER (2)

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

struct psca_pool {
	struct psca_frame *frames;

	/** Function to use to allocate memory. */
	psca_alloc_func_t  alloc_func;

	/** Function to use to free memory. */
	psca_free_func_t   free_func;

	/** Block size for allocations. */
	size_t             block_size;

	/** Allocation growth multiplier. */
	int                growth_multiplier;

	void              *context;
};

typedef struct psca_pool psca_pool_t;

/* adds a block to a chain */
static inline psca_block_t *
psca_block_add(psca_pool_t   *pool, /* in: the pool that owns the block */
               psca_block_t  *prev, /* in: previous block in the frame */
               size_t         size, /* in: the requested size */
               void         **data) /* out: pointer after the block header */
{
	/* pad the offset + size with size of the block */
	size_t offset = sizeof(psca_block_t);
	size += sizeof(psca_block_t);

	psca_block_t *block = pool->alloc_func(&size, &offset, pool->context);

	if (block == NULL) {
		return NULL;
	}

	/* we requested more than is actually usable by the user */
	block->size = size - offset;
	block->prev = prev;

	*data = (void *)((uintptr_t)block + offset);

	return block;
}

#define PSCA_POOL_P(_p) ((psca_pool_t *)(_p))
#define PSCA_FRAME_OVERHEAD (sizeof(psca_frame_t))

const void *
psca_push(psca_t p)
{
	struct psca_pool *pool = PSCA_POOL_P(p);
	psca_frame_t *prev = pool->frames;
	psca_frame_t *frame;

	// =================
	if ((prev == NULL) || (prev->free < PSCA_FRAME_OVERHEAD)) {
		/* either this is the first frame in the pool, or there is not enough
		 * room in the previous frame to store the new frame */
		psca_block_t *block = psca_block_add(pool, NULL,
		    pool->block_size, (void **)&frame);

		if (block == NULL) {
			return NULL;
		}

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
	struct psca_pool *pool = PSCA_POOL_P(p);
	psca_frame_t *frame = pool->frames;
	psca_block_t *block;

	pool->frames = frame->prev;

	block = frame->blocks;

	/* destroy all the blocks the frame owns */
	while (block) {
		psca_block_t *prev = block->prev;

		pool->free_func(block, sizeof(psca_block_t), pool->context);

		block = prev;
	}

	return (void *)frame;
}

void *
psca_malloc(const void *p,
            size_t      size)
{
	struct psca_pool *pool = PSCA_POOL_P(p);
	void *ptr;

	psca_frame_t *frame = pool->frames;

	if (frame->free < size) {
		size_t alloc_size = size;
		psca_block_t *blocks_head;

		if (alloc_size < pool->block_size) {
			alloc_size = pool->block_size;
		} else {
			alloc_size *= pool->growth_multiplier;
		}

		blocks_head = psca_block_add(pool, frame->blocks, alloc_size,
		    (void **)&frame->next);

		if (blocks_head == NULL) {
			return NULL;
		}

		frame->blocks = blocks_head;
		frame->free = blocks_head->size;
	}

	ptr = frame->next;

	frame->next += size;
	frame->free -= size;

	return ptr;
}

psca_t
psca_new(void)
{
	psca_pool_t *pool = malloc(sizeof(psca_pool_t));
	memset(pool, 0, sizeof(psca_pool_t));

	pool->alloc_func = psca_alloc_malloc;
	pool->free_func = psca_free_malloc;
	pool->block_size = PSCA_POOL_DEFAULT_BLOCK_SIZE;
	pool->growth_multiplier = PSCA_POOL_DEFAULT_MULTIPLIER;

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
psca_set_growth_multiplier(psca_t p,
                           int    value)
{
	psca_pool_t *pool = PSCA_POOL_P(p);

	pool->growth_multiplier = value;
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

