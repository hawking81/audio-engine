/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "audio_engine/modules/audio_coding/neteq4/tools/rtp_file_source.h"

#include <assert.h>
#include <string.h>
#ifdef WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include "audio_engine/modules/audio_coding/neteq4/tools/packet.h"
#include "audio_engine/modules/rtp_rtcp/interface/rtp_header_parser.h"

namespace webrtc {
namespace test {

RtpFileSource* RtpFileSource::Create(const std::string& file_name) {
  RtpFileSource* source = new RtpFileSource;
  assert(source);
  if (!source->OpenFile(file_name) || !source->SkipFileHeader()) {
    assert(false);
    delete source;
    return NULL;
  }
  return source;
}

RtpFileSource::~RtpFileSource() {
  if (in_file_)
    fclose(in_file_);
}

bool RtpFileSource::RegisterRtpHeaderExtension(RTPExtensionType type,
                                               uint8_t id) {
  assert(parser_.get());
  return parser_->RegisterRtpHeaderExtension(type, id);
}

Packet* RtpFileSource::NextPacket() {
  uint16_t length;
  if (fread(&length, sizeof(uint16_t), 1, in_file_) == 0) {
    assert(false);
    return NULL;
  }
  length = ntohs(length);

  uint16_t plen;
  if (fread(&plen, sizeof(uint16_t), 1, in_file_) == 0) {
    assert(false);
    return NULL;
  }
  plen = ntohs(plen);

  uint32_t offset;
  if (fread(&offset, sizeof(uint32_t), 1, in_file_) == 0) {
    assert(false);
    return NULL;
  }

  // Use length here because a plen of 0 specifies RTCP.
  size_t packet_size_bytes = length - kPacketHeaderSize;
  if (packet_size_bytes <= 0) {
    // May be an RTCP packet.
    return NULL;
  }
  uint8_t* packet_memory = new uint8_t[packet_size_bytes];
  if (fread(packet_memory, 1, packet_size_bytes, in_file_) !=
      packet_size_bytes) {
    assert(false);
    delete[] packet_memory;
    return NULL;
  }
  Packet* packet = new Packet(
      packet_memory, packet_size_bytes, plen, ntohl(offset), *parser_.get());
  if (!packet->valid_header()) {
    assert(false);
    delete packet;
    return NULL;
  }
  return packet;
}

bool RtpFileSource::EndOfFile() const {
  assert(in_file_);
  return ftell(in_file_) >= file_end_;
}

RtpFileSource::RtpFileSource()
    : PacketSource(),
      in_file_(NULL),
      file_end_(-1),
      parser_(RtpHeaderParser::Create()) {}

bool RtpFileSource::OpenFile(const std::string& file_name) {
  in_file_ = fopen(file_name.c_str(), "rb");
  assert(in_file_);
  if (in_file_ == NULL) {
    return false;
  }

  // Find out how long the file is.
  fseek(in_file_, 0, SEEK_END);
  file_end_ = ftell(in_file_);
  rewind(in_file_);
  return true;
}

bool RtpFileSource::SkipFileHeader() {
  char firstline[kFirstLineLength];
  assert(in_file_);
  if (fgets(firstline, kFirstLineLength, in_file_) == NULL) {
    assert(false);
    return false;
  }
  // Check that the first line is ok.
  if ((strncmp(firstline, "#!rtpplay1.0", 12) != 0) &&
      (strncmp(firstline, "#!RTPencode1.0", 14) != 0)) {
    assert(false);
    return false;
  }
  // Skip the file header.
  if (fseek(in_file_, kRtpFileHeaderSize, SEEK_CUR) != 0) {
    assert(false);
    return false;
  }
  return true;
}

}  // namespace test
}  // namespace webrtc
