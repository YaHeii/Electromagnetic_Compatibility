#pragma once

//改变舰队内部每一个船的位置
#include <cmath>
#include <random>
#include "../models/EMC_Engine.h"
#include "../core/Equipment.h"
#include "PropagationModle.h"
#include "../core/Antenna.h"
#include "../core/fleet.h"
#include "../utils/conversions.h"
#include "Path.h"
#include "PathManager.h"


class moveModel{
public:
    static void move_location(Fleet& ship_fleet, int t_step, PathManager total_path);
};

