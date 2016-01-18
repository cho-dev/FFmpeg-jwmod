/*
 * Copyright (c) 2014-2016 Coffey
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

#include "libavutil/eval.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libavutil/pixfmt.h"

#include "avfilter.h"
#include "formats.h"
#include "internal.h"
#include "video.h"

#include <math.h>

enum MonoComicMode {
    MODE_GRAY,
    MODE_TONE_DOT,
    MODE_TONE_HORIZONTAL,
    MODE_TONE_VERTICAL,
    MODE_TONE_DIAGONAL,
    MODE_TONE_PATTERN,
    MODE_NB,
};

typedef struct {
    const AVClass *class;
    enum MonoComicMode mode;
    int il, ih, ol, oh;
    double srate;
    double gamma;
    int compare;
    int mpeg_range;
    int type;
    int pattern_coefficient;
    int chroma_w, chroma_h;
    int chroma_dw, chroma_dh;
    uint8_t lut[256];
    uint8_t *tone;
    uint8_t *ptone;
} MonoComicContext;

#define OFFSET(x) offsetof(MonoComicContext, x)
#define FLAGS AV_OPT_FLAG_FILTERING_PARAM|AV_OPT_FLAG_VIDEO_PARAM

static const AVOption monocomic_options[] = {
    { "mode",       "select mode",     OFFSET(mode), AV_OPT_TYPE_INT, {.i64=MODE_TONE_DOT}, 0, MODE_NB-1, FLAGS, "mode"},
        { "gray",       "gray mode",             0, AV_OPT_TYPE_CONST, {.i64=MODE_GRAY},            .flags=FLAGS, .unit="mode"},
        { "dot",        "tone(dot) mode",        0, AV_OPT_TYPE_CONST, {.i64=MODE_TONE_DOT},        .flags=FLAGS, .unit="mode"},
        { "horizontal", "tone(horizontal) mode", 0, AV_OPT_TYPE_CONST, {.i64=MODE_TONE_HORIZONTAL}, .flags=FLAGS, .unit="mode"},
        { "vertical",   "tone(vertical) mode",   0, AV_OPT_TYPE_CONST, {.i64=MODE_TONE_VERTICAL},   .flags=FLAGS, .unit="mode"},
        { "diagonal",   "tone(diagonal) mode",   0, AV_OPT_TYPE_CONST, {.i64=MODE_TONE_DIAGONAL},   .flags=FLAGS, .unit="mode"},
        { "pattern",    "tone(pattern) mode",    0, AV_OPT_TYPE_CONST, {.i64=MODE_TONE_PATTERN},    .flags=FLAGS, .unit="mode"},
    { "il",         "input min",       OFFSET(il), AV_OPT_TYPE_INT, {.i64=80}, 0, 255, FLAGS},
    { "ih",         "input max",       OFFSET(ih), AV_OPT_TYPE_INT, {.i64=168}, 0, 255, FLAGS},
    { "s",          "saturation to contrast rate", OFFSET(srate), AV_OPT_TYPE_DOUBLE, {.dbl=0}, 0, 100, FLAGS},
    { "compare",    "compare effect",  OFFSET(compare), AV_OPT_TYPE_INT, {.i64=0}, 0, 2, FLAGS},
    { "mpeg",    "limit output to mpeg range",  OFFSET(mpeg_range), AV_OPT_TYPE_INT, {.i64=0}, 0, 1, FLAGS},
    { "type",    "filter type",                 OFFSET(type)      , AV_OPT_TYPE_INT, {.i64=0}, 0, 1, FLAGS},
    { "ptc",     "pattern coefficient",         OFFSET(pattern_coefficient) , AV_OPT_TYPE_INT, {.i64=45}, 0, 127, FLAGS},
    { "gamma",   "gamma correction",            OFFSET(gamma)     , AV_OPT_TYPE_DOUBLE, {.dbl=2.2}, 0.2, 5, FLAGS},
    { NULL }
};

AVFILTER_DEFINE_CLASS(monocomic);

static av_cold int init(AVFilterContext *ctx)
{
    MonoComicContext *rc = ctx->priv;
    uint8_t *tp;
    
    rc->tone = (uint8_t *)av_malloc(16*8*8);
    if (!rc->tone) {
        av_log(ctx, AV_LOG_ERROR, "Could not get memory for tone pattern.\n");
        return AVERROR(ENOMEM);
    }
    rc->ptone = (uint8_t *)av_malloc(64*8*8);
    if (!rc->ptone) {
        av_log(ctx, AV_LOG_ERROR, "Could not get memory for tone pattern.\n");
        return AVERROR(ENOMEM);
    }
    
    // make round tone
    tp = rc->tone;
    for (int i = 0; i < 16; i++) {
        for (int h = 0; h < 8; h++) {
            for (int w = 0; w < 8; w++) {
                int d, rd;
                int pf;
                int y;
                
                y = 15 - i;
                
                pf = 0;
                
                // rd = (4 * y * 15 / 15) * (4 * y * 15 / 15) / 225;
                rd = (4 * y) * (4 * y);
                d  = ((h - 4) * (h - 4) + (w - 4) * (w -4)) * 196;
                if (d < rd) pf++;
                d  = ((h - 0) * (h - 0) + (w - 0) * (w -0)) * 196;
                if (d < rd) pf++;
                d  = ((h - 8) * (h - 8) + (w - 8) * (w -8)) * 196;
                if (d < rd) pf++;
                d  = ((h - 0) * (h - 0) + (w - 8) * (w -8)) * 196;
                if (d < rd) pf++;
                d  = ((h - 8) * (h - 8) + (w - 0) * (w -0)) * 196;
                if (d < rd) pf++;
                /*
                // rd = (4 * y * 63 / 63) * (4 * y * 63 / 63) / 3969;
                rd = (4 * y) * (4 * y);
                d  = ((h - 4) * (h - 4) + (w - 4) * (w -4)) * 3844;
                if (d < rd) pf++;
                d  = ((h - 0) * (h - 0) + (w - 0) * (w -0)) * 3844;
                if (d < rd) pf++;
                d  = ((h - 8) * (h - 8) + (w - 8) * (w -8)) * 3844;
                if (d < rd) pf++;
                d  = ((h - 0) * (h - 0) + (w - 8) * (w -8)) * 3844;
                if (d < rd) pf++;
                d  = ((h - 8) * (h - 8) + (w - 0) * (w -0)) * 3844;
                if (d < rd) pf++;
                */
                if (pf) {
                    if (rc->mpeg_range) {
                        *(tp + w) = 16;
                    } else {
                        *(tp + w) = 0;
                    }
                } else {
                    if (rc->mpeg_range) {
                        *(tp + w) = 235;
                    } else {
                        *(tp + w) = 255;
                    }
                }
            }
            // printf("%02x %02x %02x %02x %02x %02x %02x %02x \n", *(tp+0), *(tp+1), *(tp+2), *(tp+3), *(tp+4), *(tp+5), *(tp+6), *(tp+7));
            tp += 8;
        }
    }
    
    // make pattern tone
    tp = rc->ptone;
    for (int i = 0; i < (64*8*8); i++) {
        if (rc->mpeg_range) {
            *(tp + i) = 16;
        } else {
            *(tp + i) = 0;
        }
    }
    for (int i = 0; i < 64; i++) {
        int y;
        int coe, shift;
        
        y = 0;
        coe = rc->pattern_coefficient % 64;
        shift = rc->pattern_coefficient / 64;
        for (int count = 0; count < i+(i/33); count++) {
            if (*(tp + y) > 16) {
                for (int j = 0; j <= 64; j++) {
                    y++;
                    if (y >= 64) y = y % 64;
                    if (*(tp + y) <= 16) break;
                }
            }
            if (rc->mpeg_range) {
                *(tp + y) = 235;
            } else {
                *(tp + y) = 255;
            }
            y += coe;
            if (y >= 64) y = y % 64;
        }
        if (shift) {
            for (int j = 0; j < 4; j++) {
                for (int k = 0; k < 8; k++) {
                    uint8_t tmp;
                    if (k == 0) tmp = *(tp + 16*j + 8);
                    if (k != 7) {
                        *(tp + 16*j + 8 + k) = *(tp + 16*j + 9 + k);
                    } else {
                        *(tp + 16*j + 8 + k) = tmp;
                    }
                }
            }
        }
        
        tp += 64;
    }
    
    return 0;
}

static av_cold void uninit(AVFilterContext *ctx)
{
    MonoComicContext *rc = ctx->priv;
    
    av_free(rc->tone);
    av_free(rc->ptone);
}

static int config_props_input(AVFilterLink *inlink)
{
    MonoComicContext *rc = inlink->dst->priv;
    const AVPixFmtDescriptor *pixdesc;
    
    // get width, height of chroma plane
    pixdesc = av_pix_fmt_desc_get(inlink->format);
    rc->chroma_w = FF_CEIL_RSHIFT(inlink->w, pixdesc->log2_chroma_w);
    rc->chroma_h = FF_CEIL_RSHIFT(inlink->h, pixdesc->log2_chroma_h);
    
    rc->chroma_dw = (1 << pixdesc->log2_chroma_w);
    rc->chroma_dh = (1 << pixdesc->log2_chroma_h);
    
    if ((rc->mpeg_range) && (rc->mode == MODE_GRAY)) {
        rc->ol = 16;
        rc->oh = 235;
    } else {
        rc->ol = 0;
        rc->oh = 255;
    }
    
    // make lookup table
    for (int i = 0; i < 256 ; i++ ) {
    
        int32_t y;
        int32_t din, dout;
        
        din  = rc->ih - rc->il;
        dout = rc->oh - rc->ol;
        
        if(din) {
            if      ((i < rc->il) && (rc->il < rc->ih)) y = rc->ol;
            else if ((i > rc->il) && (rc->il > rc->ih)) y = rc->ol;
            else if ((i < rc->ih) && (rc->il > rc->ih)) y = rc->oh;
            else if ((i > rc->ih) && (rc->il < rc->ih)) y = rc->oh;
            else y = rc->ol + (((i - rc->il) * dout) + (din / 2) ) / din;
        } else {
            y = (i < rc->il) ? rc->ol : rc->oh;
        }
        
        // gamma correction
        if (rc->mode != MODE_GRAY) {
            y = (int32_t)((pow((double)(y - rc->ol)/(double)(rc->oh - rc->ol), rc->gamma)) * (rc->oh - rc->ol) + rc->ol);
        }
        
        if (y > 255) y = 255;
        if (y < 0) y = 0;
        
        rc->lut[i] = (uint8_t)y;
    }
    return 0;
}

static int query_formats(AVFilterContext *ctx)
{
    static const enum AVPixelFormat pix_fmts[] = {
        AV_PIX_FMT_YUV444P,      AV_PIX_FMT_YUV422P,
        AV_PIX_FMT_YUV420P,      AV_PIX_FMT_NONE
    };

    AVFilterFormats *fmts_list = ff_make_format_list(pix_fmts);
    if (!fmts_list)
        return AVERROR(ENOMEM);
    return ff_set_common_formats(ctx, fmts_list);
}

static int filter_frame(AVFilterLink *inlink, AVFrame *inpic)
{
    MonoComicContext *rc = inlink->dst->priv;
    AVFilterLink *outlink = inlink->dst->outputs[0];
    AVFrame *outpic;
    int direct = 0;
    
    
    if (av_frame_is_writable(inpic)) {
        direct = 1;
        outpic = inpic;
    } else {
        outpic = ff_get_video_buffer(outlink, outlink->w, outlink->h);
        if (!outpic) {
            av_frame_free(&inpic);
            return AVERROR(ENOMEM);
        }
        av_frame_copy_props(outpic, inpic);
    }
    
    {
        uint8_t *src, *dst;
        uint8_t *src_cb, *src_cr, *dst_cb, *dst_cr;
        int src_linesize, dst_linesize;
        int src_cb_linesize, src_cr_linesize, dst_cb_linesize, dst_cr_linesize;
        int32_t srate;
        int w, h, wc;
        int dw, dh;
        
        src_linesize = inpic->linesize[0];
        dst_linesize = outpic->linesize[0];
        src_cb_linesize = inpic->linesize[1];
        dst_cb_linesize = outpic->linesize[1];
        src_cr_linesize = inpic->linesize[2];
        dst_cr_linesize = outpic->linesize[2];
        
        src = inpic->data[0];
        dst = outpic->data[0];
        src_cb = inpic->data[1];
        dst_cb = outpic->data[1];
        src_cr = inpic->data[2];
        dst_cr = outpic->data[2];

        w = inlink->w;
        h = inlink->h;
        dw = rc->chroma_dw;
        dh = rc->chroma_dh;
        srate = (int32_t)(rc->srate * 256 * 2.56);

        if (rc->compare) { wc = w; w = w/2; }
        for (int j = 0; j < h; j++) {
            for (int i = 0; i < w; i++) {
                int32_t y, cb, cr, a1;
                int index;
                int8_t *tp;
                
                y  = (uint32_t)src[i];
                index = i / dw;
                cb = (uint32_t)src_cb[index] - 128;
                cr = (uint32_t)src_cr[index] - 128;
                
                if (cb < 0) cb = -cb;
                if (cr < 0) cr = -cr;
                a1 = (cb > cr) ? cb : cr;
                //a1 = cb + cr;
                
                a1 = (a1 * srate) >> 8;
                a1 += 256;
                
                if (!rc->type) {
                    // pre
                    y = (y * a1) >> 8;
                    if (y > 255) y = 255;
                    y  = rc->lut[y];
                } else {
                    // post
                    y  = rc->lut[y];
                    y = (y * a1) >> 8;
                    if (y > 255) y = 255;
                }
                
                switch (rc->mode) {
                    case MODE_TONE_DOT:
                        // tp = rc->tone + ((y / 16) * 64) + ((j % 8) * 8) + (i % 8);
                        index = ((y >> 4) << 6) | ((j & 7) << 3) | (i & 7);
                        tp = rc->tone + index;
                        y = *tp;
                        break;
                    case MODE_TONE_HORIZONTAL:
                        // tp = rc->tone + ((y / 16) * 64) + ((j % 8) * 8);
                        index = ((y >> 4) << 6) | ((j & 7) << 3);
                        tp = rc->tone + index;
                        y = *tp;
                        break;
                    case MODE_TONE_VERTICAL:
                        // tp = rc->tone + ((y / 16) * 64) + (i % 8);
                        index = ((y >> 4) << 6) | (i & 7);
                        tp = rc->tone + index;
                        y = *tp;
                        break;
                    case MODE_TONE_DIAGONAL:
                        // tp = rc->tone + ((y / 16) * 64) + ((i + j) % 8);
                        index = ((y >> 4) << 6) | ((i + j) & 7);
                        tp = rc->tone + index;
                        y = *tp;
                        break;
                    case MODE_TONE_PATTERN:
                        // tp = rc->tone + ((y / 16) * 64) + ((i + j) % 8);
                        index = ((y >> 2) << 6) | ((j & 7) << 3) | (i & 7);
                        tp = rc->ptone + index;
                        y = *tp;
                        break;
                    default:
                        if (rc->mpeg_range) {
                            if (y < 16)  y = 16;
                            if (y > 235) y = 235;
                        }
                        break;
                }
                dst[i] = (uint8_t)y;
            }
            if (rc->compare && !direct) {
                for (int i = w; i < wc; i++) {
                    dst[i] = src[i];
                }
            }
            src += src_linesize;
            dst += dst_linesize;
            if (((j+1) % dh) == 0) {
                src_cb += src_cb_linesize;
                src_cr += src_cr_linesize;
                dst_cb += dst_cb_linesize;
                dst_cr += dst_cr_linesize;
            }
        }

        src_cb = inpic->data[1];
        dst_cb = outpic->data[1];
        src_cr = inpic->data[2];
        dst_cr = outpic->data[2];
        
        w = rc->chroma_w;
        h = rc->chroma_h;
        if (rc->compare) { wc = w; w = w/2; }
        for (int j = 0; j < h; j++) {
            for (int i = 0; i < w; i++) {
                dst_cb[i] = 128;
                dst_cr[i] = 128;
            }
            if (rc->compare) {
                for (int i = w; i < wc; i++) {
                    if (rc->compare == 1) {
                        dst_cb[i] = src_cb[i];
                        dst_cr[i] = src_cr[i];
                    } else {
                        dst_cb[i] = 128;
                        dst_cr[i] = 128;
                    }
                }
            }
            src_cb += src_cb_linesize;
            src_cr += src_cr_linesize;
            dst_cb += dst_cb_linesize;
            dst_cr += dst_cr_linesize;
        }
    }

    if (!direct)
        av_frame_free(&inpic);

    return ff_filter_frame(outlink, outpic);
}

static const AVFilterPad monocomic_inputs[] = {
    {
        .name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .filter_frame = filter_frame,
        .config_props = config_props_input,
    },
    { NULL }
};

static const AVFilterPad monocomic_outputs[] = {
    {
        .name = "default",
        .type = AVMEDIA_TYPE_VIDEO,
    },
    { NULL }
};

AVFilter ff_vf_monocomic = {
    .name            = "monocomic",
    .description     = NULL_IF_CONFIG_SMALL("Comic like Monochrome filter."),
    .priv_size       = sizeof(MonoComicContext),
    .init            = init,
    .uninit          = uninit,
    .query_formats   = query_formats,
    .inputs          = monocomic_inputs,
    .outputs         = monocomic_outputs,
    .priv_class      = &monocomic_class,
};
