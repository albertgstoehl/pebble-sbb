#pragma once
#include <pebble.h>
#include "data_models.h"

typedef void (*QuickRouteCallback)(Station *departure, FavoriteDestination *destination);

void quick_route_window_push(Station *departure_station, QuickRouteCallback callback);
