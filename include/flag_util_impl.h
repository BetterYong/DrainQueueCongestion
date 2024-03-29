#pragma once
#include "logging.h"
#define QUIC_RELOADABLE_FLAG_COUNT_IMPL(flag) \
  DLOG(INFO)<< "FLAG_" #flag ": " << FLAG_quic_reloadable_flag_##flag
#define QUIC_RELOADABLE_FLAG_COUNT_N_IMPL(flag, instance, total) \
  QUIC_RELOADABLE_FLAG_COUNT_IMPL(flag)

#define QUIC_RESTART_FLAG_COUNT_IMPL(flag) \
  DLOG(INFO) << "FLAG_" #flag ": " << FLAG_quic_restart_flag_##flag
#define QUIC_RESTART_FLAG_COUNT_N_IMPL(flag, instance, total) \
  QUIC_RESTART_FLAG_COUNT_IMPL(flag)


#define QUIC_RELOADABLE_FLAG_COUNT QUIC_RELOADABLE_FLAG_COUNT_IMPL
#define QUIC_RELOADABLE_FLAG_COUNT_N QUIC_RELOADABLE_FLAG_COUNT_N_IMPL

#define QUIC_RESTART_FLAG_COUNT QUIC_RESTART_FLAG_COUNT_IMPL
#define QUIC_RESTART_FLAG_COUNT_N QUIC_RESTART_FLAG_COUNT_N_IMPL
