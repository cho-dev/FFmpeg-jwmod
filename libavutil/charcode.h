/*
 * Copyright (c) 2014 Coffey
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef FFMPEG_CHARCODE_H
#define FFMPEG_CHARCODE_H

#if CONFIG_ICONV
#include "iconv.h"
#endif

enum charcodeConvertErrno {
  CHARCODE_CONVERT_ERR = -1,
  CHARCODE_CONVERT_OK = 0,
};

/* =====================================================
int char_code_convert(char **outstring, const char *instring, const char *dstcode, const char *srccode);
char **outstring : output string pointer
char *instring   : input string
    memory of 'outstring' will get by this function.
    if you don't need 'outstring' anymore, you should use av_free() to free memory.
char *dstcode    : dst char code name. (see iconv --list)
char *srccode    : src char code name. (see iconv --list)

return :
    0  (CHARCODE_CONVERT_OK) : successful
                         *outstring is converted string.
    -1 (CHARCODE_CONVERT_ERR): could not convert.
                         *outstring is copy of instring.

++++++++++++++ example code. utf-8 to cp932 +++++++++++++++++++++
char *str_cp932;
int  ret;

ret = char_code_convert( &str_cp932, "foo bar text", "CP932", "UTF-8");
if (str_cp932) {
    if (!ret) {
        printf("%s\n", str_cp932); // converted
    } else {
        printf("(not converted):%s\n", str_cp932); // not converted
    }
    av_free(str_cp932)
} else {
    printf("memory allocation error.\n");
}
======================================================== */
int charcode_convert(char **outstring, const char *instring, const char *dstcode, const char *srccode);

#endif
