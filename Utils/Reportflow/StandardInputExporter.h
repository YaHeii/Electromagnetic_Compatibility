#pragma once

#include <QJsonObject>
#include <QString>

#include "Interface/DataModel.h"

class StandardInputExporter {
public:
    static bool buildJsonObject(
        const DataModel::DataSnapshot& snapshot,
        QJsonObject* object,
        QString* errorMessage);
    static bool buildJsoncText(
        const DataModel::DataSnapshot& snapshot,
        QString* jsoncText,
        QString* errorMessage);
    static bool writeJsoncFile(
        const QString& filePath,
        const DataModel::DataSnapshot& snapshot,
        QString* errorMessage);
};
