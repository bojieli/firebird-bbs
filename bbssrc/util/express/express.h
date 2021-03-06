/*
 * express.h		-- defines some useful variable
 *	
 * A part of BBS Express Project
 *
 * Copyright (c) 1998, 1999, Edward Ping-Da Chuang <edwardc@edwardc.dhs.org>
 * of BBS Express Project, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * CVS: $Id: express.h,v 1.1 2000/01/15 01:45:32 edwardc Exp $
 */

#ifndef _EXPRESS_H_
#define _EXPRESS_H_

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#define BOARDS		".BOARDS"
#define DOTDIR		".DIR"
#define PERM_POSTMASK  0100000

#define YEA (1)			/* Booleans  (Yep, for true and false) */
#define NA  (0)

#define STRLEN               80	/* Length of most string data */

char            genbuf[256];

#endif /* _EXPRESS_H_ */
