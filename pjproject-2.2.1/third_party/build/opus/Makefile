include ../../../build.mak
include ../../../build/common.mak

export LIBDIR := ../../lib

RULES_MAK := $(PJDIR)/build/rules.mak

export OPUS_CODEC_LIB := libopuscodec-$(TARGET_NAME)$(LIBEXT)

ifeq ($(PJ_SHARED_LIBRARIES),)
else
export OPUS_CODEC_SONAME := libopuscodec.$(SHLIB_SUFFIX)
export OPUS_CODEC_SHLIB := $(OPUS_CODEC_SONAME).$(PJ_VERSION_MAJOR)
endif

###############################################################################
# Gather all flags.
#
export _CFLAGS 	:= $(CC_CFLAGS) $(OS_CFLAGS) $(HOST_CFLAGS) $(M_CFLAGS) \
		   $(CFLAGS) $(CC_INC)../.. $(CC_INC)../../opus/Headers \
		   $(CC_INC)../../../pjlib/include \
		   $(CC_INC)../../opus/celt \
		   $(CC_INC)../../opus/silk \
		   $(CC_INC)../../opus/silk/float \
		   $(CC_INC)../../opus/silk/fixed \
		   -include config.h
export _CXXFLAGS:= $(_CFLAGS) $(CC_CXXFLAGS) $(OS_CXXFLAGS) $(M_CXXFLAGS) \
		   $(HOST_CXXFLAGS) $(CXXFLAGS)
export _LDFLAGS := $(CC_LDFLAGS) $(OS_LDFLAGS) $(M_LDFLAGS) $(HOST_LDFLAGS) \
		   $(APP_LDFLAGS) $(LDFLAGS)

export OPUS_CODEC_SRCDIR = ../../opus
export OPUS_CODEC_OBJS = celt/bands.o \
		celt/celt.o \
		celt/celt_lpc.o \
		celt/cwrs.o \
		celt/entcode.o \
		celt/entdec.o \
		celt/entenc.o \
		celt/kiss_fft.o \
		celt/laplace.o \
		celt/mathops.o \
		celt/mdct.o celt/modes.o \
		celt/opus_custom_demo.o \
		celt/pitch.o \
		celt/quant_bands.o \
		celt/rate.o \
		celt/vq.o \
		silk/A2NLSF.o \
		silk/ana_filt_bank_1.o \
		silk/biquad_alt.o \
		silk/bwexpander.o \
		silk/bwexpander_32.o \
		silk/check_control_input.o \
		silk/CNG.o \
		silk/code_signs.o \
		silk/control_audio_bandwidth.o \
		silk/control_codec.o \
		silk/control_SNR.o \
		silk/debug.o \
		silk/dec_API.o \
		silk/decode_core.o \
		silk/decode_frame.o \
		silk/decode_indices.o \
		silk/decode_parameters.o \
		silk/decode_pitch.o \
		silk/decode_pulses.o \
		silk/decoder_set_fs.o \
		silk/enc_API.o \
		silk/encode_indices.o \
		silk/encode_pulses.o \
		silk/fixed/apply_sine_window_FIX.o \
		silk/fixed/autocorr_FIX.o \
		silk/fixed/burg_modified_FIX.o \
		silk/fixed/corrMatrix_FIX.o \
		silk/fixed/encode_frame_FIX.o \
		silk/fixed/find_LPC_FIX.o \
		silk/fixed/find_LTP_FIX.o \
		silk/fixed/find_pitch_lags_FIX.o \
		silk/fixed/find_pred_coefs_FIX.o \
		silk/fixed/k2a_FIX.o \
		silk/fixed/k2a_Q16_FIX.o \
		silk/fixed/LTP_analysis_filter_FIX.o \
		silk/fixed/LTP_scale_ctrl_FIX.o \
		silk/fixed/noise_shape_analysis_FIX.o \
		silk/fixed/pitch_analysis_core_FIX.o \
		silk/fixed/prefilter_FIX.o \
		silk/fixed/process_gains_FIX.o \
		silk/fixed/regularize_correlations_FIX.o \
		silk/fixed/residual_energy16_FIX.o \
		silk/fixed/residual_energy_FIX.o \
		silk/fixed/schur64_FIX.o \
		silk/fixed/schur_FIX.o \
		silk/fixed/solve_LS_FIX.o \
		silk/fixed/vector_ops_FIX.o \
		silk/fixed/warped_autocorrelation_FIX.o \
		silk/gain_quant.o \
		silk/HP_variable_cutoff.o \
		silk/init_decoder.o \
		silk/init_encoder.o \
		silk/inner_prod_aligned.o \
		silk/interpolate.o \
		silk/lin2log.o \
		silk/log2lin.o \
		silk/LP_variable_cutoff.o \
		silk/LPC_analysis_filter.o \
		silk/LPC_inv_pred_gain.o \
		silk/NLSF2A.o \
		silk/NLSF_decode.o \
		silk/NLSF_del_dec_quant.o \
		silk/NLSF_encode.o \
		silk/NLSF_stabilize.o \
		silk/NLSF_unpack.o \
		silk/NLSF_VQ.o \
		silk/NLSF_VQ_weights_laroia.o \
		silk/NSQ.o silk/NSQ_del_dec.o \
		silk/pitch_est_tables.o silk/PLC.o \
		silk/process_NLSFs.o \
		silk/quant_LTP_gains.o \
		silk/resampler.o \
		silk/resampler_down2.o \
		silk/resampler_down2_3.o \
		silk/resampler_private_AR2.o \
		silk/resampler_private_down_FIR.o \
		silk/resampler_private_IIR_FIR.o \
		silk/resampler_private_up2_HQ.o \
		silk/resampler_rom.o \
		silk/shell_coder.o \
		silk/sigm_Q15.o \
		silk/sort.o \
		silk/stereo_decode_pred.o \
		silk/stereo_encode_pred.o \
		silk/stereo_find_predictor.o \
		silk/stereo_LR_to_MS.o \
		silk/stereo_MS_to_LR.o \
		silk/stereo_quant_pred.o \
		silk/sum_sqr_shift.o \
		silk/table_LSF_cos.o \
		silk/tables_gain.o \
		silk/tables_LTP.o \
		silk/tables_NLSF_CB_NB_MB.o \
		silk/tables_NLSF_CB_WB.o \
		silk/tables_other.o \
		silk/tables_pitch_lag.o \
		silk/tables_pulses_per_block.o \
		silk/VAD.o \
		silk/VQ_WMat_EC.o \
		src/opus.o \
		src/opus_compare.o \
		src/opus_decoder.o \
		src/opus_demo.o \
		src/opus_encoder.o \
		src/opus_multistream.o \
		src/repacketizer.o

export OPUS_CODEC_CFLAGS = -DHAVE_CONFIG_H $(_CFLAGS)
export OPUS_CODEC_LDFLAGS :=$(PJLIB_LDLIB) $(_LDFLAGS)

export CC_OUT CC AR RANLIB HOST_MV HOST_RM HOST_RMDIR HOST_MKDIR OBJEXT LD LDOUT
###############################################################################
# Main entry
#
# $(TARGET) is defined in os-$(OS_NAME).mak file in current directory.
#
TARGETS := $(OPUS_CODEC_LIB) $(OPUS_CODEC_SONAME)

all: $(TARGETS)

doc:
	cd .. && doxygen docs/doxygen.cfg

dep: depend
distclean: realclean

.PHONY: all dep depend clean realclean distclean
.PHONY: $(TARGETS)
.PHONY: $(OPUS_CODEC_LIB) $(OPUS_CODEC_SONAME)

libopuscodec: $(OPUS_CODEC_LIB)
$(OPUS_CODEC_SONAME): $(OPUS_CODEC_LIB)
$(OPUS_CODEC_LIB) $(OPUS_CODEC_SONAME):
	$(MAKE) -f $(RULES_MAK) APP=OPUS_CODEC app=libopuscodec $(subst /,$(HOST_PSEP),$(LIBDIR)/$@)

clean print_lib:
	$(MAKE) -f $(RULES_MAK) APP=OPUS_CODEC app=libopuscodec $@

realclean:
	$(subst @@,$(subst /,$(HOST_PSEP),.ilbc-$(TARGET_NAME).depend),$(HOST_RMR))

	$(MAKE) -f $(RULES_MAK) APP=OPUS_CODEC app=libopuscodec $@

depend:
	$(MAKE) -f $(RULES_MAK) APP=OPUS_CODEC app=libopuscodec $@


