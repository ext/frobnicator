/*
 * This file is part of libportable.
 * Copyright (C) 2008-2011 David Sveningsson <ext@sidvind.com>
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

#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <errno.h>
#include <assert.h>

int gettimeofday(struct timeval* tv, struct timezone* tz){
  struct _timeb timebuf;
  errno_t ret;
  long tmp;
  if ( (ret=_ftime_s (&timebuf)) != 0 ){
	  errno = ret;
	  return -1;
  }
  tmp = (long)timebuf.time;
  tv->tv_sec = tmp;
  tv->tv_usec = timebuf.millitm * 1000;

  assert(tv->tv_sec > 0);
  assert(tv->tv_usec > 0);
  return 0;
}
