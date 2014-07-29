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

#define THIS_FILE       "opus.c"

static pj_status_t opus_test_alloc( pjmedia_codec_factory *factory, const pjmedia_codec_info *id );
static pj_status_t opus_default_attr( pjmedia_codec_factory *factory, const pjmedia_codec_info *id, pjmedia_codec_param *attr );
static pj_status_t opus_enum_codecs ( pjmedia_codec_factory *factory, unsigned *count, pjmedia_codec_info codecs[]);
static pj_status_t opus_alloc_codec( pjmedia_codec_factory *factory, const pjmedia_codec_info *id,pjmedia_codec **p_codec);
static pj_status_t opus_dealloc_codec( pjmedia_codec_factory *factory, pjmedia_codec *codec);
static pj_status_t  pjmedia_codec_opus_deinit(void);

static pj_status_t  opus_init( pjmedia_codec *codec, pj_pool_t *pool );
static pj_status_t  opus_open( pjmedia_codec *codec, pjmedia_codec_param *attr );
static pj_status_t  opus_close( pjmedia_codec *codec );
static pj_status_t  opus_modify( pjmedia_codec *codec, const pjmedia_codec_param *attr );
static pj_status_t  opus_parse( pjmedia_codec *codec, void *pkt, pj_size_t pkt_size, const pj_timestamp *timestamp, unsigned *frame_cnt, pjmedia_frame frames[]);
static pj_status_t  _opus_encode( pjmedia_codec *codec, const struct pjmedia_frame *input, unsigned output_buf_len, struct pjmedia_frame *output);
static pj_status_t  _opus_decode( pjmedia_codec *codec, const struct pjmedia_frame *input, unsigned output_buf_len, struct pjmedia_frame *output);
static pj_status_t  _opus_recover( pjmedia_codec *codec, unsigned output_buf_len, struct pjmedia_frame *output);


static pjmedia_codec_factory_op opus_factory_op =
{
    &opus_test_alloc,
    &opus_default_attr,
    &opus_enum_codecs,
    &opus_alloc_codec,
    &opus_dealloc_codec,
    &pjmedia_codec_opus_deinit
};

static pjmedia_codec_op opus_op =
{
    &opus_init,
    &opus_open,
    &opus_close,
    &opus_modify,
    &opus_parse,
    &_opus_encode,
    &_opus_decode,
    &_opus_recover
};

/* Speex factory */
static struct opus_factory
{
    pjmedia_codec_factory    base;
    pjmedia_endpt	         *endpt;
    pj_pool_t		         *pool;
    pj_mutex_t		         *mutex;
    pjmedia_codec	     codec_list;
    
} opus_factory;

struct opus_private
{
    pj_pool_t   *pool;        /**< Pool for each instance.    */
    
    OpusEncoder	*enc;
    OpusDecoder	*dec;
    
    pj_bool_t vad;
    pj_bool_t plc;
    
    unsigned pt;

};


PJ_DEF(pj_status_t) pjmedia_codec_opus_init(pjmedia_endpt *endpt)
{
    pjmedia_codec_mgr *codec_mgr;
    unsigned i;
    pj_status_t status;
    
    if (opus_factory.pool != NULL) {
        /* Already initialized. */
        return PJ_SUCCESS;
    }
    opus_factory.base.op = &opus_factory_op;
    opus_factory.base.factory_data = NULL;
    opus_factory.endpt = endpt;
    
    opus_factory.pool = pjmedia_endpt_create_pool(endpt, "opus", 16000, 16000);
    if (!opus_factory.pool) 
        return PJ_ENOMEM;
    pj_list_init(&opus_factory.codec_list);
    
    /* Create mutex. */
    status = pj_mutex_create_simple(opus_factory.pool, "opus", &opus_factory.mutex);
    if (status != PJ_SUCCESS)
        goto on_error;
    
    /* Get the codec manager. */
    codec_mgr = pjmedia_endpt_get_codec_mgr(endpt);
    if (!codec_mgr) {
        status = PJ_EINVALIDOP;
        goto on_error;
    }
    
    /* Register codec factory to endpoint. */
    status = pjmedia_codec_mgr_register_factory(codec_mgr, &opus_factory.base);
    if (status != PJ_SUCCESS)
        goto on_error;
    
    /* Done. */
    return PJ_SUCCESS;
    
on_error:
    pj_pool_release(opus_factory.pool);
    opus_factory.pool = NULL;
    return status;
}

PJ_DEF(pj_status_t) pjmedia_codec_opus_deinit(void)
{
    pjmedia_codec_mgr *codec_mgr;
    pj_status_t status;
    
    if (opus_factory.pool == NULL) {
        /* Already deinitialized */
        return PJ_SUCCESS;
    }
    
    pj_mutex_lock(opus_factory.mutex);
    
    /* Get the codec manager. */
    codec_mgr = pjmedia_endpt_get_codec_mgr(opus_factory.endpt);
    if (!codec_mgr) {
        pj_pool_release(opus_factory.pool);
        opus_factory.pool = NULL;
        return PJ_EINVALIDOP;
    }
    
    /* Unregister Speex codec factory. */
    status = pjmedia_codec_mgr_unregister_factory(codec_mgr, &opus_factory.base);
    
    /* Destroy mutex. */
    pj_mutex_destroy(opus_factory.mutex);
    
    /* Destroy pool. */
    pj_pool_release(opus_factory.pool);
    opus_factory.pool = NULL;
    
    return status;
}

static pj_status_t opus_test_alloc(pjmedia_codec_factory *factory, const pjmedia_codec_info *info )
{
    const pj_str_t opus_tag = { "opus", 4};
    unsigned i;
    
    PJ_UNUSED_ARG(factory);
    
    /* Type MUST be audio. */
    if (info->type != PJMEDIA_TYPE_AUDIO)
        return PJMEDIA_CODEC_EUNSUP;
    
    /* Check encoding name. */
    if (pj_stricmp(&info->encoding_name, &opus_tag) != 0)
        return PJMEDIA_CODEC_EUNSUP;
    
    return PJ_SUCCESS;
}

static pj_status_t opus_default_attr (pjmedia_codec_factory *factory, const pjmedia_codec_info *id, pjmedia_codec_param *attr )
{
    PJ_ASSERT_RETURN(factory==&opus_factory.base, PJ_EINVAL);
    
    pj_bzero(attr, sizeof(pjmedia_codec_param));
    attr->info.channel_cnt = 1;
    
    attr->info.clock_rate = 8000;
    attr->info.avg_bps = 6000;
    attr->info.max_bps = 10000;
    attr->info.pcm_bits_per_sample = 16;
    attr->info.frm_ptime =20;
    attr->info.pt = PJMEDIA_RTP_PT_OPUS;
    
    attr->setting.frm_per_pkt = 1;
    
    attr->setting.cng = 1;
    attr->setting.plc = 1;
    attr->setting.penh =1 ;
    attr->setting.vad = 1;
    
    return PJ_SUCCESS;
}

static pj_status_t opus_enum_codecs(pjmedia_codec_factory *factory, unsigned *count, pjmedia_codec_info codecs[])
{
    PJ_LOG(1, (THIS_FILE, "enum_codecs"));
    PJ_UNUSED_ARG(factory);
    
    pj_bzero(&codecs[0], sizeof(pjmedia_codec_info));
    codecs[0].encoding_name = pj_str("opus");
    codecs[0].pt = PJMEDIA_RTP_PT_OPUS;
    codecs[0].type = PJMEDIA_TYPE_AUDIO;
    codecs[0].clock_rate = 8000;
    codecs[0].channel_cnt = 1;
    
    *count = 1;
    return PJ_SUCCESS;
}

static pj_status_t opus_alloc_codec( pjmedia_codec_factory *factory, const pjmedia_codec_info *id, pjmedia_codec **p_codec)
{
    PJ_LOG(1, (THIS_FILE, "alloc_codec"));
    pjmedia_codec *codec;
    struct opus_private *opus;
    
    PJ_ASSERT_RETURN(factory && id && p_codec, PJ_EINVAL);
    PJ_ASSERT_RETURN(factory == &opus_factory.base, PJ_EINVAL);
    
    
    pj_mutex_lock(opus_factory.mutex);
    
    /* Get free nodes, if any. */
    if (!pj_list_empty(&opus_factory.codec_list)) {
        codec = opus_factory.codec_list.next;
        pj_list_erase(codec);
    } else {
        codec = PJ_POOL_ZALLOC_T(opus_factory.pool, pjmedia_codec);
        PJ_ASSERT_RETURN(codec != NULL, PJ_ENOMEM);
        codec->op = &opus_op;
        codec->factory = factory;
        codec->codec_data = pj_pool_alloc(opus_factory.pool, sizeof(struct opus_private));
    }
    
    pj_mutex_unlock(opus_factory.mutex);
    
    opus = (struct opus_private*) codec->codec_data;
    opus->enc = NULL;
    opus->dec = NULL;
    opus->pt  = PJMEDIA_RTP_PT_OPUS;
    opus->vad = PJ_TRUE;
    opus->plc = PJ_TRUE;

    *p_codec = codec;
    return PJ_SUCCESS;
}

static pj_status_t opus_dealloc_codec(pjmedia_codec_factory *factory, pjmedia_codec *codec )
{
    PJ_LOG(1, (THIS_FILE, "dealloc_codec"));
    struct opus_private *opus;
    
    PJ_ASSERT_RETURN(factory && codec, PJ_EINVAL);
    PJ_ASSERT_RETURN(factory == &opus_factory.base, PJ_EINVAL);
    
    /* Close codec, if it's not closed. */
    opus = (struct opus_private*) codec->codec_data;
    if (opus->enc != NULL || opus->dec != NULL) {
        opus_close(codec);
    }
    
    /* Put in the free list. */
    pj_mutex_lock(opus_factory.mutex);
    pj_list_push_front(&opus_factory.codec_list, codec);
    pj_mutex_unlock(opus_factory.mutex);
    
    return PJ_SUCCESS;
}

static pj_status_t opus_init( pjmedia_codec *codec, pj_pool_t *pool )
{
    PJ_UNUSED_ARG(codec);
    PJ_UNUSED_ARG(pool);
    return PJ_SUCCESS;
}

static pj_status_t opus_open(pjmedia_codec *codec, pjmedia_codec_param *attr )
{
    struct opus_private *opus;
    
    opus = (struct opus_private*) codec->codec_data;
    
    /* 
     * Create and initialize encoder. 
     */
    opus->enc = opus_encoder_create(8000, 1, OPUS_APPLICATION_VOIP, NULL);
    if (!opus->enc){
        opus_close(codec);
        return PJMEDIA_CODEC_EFAILED;
    }
    opus_encoder_ctl(opus->enc, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_NARROWBAND));
    //opus_encoder_ctl(opus->enc, OPUS_SET_PACKET_LOSS_PERC(10));
    opus_encoder_ctl(opus->enc, OPUS_SET_COMPLEXITY(6));
    opus_encoder_ctl(opus->enc, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(opus->enc, OPUS_SET_DTX(1));
    
    //PJ_LOG (1, (THIS_FILE, "size: %d", rate));
    
    /* 
     * Create and initialize decoder. 
     */
    opus->dec = opus_decoder_create(8000, 1, NULL);
    
    if (!opus->dec) {
        opus_close(codec);
        return PJMEDIA_CODEC_EFAILED;
    }
    
    opus->vad = (attr->setting.vad != 0);
    opus->plc = (attr->setting.plc != 0);
    
    return PJ_SUCCESS;
}

static pj_status_t opus_close( pjmedia_codec *codec )
{
    struct opus_private *opus;
    
    opus = (struct opus_private*) codec->codec_data;
    
    /* Destroy encoder*/
    if (opus->enc) {
        opus_encoder_destroy( opus->enc );
        opus->enc = NULL;
    }
    
    /* Destroy decoder */
    if (opus->dec) {
        opus_decoder_destroy( opus->dec);
        opus->dec = NULL;
    }
    
    return PJ_SUCCESS;

}

static pj_status_t  opus_modify(pjmedia_codec *codec, const pjmedia_codec_param *attr )
{
    struct opus_private *opus;
    opus = (struct opus_private*) codec->codec_data;
    
    opus->vad = (attr->setting.vad != 0);
    opus->plc = (attr->setting.plc != 0);
    
    opus_encoder_ctl(opus->enc, OPUS_SET_DTX(opus->vad));
    
    return PJ_SUCCESS;
}

static pj_status_t  opus_parse( pjmedia_codec *codec, void *pkt, pj_size_t pkt_size, const pj_timestamp *ts, unsigned *frame_cnt, pjmedia_frame frames[])
{
    frames[0].type = PJMEDIA_FRAME_TYPE_AUDIO;
    frames[0].buf = pkt;
    frames[0].size = pkt_size;
    frames[0].timestamp.u64 = ts->u64;
    frames[0].bit_info = 0;
    *frame_cnt = 1;
    return PJ_SUCCESS;
}

static pj_status_t  _opus_encode(pjmedia_codec *codec, const struct pjmedia_frame *input, unsigned output_buf_len, struct pjmedia_frame *output)
{
    struct opus_private *opus;
    opus = (struct opus_private*) codec->codec_data;
    
    OpusEncoder *enc = ((struct opus_private *)(codec->codec_data))->enc;
    int size = opus_encode(enc, input->buf, (input->size)/sizeof(opus_int16), output->buf, output_buf_len);
    output->size = size;
    if ((size == 0 || size == 1) && opus->vad) {
        output->size = 0;
        output->buf = NULL;
        output->type = PJMEDIA_FRAME_TYPE_NONE;
        output->timestamp = input->timestamp;
    }
    else if (size > 1 || ((size == 1 || size == 0) && !opus->vad)) {
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

static pj_status_t  _opus_decode(pjmedia_codec *codec, const struct pjmedia_frame *input, unsigned output_buf_len, struct pjmedia_frame *output)
{
    OpusDecoder *dec = ((struct opus_private *)(codec->codec_data))->dec;
    int size = opus_decode(dec,input->buf, input->size, output->buf, output_buf_len/sizeof(opus_int16), 0);
    output->size = size*sizeof(opus_int16);
    output->type = PJMEDIA_FRAME_TYPE_AUDIO;
    output->timestamp = input->timestamp;
    return PJ_SUCCESS;
}

static pj_status_t _opus_recover(pjmedia_codec * codec, unsigned out_size, struct pjmedia_frame * output)
{
    OpusDecoder *dec = ((struct opus_private *)(codec->codec_data))->dec;
    int size = opus_decode(dec, NULL, 0, output->buf, out_size/sizeof(opus_int16), 0);
    output->size = size*sizeof(opus_int16);
    output->type = PJMEDIA_FRAME_TYPE_AUDIO;
    return PJ_SUCCESS;
}

#endif
