/*
 * PNM image parser
 * Copyright (c) 2002, 2003 Fabrice Bellard
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

#include "libavutil/imgutils.h"

#include "parser.h" //for ParseContext
#include "pnm.h"


static int pnm_parse(AVCodecParserContext *s, AVCodecContext *avctx,
                     const uint8_t **poutbuf, int *poutbuf_size,
                     const uint8_t *buf, int buf_size)
{
    ParseContext *pc = s->priv_data;
    PNMContext pnmctx;
    int next;
    int skip = 0;

    for (; pc->overread > 0; pc->overread--) {
        pc->buffer[pc->index++]= pc->buffer[pc->overread_index++];
    }
retry:
    if (pc->index) {
        pnmctx.bytestream_start =
        pnmctx.bytestream       = pc->buffer;
        pnmctx.bytestream_end   = pc->buffer + pc->index;
    } else {
        pnmctx.bytestream_start =
        pnmctx.bytestream       = (uint8_t *) buf + skip; /* casts avoid warnings */
        pnmctx.bytestream_end   = (uint8_t *) buf + buf_size - skip;
    }
    next = END_NOT_FOUND;
    if (ff_pnm_decode_header(avctx, &pnmctx) < 0) {
        if (pnmctx.bytestream < pnmctx.bytestream_end) {
            if (pc->index) {
                pc->index = 0;
            } else {
                unsigned step = FFMAX(1, pnmctx.bytestream - pnmctx.bytestream_start);

                skip += step;
            }
            goto retry;
        }
    } else if (pnmctx.type < 4) {
              uint8_t *bs  = pnmctx.bytestream;
        const uint8_t *end = pnmctx.bytestream_end;

        while (bs < end) {
            int c = *bs++;
            if (c == '#')  {
                while (c != '\n' && bs < end)
                    c = *bs++;
            } else if (c == 'P') {
                next = bs - pnmctx.bytestream_start + skip - 1;
                break;
            }
        }
    } else {
        next = pnmctx.bytestream - pnmctx.bytestream_start + skip
               + av_image_get_buffer_size(avctx->pix_fmt, avctx->width, avctx->height, 1);
    }
    if (next != END_NOT_FOUND && pnmctx.bytestream_start != buf + skip)
        next -= pc->index;
    if (next > buf_size)
        next = END_NOT_FOUND;

    if (ff_combine_frame(pc, next, &buf, &buf_size) < 0) {
        *poutbuf      = NULL;
        *poutbuf_size = 0;
        return buf_size;
    }
    *poutbuf      = buf;
    *poutbuf_size = buf_size;
    return next;
}

AVCodecParser ff_pnm_parser = {
    .codec_ids      = { AV_CODEC_ID_PGM, AV_CODEC_ID_PGMYUV, AV_CODEC_ID_PPM,
                        AV_CODEC_ID_PBM, AV_CODEC_ID_PAM },
    .priv_data_size = sizeof(ParseContext),
    .parser_parse   = pnm_parse,
    .parser_close   = ff_parse_close,
};
