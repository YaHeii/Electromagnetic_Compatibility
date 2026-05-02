#include "Utils/Reportflow/ReportflowCliBridge.h"

#include <QDir>

#include "Utils/Reportflow/ReportContextBuilder.h"
#include "Utils/Reportflow/ReportFlowJsonIO.h"

ReportflowCliRunResult ReportflowCliBridge::exportSimulationOutputs(
    const SimulationTaskResult& result,
    const QString& outputDirectory,
    bool writeReportContext) {
    ReportflowCliRunResult runResult;
    runResult.outputDirectory = outputDirectory;

    const auto validation = result.validate();
    if (!validation.first) {
        runResult.errorMessage = QStringLiteral("仿真结果校验失败：%1").arg(validation.second);
        return runResult;
    }

    if (!ReportFlowJsonIO::ensureDirectory(outputDirectory, &runResult.errorMessage)) {
        return runResult;
    }

    QDir dir(outputDirectory);
    runResult.simulationResultFilePath =
        dir.filePath(QString::fromLatin1(ReportFlow::kSimulationResultFileName));
    runResult.reportContextFilePath =
        dir.filePath(QString::fromLatin1(ReportFlow::kReportContextFileName));

    if (!ReportFlowJsonIO::writeJsonFile(
            runResult.simulationResultFilePath,
            ReportFlowJsonIO::serializeSimulationTaskResult(result),
            &runResult.errorMessage)) {
        return runResult;
    }

    if (writeReportContext) {
        if (!ReportFlowJsonIO::writeJsonFile(
                runResult.reportContextFilePath,
                ReportContextBuilder::build(result),
                &runResult.errorMessage)) {
            return runResult;
        }
    } else {
        runResult.reportContextFilePath.clear();
    }

    runResult.success = true;
    runResult.exitCode = 0;
    return runResult;
}
