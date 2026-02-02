#pragma once
#include <vector>
#include <memory>
#include "ship.h"

/// <summary>
/// @param _ships 舰队中的船只列表
/// @func addShip 添加舰船
/// @func getShips 返回船列表
/// @func findShipByID 返回船  ship*
/// </summary>
class Fleet {
public:
    //添加舰船
    void addShip(std::unique_ptr<ship> ship) { _ships.push_back(std::move(ship)); }
    //返回船列表
    const std::vector<std::unique_ptr<ship>>& getShips() const { return _ships; }
    //返回船
    ship* findShipByID(const std::string& ship_id) const {
        for (const auto& s : _ships) {
            if (s && s->getID() == ship_id) {
                return s.get();
            }
        }
        return nullptr;
    }

private:
    std::vector<std::unique_ptr<ship>> _ships;
};

