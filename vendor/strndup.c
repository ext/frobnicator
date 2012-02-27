/*
 * This file is part of libportable.
 * Copyright (C) 2008-2012 David Sveningsson <ext@sidvind.com>
 * 
 * libportable is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * libportable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with libportable.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef WIN32
#error Only implemented for windows, file I bug if you require this for another platform
#endif

char* strndup(const char* src, size_t n){
	if ( src ){
		const size_t srclen = strlen(src);
		const size_t bytes = srclen < n ? srclen : n;
		char* dst = malloc(bytes+1);
		if ( !dst ){
			/* assumes errno already set */
			return NULL;
		}

		strncpy_s(dst, bytes+1, src, bytes);
		return dst;
	} else {
		errno = EINVAL;
		return NULL;
	}
}
