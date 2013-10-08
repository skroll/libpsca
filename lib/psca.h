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

#ifndef _PSCA_H_
#define _PSCA_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup psca The psca API
 *
 * @{
 */

#define PSCA_VERSION_MAJOR 0
#define PSCA_VERSION_MINOR 0
#define PSCA_VERSION_PATCH 1

typedef void * (* psca_alloc_func_t)(void *pool, size_t *size, size_t *offset);
typedef void (* psca_free_func_t)(void *pool, void *block, size_t offset);

/**
 * @brief Allocation pool and configuration.
 */
struct psca_pool {
	struct psca_frame *frames;

	/** Function to use to allocate memory. */
	psca_alloc_func_t  alloc_func;

	/** Function to use to free memory. */
	psca_free_func_t   free_func;

	/** Default block size for allocations. */
	size_t             default_block_size;

	/** Allocation multiplier. */
	size_t             alloc_multiplier;
};

#define PSCA_POOL_DEFAULT_INIT \
	{                          \
		NULL,                  \
		psca_alloc_malloc,     \
		psca_free_malloc,      \
		(64 * 1024),           \
		2,                     \
	}

/**
 * @brief Push a new frame onto the pool allocation stack.
 */
const void *psca_frame_push(const void *pool);

/**
 * @brief Pop a frame from the pool allocation stack.
 */
const void *psca_frame_pop(const void *pool);

/**
 * @brief Allocate memory from the pool allocation stack.
 */
void *psca_malloc(const void *pool, size_t size);

/**
 * @defgroup psca_alloc psca allocation implementations
 *
 * @{
 */

/**
 * @brief Default malloc implementation of allocator.
 */
void * psca_alloc_malloc(void *pool, size_t *size, size_t *offset);

/**
 * @brief Default malloc implementation of free.
 */
void psca_free_malloc(void *pool, void *block, size_t offset);

/** @} **********************************************************************/

/** @} **********************************************************************/

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* _PSCA_H_ */

