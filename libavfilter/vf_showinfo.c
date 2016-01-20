/*
 * Copyright (c) 2011 Stefano Sabatini
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

/**
 * @file
 * filter for showing textual video frame information
 */

#include <inttypes.h>

#include "libavutil/adler32.h"
#include "libavutil/display.h"
#include "libavutil/imgutils.h"
#include "libavutil/internal.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libavutil/stereo3d.h"
#include "libavutil/timestamp.h"

#include "avfilter.h"
#include "internal.h"
#include "video.h"

enum ShowinfoMode {
    MODE_LOG,
    MODE_METADATA,
    MODE_ALL,
    MODE_NB,
};

typedef struct {
    const AVClass *class;
    enum ShowinfoMode mode;
    double t;
    double t_prev;
    double t_delta;
    double packet_sum;
    double packet_t;
    double bitrate;
    double key_n_prev;
    double key_interval;
} ShowinfoContext;

#define OFFSET(x) offsetof(ShowinfoContext, x)
#define FLAGS AV_OPT_FLAG_FILTERING_PARAM|AV_OPT_FLAG_VIDEO_PARAM

static const AVOption showinfo_options[] = {
    { "mode", "select mode", OFFSET(mode), AV_OPT_TYPE_INT, {.i64=MODE_LOG}, 0, MODE_NB-1, FLAGS, "mode"},
        { "log",       "to terminal",            0, AV_OPT_TYPE_CONST, {.i64=MODE_LOG},      .flags=FLAGS, .unit="mode"},
        { "metadata",  "to metadata",            0, AV_OPT_TYPE_CONST, {.i64=MODE_METADATA}, .flags=FLAGS, .unit="mode"},
        { "all",       "terminal and metadata",  0, AV_OPT_TYPE_CONST, {.i64=MODE_ALL},      .flags=FLAGS, .unit="mode"},
    { NULL }
};

AVFILTER_DEFINE_CLASS(showinfo);

static void dump_stereo3d(AVFilterContext *ctx, AVFrameSideData *sd)
{
    AVStereo3D *stereo;

    av_log(ctx, AV_LOG_INFO, "stereoscopic information: ");
    if (sd->size < sizeof(*stereo)) {
        av_log(ctx, AV_LOG_INFO, "invalid data");
        return;
    }

    stereo = (AVStereo3D *)sd->data;

    av_log(ctx, AV_LOG_INFO, "type - ");
    switch (stereo->type) {
    case AV_STEREO3D_2D:                  av_log(ctx, AV_LOG_INFO, "2D");                     break;
    case AV_STEREO3D_SIDEBYSIDE:          av_log(ctx, AV_LOG_INFO, "side by side");           break;
    case AV_STEREO3D_TOPBOTTOM:           av_log(ctx, AV_LOG_INFO, "top and bottom");         break;
    case AV_STEREO3D_FRAMESEQUENCE:       av_log(ctx, AV_LOG_INFO, "frame alternate");        break;
    case AV_STEREO3D_CHECKERBOARD:        av_log(ctx, AV_LOG_INFO, "checkerboard");           break;
    case AV_STEREO3D_LINES:               av_log(ctx, AV_LOG_INFO, "interleaved lines");      break;
    case AV_STEREO3D_COLUMNS:             av_log(ctx, AV_LOG_INFO, "interleaved columns");    break;
    case AV_STEREO3D_SIDEBYSIDE_QUINCUNX: av_log(ctx, AV_LOG_INFO, "side by side "
                                                                   "(quincunx subsampling)"); break;
    default:                              av_log(ctx, AV_LOG_WARNING, "unknown");             break;
    }

    if (stereo->flags & AV_STEREO3D_FLAG_INVERT)
        av_log(ctx, AV_LOG_INFO, " (inverted)");
}

static void update_sample_stats(const uint8_t *src, int len, int64_t *sum, int64_t *sum2)
{
    int i;

    for (i = 0; i < len; i++) {
        *sum += src[i];
        *sum2 += src[i] * src[i];
    }
}

static int filter_frame(AVFilterLink *inlink, AVFrame *frame)
{
    AVFilterContext *ctx = inlink->dst;
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(inlink->format);
    uint32_t plane_checksum[4] = {0}, checksum = 0;
    int64_t sum[4] = {0}, sum2[4] = {0};
    int32_t pixelcount[4] = {0};
    int i, plane, vsub = desc->log2_chroma_h;
    ShowinfoContext *showinfo = ctx->priv;    // add private data 20141212
    char metabuf[128], strbuf[128];    // add metadata buffer 20141212

    for (plane = 0; plane < 4 && frame->data[plane] && frame->linesize[plane]; plane++) {
        uint8_t *data = frame->data[plane];
        int h = plane == 1 || plane == 2 ? FF_CEIL_RSHIFT(inlink->h, vsub) : inlink->h;
        int linesize = av_image_get_linesize(frame->format, frame->width, plane);

        if (linesize < 0)
            return linesize;

        for (i = 0; i < h; i++) {
            plane_checksum[plane] = av_adler32_update(plane_checksum[plane], data, linesize);
            checksum = av_adler32_update(checksum, data, linesize);

            update_sample_stats(data, linesize, sum+plane, sum2+plane);
            pixelcount[plane] += linesize;
            data += frame->linesize[plane];
        }
    }

    if (showinfo->mode == MODE_LOG || showinfo->mode == MODE_ALL) {
        av_log(ctx, AV_LOG_INFO,
               "n:%4"PRId64" pts:%7s pts_time:%-7s pos:%9"PRId64" "
               "fmt:%s sar:%d/%d s:%dx%d i:%c iskey:%d type:%c "
               "checksum:%08"PRIX32" plane_checksum:[%08"PRIX32,
               inlink->frame_count,
               av_ts2str(frame->pts), av_ts2timestr(frame->pts, &inlink->time_base), av_frame_get_pkt_pos(frame),
               desc->name,
               frame->sample_aspect_ratio.num, frame->sample_aspect_ratio.den,
               frame->width, frame->height,
               !frame->interlaced_frame ? 'P' :         /* Progressive  */
               frame->top_field_first   ? 'T' : 'B',    /* Top / Bottom */
               frame->key_frame,
               av_get_picture_type_char(frame->pict_type),
               checksum, plane_checksum[0]);

        for (plane = 1; plane < 4 && frame->data[plane] && frame->linesize[plane]; plane++)
            av_log(ctx, AV_LOG_INFO, " %08"PRIX32, plane_checksum[plane]);
        av_log(ctx, AV_LOG_INFO, "] mean:[");
        for (plane = 0; plane < 4 && frame->data[plane] && frame->linesize[plane]; plane++)
            av_log(ctx, AV_LOG_INFO, "%"PRId64" ", (sum[plane] + pixelcount[plane]/2) / pixelcount[plane]);
        av_log(ctx, AV_LOG_INFO, "\b] stdev:[");
        for (plane = 0; plane < 4 && frame->data[plane] && frame->linesize[plane]; plane++)
            av_log(ctx, AV_LOG_INFO, "%3.1f ",
                   sqrt((sum2[plane] - sum[plane]*(double)sum[plane]/pixelcount[plane])/pixelcount[plane]));
        av_log(ctx, AV_LOG_INFO, "\b]\n");

        for (i = 0; i < frame->nb_side_data; i++) {
            AVFrameSideData *sd = frame->side_data[i];

            av_log(ctx, AV_LOG_INFO, "  side data - ");
            switch (sd->type) {
            case AV_FRAME_DATA_PANSCAN:
                av_log(ctx, AV_LOG_INFO, "pan/scan");
                break;
            case AV_FRAME_DATA_A53_CC:
                av_log(ctx, AV_LOG_INFO, "A/53 closed captions (%d bytes)", sd->size);
                break;
            case AV_FRAME_DATA_STEREO3D:
                dump_stereo3d(ctx, sd);
                break;
            case AV_FRAME_DATA_DISPLAYMATRIX:
                av_log(ctx, AV_LOG_INFO, "displaymatrix: rotation of %.2f degrees",
                       av_display_rotation_get((int32_t *)sd->data));
                break;
            case AV_FRAME_DATA_AFD:
                av_log(ctx, AV_LOG_INFO, "afd: value of %"PRIu8, sd->data[0]);
                break;
            default:
                av_log(ctx, AV_LOG_WARNING, "unknown side data type %d (%d bytes)",
                       sd->type, sd->size);
                break;
            }

            av_log(ctx, AV_LOG_INFO, "\n");
        }
    }

    if (showinfo->mode == MODE_METADATA || showinfo->mode == MODE_ALL) {
        showinfo->t =  (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(inlink->time_base);
        showinfo->t_delta = (isnan(showinfo->t) || isnan(showinfo->t_prev)) ? NAN : showinfo->t - showinfo->t_prev;
        showinfo->t_prev = showinfo->t;
        showinfo->packet_sum += (double)av_frame_get_pkt_size(frame);
        if (isnan(showinfo->packet_t) || showinfo->packet_t > showinfo->t) {
            showinfo->packet_t = showinfo->t;
            showinfo->packet_sum = 0;
            showinfo->bitrate = NAN;
        }
        if (showinfo->t - showinfo->packet_t > 1) {
            showinfo->bitrate = showinfo->packet_sum * 8 / (showinfo->t - showinfo->packet_t);
            showinfo->packet_t = showinfo->t;
            showinfo->packet_sum = 0;
        }

#define SET_META(key, fmt, val) do {                   \
    AVDictionary *metadata;                            \
    metadata = av_frame_get_metadata(frame);           \
    snprintf(metabuf, sizeof(metabuf), fmt, val);      \
    av_dict_set(&metadata, key, metabuf, 0);           \
    av_frame_set_metadata(frame, metadata);            \
} while (0)
#define SHOWINFO_META_KEY_PREFIX "lavfi.showinfo." 

        SET_META( SHOWINFO_META_KEY_PREFIX"N",            "%"PRId64, (int64_t)inlink->frame_count);
        SET_META( SHOWINFO_META_KEY_PREFIX"PTS",          "%s", av_ts2str(frame->pts));
        
        if (isnan(showinfo->t)) {
            SET_META( SHOWINFO_META_KEY_PREFIX"T",            "%s", "n/a");
            SET_META( SHOWINFO_META_KEY_PREFIX"T.HMS",        "%s", " ??:??:??.???");
        } else {
            SET_META( SHOWINFO_META_KEY_PREFIX"T",            "%.6f", showinfo->t);
            do {
                int64_t ms = round(showinfo->t * 1000);
                char sign = ' ';
                if (ms < 0) {
                    sign = '-';
                    ms = -ms;
                }
                snprintf(strbuf, sizeof(strbuf), "%c%02d:%02d:%02d.%03d", sign,
                           (int)(ms / (60 * 60 * 1000)),
                           (int)(ms / (60 * 1000)) % 60,
                           (int)(ms / 1000) % 60,
                           (int)ms % 1000);
             } while(0);
            SET_META( SHOWINFO_META_KEY_PREFIX"T.HMS",  "%s", strbuf);
        }
        
        if (isnan(showinfo->t_delta)) {
            SET_META( SHOWINFO_META_KEY_PREFIX"T_DELTA",       "%s", "n/a");
            SET_META( SHOWINFO_META_KEY_PREFIX"T_DELTA.FORM",  "%s", "--.--ms(--.--fps)");
        } else {
            SET_META( SHOWINFO_META_KEY_PREFIX"T_DELTA",       "%.6f", showinfo->t_delta);
            do {
                int64_t delta_fps;
                int64_t ms = round(showinfo->t_delta * 100000);
                char sign = '+';
                if (ms < 0) {
                    sign = '-';
                    ms = -ms;
                }
                if (showinfo->t_delta < 0.001) delta_fps = 100000; else delta_fps = round(100 / showinfo->t_delta);
                if (delta_fps >= 100000) {
                    snprintf(strbuf, sizeof(strbuf), "%c%d.%02dms(--.--fps)", sign,
                           (int)(ms / 100),
                           (int)ms % 100
                           );
                } else {
                    snprintf(strbuf, sizeof(strbuf), "%c%d.%02dms(%d.%02dfps)", sign,
                           (int)(ms / 100),
                           (int)ms % 100,
                           (int)(delta_fps / 100),
                           (int)delta_fps % 100
                           );
                }
            } while(0);
            SET_META( SHOWINFO_META_KEY_PREFIX"T_DELTA.FORM",  "%s", strbuf);
        }
        
        do {
            int64_t stream_pos = av_frame_get_pkt_pos(frame);
            SET_META( SHOWINFO_META_KEY_PREFIX"POS",          "%"PRId64, stream_pos);
            snprintf(strbuf, sizeof(strbuf), "%"PRId64, stream_pos);
            for (int i = strlen(strbuf) % 3; i < strlen(strbuf); i += 3 ) {
                if (i > (strbuf[0] == '-')) {
                    for (int j = strlen(strbuf); j >= i ; j-- ) strbuf[j+1] = strbuf[j];
                    strbuf[i] = ',';
                    i++;
                }
            }
            SET_META( SHOWINFO_META_KEY_PREFIX"POS.SEP3",      "%s", strbuf);
        } while(0);
        if (isnan(showinfo->bitrate)) {
            SET_META( SHOWINFO_META_KEY_PREFIX"BITRATE",       "%s", "n/a");
            SET_META( SHOWINFO_META_KEY_PREFIX"BITRATE.FORM",  "%s", "---kbps");
        } else {
            SET_META( SHOWINFO_META_KEY_PREFIX"BITRATE",       "%"PRId64, (int64_t)showinfo->bitrate);
            SET_META( SHOWINFO_META_KEY_PREFIX"BITRATE.FORM",  "%"PRId64"kbps", (int64_t)showinfo->bitrate / 1000);
        }
        SET_META( SHOWINFO_META_KEY_PREFIX"PACKET_SIZE",       "%"PRId64, (int64_t)av_frame_get_pkt_size(frame));
        SET_META( SHOWINFO_META_KEY_PREFIX"FORMAT",       "%s", desc->name);
        if (inlink->time_base.num && inlink->time_base.den) {
            SET_META( SHOWINFO_META_KEY_PREFIX"TIMEBASE",      "%.9f", ((double)inlink->time_base.num / (double)inlink->time_base.den));
            SET_META( SHOWINFO_META_KEY_PREFIX"TIMEBASE.FORM", "%.2fus", ((double)inlink->time_base.num / (double)inlink->time_base.den)*1000000);
        } else {
            SET_META( SHOWINFO_META_KEY_PREFIX"TIMEBASE",      "%s", "n/a");
            SET_META( SHOWINFO_META_KEY_PREFIX"TIMEBASE.FORM", "%s", "n/a");
        }
        SET_META( SHOWINFO_META_KEY_PREFIX"TIMEBASE.NUM",  "%d", inlink->time_base.num);
        SET_META( SHOWINFO_META_KEY_PREFIX"TIMEBASE.DEN",  "%d", inlink->time_base.den);
        if ( !frame->sample_aspect_ratio.den ) {
            SET_META( SHOWINFO_META_KEY_PREFIX"SAR",          "%s", "n/a");
        } else if ( frame->sample_aspect_ratio.den == 1 && frame->sample_aspect_ratio.num == 0 ) {
            SET_META( SHOWINFO_META_KEY_PREFIX"SAR",          "%s", "default");
        } else {
            SET_META( SHOWINFO_META_KEY_PREFIX"SAR",          "%.3f", (double)frame->sample_aspect_ratio.num / (double)frame->sample_aspect_ratio.den );
        }
        SET_META( SHOWINFO_META_KEY_PREFIX"SAR.NUM",      "%d", frame->sample_aspect_ratio.num);
        SET_META( SHOWINFO_META_KEY_PREFIX"SAR.DEN",      "%d", frame->sample_aspect_ratio.den);
        SET_META( SHOWINFO_META_KEY_PREFIX"REPEAT_PICT",          "%d", frame->repeat_pict);
        SET_META( SHOWINFO_META_KEY_PREFIX"WIDTH",        "%d", frame->width);
        SET_META( SHOWINFO_META_KEY_PREFIX"HEIGHT",       "%d", frame->height);
        SET_META( SHOWINFO_META_KEY_PREFIX"ISKEY",        "%d", frame->key_frame);
        if (frame->key_frame) {
            if (!isnan(showinfo->key_n_prev))
                showinfo->key_interval = (double)inlink->frame_count - (double)showinfo->key_n_prev;
            showinfo->key_n_prev = (double)inlink->frame_count;
        }
        if (isnan(showinfo->key_interval)) {
            SET_META( SHOWINFO_META_KEY_PREFIX"KEY_INTERVAL",        "%s", "--");
        } else {
            SET_META( SHOWINFO_META_KEY_PREFIX"KEY_INTERVAL",        "%d", (int)showinfo->key_interval);
        }
        SET_META( SHOWINFO_META_KEY_PREFIX"FRAME_TYPE",   "%c", !frame->interlaced_frame ? 'P' :frame->top_field_first ? 'T' : 'B');
        
        SET_META( SHOWINFO_META_KEY_PREFIX"FRAME_TYPE.L", "%s", !frame->interlaced_frame ? 
                "Progressive" :frame->top_field_first ? "Interlaced(TFF)" : "Interlaced(BFF)");
        SET_META( SHOWINFO_META_KEY_PREFIX"FIELD_TYPE",   "%s", frame->top_field_first ? "T" : "B");
        SET_META( SHOWINFO_META_KEY_PREFIX"FIELD_TYPE.L", "%s", frame->top_field_first ? "TFF" : "BFF");
        SET_META( SHOWINFO_META_KEY_PREFIX"PICT_TYPE",    "%c", av_get_picture_type_char(frame->pict_type));
        SET_META( SHOWINFO_META_KEY_PREFIX"COLOR_RANGE",  "%d", av_frame_get_color_range(frame));
        
        for (plane = 0; plane < 4 && frame->data[plane] && frame->linesize[plane]; plane++) {
            int64_t mean;
            char keyname[128];
            snprintf(keyname, sizeof(keyname),  SHOWINFO_META_KEY_PREFIX"MEAN%d", plane);
            mean = pixelcount[plane] ? (sum[plane] + pixelcount[plane]/2) / pixelcount[plane] : 0;
            SET_META(keyname,        "%"PRId64, mean);
        }
    }

    return ff_filter_frame(inlink->dst->outputs[0], frame);
}

static int config_props(AVFilterContext *ctx, AVFilterLink *link, int is_out)
{
    ShowinfoContext *s = ctx->priv;

    if (s->mode == MODE_LOG || s->mode == MODE_ALL) {
        av_log(ctx, AV_LOG_INFO, "config %s time_base: %d/%d, frame_rate: %d/%d\n",
               is_out ? "out" : "in",
               link->time_base.num, link->time_base.den,
               link->frame_rate.num, link->frame_rate.den);
    }

    return 0;
}

static int config_props_in(AVFilterLink *link)
{
    AVFilterContext *ctx = link->dst;
    ShowinfoContext *s = ctx->priv;

    s->t          = NAN;
    s->t_prev     = NAN;
    s->t_delta    = NAN;
    s->packet_sum = 0;
    s->packet_t   = NAN;
    s->bitrate    = NAN;
    s->key_n_prev = NAN;
    s->key_interval = NAN;

    return config_props(ctx, link, 0);
}

static int config_props_out(AVFilterLink *link)
{
    AVFilterContext *ctx = link->src;
    return config_props(ctx, link, 1);
}

static const AVFilterPad avfilter_vf_showinfo_inputs[] = {
    {
        .name             = "default",
        .type             = AVMEDIA_TYPE_VIDEO,
        .filter_frame     = filter_frame,
        .config_props     = config_props_in,
    },
    { NULL }
};

static const AVFilterPad avfilter_vf_showinfo_outputs[] = {
    {
        .name = "default",
        .type = AVMEDIA_TYPE_VIDEO,
        .config_props  = config_props_out,
    },
    { NULL }
};

AVFilter ff_vf_showinfo = {
    .name        = "showinfo",
    .description = NULL_IF_CONFIG_SMALL("Show textual information for each video frame."),
    .priv_size   = sizeof(ShowinfoContext),
    .inputs      = avfilter_vf_showinfo_inputs,
    .outputs     = avfilter_vf_showinfo_outputs,
    .priv_class  = &showinfo_class,
};
