/* $Id$ */
/*
 * Copyright (C) 2009 Samuel Vinson <samuelv0304@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <pjmedia/codec.h>
#include <pjmedia/alaw_ulaw.h>
#include <pjmedia/endpoint.h>
#include <pjmedia/errno.h>
#include <pjmedia/port.h>
#include <pjmedia/plc.h>
#include <pjmedia/silencedet.h>
#include <pj/pool.h>
#include <pj/string.h>
#include <pj/assert.h>
#include <pj/log.h>
#include "../../third_party/opus/Headers/opus.h"

#if defined(PJMEDIA_HAS_OPUS_CODEC) && (PJMEDIA_HAS_OPUS_CODEC!=0)

#define THIS_FILE       "bpus120.c"

static pj_status_t bpus120_test_alloc( pjmedia_codec_factory *factory, const pjmedia_codec_info *id );
static pj_status_t bpus120_default_attr( pjmedia_codec_factory *factory, const pjmedia_codec_info *id, pjmedia_codec_param *attr );
static pj_status_t bpus120_enum_codecs ( pjmedia_codec_factory *factory, unsigned *count, pjmedia_codec_info codecs[]);
static pj_status_t bpus120_alloc_codec( pjmedia_codec_factory *factory, const pjmedia_codec_info *id,pjmedia_codec **p_codec);
static pj_status_t bpus120_dealloc_codec( pjmedia_codec_factory *factory, pjmedia_codec *codec);
static pj_status_t  pjmedia_codec_bpus120_deinit(void);

static pj_status_t  bpus120_init( pjmedia_codec *codec, pj_pool_t *pool );
static pj_status_t  bpus120_open( pjmedia_codec *codec, pjmedia_codec_param *attr );
static pj_status_t  bpus120_close( pjmedia_codec *codec );
static pj_status_t  bpus120_modify( pjmedia_codec *codec, const pjmedia_codec_param *attr );
static pj_status_t  bpus120_parse( pjmedia_codec *codec, void *pkt, pj_size_t pkt_size, const pj_timestamp *timestamp, unsigned *frame_cnt, pjmedia_frame frames[]);
static pj_status_t  _bpus120_encode( pjmedia_codec *codec, const struct pjmedia_frame *input, unsigned output_buf_len, struct pjmedia_frame *output);
static pj_status_t  _bpus120_decode( pjmedia_codec *codec, const struct pjmedia_frame *input, unsigned output_buf_len, struct pjmedia_frame *output);
static pj_status_t  _bpus120_recover( pjmedia_codec *codec, unsigned output_buf_len, struct pjmedia_frame *output);


static pjmedia_codec_factory_op bpus120_factory_op =
{
    &bpus120_test_alloc,
    &bpus120_default_attr,
    &bpus120_enum_codecs,
    &bpus120_alloc_codec,
    &bpus120_dealloc_codec,
    &pjmedia_codec_bpus120_deinit
};

static pjmedia_codec_op bpus120_op =
{
    &bpus120_init,
    &bpus120_open,
    &bpus120_close,
    &bpus120_modify,
    &bpus120_parse,
    &_bpus120_encode,
    &_bpus120_decode,
    &_bpus120_recover
};

/* Speex factory */
static struct bpus120_factory
{
    pjmedia_codec_factory    base;
    pjmedia_endpt	         *endpt;
    pj_pool_t		         *pool;
    pj_mutex_t		         *mutex;
    pjmedia_codec	     codec_list;
    
} bpus120_factory;

struct bpus120_private
{
    pj_pool_t   *pool;        /**< Pool for each instance.    */
    
    OpusEncoder	*enc;
    OpusDecoder	*dec;
    
    pj_bool_t vad;
    pj_bool_t plc;
    
    unsigned pt;

};


PJ_DEF(pj_status_t) pjmedia_codec_bpus120_init(pjmedia_endpt *endpt)
{
    pjmedia_codec_mgr *codec_mgr;
    unsigned i;
    pj_status_t status;
    
    if (bpus120_factory.pool != NULL) {
        /* Already initialized. */
        return PJ_SUCCESS;
    }
    bpus120_factory.base.op = &bpus120_factory_op;
    bpus120_factory.base.factory_data = NULL;
    bpus120_factory.endpt = endpt;
    
    bpus120_factory.pool = pjmedia_endpt_create_pool(endpt, "bpus120", 16000, 16000);
    if (!bpus120_factory.pool) 
        return PJ_ENOMEM;
    pj_list_init(&bpus120_factory.codec_list);
    
    /* Create mutex. */
    status = pj_mutex_create_simple(bpus120_factory.pool, "bpus120", &bpus120_factory.mutex);
    if (status != PJ_SUCCESS)
        goto on_error;
    
    /* Get the codec manager. */
    codec_mgr = pjmedia_endpt_get_codec_mgr(endpt);
    if (!codec_mgr) {
        status = PJ_EINVALIDOP;
        goto on_error;
    }
    
    /* Register codec factory to endpoint. */
    status = pjmedia_codec_mgr_register_factory(codec_mgr, &bpus120_factory.base);
    if (status != PJ_SUCCESS)
        goto on_error;
    
    /* Done. */
    return PJ_SUCCESS;
    
on_error:
    pj_pool_release(bpus120_factory.pool);
    bpus120_factory.pool = NULL;
    return status;
}

PJ_DEF(pj_status_t) pjmedia_codec_bpus120_deinit(void)
{
    pjmedia_codec_mgr *codec_mgr;
    pj_status_t status;
    
    if (bpus120_factory.pool == NULL) {
        /* Already deinitialized */
        return PJ_SUCCESS;
    }
    
    pj_mutex_lock(bpus120_factory.mutex);
    
    /* Get the codec manager. */
    codec_mgr = pjmedia_endpt_get_codec_mgr(bpus120_factory.endpt);
    if (!codec_mgr) {
        pj_pool_release(bpus120_factory.pool);
        bpus120_factory.pool = NULL;
        return PJ_EINVALIDOP;
    }
    
    /* Unregister Speex codec factory. */
    status = pjmedia_codec_mgr_unregister_factory(codec_mgr, &bpus120_factory.base);
    
    /* Destroy mutex. */
    pj_mutex_destroy(bpus120_factory.mutex);
    
    /* Destroy pool. */
    pj_pool_release(bpus120_factory.pool);
    bpus120_factory.pool = NULL;
    
    return status;
}

static pj_status_t bpus120_test_alloc(pjmedia_codec_factory *factory, const pjmedia_codec_info *info )
{
    const pj_str_t bpus120_tag = { "bpus120", 7};
    unsigned i;
    
    PJ_UNUSED_ARG(factory);
    
    /* Type MUST be audio. */
    if (info->type != PJMEDIA_TYPE_AUDIO)
        return PJMEDIA_CODEC_EUNSUP;
    
    /* Check encoding name. */
    if (pj_stricmp(&info->encoding_name, &bpus120_tag) != 0)
        return PJMEDIA_CODEC_EUNSUP;
    
    return PJ_SUCCESS;
}

static pj_status_t bpus120_default_attr (pjmedia_codec_factory *factory, const pjmedia_codec_info *id, pjmedia_codec_param *attr )
{
    PJ_ASSERT_RETURN(factory==&bpus120_factory.base, PJ_EINVAL);
    
    pj_bzero(attr, sizeof(pjmedia_codec_param));
    attr->info.channel_cnt = 1;
    
    attr->info.clock_rate = 8000;
    attr->info.avg_bps = 6000;
    attr->info.max_bps = 20000;
    attr->info.pcm_bits_per_sample = 16;
    attr->info.frm_ptime = 60;
    attr->info.pt = PJMEDIA_RTP_PT_BPUS120;
    
    attr->setting.frm_per_pkt = 2;
    
    attr->setting.cng = 1;
    attr->setting.plc = 1;
    attr->setting.penh =1 ;
    attr->setting.vad = 1;
    
    return PJ_SUCCESS;
}

static pj_status_t bpus120_enum_codecs(pjmedia_codec_factory *factory, unsigned *count, pjmedia_codec_info codecs[])
{
    PJ_LOG(1, (THIS_FILE, "enum_codecs"));
    PJ_UNUSED_ARG(factory);
    
    pj_bzero(&codecs[0], sizeof(pjmedia_codec_info));
    codecs[0].encoding_name = pj_str("bpus120");
    codecs[0].pt = PJMEDIA_RTP_PT_BPUS120;
    codecs[0].type = PJMEDIA_TYPE_AUDIO;
    codecs[0].clock_rate = 8000;
    codecs[0].channel_cnt = 1;
    
    *count = 1;
    return PJ_SUCCESS;
}

static pj_status_t bpus120_alloc_codec( pjmedia_codec_factory *factory, const pjmedia_codec_info *id, pjmedia_codec **p_codec)
{
    PJ_LOG(1, (THIS_FILE, "alloc_codec"));
    pjmedia_codec *codec;
    struct bpus120_private *bpus120;
    
    PJ_ASSERT_RETURN(factory && id && p_codec, PJ_EINVAL);
    PJ_ASSERT_RETURN(factory == &bpus120_factory.base, PJ_EINVAL);
    
    
    pj_mutex_lock(bpus120_factory.mutex);
    
    /* Get free nodes, if any. */
    if (!pj_list_empty(&bpus120_factory.codec_list)) {
        codec = bpus120_factory.codec_list.next;
        pj_list_erase(codec);
    } else {
        codec = PJ_POOL_ZALLOC_T(bpus120_factory.pool, pjmedia_codec);
        PJ_ASSERT_RETURN(codec != NULL, PJ_ENOMEM);
        codec->op = &bpus120_op;
        codec->factory = factory;
        codec->codec_data = pj_pool_alloc(bpus120_factory.pool, sizeof(struct bpus120_private));
    }
    
    pj_mutex_unlock(bpus120_factory.mutex);
    
    bpus120 = (struct bpus120_private*) codec->codec_data;
    bpus120->enc = NULL;
    bpus120->dec = NULL;
    bpus120->pt  = PJMEDIA_RTP_PT_BPUS120;
    bpus120->vad = PJ_TRUE;
    bpus120->plc = PJ_TRUE;

    *p_codec = codec;
    return PJ_SUCCESS;
}

static pj_status_t bpus120_dealloc_codec(pjmedia_codec_factory *factory, pjmedia_codec *codec )
{
    PJ_LOG(1, (THIS_FILE, "dealloc_codec"));
    struct bpus120_private *bpus120;
    
    PJ_ASSERT_RETURN(factory && codec, PJ_EINVAL);
    PJ_ASSERT_RETURN(factory == &bpus120_factory.base, PJ_EINVAL);
    
    /* Close codec, if it's not closed. */
    bpus120 = (struct bpus120_private*) codec->codec_data;
    if (bpus120->enc != NULL || bpus120->dec != NULL) {
        bpus120_close(codec);
    }
    
    /* Put in the free list. */
    pj_mutex_lock(bpus120_factory.mutex);
    pj_list_push_front(&bpus120_factory.codec_list, codec);
    pj_mutex_unlock(bpus120_factory.mutex);
    
    return PJ_SUCCESS;
}

static pj_status_t bpus120_init( pjmedia_codec *codec, pj_pool_t *pool )
{
    PJ_UNUSED_ARG(codec);
    PJ_UNUSED_ARG(pool);
    return PJ_SUCCESS;
}

static pj_status_t bpus120_open(pjmedia_codec *codec, pjmedia_codec_param *attr )
{
    struct bpus120_private *bpus120;
    
    bpus120 = (struct bpus120_private*) codec->codec_data;
    
    /* 
     * Create and initialize encoder. 
     */
    bpus120->enc = opus_encoder_create(8000, 1, OPUS_APPLICATION_VOIP, NULL);
    if (!bpus120->enc){
        bpus120_close(codec);
        return PJMEDIA_CODEC_EFAILED;
    }
    //bpus120_encoder_ctl(bpus120->enc, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_NARROWBAND));
    opus_encoder_ctl(bpus120->enc, OPUS_SET_PACKET_LOSS_PERC(5));
    opus_encoder_ctl(bpus120->enc, OPUS_SET_COMPLEXITY(6));
    opus_encoder_ctl(bpus120->enc, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(bpus120->enc, OPUS_SET_DTX(1));
    opus_encoder_ctl(bpus120->enc, OPUS_RESET_STATE);
    
    //PJ_LOG (1, (THIS_FILE, "size: %d", rate));
    
    /* 
     * Create and initialize decoder. 
     */
    bpus120->dec = opus_decoder_create(8000, 1, NULL);
    
    if (!bpus120->dec) {
        bpus120_close(codec);
        return PJMEDIA_CODEC_EFAILED;
    }
    
    opus_decoder_ctl(bpus120->dec, OPUS_RESET_STATE);
    
    bpus120->vad = (attr->setting.vad != 0);
    bpus120->plc = (attr->setting.plc != 0);
    
    return PJ_SUCCESS;
}

static pj_status_t bpus120_close( pjmedia_codec *codec )
{
    struct bpus120_private *bpus120;
    
    bpus120 = (struct bpus120_private*) codec->codec_data;
    
    /* Destroy encoder*/
    if (bpus120->enc) {
        opus_encoder_destroy( bpus120->enc );
        bpus120->enc = NULL;
    }
    
    /* Destroy decoder */
    if (bpus120->dec) {
        opus_decoder_destroy( bpus120->dec);
        bpus120->dec = NULL;
    }
    
    return PJ_SUCCESS;

}

static pj_status_t  bpus120_modify(pjmedia_codec *codec, const pjmedia_codec_param *attr )
{
    struct bpus120_private *bpus120;
    bpus120 = (struct bpus120_private*) codec->codec_data;
    
    bpus120->vad = (attr->setting.vad != 0);
    bpus120->plc = (attr->setting.plc != 0);
    
    opus_encoder_ctl(bpus120->enc, OPUS_SET_DTX(bpus120->vad));
    
    return PJ_SUCCESS;
}

static pj_status_t  bpus120_parse( pjmedia_codec *codec, void *pkt, pj_size_t pkt_size, const pj_timestamp *ts, unsigned *frame_cnt, pjmedia_frame frames[])
{
//    PJ_LOG(1, ("bpus120parse", "%d, %d, %d", pkt_size, *frame_cnt, ts->u64));
    char* buf = pkt;
    frames[0].type = PJMEDIA_FRAME_TYPE_AUDIO;
    frames[0].buf = pkt+1;
    frames[0].size = buf[0];
    frames[0].timestamp.u64 = ts->u64;
    frames[0].bit_info = 0;
    
    frames[1].type = PJMEDIA_FRAME_TYPE_AUDIO;
    frames[1].buf = pkt+frames[0].size+2;
    frames[1].size = buf[frames[0].size+1];
    frames[1].timestamp.u64 = ts->u64 + 480;
    frames[1].bit_info = 0;
    
    *frame_cnt = 2;
    return PJ_SUCCESS;
}

static pj_status_t  _bpus120_encode(pjmedia_codec *codec, const struct pjmedia_frame *input, unsigned output_buf_len, struct pjmedia_frame *output)
{
    struct bpus120_private *bpus120;
    bpus120 = (struct bpus120_private*) codec->codec_data;
    
    OpusEncoder *enc = ((struct bpus120_private *)(codec->codec_data))->enc;

    char *buffer = output->buf;
    int size1 = opus_encode(enc, input->buf, 480, output->buf+1, output_buf_len);
    buffer[0] = size1;
    int size2 = opus_encode(enc, input->buf+960, 480, output->buf+size1+2, output_buf_len);
    buffer[size1+1] = size2;
    
//    PJ_LOG(1, ("bpus120encode", "%d, %d, %d, %d, %d", input->size, output->size, output_buf_len, size1, size2));
    
    if ((size1 == 0 || size1 == 1) && (size2 == 0 || size2 ==1) && bpus120->vad) {
        output->size = 0;
        output->buf = NULL;
        output->type = PJMEDIA_FRAME_TYPE_NONE;
        output->timestamp = input->timestamp;
    }
    else if (size1 > 1 || ((size1 == 1 || size1 == 0) && !bpus120->vad) ||
             size2 > 1 || ((size2 == 1 || size2 == 0) && !bpus120->vad)) {
        output->size = size1 + size2 + 2;
        output->type = PJMEDIA_FRAME_TYPE_AUDIO;
        output->timestamp = input->timestamp;
        //PJ_LOG(1, (THIS_FILE, "size:%d", size));
    }
    else {
        output->size = 0;
        output->buf = NULL;
        output->type = PJMEDIA_FRAME_TYPE_NONE;
        output->timestamp = input->timestamp;
    }
    return PJ_SUCCESS;
}

static pj_status_t  _bpus120_decode(pjmedia_codec *codec, const struct pjmedia_frame *input, unsigned output_buf_len, struct pjmedia_frame *output)
{
    OpusDecoder *dec = ((struct bpus120_private *)(codec->codec_data))->dec;
    int size = opus_decode(dec, input->buf, input->size, output->buf, output_buf_len/sizeof(opus_int16), 0);
//    PJ_LOG(1, ("bpus120decode", "%d, %d, %d", input->size, size, output_buf_len));
    output->size = size*sizeof(opus_int16);
    output->type = PJMEDIA_FRAME_TYPE_AUDIO;
    output->timestamp = input->timestamp;
 
    
    return PJ_SUCCESS;
}

static pj_status_t _bpus120_recover(pjmedia_codec * codec, unsigned out_size, struct pjmedia_frame * output)
{
    OpusDecoder *dec = ((struct bpus120_private *)(codec->codec_data))->dec;
    int size = opus_decode(dec, NULL, 0, output->buf, out_size/sizeof(opus_int16), 0);
//    PJ_LOG(1, ("bpus120recovery", "%d, %d", size, out_size));
    output->size = size*sizeof(opus_int16);
    output->type = PJMEDIA_FRAME_TYPE_AUDIO;
    return PJ_SUCCESS;
}

#endif
