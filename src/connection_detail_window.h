#pragma once
#include <pebble.h>
#include "data_models.h"

void connection_detail_window_push(SavedConnection *connection);
void connection_detail_window_update_data(Connection *connections, int count);
