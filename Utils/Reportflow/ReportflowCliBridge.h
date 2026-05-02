#pragma once

#include <QString>

#include "Interface/ReportFlowContract.h"
#include "Interface/SimulationResult.h"

struct ReportflowCliRunResult {
    bool success{false};
    int exitCode{1};
    QString errorMessage;
    QString outputDirectory;
    QString simulationResultFilePath;
    QString reportContextFilePath;
};

class ReportflowCliBridge {
public:
    static ReportflowCliRunResult exportSimulationOutputs(
        const SimulationTaskResult& result,
        const QString& outputDirectory,
        bool writeReportContext = true);
};
