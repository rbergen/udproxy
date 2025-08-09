#pragma once
// This header is intentionally deprecated. Include the shared header instead:
//   #include "../socket/panel_packet.h"
#ifdef __has_include
#  if __has_include("../socket/panel_packet.h")
#    include "../socket/panel_packet.h"
#  else
#    error "panel_packet.h not found; expected at ../socket/panel_packet.h"
#  endif
#else
#  include "../socket/panel_packet.h"
#endif