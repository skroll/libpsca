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
#include <stdio.h>
#include <unistd.h>

#include <psca.h>

#define likely(x) __builtin_expect((x),1)
#define unlikely(x) __builtin_expect((x),0)

typedef struct {
	const psca_t *psca_pool;

	size_t alloc_size;
	size_t num_allocations;
	size_t num_deallocations;
} t_pool_t;

static void *
t_alloc(size_t *s, void *context)
{
	size_t size = *s;
	size_t pagelessone = getpagesize() - 1;

	t_pool_t *pool = (t_pool_t *)context;

	size = (size + pagelessone) & ~pagelessone;
	*s = size;

	pool->num_allocations++;
	pool->alloc_size += size;

	return malloc(size);
}

static void
t_free(void *block, void *context)
{
	t_pool_t *pool = (t_pool_t *)context;

	pool->num_deallocations++;

	free(block);
}

static t_pool_t g_pool_s;

static inline void
t_scope_cleanup(const void **frame)
{
	if (unlikely(*frame != psca_pop(g_pool_s.psca_pool))) {
		fprintf(stderr, "Unbalanced psca stack!\n");
		abort();
	}
}

#define t_new(_type) (_type *)psca_malloc(g_pool_s.psca_pool, sizeof(_type))
#define t_scope__(n) const void *psca_scope_##n __attribute__((unused,cleanup(t_scope_cleanup))) = psca_push(g_pool_s.psca_pool);
#define t_scope_(n) t_scope__(n)
#define t_scope t_scope_(__LINE__)

struct list {
	struct list *next;
};

#define NUM_LOOPS 3
#define LIST_SIZE 10000000UL

static const size_t obj_size = LIST_SIZE * sizeof(struct list);

void
init_pool(void)
{
	fprintf(stderr, "psca version: %d.%d.%d\n\n", psca_version_major(),
	    psca_version_minor(), psca_version_patch());
	g_pool_s.psca_pool = psca_new();
	g_pool_s.alloc_size = 0;
	g_pool_s.num_allocations = 0;
	g_pool_s.num_deallocations = 0;
	psca_set_funcs(g_pool_s.psca_pool, t_alloc, t_free, &g_pool_s);
}

int
main(int          argc,
     const char **argv)
{
	int i;

	init_pool();

	for (i = 0; i < NUM_LOOPS; i++) {
		t_scope;

		struct list *head = NULL;
		struct list *tail = NULL;
		int j;
		
		head = tail = t_new(struct list);

		for (j = 0; j < (LIST_SIZE - 1); j++) {
			tail->next = t_new(struct list);
			tail = tail->next;
		}

		tail->next = NULL;
	}

	fprintf(stdout, "statistics:\n");
	fprintf(stdout, "===========\n");
	fprintf(stdout, "number of loops: %d\n", NUM_LOOPS);
	fprintf(stdout, "object size: %lu bytes\n", sizeof(struct list));
	fprintf(stdout, "number of objects (per loop): %lu\n", LIST_SIZE);
	fprintf(stdout, "total object size (per loop): %lu bytes\n", LIST_SIZE * sizeof(struct list));
	fprintf(stdout, "total object size (all loops): %lu bytes\n", LIST_SIZE * sizeof(struct list) * NUM_LOOPS);
	fprintf(stdout, "allocated %lu bytes\n", g_pool_s.alloc_size);
	fprintf(stdout, "# of allocations: %lu\n", g_pool_s.num_allocations);
	fprintf(stdout, "# of deallocations: %lu\n", g_pool_s.num_deallocations);
	fprintf(stdout, "overhead: %lu bytes\n", g_pool_s.alloc_size - (LIST_SIZE * sizeof(struct list) * NUM_LOOPS));

	return 0;
}

