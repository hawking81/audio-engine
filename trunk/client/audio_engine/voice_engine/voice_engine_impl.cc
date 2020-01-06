/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#if defined(VOIP_ANDROID)
#include "audio_engine/modules/audio_device/android/audio_device_template.h"
#include "audio_engine/modules/audio_device/android/audio_record_jni.h"
#include "audio_engine/modules/audio_device/android/audio_track_jni.h"
#if !defined(WEBRTC_CHROMIUM_BUILD)
#include "audio_engine/modules/audio_device/android/opensles_input.h"
#include "audio_engine/modules/audio_device/android/opensles_output.h"
#endif
#endif

#include "audio_engine/modules/audio_coding/main/interface/audio_coding_module.h"
#include "audio_engine/system_wrappers/interface/trace.h"
#include "audio_engine/voice_engine/voice_engine_impl.h"

namespace VoIP
{

// Counter to be ensure that we can add a correct ID in all static trace
// methods. It is not the nicest solution, especially not since we already
// have a counter in VoEBaseImpl. In other words, there is room for
// improvement here.
static int32_t gVoiceEngineInstanceCounter = 0;

AudioEngine* GetVoiceEngine(const Config* config, bool owns_config)
{
#if (defined _WIN32)
  HMODULE hmod = LoadLibrary(TEXT("VoiceEngineTestingDynamic.dll"));

  if (hmod) {
    typedef AudioEngine* (*PfnGetVoiceEngine)(void);
    PfnGetVoiceEngine pfn = (PfnGetVoiceEngine)GetProcAddress(
        hmod,"GetVoiceEngine");
    if (pfn) {
      AudioEngine* self = pfn();
      if (owns_config) {
        delete config;
      }
      return (self);
    }
  }
#endif

    VoiceEngineImpl* self = new VoiceEngineImpl(config, owns_config);
    if (self != NULL)
    {
        self->AddRef();  // First reference.  Released in AudioEngine::Delete.
        gVoiceEngineInstanceCounter++;
    }
    return self;
}

int VoiceEngineImpl::AddRef() {
  return ++_ref_count;
}

// This implements the Release() method for all the inherited interfaces.
int VoiceEngineImpl::Release() {
  int new_ref = --_ref_count;
  assert(new_ref >= 0);
  if (new_ref == 0) {
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice, -1,
                 "VoiceEngineImpl self deleting (voiceEngine=0x%p)",
                 this);

    // Clear any pointers before starting destruction. Otherwise worker-
    // threads will still have pointers to a partially destructed object.
    // Example: AudioDeviceBuffer::RequestPlayoutData() can access a
    // partially deconstructed |_ptrCbAudioTransport| during destruction
    // if we don't call Terminate here.
    Terminate();
    delete this;
  }

  return new_ref;
}

AudioEngine* AudioEngine::Create() {
  Config* config = new Config();
  config->Set<AudioCodingModuleFactory>(new AudioCodingModuleFactory());

  return GetVoiceEngine(config, true);
}

AudioEngine* AudioEngine::Create(const Config& config) {
  return GetVoiceEngine(&config, false);
}

int AudioEngine::SetTraceFilter(unsigned int filter)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice,
                 VoEId(gVoiceEngineInstanceCounter, -1),
                 "SetTraceFilter(filter=0x%x)", filter);

    // Remember old filter
    uint32_t oldFilter = Trace::level_filter();
    Trace::set_level_filter(filter);

    // If previous log was ignored, log again after changing filter
    if (kTraceNone == oldFilter)
    {
        WEBRTC_TRACE(kTraceApiCall, kTraceVoice, -1,
                     "SetTraceFilter(filter=0x%x)", filter);
    }

    return 0;
}

int AudioEngine::SetTraceFile(const char* fileNameUTF8,
                              bool addFileCounter)
{
    int ret = Trace::SetTraceFile(fileNameUTF8, addFileCounter);
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice,
                 VoEId(gVoiceEngineInstanceCounter, -1),
                 "SetTraceFile(fileNameUTF8=%s, addFileCounter=%d)",
                 fileNameUTF8, addFileCounter);
    return (ret);
}

int AudioEngine::SetTraceCallback(TraceCallback* callback)
{
    WEBRTC_TRACE(kTraceApiCall, kTraceVoice,
                 VoEId(gVoiceEngineInstanceCounter, -1),
                 "SetTraceCallback(callback=0x%x)", callback);
    return (Trace::SetTraceCallback(callback));
}

bool AudioEngine::Delete(AudioEngine*& voiceEngine)
{
    if (voiceEngine == NULL)
        return false;

    VoiceEngineImpl* s = static_cast<VoiceEngineImpl*>(voiceEngine);
    // Release the reference that was added in GetVoiceEngine.
    int ref = s->Release();
    voiceEngine = NULL;

    if (ref != 0) {
        WEBRTC_TRACE(kTraceWarning, kTraceVoice, -1,
            "AudioEngine::Delete did not release the very last reference.  "
            "%d references remain.", ref);
    }

    return true;
}

}  // namespace VoIP