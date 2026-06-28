#pragma once

#include <QString>

namespace qdee_client_stats {

void start(int instance_id);
void stop();

void set_fps(unsigned int fps);
void set_encode_ms(unsigned int ms);
void set_rtt_ms(unsigned int ms);
void add_dropped(unsigned int count = 1);
void set_bitrate_kbps(unsigned int kbps);
void set_session_active(bool active);

QString stats_file_path(int instance_id);

}
