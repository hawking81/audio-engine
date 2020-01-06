
#include "audio_engine/voice_engine/utility.h"

#include "audio_engine/common_audio/resampler/include/push_resampler.h"
#include "audio_engine/common_audio/signal_processing/include/signal_processing_library.h"
#include "audio_engine/include/common_types.h"
#include "audio_engine/modules/interface/module_common_types.h"
#include "audio_engine/modules/utility/interface/audio_frame_operations.h"
#include "audio_engine/system_wrappers/interface/logging.h"
#include "audio_engine/voice_engine/audio_engine_defines.h"

namespace VoIP {
namespace aoe {

// TODO(ajm): There is significant overlap between RemixAndResample and
// ConvertToCodecFormat, but if we're to consolidate we should probably make a
// real converter class.
void RemixAndResample(const AudioFrame& src_frame,
                      PushResampler<int16_t>* resampler,
                      AudioFrame* dst_frame) {
  const int16_t* audio_ptr = src_frame.data_;
  int audio_ptr_num_channels = src_frame.num_channels_;
  int16_t mono_audio[AudioFrame::kMaxDataSizeSamples];

  // Downmix before resampling.
  if (src_frame.num_channels_ == 2 && dst_frame->num_channels_ == 1) {
    AudioFrameOperations::StereoToMono(src_frame.data_,
                                       src_frame.samples_per_channel_,
                                       mono_audio);
    audio_ptr = mono_audio;
    audio_ptr_num_channels = 1;
  }

  if (resampler->InitializeIfNeeded(src_frame.sample_rate_hz_,
                                    dst_frame->sample_rate_hz_,
                                    audio_ptr_num_channels) == -1) {
    dst_frame->CopyFrom(src_frame);
    LOG_FERR3(LS_ERROR, InitializeIfNeeded, src_frame.sample_rate_hz_,
              dst_frame->sample_rate_hz_, audio_ptr_num_channels);
    assert(false);
  }

  const int src_length = src_frame.samples_per_channel_ *
                         audio_ptr_num_channels;
  int out_length = resampler->Resample(audio_ptr, src_length, dst_frame->data_,
                                       AudioFrame::kMaxDataSizeSamples);
  if (out_length == -1) {
    dst_frame->CopyFrom(src_frame);
    LOG_FERR3(LS_ERROR, Resample, audio_ptr, src_length, dst_frame->data_);
    assert(false);
  }
  dst_frame->samples_per_channel_ = out_length / audio_ptr_num_channels;

  // Upmix after resampling.
  if (src_frame.num_channels_ == 1 && dst_frame->num_channels_ == 2) {
    // The audio in dst_frame really is mono at this point; MonoToStereo will
    // set this back to stereo.
    dst_frame->num_channels_ = 1;
    AudioFrameOperations::MonoToStereo(dst_frame);
  }
}

void DownConvertToCodecFormat(const int16_t* src_data,
                              int samples_per_channel,
                              int num_channels,
                              int sample_rate_hz,
                              int codec_num_channels,
                              int codec_rate_hz,
                              int16_t* mono_buffer,
                              PushResampler<int16_t>* resampler,
                              AudioFrame* dst_af) {
  assert(samples_per_channel <= kMaxMonoDataSizeSamples);
  assert(num_channels == 1 || num_channels == 2);
  assert(codec_num_channels == 1 || codec_num_channels == 2);

  // Never upsample the capture signal here. This should be done at the
  // end of the send chain.
  int destination_rate = std::min(codec_rate_hz, sample_rate_hz);

  // If no stereo codecs are in use, we downmix a stereo stream from the
  // device early in the chain, before resampling.
  if (num_channels == 2 && codec_num_channels == 1) {
    AudioFrameOperations::StereoToMono(src_data, samples_per_channel,
                                       mono_buffer);
    src_data = mono_buffer;
    num_channels = 1;
  }

  if (resampler->InitializeIfNeeded(
          sample_rate_hz, destination_rate, num_channels) != 0) {
    LOG_FERR3(LS_ERROR,
              InitializeIfNeeded,
              sample_rate_hz,
              destination_rate,
              num_channels);
    assert(false);
  }

  const int in_length = samples_per_channel * num_channels;
  int out_length = resampler->Resample(
      src_data, in_length, dst_af->data_, AudioFrame::kMaxDataSizeSamples);
  if (out_length == -1) {
    LOG_FERR3(LS_ERROR, Resample, src_data, in_length, dst_af->data_);
    assert(false);
  }

  dst_af->samples_per_channel_ = out_length / num_channels;
  dst_af->sample_rate_hz_ = destination_rate;
  dst_af->num_channels_ = num_channels;
  dst_af->timestamp_ = -1;
  dst_af->speech_type_ = AudioFrame::kNormalSpeech;
  dst_af->vad_activity_ = AudioFrame::kVadUnknown;
}

void MixWithSat(int16_t target[],
                int target_channel,
                const int16_t source[],
                int source_channel,
                int source_len) {
  assert(target_channel == 1 || target_channel == 2);
  assert(source_channel == 1 || source_channel == 2);

  if (target_channel == 2 && source_channel == 1) {
    // Convert source from mono to stereo.
    int32_t left = 0;
    int32_t right = 0;
    for (int i = 0; i < source_len; ++i) {
      left = source[i] + target[i * 2];
      right = source[i] + target[i * 2 + 1];
      target[i * 2] = VoipSpl_SatW32ToW16(left);
      target[i * 2 + 1] = VoipSpl_SatW32ToW16(right);
    }
  } else if (target_channel == 1 && source_channel == 2) {
    // Convert source from stereo to mono.
    int32_t temp = 0;
    for (int i = 0; i < source_len / 2; ++i) {
      temp = ((source[i * 2] + source[i * 2 + 1]) >> 1) + target[i];
      target[i] = VoipSpl_SatW32ToW16(temp);
    }
  } else {
    int32_t temp = 0;
    for (int i = 0; i < source_len; ++i) {
      temp = source[i] + target[i];
      target[i] = VoipSpl_SatW32ToW16(temp);
    }
  }
}

}  // namespace aoe
}  // namespace VoIP
