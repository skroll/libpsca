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
 * This prototype describes a callback method used by the pool to allocate
 * memory. It allows for memory to be allocated from any source to be used
 * by the pool. `size` is passed as a pointer since the allocator is allowed
 * to allocate more than the requested size.
 *
 * Here is an example of an allocator that scribbles a value at the beginning
 * of a block:
 * @code
 *     void * my_alloc_func(size_t *size, void *context)
 *     {
 *         size_t sz = *size;
 *         uint32_t magic = 0xDEADBEEF;
 *         void *block = malloc(sz + sizeof(magic));
 *
 *         if (block == NULL) return NULL;
 *
 *         memcpy(block, &magic, sizeof(magic));
 *
 *         *size = sz;
 *
 *         return block + sizeof(magic);
 *     }
 * @endcode
 *
 * @param[in,out]  size     Number of bytes to allocate. On return, the
 *                          function should set the actual number of bytes
 *                          allocated that are usable by the caller.
 *
 * @param[in]      context  User data set by psca_set_funcs().
 *
 * @return                  Pointer to newly allocated block of memory, or
 *                          NULL on error.
 *
 * @see psca_set_funcs()
 * @see psca_free_func_t
 */
typedef void * (* psca_alloc_func_t)(size_t *size, void *context);

/**
 * @brief Memory deallocation function pointer.
 *
 * This prototype describes a callback method used by the pool to deallocate
 * memory that was allocated by the psca_alloc_func_t callback.
 *
 * Here is an example of a deallocator that checks a value prepended to the
 * beginning of a block.
 *
 * @code
 *     void my_free_func(void *block, void *context)
 *     {
 *         void *p = block - sizeof(uint32_t);
 *         uint32_t magic;
 *
 *         memcpy(&magic, p, sizeof(magic));
 *
 *         if (magic != 0xDEADBEEF) {
 *             .... log some error ....
 *         }
 *
 *         free(p);
 *     }
 * @endcode
 *
 * @param[in]  block    Block to free.
 *
 * @param[in]  context  User data set by psca_set_funcs().
 *
 * @see psca_set_funcs()
 * @see psca_alloc_func_t
 */
typedef void (* psca_free_func_t)(void *block, void *context);

/**
 * @brief Handle for a psca pool.
 *
 * This is a simple opaque pointer used for all functions that access
 * or modify the pool.
 */
typedef const void * psca_t;

/**
 * @brief Initialize a new pool.
 *
 * @return              New pool or NULL on error.
 *
 * @see psca_destroy()
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
 *
 * @see psca_new()
 */
int psca_destroy(psca_t pool);

/**
 * @brief Set allocation/deallocation functions for a pool.
 *
 * @param[in]  pool       The pool to set the functions for.
 *
 * @param[in]  alloc_func The allocation function to use.
 *
 * @param[in]  free_func  The deallocation function to use.
 *
 * @param[in]  context    User data supplied in callback.
 *
 * @warning               Do not change allocation functions if anything
 *                        has been allocated from the pool, it will result
 *                        in undefined (and probably unstable) behavior.
 *
 * @see psca_alloc_func_t
 * @see psca_free_func_t
 */
void psca_set_funcs(psca_t pool, psca_alloc_func_t alloc_func,
                    psca_free_func_t free_func, void *context);

/**
 * @brief Set block size for a pool.
 *
 * The block size is the default size for block allocations by the pool. It
 * is ideal to set this to a value large enough to hold memory for
 * several frames.
 *
 * @param[in]  pool     The pool to set the block size for.
 *
 * @param[in]  value    The size (in bytes) to set the block size to.
 */
void psca_set_block_size(psca_t pool, size_t value);

/**
 * @brief Set the growth factor for a pool.
 *
 * The growth factor is used when an allocation greater than the block size
 * for the pool is requested. When such an allocation is requested, the size
 * is multiplied by the growth factor before the block is allocated, so that
 * further allocations will not require a new block.
 *
 * @param[in]  pool     The pool to set the growth factor for.
 *
 * @param[in]  value    The growth factor.
 */
void psca_set_growth_factor(psca_t pool, int value);

/**
 * @brief Push a new frame onto the pool allocation stack.
 *
 * A frame is pushed in the pool when the context of memory allocations
 * should change. All memory allocated from the pool will do so within the
 * context of the top-most frame. When the frame is popped off the stack,
 * it's allocations are released.
 *
 * @param[in]  pool     The pool to push the frame onto.
 *
 * @return              Pointer to newly pushed frame. This can be used to
 *                      verify the return value in psca_pop() to check for
 *                      stack imbalances.
 *
 * @see psca_pop()
 */
const void *psca_push(psca_t pool);

/**
 * @brief Pop a frame from the pool allocation stack.
 *
 * A frame the is popped will have all of it's allocations released. If the
 * frame caused any blocks to be allocated, those will be freed as well.
 *
 * @param[in]  pool     The pool to pop a frame from.
 *
 * @return              Pointer to popped frame. This can be used to verify
 *                      the value returned by psca_push() to check for stack
 *                      imbalances.
 *
 * @see psca_push()
 */
const void *psca_pop(psca_t pool);

/**
 * @brief Allocate memory from the pool allocation stack.
 *
 * @param[in]  pool     The pool to allocate from.
 *
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

