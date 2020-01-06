#ifndef VOIP_SYSTEM_WRAPPERS_SOURCE_TRACE_IMPL_H_
#define VOIP_SYSTEM_WRAPPERS_SOURCE_TRACE_IMPL_H_

#include "audio_engine/system_wrappers/interface/critical_section_wrapper.h"
#include "audio_engine/system_wrappers/interface/event_wrapper.h"
#include "audio_engine/system_wrappers/interface/file_wrapper.h"
#include "audio_engine/system_wrappers/interface/static_instance.h"
#include "audio_engine/system_wrappers/interface/thread_wrapper.h"
#include "audio_engine/system_wrappers/interface/trace.h"

namespace VoIP {

// TODO(pwestin) VOIP_TRACE_MAX_QUEUE needs to be tweaked
// TODO(hellner) the buffer should be close to how much the system can write to
//               file. Increasing the buffer will not solve anything. Sooner or
//               later the buffer is going to fill up anyways.
#if defined(VOIP_IOS)
#define VOIP_TRACE_MAX_QUEUE  2000
#else
#define VOIP_TRACE_MAX_QUEUE  8000
#endif
#define VOIP_TRACE_NUM_ARRAY 2
#define VOIP_TRACE_MAX_MESSAGE_SIZE 256
// Total buffer size is VOIP_TRACE_NUM_ARRAY (number of buffer partitions) *
// VOIP_TRACE_MAX_QUEUE (number of lines per buffer partition) *
// VOIP_TRACE_MAX_MESSAGE_SIZE (number of 1 byte charachters per line) =
// 1 or 4 Mbyte.

#define VOIP_TRACE_MAX_FILE_SIZE 100*1000
// Number of rows that may be written to file. On average 110 bytes per row (max
// 256 bytes per row). So on average 110*100*1000 = 11 Mbyte, max 256*100*1000 =
// 25.6 Mbyte

class TraceImpl : public Trace {
 public:
  virtual ~TraceImpl();

  static TraceImpl* CreateInstance();
  static TraceImpl* GetTrace(const TraceLevel level = kTraceAll);

  int32_t SetTraceFileImpl(const char* file_name, const bool add_file_counter);
  int32_t TraceFileImpl(char file_name[FileWrapper::kMaxFileNameSize]);

  int32_t SetTraceCallbackImpl(TraceCallback* callback);

  void AddImpl(const TraceLevel level, const TraceModule module,
               const int32_t id, const char* msg);

  bool StopThread();

  bool TraceCheck(const TraceLevel level) const;

 protected:
  TraceImpl();

  static TraceImpl* StaticInstance(CountOperation count_operation,
                                   const TraceLevel level = kTraceAll);

  int32_t AddThreadId(char* trace_message) const;

  // OS specific implementations.
  virtual int32_t AddTime(char* trace_message,
                          const TraceLevel level) const = 0;

  virtual int32_t AddBuildInfo(char* trace_message) const = 0;
  virtual int32_t AddDateTimeInfo(char* trace_message) const = 0;

  static bool Run(void* obj);
  bool Process();

 private:
  friend class Trace;

  int32_t AddLevel(char* sz_message, const TraceLevel level) const;

  int32_t AddModuleAndId(char* trace_message, const TraceModule module,
                         const int32_t id) const;

  int32_t AddMessage(char* trace_message,
                     const char msg[VOIP_TRACE_MAX_MESSAGE_SIZE],
                     const uint16_t written_so_far) const;

  void AddMessageToList(
    const char trace_message[VOIP_TRACE_MAX_MESSAGE_SIZE],
    const uint16_t length,
    const TraceLevel level);

  bool UpdateFileName(
    const char file_name_utf8[FileWrapper::kMaxFileNameSize],
    char file_name_with_counter_utf8[FileWrapper::kMaxFileNameSize],
    const uint32_t new_count) const;

  bool CreateFileName(
    const char file_name_utf8[FileWrapper::kMaxFileNameSize],
    char file_name_with_counter_utf8[FileWrapper::kMaxFileNameSize],
    const uint32_t new_count) const;

  void WriteToFile();

  CriticalSectionWrapper* critsect_interface_;
  TraceCallback* callback_;
  uint32_t row_count_text_;
  uint32_t file_count_text_;

  FileWrapper& trace_file_;
  ThreadWrapper& thread_;
  EventWrapper& event_;

  // critsect_array_ protects active_queue_.
  CriticalSectionWrapper* critsect_array_;
  uint16_t next_free_idx_[VOIP_TRACE_NUM_ARRAY];
  TraceLevel level_[VOIP_TRACE_NUM_ARRAY][VOIP_TRACE_MAX_QUEUE];
  uint16_t length_[VOIP_TRACE_NUM_ARRAY][VOIP_TRACE_MAX_QUEUE];
  char* message_queue_[VOIP_TRACE_NUM_ARRAY][VOIP_TRACE_MAX_QUEUE];
  uint8_t active_queue_;
};

}  // namespace VoIP

#endif  // VOIP_SYSTEM_WRAPPERS_SOURCE_TRACE_IMPL_H_
