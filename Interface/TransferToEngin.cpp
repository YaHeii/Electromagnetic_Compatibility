#include "TransferToEngin.h"

#include <iostream>
#include <unordered_map>

#include "Interface/SchemaConstants.h"
#include <spdlog/spdlog.h>

AntennaType TransferToEngine::stringToAntennaType(const QString& typeStr) {
    if (typeStr == QString::fromLatin1(SchemaValues::Directional)) {
        return AntennaType::DIRECTIONAL;
    }
    if (typeStr == QString::fromLatin1(SchemaValues::Horn)) {
        return AntennaType::HORN;
    }
    if (typeStr == QString::fromLatin1(SchemaValues::ShapedBeam)) {
        return AntennaType::ShapedBeam;
    }
    if (typeStr == QString::fromLatin1(SchemaValues::Reflector)) {
        return AntennaType::Reflector;
    }
    return AntennaType::OMNI;
}

PolarizationMethod TransferToEngine::stringToPolarization(const QString& polStr) {
    if (polStr == QString::fromLatin1(SchemaValues::Horizontal)) {
        return PolarizationMethod::HORIZONTAL;
    }
    return PolarizationMethod::VERTICAL;
}

std::unique_ptr<Fleet> TransferToEngine::convertDataModelToFleet(const DataModel::DataSnapshot& dataSnapshot) {
    auto fleet = std::make_unique<Fleet>();

    std::unordered_map<std::string, const EquipmentData*> equipMap;
    for (const auto& eq : dataSnapshot.allEquipments) {
        equipMap[eq.equipmentId.toStdString()] = &eq;
    }

    for (const ShipData& shipData : dataSnapshot.allShips) {
        std::unique_ptr<ship> convertedShip = convertShipDataToShip(shipData, equipMap);
        if (convertedShip) {
            fleet->addShip(std::move(convertedShip));
        }
    }
    return fleet;
}

std::unique_ptr<ship> TransferToEngine::convertShipDataToShip(
    const ShipData& shipData,
    const std::unordered_map<std::string, const EquipmentData*>& equipMap) {
    Point3D location{shipData.worldX, shipData.worldY, shipData.worldZ};

    auto shipObj = std::make_unique<ship>(
        shipData.shipId,
        location,
        shipData.shipOrientationDeg,
        shipData.shipSpeedMps);

    for (const EquipmentOnShip& shipEq : shipData.equipmentRefs) {
        const std::string id = shipEq.equipmentId.toStdString();
        auto it = equipMap.find(id);

        if (it != equipMap.end()) {
            std::unique_ptr<Equipment> deviceObj = convertDeviceDataToEquipment(*(it->second));
            if (deviceObj) {
                shipObj->addEquipment(std::move(deviceObj));
            }
        } else {
            spdlog::debug("Ship {} has unknown equipment ID: {}", shipData.shipId, id);
        }
    }
    return shipObj;
}

std::unique_ptr<Equipment> TransferToEngine::convertDeviceDataToEquipment(const EquipmentData& d) {
    const std::string id = d.equipmentId.toStdString();
    Point3D pos{d.offsetX, d.offsetY, d.offsetZ};

    if (d.equipmentType == QString::fromLatin1(SchemaValues::Transmitter)) {
        TxParams params;
        params._centralF_Ghz = d.transmitterCenterFrequencyGHz;
        params._bandwidth_Mhz = d.transmitterBandwidthMHz;
        params._power_dbm = d.transmitterPowerDbm;
        params._antennaPhi = d.transmitterAntennaPhiDeg;
        params._beamWidth = d.transmitterBeamWidthDeg;
        params._polarization = stringToPolarization(d.transmitterPolarization);
        params._antennaType = stringToAntennaType(d.transmitterAntennaType);

        return std::make_unique<Transmitter>(id, params, pos);
    }

    if (d.equipmentType == QString::fromLatin1(SchemaValues::Receiver)) {
        RxParams params;
        params._centralF_Ghz = d.receiverCenterFrequencyGHz;
        params._bandwidth_Mhz = d.receiverBandwidthMHz;
        params._sensitivity_dbm = d.receiverSensitivityDbm;
        params._noise_figure_db = d.receiverNoiseFigureDb;
        params._SINR_threshold_db = d.receiverSinrMarginDb;
        params._interference_threshold_db = d.receiverInterferenceMarginDb;

        return std::make_unique<Receiver>(id, params, "", pos);
    }

    if (d.equipmentType == QString::fromLatin1(SchemaValues::Transceiver)) {
        TxParams txParams;
        txParams._centralF_Ghz = d.transmitterCenterFrequencyGHz;
        txParams._bandwidth_Mhz = d.transmitterBandwidthMHz;
        txParams._power_dbm = d.transmitterPowerDbm;
        txParams._antennaPhi = d.transmitterAntennaPhiDeg;
        txParams._beamWidth = d.transmitterBeamWidthDeg;
        txParams._polarization = stringToPolarization(d.transmitterPolarization);
        txParams._antennaType = stringToAntennaType(d.transmitterAntennaType);

        RxParams rxParams;
        rxParams._centralF_Ghz = d.receiverCenterFrequencyGHz;
        rxParams._bandwidth_Mhz = d.receiverBandwidthMHz;
        rxParams._sensitivity_dbm = d.receiverSensitivityDbm;
        rxParams._noise_figure_db = d.receiverNoiseFigureDb;
        rxParams._SINR_threshold_db = d.receiverSinrMarginDb;
        rxParams._interference_threshold_db = d.receiverInterferenceMarginDb;

        return std::make_unique<Transceiver>(id, txParams, rxParams, pos);
    }

    return nullptr;
}
