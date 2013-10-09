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

#include "psca.h"

void *
psca_alloc_malloc(size_t *size,
                  size_t *offset,
                  void   *context)
{
	size_t sz = *size;
	size_t of = *offset;

	void *block = malloc(sz);

	if (block == NULL) {
		return NULL;
	}

	*size = sz;
	*offset = of;

	return block;
}

void
psca_free_malloc(void   *block,
                 size_t  offset,
                 void   *context)
{
	free((void *)block);
}

