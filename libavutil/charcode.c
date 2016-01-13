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

#include <string.h>
#include "mem.h"
#include "avstring.h"
#include "charcode.h"

int charcode_convert(char **outstring, const char *instring, const char *dstcode, const char *srccode)
{
#if CONFIG_ICONV
    iconv_t cd;
    char    *inbuf, *outbuf;
    char    *inp, *outp;
    size_t  instring_len;
    size_t  guessed_max_outstring_len;
    size_t  inbuf_size, outbuf_size;
    size_t  iconv_ret;
    int     ret;
    
    instring_len = strlen(instring);
    guessed_max_outstring_len = instring_len * 4;
    
    inbuf  = av_malloc( instring_len + 1 );
    outbuf = av_malloc( guessed_max_outstring_len );
    
    if (!inbuf || !outbuf) {
        if (inbuf)
            av_free(inbuf);
        if (outbuf)
            av_free(outbuf);
        *outstring = NULL;
        return CHARCODE_CONVERT_ERR;
    }
    
    av_strlcpy(inbuf, instring, instring_len + 1);  // inbuf  <- instr
    *outbuf = 0;                                    // outbuf <- null string
    
    inbuf_size  = instring_len + 1;                 // include tail '0x00'
    outbuf_size = guessed_max_outstring_len - 1;
    inp  = inbuf;
    outp = outbuf;
    
    cd = iconv_open(dstcode, srccode);
    if (cd || (cd != (iconv_t)-1)) {
        iconv_ret = iconv(cd, &inp, &inbuf_size, &outp, &outbuf_size);
        if (iconv_ret != -1) {  // convert successful
            *outp = 0;
            ret = CHARCODE_CONVERT_OK;
        } else {                // convert failed
           av_strlcpy(outbuf, instring, guessed_max_outstring_len);
           ret = CHARCODE_CONVERT_ERR;
        }
        iconv_close(cd);
    } else {                    // iconv cannot open
        av_strlcpy(outbuf, instring, guessed_max_outstring_len);
        ret = CHARCODE_CONVERT_ERR;
    }
    
    av_free(inbuf);
    *outstring = outbuf;
    
    return ret;
#else
    *outstring = NULL;
    return CHARCODE_CONVERT_ERR;
#endif
}

