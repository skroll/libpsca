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

int psca_version_major(void);
int psca_version_minor(void);
int psca_version_patch(void);

/**
 * @brief Memory allocation function pointer.
 *
 * @param[in,out]  size
 * @param[in,out]  offset
 * @param[in]      context
 */
typedef void * (* psca_alloc_func_t)(size_t *size, void *context);

/**
 * @brief Memory deallocation function pointer.
 *
 * @param[in]  block
 * @param[in]  offset
 * @param[in]  context
 */
typedef void (* psca_free_func_t)(void *block, void *context);

/**
 * @brief Handle for a psca pool.
 */
typedef const void * psca_t;

/**
 * @brief Initialize a new pool.
 *
 * @return              New pool or NULL on error.
 */
psca_t psca_new(void);

/**
 * @brief Destroy a pool.
 *
 * @param[in]  pool     The pool to be destroyed.
 *
 * @return              Returns 0 on success and -1 on error. A possible
 *                      failure condition is if the pool still had frames
 *                      on the stack when it was being destroyed.
 */
int psca_destroy(psca_t pool);

/**
 * @brief Set allocation/deallocation functions for a pool.
 *
 * @param[in]  pool       The pool to set the functions for.
 * @param[in]  alloc_func The allocation function to use.
 * @param[in]  free_func  The deallocation function to use.
 * @param[in]  context    User data supplied in callback.
 *
 * @warning               Do not change allocation functions if anything
 *                        has been allocated from the pool, it will result
 *                        in undefined (and probably unstable) behavior.
 */
void psca_set_funcs(psca_t pool, psca_alloc_func_t alloc_func,
                    psca_free_func_t free_func, void *context);

/**
 * @brief Set block size for a pool.
 *
 * @param[in]  pool
 * @param[in]  value
 */
void psca_set_block_size(psca_t pool, size_t value);

/**
 * @brief Set the growth factor for a pool.
 *
 * @param[in]  pool
 * @param[in]  value
 */
void psca_set_growth_factor(psca_t pool, int value);

/**
 * @brief Push a new frame onto the pool allocation stack.
 *
 * @param[in]  pool
 *
 * @return              Pointer to newly pushed frame.
 */
const void *psca_push(psca_t pool);

/**
 * @brief Pop a frame from the pool allocation stack.
 *
 * @return              Pointer to popped frame.
 */
const void *psca_pop(psca_t pool);

/**
 * @brief Allocate memory from the pool allocation stack.
 *
 * @param[in]  pool     The pool to allocate from.
 * @param[in]  size     Number of bytes to allocate.
 *
 * @return              Allocated memory, NULL on error.
 */
void *psca_malloc(psca_t pool, size_t size);

/** @} **********************************************************************/

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* _PSCA_H_ */

