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

#define THIS_FILE       "bpus.c"

static pj_status_t bpus_test_alloc( pjmedia_codec_factory *factory, const pjmedia_codec_info *id );
static pj_status_t bpus_default_attr( pjmedia_codec_factory *factory, const pjmedia_codec_info *id, pjmedia_codec_param *attr );
static pj_status_t bpus_enum_codecs ( pjmedia_codec_factory *factory, unsigned *count, pjmedia_codec_info codecs[]);
static pj_status_t bpus_alloc_codec( pjmedia_codec_factory *factory, const pjmedia_codec_info *id,pjmedia_codec **p_codec);
static pj_status_t bpus_dealloc_codec( pjmedia_codec_factory *factory, pjmedia_codec *codec);
static pj_status_t  pjmedia_codec_bpus_deinit(void);

static pj_status_t  bpus_init( pjmedia_codec *codec, pj_pool_t *pool );
static pj_status_t  bpus_open( pjmedia_codec *codec, pjmedia_codec_param *attr );
static pj_status_t  bpus_close( pjmedia_codec *codec );
static pj_status_t  bpus_modify( pjmedia_codec *codec, const pjmedia_codec_param *attr );
static pj_status_t  bpus_parse( pjmedia_codec *codec, void *pkt, pj_size_t pkt_size, const pj_timestamp *timestamp, unsigned *frame_cnt, pjmedia_frame frames[]);
static pj_status_t  _bpus_encode( pjmedia_codec *codec, const struct pjmedia_frame *input, unsigned output_buf_len, struct pjmedia_frame *output);
static pj_status_t  _bpus_decode( pjmedia_codec *codec, const struct pjmedia_frame *input, unsigned output_buf_len, struct pjmedia_frame *output);
static pj_status_t  _bpus_recover( pjmedia_codec *codec, unsigned output_buf_len, struct pjmedia_frame *output);


static pjmedia_codec_factory_op bpus_factory_op =
{
    &bpus_test_alloc,
    &bpus_default_attr,
    &bpus_enum_codecs,
    &bpus_alloc_codec,
    &bpus_dealloc_codec,
    &pjmedia_codec_bpus_deinit
};

static pjmedia_codec_op bpus_op =
{
    &bpus_init,
    &bpus_open,
    &bpus_close,
    &bpus_modify,
    &bpus_parse,
    &_bpus_encode,
    &_bpus_decode,
    &_bpus_recover
};

/* Speex factory */
static struct bpus_factory
{
    pjmedia_codec_factory    base;
    pjmedia_endpt	         *endpt;
    pj_pool_t		         *pool;
    pj_mutex_t		         *mutex;
    pjmedia_codec	     codec_list;
    
} bpus_factory;

struct bpus_private
{
    pj_pool_t   *pool;        /**< Pool for each instance.    */
    
    OpusEncoder	*enc;
    OpusDecoder	*dec;
    
    pj_bool_t vad;
    pj_bool_t plc;
    
    unsigned pt;

};


PJ_DEF(pj_status_t) pjmedia_codec_bpus_init(pjmedia_endpt *endpt)
{
    pjmedia_codec_mgr *codec_mgr;
    unsigned i;
    pj_status_t status;
    
    if (bpus_factory.pool != NULL) {
        /* Already initialized. */
        return PJ_SUCCESS;
    }
    bpus_factory.base.op = &bpus_factory_op;
    bpus_factory.base.factory_data = NULL;
    bpus_factory.endpt = endpt;
    
    bpus_factory.pool = pjmedia_endpt_create_pool(endpt, "bpus", 16000, 16000);
    if (!bpus_factory.pool) 
        return PJ_ENOMEM;
    pj_list_init(&bpus_factory.codec_list);
    
    /* Create mutex. */
    status = pj_mutex_create_simple(bpus_factory.pool, "bpus", &bpus_factory.mutex);
    if (status != PJ_SUCCESS)
        goto on_error;
    
    /* Get the codec manager. */
    codec_mgr = pjmedia_endpt_get_codec_mgr(endpt);
    if (!codec_mgr) {
        status = PJ_EINVALIDOP;
        goto on_error;
    }
    
    /* Register codec factory to endpoint. */
    status = pjmedia_codec_mgr_register_factory(codec_mgr, &bpus_factory.base);
    if (status != PJ_SUCCESS)
        goto on_error;
    
    /* Done. */
    return PJ_SUCCESS;
    
on_error:
    pj_pool_release(bpus_factory.pool);
    bpus_factory.pool = NULL;
    return status;
}

PJ_DEF(pj_status_t) pjmedia_codec_bpus_deinit(void)
{
    pjmedia_codec_mgr *codec_mgr;
    pj_status_t status;
    
    if (bpus_factory.pool == NULL) {
        /* Already deinitialized */
        return PJ_SUCCESS;
    }
    
    pj_mutex_lock(bpus_factory.mutex);
    
    /* Get the codec manager. */
    codec_mgr = pjmedia_endpt_get_codec_mgr(bpus_factory.endpt);
    if (!codec_mgr) {
        pj_pool_release(bpus_factory.pool);
        bpus_factory.pool = NULL;
        return PJ_EINVALIDOP;
    }
    
    /* Unregister Speex codec factory. */
    status = pjmedia_codec_mgr_unregister_factory(codec_mgr, &bpus_factory.base);
    
    /* Destroy mutex. */
    pj_mutex_destroy(bpus_factory.mutex);
    
    /* Destroy pool. */
    pj_pool_release(bpus_factory.pool);
    bpus_factory.pool = NULL;
    
    return status;
}

static pj_status_t bpus_test_alloc(pjmedia_codec_factory *factory, const pjmedia_codec_info *info )
{
    const pj_str_t bpus_tag = { "bpus", 4};
    unsigned i;
    
    PJ_UNUSED_ARG(factory);
    
    /* Type MUST be audio. */
    if (info->type != PJMEDIA_TYPE_AUDIO)
        return PJMEDIA_CODEC_EUNSUP;
    
    /* Check encoding name. */
    if (pj_stricmp(&info->encoding_name, &bpus_tag) != 0)
        return PJMEDIA_CODEC_EUNSUP;
    
    return PJ_SUCCESS;
}

static pj_status_t bpus_default_attr (pjmedia_codec_factory *factory, const pjmedia_codec_info *id, pjmedia_codec_param *attr )
{
    PJ_ASSERT_RETURN(factory==&bpus_factory.base, PJ_EINVAL);
    
    pj_bzero(attr, sizeof(pjmedia_codec_param));
    attr->info.channel_cnt = 1;
    
    attr->info.clock_rate = 8000;
    attr->info.avg_bps = 6000;
    attr->info.max_bps = 10000;
    attr->info.pcm_bits_per_sample = 16;
    attr->info.frm_ptime =60;
    attr->info.pt = PJMEDIA_RTP_PT_BPUS;
    
    attr->setting.frm_per_pkt = 1;
    
    attr->setting.cng = 1;
    attr->setting.plc = 1;
    attr->setting.penh =1 ;
    attr->setting.vad = 1;
    
    return PJ_SUCCESS;
}

static pj_status_t bpus_enum_codecs(pjmedia_codec_factory *factory, unsigned *count, pjmedia_codec_info codecs[])
{
    PJ_LOG(1, (THIS_FILE, "enum_codecs"));
    PJ_UNUSED_ARG(factory);
    
    pj_bzero(&codecs[0], sizeof(pjmedia_codec_info));
    codecs[0].encoding_name = pj_str("bpus");
    codecs[0].pt = PJMEDIA_RTP_PT_BPUS;
    codecs[0].type = PJMEDIA_TYPE_AUDIO;
    codecs[0].clock_rate = 8000;
    codecs[0].channel_cnt = 1;
    
    *count = 1;
    return PJ_SUCCESS;
}

static pj_status_t bpus_alloc_codec( pjmedia_codec_factory *factory, const pjmedia_codec_info *id, pjmedia_codec **p_codec)
{
    PJ_LOG(1, (THIS_FILE, "alloc_codec"));
    pjmedia_codec *codec;
    struct bpus_private *bpus;
    
    PJ_ASSERT_RETURN(factory && id && p_codec, PJ_EINVAL);
    PJ_ASSERT_RETURN(factory == &bpus_factory.base, PJ_EINVAL);
    
    
    pj_mutex_lock(bpus_factory.mutex);
    
    /* Get free nodes, if any. */
    if (!pj_list_empty(&bpus_factory.codec_list)) {
        codec = bpus_factory.codec_list.next;
        pj_list_erase(codec);
    } else {
        codec = PJ_POOL_ZALLOC_T(bpus_factory.pool, pjmedia_codec);
        PJ_ASSERT_RETURN(codec != NULL, PJ_ENOMEM);
        codec->op = &bpus_op;
        codec->factory = factory;
        codec->codec_data = pj_pool_alloc(bpus_factory.pool, sizeof(struct bpus_private));
    }
    
    pj_mutex_unlock(bpus_factory.mutex);
    
    bpus = (struct bpus_private*) codec->codec_data;
    bpus->enc = NULL;
    bpus->dec = NULL;
    bpus->pt  = PJMEDIA_RTP_PT_BPUS;
    bpus->vad = PJ_TRUE;
    bpus->plc = PJ_TRUE;

    *p_codec = codec;
    return PJ_SUCCESS;
}

static pj_status_t bpus_dealloc_codec(pjmedia_codec_factory *factory, pjmedia_codec *codec )
{
    PJ_LOG(1, (THIS_FILE, "dealloc_codec"));
    struct bpus_private *bpus;
    
    PJ_ASSERT_RETURN(factory && codec, PJ_EINVAL);
    PJ_ASSERT_RETURN(factory == &bpus_factory.base, PJ_EINVAL);
    
    /* Close codec, if it's not closed. */
    bpus = (struct bpus_private*) codec->codec_data;
    if (bpus->enc != NULL || bpus->dec != NULL) {
        bpus_close(codec);
    }
    
    /* Put in the free list. */
    pj_mutex_lock(bpus_factory.mutex);
    pj_list_push_front(&bpus_factory.codec_list, codec);
    pj_mutex_unlock(bpus_factory.mutex);
    
    return PJ_SUCCESS;
}

static pj_status_t bpus_init( pjmedia_codec *codec, pj_pool_t *pool )
{
    PJ_UNUSED_ARG(codec);
    PJ_UNUSED_ARG(pool);
    return PJ_SUCCESS;
}

static pj_status_t bpus_open(pjmedia_codec *codec, pjmedia_codec_param *attr )
{
    struct bpus_private *bpus;
    
    bpus = (struct bpus_private*) codec->codec_data;
    
    /* 
     * Create and initialize encoder. 
     */
    bpus->enc = opus_encoder_create(8000, 1, OPUS_APPLICATION_VOIP, NULL);
    if (!bpus->enc){
        bpus_close(codec);
        return PJMEDIA_CODEC_EFAILED;
    }
    //bpus_encoder_ctl(bpus->enc, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_NARROWBAND));
    opus_encoder_ctl(bpus->enc, OPUS_SET_PACKET_LOSS_PERC(5));
    opus_encoder_ctl(bpus->enc, OPUS_SET_COMPLEXITY(6));
    opus_encoder_ctl(bpus->enc, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(bpus->enc, OPUS_SET_DTX(1));
    opus_encoder_ctl(bpus->enc, OPUS_RESET_STATE);
    
    //PJ_LOG (1, (THIS_FILE, "size: %d", rate));
    
    /* 
     * Create and initialize decoder. 
     */
    bpus->dec = opus_decoder_create(8000, 1, NULL);
    
    if (!bpus->dec) {
        bpus_close(codec);
        return PJMEDIA_CODEC_EFAILED;
    }
    
    opus_decoder_ctl(bpus->dec, OPUS_RESET_STATE);
    
    bpus->vad = (attr->setting.vad != 0);
    bpus->plc = (attr->setting.plc != 0);
    
    return PJ_SUCCESS;
}

static pj_status_t bpus_close( pjmedia_codec *codec )
{
    struct bpus_private *bpus;
    
    bpus = (struct bpus_private*) codec->codec_data;
    
    /* Destroy encoder*/
    if (bpus->enc) {
        opus_encoder_destroy( bpus->enc );
        bpus->enc = NULL;
    }
    
    /* Destroy decoder */
    if (bpus->dec) {
        opus_decoder_destroy( bpus->dec);
        bpus->dec = NULL;
    }
    
    return PJ_SUCCESS;

}

static pj_status_t  bpus_modify(pjmedia_codec *codec, const pjmedia_codec_param *attr )
{
    struct bpus_private *bpus;
    bpus = (struct bpus_private*) codec->codec_data;
    
    bpus->vad = (attr->setting.vad != 0);
    bpus->plc = (attr->setting.plc != 0);
    
    opus_encoder_ctl(bpus->enc, OPUS_SET_DTX(bpus->vad));
    
    return PJ_SUCCESS;
}

static pj_status_t  bpus_parse( pjmedia_codec *codec, void *pkt, pj_size_t pkt_size, const pj_timestamp *ts, unsigned *frame_cnt, pjmedia_frame frames[])
{
    frames[0].type = PJMEDIA_FRAME_TYPE_AUDIO;
    frames[0].buf = pkt;
    frames[0].size = pkt_size;
    frames[0].timestamp.u64 = ts->u64;
    frames[0].bit_info = 0;
    *frame_cnt = 1;
    return PJ_SUCCESS;
}

static pj_status_t  _bpus_encode(pjmedia_codec *codec, const struct pjmedia_frame *input, unsigned output_buf_len, struct pjmedia_frame *output)
{
    struct bpus_private *bpus;
    bpus = (struct bpus_private*) codec->codec_data;
    
    OpusEncoder *enc = ((struct bpus_private *)(codec->codec_data))->enc;
    int size = opus_encode(enc, input->buf, (input->size)/sizeof(opus_int16), output->buf, output_buf_len);
    output->size = size;
    if ((size == 0 || size == 1) && bpus->vad) {
        output->size = 0;
        output->buf = NULL;
        output->type = PJMEDIA_FRAME_TYPE_NONE;
        output->timestamp = input->timestamp;
    }
    else if (size > 1 || ((size == 1 || size == 0) && !bpus->vad)) {
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

static pj_status_t  _bpus_decode(pjmedia_codec *codec, const struct pjmedia_frame *input, unsigned output_buf_len, struct pjmedia_frame *output)
{
    OpusDecoder *dec = ((struct bpus_private *)(codec->codec_data))->dec;
    int size = opus_decode(dec,input->buf, input->size, output->buf, output_buf_len/sizeof(opus_int16), 0);
    output->size = size*sizeof(opus_int16);
    output->type = PJMEDIA_FRAME_TYPE_AUDIO;
    output->timestamp = input->timestamp;
    return PJ_SUCCESS;
}

static pj_status_t _bpus_recover(pjmedia_codec * codec, unsigned out_size, struct pjmedia_frame * output)
{
    OpusDecoder *dec = ((struct bpus_private *)(codec->codec_data))->dec;
    int size = opus_decode(dec, NULL, 0, output->buf, out_size/sizeof(opus_int16), 0);
    output->size = size*sizeof(opus_int16);
    output->type = PJMEDIA_FRAME_TYPE_AUDIO;
    return PJ_SUCCESS;
}

#endif
