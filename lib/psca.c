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

#include "psca.h"

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

	psca_block_t *block = pool->alloc_func(pool, &size, &offset);

	if (block == NULL) {
		return NULL;
	}

	/* we requested more than is actually usable by the user */
	block->size = size - offset;
	block->prev = prev;

	*data = (void *)((uintptr_t)block + offset);

	return block;
}

/* removes a block from a chain */
static inline psca_block_t *
psca_block_remove(psca_pool_t  *pool,  /* in: pool that owns the block */
                  psca_block_t *block) /* in: block to remove */
{
	psca_block_t *prev = block->prev;

	pool->free_func(pool, block, sizeof(psca_block_t));

	return prev;
}

#define PSCA_FRAME_OVERHEAD (sizeof(psca_frame_t))

/* add a frame to a pool */
static inline psca_frame_t *
psca_frame_add(psca_pool_t  *pool, /* in: pool to add the frame to */
               psca_frame_t *prev) /* in: previous frame in stack */
{
	psca_frame_t *frame;

	if ((prev == NULL) || (prev->free < PSCA_FRAME_OVERHEAD)) {
		/* either this is the first frame in the pool, or there is not enough
		 * room in the previous frame to store the new frame */
		psca_block_t *block = psca_block_add(pool, NULL,
		    pool->default_block_size, (void **)&frame);

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

	return frame;
}

/* remove a frame from a pool */
static inline psca_frame_t *
psca_frame_remove(psca_pool_t  *pool,  /* in: pool to remove a frame from */
                  psca_frame_t *frame) /* in: frame to remove */
{
	psca_frame_t *prev = frame->prev;
	psca_block_t *block = frame->blocks;

	/* destroy all the blocks the frame owns */
	while (block) {
		block = psca_block_remove(pool, block);
	}

	return prev;
}

#define PSCA_POOL_P(_p) ((struct psca_pool *)(_p))

const void *
psca_frame_push(const void *p)
{
	struct psca_pool *pool = PSCA_POOL_P(p);

	pool->frames = psca_frame_add(pool, pool->frames);

	return (void *)pool->frames;
}

const void *
psca_frame_pop(const void *p)
{
	struct psca_pool *pool = PSCA_POOL_P(p);

	psca_frame_t *frame = pool->frames;

	pool->frames = psca_frame_remove(pool, frame);

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

		if (alloc_size < pool->default_block_size) {
			alloc_size = pool->default_block_size;
		} else {
			alloc_size *= pool->alloc_multiplier;
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

