#pragma once

#include <optional>

#include <QtGlobal>
#include <QString>
#include <QStringList>

namespace ReportFlow {

inline constexpr char kBundleVersion[] = "1.0.0";
inline constexpr char kReportContextVersion[] = "1.0.0";

inline constexpr char kRequestFileName[] = "request.json";
inline constexpr char kSimulationResultFileName[] = "simulation-result.json";
inline constexpr char kReportContextFileName[] = "report-context.json";
inline constexpr char kStatusFileName[] = "status.json";
inline constexpr char kBaselineInputFileName[] = "baseline-input.jsonc";
inline constexpr char kComparisonSummaryFileName[] = "comparison-summary.json";
inline constexpr char kFinalReportMarkdownFileName[] = "final-report.md";
inline constexpr char kFinalReportHtmlFileName[] = "final-report.html";
inline constexpr char kExperimentPlanFileName[] = "plan.json";

inline constexpr char kAssetsDirName[] = "assets";
inline constexpr char kOutputsDirName[] = "outputs";
inline constexpr char kLogsDirName[] = "logs";
inline constexpr char kExperimentsDirName[] = "experiments";

inline constexpr char kAggregatedFieldAssetFile[] = "assets/aggregated-field.png";
inline constexpr char kReferenceEmitterAssetFile[] = "assets/reference-emitter.png";
inline constexpr char kScfAssetFile[] = "assets/scf-matrix.png";
inline constexpr char kS3iAssetFile[] = "assets/s3i-curve.png";
inline constexpr char kTElevAssetFile[] = "assets/t-elev.png";
inline constexpr char kDDesenseAssetFile[] = "assets/d-desense.png";

inline constexpr char kTemplateOnlyMode[] = "template-only";
inline constexpr char kAgentExperimentMode[] = "agent-experiment";
inline constexpr char kDefaultLanguage[] = "zh-CN";
inline constexpr char kDefaultTemplateId[] = "default-emc-report";
inline constexpr char kMarkdownFormat[] = "md";
inline constexpr char kHtmlFormat[] = "html";
inline constexpr char kGoalModeImprovement[] = "improvement";

inline constexpr char kChartIdAggregatedField[] = "aggregatedField";
inline constexpr char kChartIdReferenceEmitter[] = "referenceEmitter";
inline constexpr char kChartIdScf[] = "scf";
inline constexpr char kChartIdS3i[] = "s3i";
inline constexpr char kChartIdTElev[] = "tElev";
inline constexpr char kChartIdDDesense[] = "dDesense";

namespace Keys {

inline constexpr char ReportBundleVersion[] = "reportBundleVersion";
inline constexpr char TaskId[] = "taskId";
inline constexpr char Mode[] = "mode";
inline constexpr char Language[] = "language";
inline constexpr char TemplateId[] = "templateId";
inline constexpr char OutputFormats[] = "outputFormats";
inline constexpr char InputFiles[] = "inputFiles";
inline constexpr char AssetFiles[] = "assetFiles";
inline constexpr char Agent[] = "agent";

inline constexpr char BaselineInput[] = "baselineInput";
inline constexpr char SimulationResult[] = "simulationResult";
inline constexpr char ReportContext[] = "reportContext";
inline constexpr char AggregatedField[] = "aggregatedField";
inline constexpr char ReferenceEmitter[] = "referenceEmitter";
inline constexpr char Scf[] = "scf";
inline constexpr char S3i[] = "s3i";
inline constexpr char TElev[] = "tElev";
inline constexpr char DDesense[] = "dDesense";

inline constexpr char GoalMode[] = "goalMode";
inline constexpr char MaxExperimentCount[] = "maxExperimentCount";
inline constexpr char MutationScopes[] = "mutationScopes";
inline constexpr char RankingPolicy[] = "rankingPolicy";
inline constexpr char ProviderProfile[] = "providerProfile";

inline constexpr char State[] = "state";
inline constexpr char Stage[] = "stage";
inline constexpr char UpdatedAtUtcMs[] = "updatedAtUtcMs";
inline constexpr char StartedAtUtcMs[] = "startedAtUtcMs";
inline constexpr char FinishedAtUtcMs[] = "finishedAtUtcMs";
inline constexpr char ErrorMessage[] = "errorMessage";
inline constexpr char Errors[] = "errors";

}  // namespace Keys

enum class JobMode {
    TemplateOnly,
    AgentExperiment
};

enum class JobState {
    Pending,
    Running,
    Succeeded,
    Failed,
    Cancelled
};

enum class JobStage {
    ValidateBundle,
    DiagnoseBaseline,
    PlanExperiments,
    MaterializeInputs,
    RunExperiments,
    RankCandidates,
    RenderMarkdown,
    RenderHtml,
    Completed
};

struct RequestInputFiles {
    QString simulationResultFileName{QString::fromLatin1(kSimulationResultFileName)};
    QString reportContextFileName{QString::fromLatin1(kReportContextFileName)};
    QString baselineInputFileName{QString::fromLatin1(kBaselineInputFileName)};
};

struct RequestAssetFiles {
    QString aggregatedFieldFile{QString::fromLatin1(kAggregatedFieldAssetFile)};
    QString referenceEmitterFile{QString::fromLatin1(kReferenceEmitterAssetFile)};
    QString scfFile{QString::fromLatin1(kScfAssetFile)};
    QString s3iFile{QString::fromLatin1(kS3iAssetFile)};
    QString tElevFile{QString::fromLatin1(kTElevAssetFile)};
    QString dDesenseFile{QString::fromLatin1(kDDesenseAssetFile)};
};

struct AgentConfig {
    QString goalMode{QString::fromLatin1(kGoalModeImprovement)};
    int maxExperimentCount{5};
    QStringList mutationScopes;
    QString rankingPolicy;
    QString providerProfile;
};

struct Request {
    QString reportBundleVersion{QString::fromLatin1(kBundleVersion)};
    QString taskId;
    JobMode mode{JobMode::TemplateOnly};
    QString language{QString::fromLatin1(kDefaultLanguage)};
    QString templateId{QString::fromLatin1(kDefaultTemplateId)};
    QStringList outputFormats{QString::fromLatin1(kMarkdownFormat), QString::fromLatin1(kHtmlFormat)};
    RequestInputFiles inputFiles;
    std::optional<RequestAssetFiles> assetFiles{RequestAssetFiles{}};
    std::optional<AgentConfig> agent;
};

struct Status {
    QString taskId;
    JobState state{JobState::Pending};
    JobStage stage{JobStage::ValidateBundle};
    qint64 updatedAtUtcMs{0};
    std::optional<qint64> startedAtUtcMs;
    std::optional<qint64> finishedAtUtcMs;
    QString errorMessage;
    QStringList errors;
};

inline const char* toString(JobMode mode) {
    switch (mode) {
    case JobMode::TemplateOnly:
        return kTemplateOnlyMode;
    case JobMode::AgentExperiment:
        return kAgentExperimentMode;
    }
    return kTemplateOnlyMode;
}

inline const char* toString(JobState state) {
    switch (state) {
    case JobState::Pending:
        return "pending";
    case JobState::Running:
        return "running";
    case JobState::Succeeded:
        return "succeeded";
    case JobState::Failed:
        return "failed";
    case JobState::Cancelled:
        return "cancelled";
    }
    return "pending";
}

inline const char* toString(JobStage stage) {
    switch (stage) {
    case JobStage::ValidateBundle:
        return "validate_bundle";
    case JobStage::DiagnoseBaseline:
        return "diagnose_baseline";
    case JobStage::PlanExperiments:
        return "plan_experiments";
    case JobStage::MaterializeInputs:
        return "materialize_inputs";
    case JobStage::RunExperiments:
        return "run_experiments";
    case JobStage::RankCandidates:
        return "rank_candidates";
    case JobStage::RenderMarkdown:
        return "render_markdown";
    case JobStage::RenderHtml:
        return "render_html";
    case JobStage::Completed:
        return "completed";
    }
    return "validate_bundle";
}

}  // namespace ReportFlow
