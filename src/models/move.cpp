#include "../../include/models/move.h"

void moveModel::move_location(Fleet& ship_fleet, int t_step, PathManager total_path){
    for(auto& ship : ship_fleet.getShips()){
        Point2D position = ship->getLocation();
        Path path = total_path.Id_findPath(ship->getID(),total_path.getPathList());
        Point2D position_new = {position.x + ship->getSpeed() * cos(ship->getOrientationDeg() * M_PI / 180.0) * t_step,
                                position.y + ship->getSpeed() * sin(ship->getOrientationDeg() * M_PI / 180.0) * t_step};
        ship->setLocation(position_new);
    }
}
