#pragma once
#include <pebble.h>
#include "data_models.h"

typedef void (*StationSelectCallback)(Station *station);

void station_select_window_push(StationSelectCallback callback);
void station_select_window_add_station(Station station);
void station_select_window_clear_stations(void);
