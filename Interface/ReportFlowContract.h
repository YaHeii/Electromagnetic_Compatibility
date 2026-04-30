#pragma once

namespace ReportFlow {

inline constexpr char kBundleVersion[] = "1.0.0";
inline constexpr char kReportContextVersion[] = "1.0.0";

inline constexpr char kRequestFileName[] = "request.json";
inline constexpr char kSimulationResultFileName[] = "simulation-result.json";
inline constexpr char kReportContextFileName[] = "report-context.json";
inline constexpr char kStatusFileName[] = "status.json";

inline constexpr char kAssetsDirName[] = "assets";
inline constexpr char kOutputsDirName[] = "outputs";
inline constexpr char kLogsDirName[] = "logs";

inline constexpr char kAggregatedFieldAssetFile[] = "assets/aggregated-field.png";
inline constexpr char kReferenceEmitterAssetFile[] = "assets/reference-emitter.png";
inline constexpr char kScfAssetFile[] = "assets/scf-matrix.png";
inline constexpr char kS3iAssetFile[] = "assets/s3i-curve.png";
inline constexpr char kTElevAssetFile[] = "assets/t-elev.png";
inline constexpr char kDDesenseAssetFile[] = "assets/d-desense.png";

inline constexpr char kTemplateOnlyMode[] = "template-only";
inline constexpr char kDefaultLanguage[] = "zh-CN";
inline constexpr char kDefaultTemplateId[] = "default-emc-report";
inline constexpr char kMarkdownFormat[] = "md";
inline constexpr char kHtmlFormat[] = "html";

inline constexpr char kChartIdAggregatedField[] = "aggregatedField";
inline constexpr char kChartIdReferenceEmitter[] = "referenceEmitter";
inline constexpr char kChartIdScf[] = "scf";
inline constexpr char kChartIdS3i[] = "s3i";
inline constexpr char kChartIdTElev[] = "tElev";
inline constexpr char kChartIdDDesense[] = "dDesense";

enum class JobState {
    Pending,
    Running,
    Succeeded,
    Failed,
    Cancelled
};

enum class JobStage {
    ValidateBundle,
    RenderMarkdown,
    RenderHtml,
    Completed
};

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
