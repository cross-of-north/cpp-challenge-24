#include "common.h"

#include "utils.h"

bool DEBUG_OUTPUT = true;
bool DUMP_TO_STDOUT = true;
std::string filename;

CEventBuffers request_map;
CEventBuffers response_map;
CLineBuffers filling_line_buffers;
CLineBuffers ready_line_buffers;
CAggregatedStatsCollection stats;
