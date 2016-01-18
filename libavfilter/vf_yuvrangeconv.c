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

#include "libavutil/eval.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libavutil/pixfmt.h"

#include "avfilter.h"
#include "formats.h"
#include "internal.h"
#include "video.h"

enum YUVRangeConvMode {
    MODE_Y,
    MODE_CB,
    MODE_CR,
    MODE_A,
    MODE_FULL,
    MODE_MPEG,
    MODE_THROUGH,
    MODE_NB,
};

typedef struct {
    const AVClass *class;
    enum YUVRangeConvMode mode;
    int il, ih, ol, oh;
    int chroma_w, chroma_h;
    uint8_t lut[4][256];
    uint8_t lut_mtof[4][256];
    uint8_t lut_ftom[4][256];
    int compare;
    int autodetect;
} YUVRangeConvContext;

#define OFFSET(x) offsetof(YUVRangeConvContext, x)
#define FLAGS AV_OPT_FLAG_FILTERING_PARAM|AV_OPT_FLAG_VIDEO_PARAM

static const AVOption yuvrangeconv_options[] = {
    { "mode", "select mode", OFFSET(mode), AV_OPT_TYPE_INT, {.i64=MODE_FULL}, 0, MODE_NB-1, FLAGS, "mode"},
        { "y",          "convert y",           0, AV_OPT_TYPE_CONST, {.i64=MODE_Y},       .flags=FLAGS, .unit="mode"},
        { "u",          "convert Cb",          0, AV_OPT_TYPE_CONST, {.i64=MODE_CB},      .flags=FLAGS, .unit="mode"},
        { "v",          "convert Cr",          0, AV_OPT_TYPE_CONST, {.i64=MODE_CR},      .flags=FLAGS, .unit="mode"},
        { "a",          "convert alpha",       0, AV_OPT_TYPE_CONST, {.i64=MODE_A},       .flags=FLAGS, .unit="mode"},
        { "cb",         "convert Cb",          0, AV_OPT_TYPE_CONST, {.i64=MODE_CB},      .flags=FLAGS, .unit="mode"},
        { "cr",         "convert Cr",          0, AV_OPT_TYPE_CONST, {.i64=MODE_CR},      .flags=FLAGS, .unit="mode"},
        { "full",       "mpeg to full range",  0, AV_OPT_TYPE_CONST, {.i64=MODE_FULL},    .flags=FLAGS, .unit="mode"},
        { "jpeg",       "mpeg to full range",  0, AV_OPT_TYPE_CONST, {.i64=MODE_FULL},    .flags=FLAGS, .unit="mode"},
        { "mpeg",       "full to mpeg range",  0, AV_OPT_TYPE_CONST, {.i64=MODE_MPEG},    .flags=FLAGS, .unit="mode"},
        { "tv",         "full to mpeg range",  0, AV_OPT_TYPE_CONST, {.i64=MODE_MPEG},    .flags=FLAGS, .unit="mode"},
        { "through",    "no convert",          0, AV_OPT_TYPE_CONST, {.i64=MODE_THROUGH}, .flags=FLAGS, .unit="mode"},
    { "il",         "input min",               OFFSET(il),         AV_OPT_TYPE_INT, {.i64=0},   0, 255, FLAGS},
    { "ih",         "input max",               OFFSET(ih),         AV_OPT_TYPE_INT, {.i64=255}, 0, 255, FLAGS},
    { "ol",         "output min",              OFFSET(ol),         AV_OPT_TYPE_INT, {.i64=0},   0, 255, FLAGS},
    { "oh",         "output max",              OFFSET(oh),         AV_OPT_TYPE_INT, {.i64=255}, 0, 255, FLAGS},
    { "compare",    "compare effect",          OFFSET(compare),    AV_OPT_TYPE_INT, {.i64=0},   0, 1,   FLAGS},
    { "autodetect", "convert by auto detect" , OFFSET(autodetect), AV_OPT_TYPE_INT, {.i64=0},   0, 1,   FLAGS},
    { NULL }
};

AVFILTER_DEFINE_CLASS(yuvrangeconv);

static int config_props_input(AVFilterLink *inlink)
{
    YUVRangeConvContext *rc = inlink->dst->priv;
    const AVPixFmtDescriptor *pixdesc;
    
    // get width, height of chroma plane
    pixdesc = av_pix_fmt_desc_get(inlink->format);
    rc->chroma_w = FF_CEIL_RSHIFT(inlink->w, pixdesc->log2_chroma_w);
    rc->chroma_h = FF_CEIL_RSHIFT(inlink->h, pixdesc->log2_chroma_h);
    
    // alias:
    // 'yuvrangeconv=full:1'  same as 'yuvrangeconv=full:autodetect=1'
    // 'yuvrangeconv=mpeg:1'  same as 'yuvrangeconv=mpeg:autodetect=1'
    if (((rc->mode == MODE_MPEG) || (rc->mode == MODE_FULL)) && rc->il)
        rc->autodetect = 1;
    
    // make lookup table
    for (int i = 0; i < 256 ; i++ ) {
    
        int32_t y;
        
        rc->lut[0][i] = (uint8_t)i;
        rc->lut[1][i] = (uint8_t)i;
        rc->lut[2][i] = (uint8_t)i;
        rc->lut[3][i] = (uint8_t)i;
        
        if (rc->mode == MODE_Y || rc->mode == MODE_CB || rc->mode == MODE_CR || rc->mode == MODE_A) {
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
            
            if (y > 255) y = 255;
            if (y < 0) y = 0;
            
            if (rc->mode == MODE_Y)
                rc->lut[0][i] = (uint8_t)y;
            if (rc->mode == MODE_CB)
                rc->lut[1][i] = (uint8_t)y;
            if (rc->mode == MODE_CR)
                rc->lut[2][i] = (uint8_t)y;
            if (rc->mode == MODE_A)
                rc->lut[3][i] = (uint8_t)y;
        }
        
        // mpeg range (Y16-235, CbCr16-240) to full range (0-255)
        y = (((i - 16) * 255) + 110) / 219;
        if (y > 255) y = 255;
        if (y < 0) y = 0;
        rc->lut_mtof[0][i] = (uint8_t)y;  // lumi
        
        y = (((i - 128) * 128) + (112 * 128) + 56) / 112;
        if (y > 255) y = 255;
        if (y < 0) y = 0;
        rc->lut_mtof[1][i] = (uint8_t)y;  // chroma
        rc->lut_mtof[2][i] = (uint8_t)y;  // chroma
        rc->lut_mtof[3][i] = (uint8_t)i;  // alpha

        // full range (0-255) to mpeg range (Y16-235, CbCr16-240)
        y = (((i * 219) + 128) / 255) + 16;
        if (y > 235) y = 235;
        if (y < 16) y = 16;
        rc->lut_ftom[0][i] = (uint8_t)y;  // lumi
        
        y = ( ((i - 128) * 112 ) + (128 * 128) + 64) / 128;
        if (y > 240) y = 240;
        if (y < 16) y = 16;
        rc->lut_ftom[1][i] = (uint8_t)y;  // chroma
        rc->lut_ftom[2][i] = (uint8_t)y;  // chroma
        rc->lut_ftom[3][i] = (uint8_t)i;  // alpha
        
    }
    
    return 0;
}

static int query_formats(AVFilterContext *ctx)
{
    static const enum AVPixelFormat pix_fmts[] = {
        AV_PIX_FMT_YUV444P,      AV_PIX_FMT_YUV422P,
        AV_PIX_FMT_YUV420P,      AV_PIX_FMT_YUV411P,
        AV_PIX_FMT_YUV410P,      AV_PIX_FMT_YUV440P,
        AV_PIX_FMT_YUVA444P,     AV_PIX_FMT_YUVA422P,
        AV_PIX_FMT_YUVA420P,
        AV_PIX_FMT_NONE
    };

    AVFilterFormats *fmts_list = ff_make_format_list(pix_fmts);
    if (!fmts_list)
        return AVERROR(ENOMEM);
    return ff_set_common_formats(ctx, fmts_list);
}

static int filter_frame(AVFilterLink *inlink, AVFrame *inpic)
{
    YUVRangeConvContext *rc = inlink->dst->priv;
    AVFilterLink *outlink = inlink->dst->outputs[0];
    enum AVColorRange color_range;
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
    
    color_range = av_frame_get_color_range(inpic);
    
    for (int plane = 0; plane < 4; plane++) {
        uint8_t *src, *dst;
        uint8_t *lut;
        int w, h, wc;
        int src_linesize, dst_linesize;
        
        lut = rc->lut[plane];
        if (rc->autodetect) {
            if ((rc->mode == MODE_MPEG) && (color_range != AVCOL_RANGE_MPEG))
               lut = rc->lut_ftom[plane];
            else if ((rc->mode == MODE_FULL) && (color_range == AVCOL_RANGE_MPEG))
               lut = rc->lut_mtof[plane];
        } else {
            if (rc->mode == MODE_MPEG)
                lut = rc->lut_ftom[plane];
            else if (rc->mode == MODE_FULL)
                lut = rc->lut_mtof[plane];
        }
        
        if (inpic->data[plane]) {
            src = inpic->data[plane];
            dst = outpic->data[plane];
            src_linesize = inpic->linesize[plane];
            dst_linesize = outpic->linesize[plane];
            
            if ((plane == 1) || (plane == 2)) {       // chroma Cb or Cr
                w = rc->chroma_w;
                h = rc->chroma_h;
            } else {                                  // luminance or alpah(if available)
                w = inlink->w;
                h = inlink->h;
            }
            
            if (rc->compare) { wc = w; w = w/2; }
            while (h--) {
                for (int i = 0; i < w; i++)
                    dst[i] = lut[src[i]];
                if (rc->compare && !direct) {
                    for (int i = w; i < wc; i++)
                        dst[i] = src[i];
                }
                src += src_linesize;
                dst += dst_linesize;
            }
        }
    }
    
    if (!direct)
        av_frame_free(&inpic);

    return ff_filter_frame(outlink, outpic);
}

static const AVFilterPad yuvrangeconv_inputs[] = {
    {
        .name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .filter_frame = filter_frame,
        .config_props = config_props_input,
    },
    { NULL }
};

static const AVFilterPad yuvrangeconv_outputs[] = {
    {
        .name = "default",
        .type = AVMEDIA_TYPE_VIDEO,
    },
    { NULL }
};

AVFilter ff_vf_yuvrangeconv = {
    .name            = "yuvrangeconv",
    .description     = NULL_IF_CONFIG_SMALL("YUV range converter."),
    .priv_size       = sizeof(YUVRangeConvContext),
    .query_formats   = query_formats,
    .inputs          = yuvrangeconv_inputs,
    .outputs         = yuvrangeconv_outputs,
    .priv_class      = &yuvrangeconv_class,
};
