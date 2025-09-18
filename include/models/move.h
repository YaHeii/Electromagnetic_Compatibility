#pragma once

//改变舰队内部每一个船的位置
#include <cmath>
#include <random>
#include "../core/EMC_Engine.h"
#include "../core/Equipment.h"
#include "PropagationModle.h"
#include "../core/Antenna.h"
#include "../core/fleet.h"
#include "../utils/conversions.h"
#include "Path.h"
#include "PathManager.h"
#define M_PI 3.14159265358979323846
namespace Electromagnetic_compatibility {
    namespace models {
        void move_location(core::Fleet& ship_fleet, int t_step, Electromagnetic_compatibility::models::PathManager total_path){
            for(auto& ship : ship_fleet.getShips()){
                utils::Point2D position = ship->getLocation();
                Electromagnetic_compatibility::models::Path path = total_path.Id_findPath(ship->getID(),total_path.getPathList());
                utils::Point2D position_new = {position.x + ship->getSpeed() * cos(ship->getOrientationDeg() * M_PI / 180.0) * t_step,
                                        position.y + ship->getSpeed() * sin(ship->getOrientationDeg() * M_PI / 180.0) * t_step};
                ship->setLocation(position_new);
            }
        }
    }
}
