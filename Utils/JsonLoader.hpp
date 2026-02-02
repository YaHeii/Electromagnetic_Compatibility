#pragma once
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDebug>
#include <QStringList>
#include "Interface/DataModel.h"
#include "spdlog/spdlog.h" 

class JsonLoader {
public:
    static bool LoadFile(const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            spdlog::error("cannot open file: {}, {}", filePath.toStdString(), file.errorString().toStdString());
            return false;
        }

        QString jsonString = file.readAll();
        file.close();

        // 1. 清洗注释
        QRegularExpression re("//[^\n]*");
        jsonString.replace(re, "");

        // 2. 解析文档
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError) {
            spdlog::error("JSON 解析错误: {}", error.errorString().toStdString());
            return false;
        }

        if (!doc.isObject()) {
            spdlog::error("JSON 格式错误: 根节点必须是对象");
            return false;
        }

        DataModel* model = DataModel::instance();
        model->allShips.clear();
        model->allEquipments.clear();

        QJsonObject rootObj = doc.object();

        // 遍历所有船只 
        for (auto it = rootObj.begin(); it != rootObj.end(); ++it) {
            if (!it.value().isObject()) continue;

            QString shipKey = it.key(); // 例如 "USV1"
            QJsonObject shipObj = it.value().toObject();

            ShipData ship;
            ship.shipID = shipObj.contains("ID") ? shipObj["ID"].toString().toStdString() : shipKey.toStdString();

            // --- 解析位置 ---
            if (shipObj.contains("Location")) {
                QJsonObject locObj = shipObj["Location"].toObject();
                QJsonArray coords = locObj["coordinates"].toArray();
                if (coords.size() >= 3) {
                    ship.X_offset = coords[0].toDouble();
                    ship.Y_offset = coords[1].toDouble();
                    ship.Z_offset = coords[2].toDouble();
                }
            }

            ship.ship_Speed = parseDouble(shipObj["Speed"]);
            ship.ship_Orienteation = parseDouble(shipObj["Orientation"]);

            // --- 解析设备 (遍历船只对象内的所有 Key) ---
            for (auto devIt = shipObj.begin(); devIt != shipObj.end(); ++devIt) {

                if (!devIt.value().isObject()) continue;

                QJsonObject devObj = devIt.value().toObject();

                // 根据 type 字段决定解析方式
                QString type = devObj["type"].toString().toUpper();

                if (type == "TRANSMITTER") {
                    EquipmentData txEquip = parseTransmitter(devObj);
                    model->allEquipments.push_back(txEquip);
                    EquipmentOnShip eos;
                    eos.equipmentID = txEquip.equipmentID;
                    eos.isEnabled = true;
                    ship.Equipments.push_back(eos);
                }
                else if ( type == "RECEIVER") { 
                    EquipmentData rxEquip = parseReceiver(devObj);
                    model->allEquipments.push_back(rxEquip);
                    EquipmentOnShip eos;
                    eos.equipmentID = rxEquip.equipmentID;
                    eos.isEnabled = true;
                    ship.Equipments.push_back(eos);
                }
            }

            // 校验并保存
            auto validateRes = ship.validate_Ship();
            if (validateRes.first) {
                model->allShips.push_back(ship);
            }
            else {
                spdlog::warn("忽略无效船只( {} ): {}", ship.shipID, validateRes.second.toStdString());
            }
        }

        return true;
    }

private:
    // 辅助函数：兼容数字类型和字符串类型的数字解析
    static double parseDouble(const QJsonValue& val) {
        if (val.isString()) return val.toString().toDouble();
        if (val.isDouble()) return val.toDouble();
        return 0.0;
    }

    static EquipmentData parseTransmitter(const QJsonObject& json) {
        EquipmentData eq;
        eq.equipmentID = json["ID"].toString();
        eq.equipmentType = "发射机";
        eq.Gain = parseDouble(json["Gain"]);

        if (json.contains("Location_Offset")) {
            QJsonArray offset = json["Location_Offset"].toArray();
            if (offset.size() >= 3) {
                eq.X_offset = offset[0].toDouble();
                eq.Y_offset = offset[1].toDouble();
                eq.Z_offset = offset[2].toDouble();
            }
        }

        // --- 发射机特有字段 ---
        eq.CentralF_Transmitter = parseDouble(json["Central_F"]);
        eq.Bandwidth_Transmitter = parseDouble(json["Bandwith"]);
        eq.Power_Transmitter = parseDouble(json["Power"]);
        eq.antennaPhi_Transmitter = parseDouble(json["angle"]);
        eq.Beamwidth_Transmitter = parseDouble(json["BeamWidth"]);

        if (json.contains("PolarizationMethod"))
            eq.PolarizationMethod_Transmitter = json["PolarizationMethod"].toString();

        if (json.contains("AntennaType"))
            eq.antennaType_Transmitter = json["AntennaType"].toString();

        return eq;
    }

    // 辅助: 解析接收机
    static EquipmentData parseReceiver(const QJsonObject& json) {
        EquipmentData eq;
        eq.equipmentID = json["ID"].toString();
        eq.equipmentType = "接收机";

        // --- 通用字段 ---
        eq.Gain = parseDouble(json["Gain"]);

        if (json.contains("Location_Offset")) {
            QJsonArray offset = json["Location_Offset"].toArray();
            if (offset.size() >= 3) {
                eq.X_offset = offset[0].toDouble();
                eq.Y_offset = offset[1].toDouble();
                eq.Z_offset = offset[2].toDouble();
            }
        }

        // --- 接收机特有字段 ---
        eq.CentralF_Reciever = parseDouble(json["Central_F"]);
        eq.Bandwidth_Reciever = parseDouble(json["Bandwith"]);
        eq.Sensitive_reciever = parseDouble(json["Sensitivity"]);
        eq.interferenceMargin = parseDouble(json["interferenceMargin"]);
        eq.SINRMargin = parseDouble(json["SINRMargin"]);
        eq.noiseFigure = parseDouble(json["noiseFigure"]);

        return eq;
    }
};

