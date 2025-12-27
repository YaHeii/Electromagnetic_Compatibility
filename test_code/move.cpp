#include "../../include/models/move.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
void moveModel::move_location(Fleet& ship_fleet, int t_step, PathManager total_path){
    for(auto& ship : ship_fleet.getShips()){
        Point3D position = ship->getLocation();
        Path path = total_path.Id_findPath(ship->getID(),total_path.getPathList());
        Point3D position_new = {position._x + ship->getSpeed() * cos(ship->getOrientationDeg() * M_PI / 180.0) * t_step,
                                position._y + ship->getSpeed() * sin(ship->getOrientationDeg() * M_PI / 180.0) * t_step,
                                position._z + ship->getSpeed() * sin(ship->getOrientationDeg() * M_PI / 180.0) * t_step};
        ship->setLocation(position_new);
    }
}
