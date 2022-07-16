/*
 * Musepack audio compression
 * Copyright (c) 2005-2009, The Musepack Development Team
 * Copyright (C) 1999-2004 Buschmann/Klemm/Piecha/Wolf
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Test the fast float-to-int rounding trick works */

#define HAVE_IEEE754_FLOAT
#define HAVE_IEEE754_DOUBLE


/* Test the presence of a 80-bit floating point type for writing AIFF headers */

#define HAVE_IEEE854_LONGDOUBLE


/* parsed values from file "version" */

#define MPCENC_MAJOR 1
#define MPCENC_MINOR 30
#define MPCENC_BUILD 1

/* end of config.h */
