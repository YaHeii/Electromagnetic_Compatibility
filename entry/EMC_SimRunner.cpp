#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>

#include <optional>

#include "Interface/DataModel.h"
#include "Simulation/simSchedulerCtx.h"
#include "Utils/JsonLoader.hpp"
#include "Utils/Reportflow/ReportflowCliBridge.h"
#include "spdlog/spdlog.h"

namespace {

void configureParser(QCommandLineParser& parser) {
    parser.setApplicationDescription(QStringLiteral("EMC headless simulation runner"));
    parser.addHelpOption();

    parser.addOption(QCommandLineOption(
        QStringList{QStringLiteral("input")},
        QStringLiteral("输入标准 schema JSONC 文件路径"),
        QStringLiteral("input")));
    parser.addOption(QCommandLineOption(
        QStringList{QStringLiteral("output-dir")},
        QStringLiteral("结果输出目录"),
        QStringLiteral("output-dir")));
}

}  // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("EMC_SimRunner"));

    QCommandLineParser parser;
    configureParser(parser);
    parser.process(app);

    const QString inputPath = parser.value(QStringLiteral("input"));
    const QString outputDir = parser.value(QStringLiteral("output-dir"));

    if (inputPath.trimmed().isEmpty() || outputDir.trimmed().isEmpty()) {
        spdlog::error("缺少必要参数：--input 和 --output-dir");
        return 2;
    }

    if (!QFileInfo::exists(inputPath)) {
        spdlog::error("输入文件不存在：{}", inputPath.toStdString());
        return 2;
    }

    if (!JsonLoader::LoadFile(inputPath)) {
        spdlog::error("输入文件加载失败：{}", inputPath.toStdString());
        return 1;
    }

    DataModel* model = DataModel::instance();
    const auto validationResult = model->validateCurrentModel();
    if (!validationResult.first) {
        spdlog::error("当前模型校验失败：{}", validationResult.second.toStdString());
        return 1;
    }

    const DataModel::DataSnapshot snapshot = model->createSnapshot();
    simSchedulerCtx scheduler(
        ModelType::PE,
        snapshot,
        FormationSource::ManualInput,
        std::nullopt);

    const SimulationTaskResult result = scheduler.run();
    const ReportflowCliRunResult exportResult =
        ReportflowCliBridge::exportSimulationOutputs(result, outputDir, true);
    if (!exportResult.success) {
        spdlog::error("结果导出失败：{}", exportResult.errorMessage.toStdString());
        return 1;
    }

    if (result.status != SimulationResultStatus::Succeeded) {
        spdlog::error("仿真未成功完成：{}", result.errorMessage.toStdString());
        return 1;
    }

    spdlog::info(
        "Headless simulation finished. Result: {}",
        exportResult.simulationResultFilePath.toStdString());
    return 0;
}
