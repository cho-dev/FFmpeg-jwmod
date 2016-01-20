/*
 * Copyright (c) 2013 Paul B Mahol
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

#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "avfilter.h"
#include "internal.h"

enum SeparateFieldsMode {
    MODE_NORMAL,
    MODE_REPEAT,
    MODE_NB,
};

typedef struct {
    const AVClass *class;
    enum SeparateFieldsMode mode;
    int nb_planes;
    AVFrame *second;
    AVFrame *third;  // for RFF flag (repeat_pict==1 then make clone third field)
} SeparateFieldsContext;

#define OFFSET(x) offsetof(SeparateFieldsContext, x)
#define FLAGS AV_OPT_FLAG_FILTERING_PARAM|AV_OPT_FLAG_VIDEO_PARAM

static const AVOption separatefields_options[] = {
    { "mode", "select mode", OFFSET(mode), AV_OPT_TYPE_INT, {.i64=MODE_NORMAL}, 0, MODE_NB-1, FLAGS, "mode"},
        { "normal",       "normal separate", 0, AV_OPT_TYPE_CONST, {.i64=MODE_NORMAL}, .flags=FLAGS, .unit="mode"},
        { "repeat",       "add field when frame has repeat flag", 0, AV_OPT_TYPE_CONST, {.i64=MODE_REPEAT}, .flags=FLAGS, .unit="mode"},
        { "rff",          "add field when frame has repeat flag", 0, AV_OPT_TYPE_CONST, {.i64=MODE_REPEAT}, .flags=FLAGS, .unit="mode"},
    { NULL }
};

AVFILTER_DEFINE_CLASS(separatefields);

static int config_props_input(AVFilterLink *inlink)
{
    AVFilterContext *ctx = inlink->dst;
    SeparateFieldsContext *s = ctx->priv;
    
    // pointer initialize
    s->second = NULL;
    s->third = NULL;
    
    return 0;
}

static int config_props_output(AVFilterLink *outlink)
{
    AVFilterContext *ctx = outlink->src;
    SeparateFieldsContext *s = ctx->priv;
    AVFilterLink *inlink = ctx->inputs[0];

    s->nb_planes = av_pix_fmt_count_planes(inlink->format);

    if (inlink->h & 1) {
        av_log(ctx, AV_LOG_ERROR, "height must be even\n");
        return AVERROR_INVALIDDATA;
    }

    outlink->time_base.num = inlink->time_base.num;
    outlink->time_base.den = inlink->time_base.den * 2;
    outlink->frame_rate.num = inlink->frame_rate.num * 2;
    outlink->frame_rate.den = inlink->frame_rate.den;
    outlink->w = inlink->w;
    outlink->h = inlink->h / 2;

    return 0;
}

static void extract_field(AVFrame *frame, int nb_planes, int type)
{
    int i;

    for (i = 0; i < nb_planes; i++) {
        if (type)
            frame->data[i] = frame->data[i] + frame->linesize[i];
        frame->linesize[i] *= 2;
    }
}

static int filter_frame(AVFilterLink *inlink, AVFrame *inpicref)
{
    AVFilterContext *ctx = inlink->dst;
    SeparateFieldsContext *s = ctx->priv;
    AVFilterLink *outlink = ctx->outputs[0];
    int ret;
    char metabuf[128]; // add metadata buffer 20141215

    inpicref->height = outlink->h;
    inpicref->interlaced_frame = 0;

    if (s->second) {
        AVFrame *second = s->second;

        extract_field(second, s->nb_planes, second->top_field_first);

        snprintf(metabuf, sizeof(metabuf), "%s", second->top_field_first ? "bottom" : "top" );
        av_dict_set(&second->metadata, "lavfi.separatefields.TYPE.L", metabuf, 0);
        snprintf(metabuf, sizeof(metabuf), "%s", second->top_field_first ? "B" : "T" );
        av_dict_set(&second->metadata, "lavfi.separatefields.TYPE", metabuf, 0);
        snprintf(metabuf, sizeof(metabuf), "%s", "2" );
        av_dict_set(&second->metadata, "lavfi.separatefields.ORDER", metabuf, 0);

        if (second->pts != AV_NOPTS_VALUE &&
            inpicref->pts != AV_NOPTS_VALUE)
            if (s->third) {
                second->pts = (second->pts * 2) + (inpicref->pts - second->pts) * 2 / 3;
            } else {
                second->pts += inpicref->pts;
            }
        else
            second->pts = AV_NOPTS_VALUE;

        ret = ff_filter_frame(outlink, second);
        if (ret < 0)
            return ret;
    }

    if (s->third) {  // rff : add 1 field after second field. field type is different between second field. 20141215
        AVFrame *third = s->third;

        extract_field(third, s->nb_planes, !third->top_field_first);

        snprintf(metabuf, sizeof(metabuf), "%s", third->top_field_first ? "top" : "bottom" );
        av_dict_set(&third->metadata, "lavfi.separatefields.TYPE.L", metabuf, 0);
        snprintf(metabuf, sizeof(metabuf), "%s", third->top_field_first ? "T" : "B" );
        av_dict_set(&third->metadata, "lavfi.separatefields.TYPE", metabuf, 0);
        snprintf(metabuf, sizeof(metabuf), "%s", "3" );
        av_dict_set(&third->metadata, "lavfi.separatefields.ORDER", metabuf, 0);

        if (third->pts != AV_NOPTS_VALUE &&
            inpicref->pts != AV_NOPTS_VALUE)
            third->pts = (third->pts * 2) + (inpicref->pts - third->pts) * 4 / 3;
        else
            third->pts = AV_NOPTS_VALUE;

        ret = ff_filter_frame(outlink, third);
        if (ret < 0)
            return ret;
    }

    s->second = av_frame_clone(inpicref);
    if (!s->second)
        return AVERROR(ENOMEM);

    if ((inpicref->repeat_pict == 1) && (s->mode == MODE_REPEAT)) {  // repeat_pict == 1: add 1 field.
        s->third = av_frame_clone(inpicref);
        if (!s->third)
            return AVERROR(ENOMEM);
    } else {
        s->third = NULL;
    }

    extract_field(inpicref, s->nb_planes, !inpicref->top_field_first);

    snprintf(metabuf, sizeof(metabuf), "%s", inpicref->top_field_first ? "top" : "bottom" );
    av_dict_set(&inpicref->metadata, "lavfi.separatefields.TYPE.L", metabuf, 0);
    snprintf(metabuf, sizeof(metabuf), "%s", inpicref->top_field_first ? "T" : "B" );
    av_dict_set(&inpicref->metadata, "lavfi.separatefields.TYPE", metabuf, 0);
    snprintf(metabuf, sizeof(metabuf), "%s", "1" );
    av_dict_set(&inpicref->metadata, "lavfi.separatefields.ORDER", metabuf, 0);

    if (inpicref->pts != AV_NOPTS_VALUE)
        inpicref->pts *= 2;

    return ff_filter_frame(outlink, inpicref);
}

static int request_frame(AVFilterLink *outlink)
{
    AVFilterContext *ctx = outlink->src;
    SeparateFieldsContext *s = ctx->priv;
    int ret;
    char metabuf[128]; // add metadata buffer 20141215

    ret = ff_request_frame(ctx->inputs[0]);
    if (ret == AVERROR_EOF && s->second) {
        // s->second->pts *= 2;
        s->second->pts = s->second->pts * 2 + 1 ;  // to prevent same pts as first field 20141215
        extract_field(s->second, s->nb_planes, s->second->top_field_first);

        snprintf(metabuf, sizeof(metabuf), "%s", s->second->top_field_first ? "bottom" : "top" );
        av_dict_set(&s->second->metadata, "lavfi.separatefields.TYPE.L", metabuf, 0);
        snprintf(metabuf, sizeof(metabuf), "%s", s->second->top_field_first ? "B" : "T" );
        av_dict_set(&s->second->metadata, "lavfi.separatefields.TYPE", metabuf, 0);
        snprintf(metabuf, sizeof(metabuf), "%s", "2" );
        av_dict_set(&s->second->metadata, "lavfi.separatefields.ORDER", metabuf, 0);

        ret = ff_filter_frame(outlink, s->second);
        s->second = NULL;
    }

    return ret;
}

static const AVFilterPad separatefields_inputs[] = {
    {
        .name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .config_props = config_props_input,
        .filter_frame = filter_frame,
    },
    { NULL }
};

static const AVFilterPad separatefields_outputs[] = {
    {
        .name          = "default",
        .type          = AVMEDIA_TYPE_VIDEO,
        .config_props  = config_props_output,
        .request_frame = request_frame,
    },
    { NULL }
};

AVFilter ff_vf_separatefields = {
    .name        = "separatefields",
    .description = NULL_IF_CONFIG_SMALL("Split input video frames into fields."),
    .priv_size   = sizeof(SeparateFieldsContext),
    .inputs      = separatefields_inputs,
    .outputs     = separatefields_outputs,
    .priv_class  = &separatefields_class,
};
