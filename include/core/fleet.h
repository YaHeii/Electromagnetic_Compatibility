#pragma once
#include <vector>
#include <memory>
#include "ship.h"
class Fleet {
public:
    //添加舰船
    void addShip(std::unique_ptr<ship> ship) { m_ships.push_back(std::move(ship)); }
    //返回船列表
    const std::vector<std::unique_ptr<ship>>& getShips() const { return m_ships; }
    //返回船
    ship* findShipByID(const std::string& ship_id) const {
        for (const auto& s : m_ships) {
            if (s && s->getID() == ship_id) {
                return s.get();
            }
        }
        return nullptr;
    }

private:
    std::vector<std::unique_ptr<ship>> m_ships;
};

