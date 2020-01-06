// This sub-API supports the following functionalities:
//
//  - Enables full duplex VoIP sessions via RTP using G.711 (mu-Law or A-Law).
//  - Initialization and termination.
//  - Trace information on text files or via callbacks.
//  - Multi-channel support (mixing, sending to multiple destinations etc.).
//
// To support other codecs than G.711, the AoECodec sub-API must be utilized.
//
// Usage example, omitting error checking:
//
//  using namespace voip;
//  AudioEngine* voe = AudioEngine::Create();
//  AoEBase* base = AoEBase::GetInterface(voe);
//  base->Init();
//  int ch = base->CreateChannel();
//  base->StartPlayout(ch);
//  ...
//  base->DeleteChannel(ch);
//  base->Terminate();
//  base->Release();
//  AudioEngine::Delete(voe);
//
#ifndef VOIP_AUDIO_ENGINE_AOE_BASE_H
#define VOIP_AUDIO_ENGINE_AOE_BASE_H

#include "audio_engine/include/aoe_base.h"


#endif  //  VOIP_AUDIO_ENGINE_AOE_BASE_H
