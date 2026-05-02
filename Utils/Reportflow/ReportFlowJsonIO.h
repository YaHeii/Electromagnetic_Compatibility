#pragma once

#include <QJsonObject>
#include <QString>

#include "Interface/ReportFlowContract.h"
#include "Interface/SimulationResult.h"

class ReportFlowJsonIO {
public:
    static QJsonObject serializeSimulationTaskResult(const SimulationTaskResult& result);
    static QJsonObject buildRequestObject(const ReportFlow::Request& request);
    static QJsonObject buildStatusObject(const ReportFlow::Status& status);
    static bool writeJsonFile(
        const QString& filePath,
        const QJsonObject& object,
        QString* errorMessage);
    static bool ensureDirectory(
        const QString& path,
        QString* errorMessage);
};
