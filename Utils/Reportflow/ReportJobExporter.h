#pragma once

#include <vector>

#include <QString>

#include "Interface/SimulationResult.h"

struct ReportJobExportResult {
    bool success{false};
    QString errorMessage;
    QString jobDirectory;
    QString assetsDirectory;
    QString outputsDirectory;
    QString logsDirectory;
    QString requestFilePath;
    QString simulationResultFilePath;
    QString reportContextFilePath;
    QString statusFilePath;
    std::vector<QString> assetFilePaths;
};

class ReportJobExporter {
public:
    static ReportJobExportResult exportBundle(
        const SimulationTaskResult& result,
        const QString& workRootDir);
};
