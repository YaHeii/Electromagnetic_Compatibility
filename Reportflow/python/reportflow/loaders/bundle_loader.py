"""Load and validate a report job bundle from disk."""

from __future__ import annotations

from pathlib import Path

from ..contracts import ReportJobBundle
from ..utils.errors import BundleValidationError
from ..utils.fs import read_json_file


def _require_file(path: Path, label: str) -> None:
    if not path.exists():
        raise BundleValidationError(f"\u7f3a\u5c11\u5fc5\u8981\u6587\u4ef6\uff1a{label}")
    if not path.is_file():
        raise BundleValidationError(f"\u5fc5\u8981\u6587\u4ef6\u4e0d\u662f\u666e\u901a\u6587\u4ef6\uff1a{label}")


def _require_object(value: object, label: str) -> dict:
    if not isinstance(value, dict):
        raise BundleValidationError(f"{label} \u5fc5\u987b\u662f JSON \u5bf9\u8c61")
    return value


def load_bundle(job_dir: str | Path) -> ReportJobBundle:
    job_path = Path(job_dir)
    if not job_path.exists():
        raise BundleValidationError("\u4efb\u52a1\u76ee\u5f55\u4e0d\u5b58\u5728")
    if not job_path.is_dir():
        raise BundleValidationError("\u4efb\u52a1\u8def\u5f84\u4e0d\u662f\u76ee\u5f55")

    request_path = job_path / "request.json"
    simulation_result_path = job_path / "simulation-result.json"
    report_context_path = job_path / "report-context.json"

    _require_file(request_path, "request.json")
    _require_file(simulation_result_path, "simulation-result.json")
    _require_file(report_context_path, "report-context.json")

    request = _require_object(read_json_file(request_path), "request.json")
    simulation_result = _require_object(read_json_file(simulation_result_path), "simulation-result.json")
    report_context = _require_object(read_json_file(report_context_path), "report-context.json")

    for key in ("reportBundleVersion", "taskId", "mode", "language", "templateId", "outputFormats", "inputFiles"):
        if key not in request:
            raise BundleValidationError(f"request.json \u7f3a\u5c11\u5b57\u6bb5\uff1a{key}")
    if "reportContextVersion" not in report_context:
        raise BundleValidationError("report-context.json \u7f3a\u5c11\u5b57\u6bb5\uff1areportContextVersion")
    if "resultSchemaVersion" not in simulation_result:
        raise BundleValidationError("simulation-result.json \u7f3a\u5c11\u5b57\u6bb5\uff1aresultSchemaVersion")

    output_formats = request.get("outputFormats")
    if not isinstance(output_formats, list) or not {"md", "html"}.issubset({str(item) for item in output_formats}):
        raise BundleValidationError("request.json.outputFormats \u5fc5\u987b\u540c\u65f6\u5305\u542b md \u548c html")

    input_files = request.get("inputFiles")
    if not isinstance(input_files, dict):
        raise BundleValidationError("request.json.inputFiles \u5fc5\u987b\u662f JSON \u5bf9\u8c61")

    asset_files = request.get("assetFiles")
    if isinstance(asset_files, dict):
        missing_assets = []
        for relative_path in asset_files.values():
            asset_path = job_path / str(relative_path)
            if not asset_path.exists():
                missing_assets.append(str(relative_path))
        if missing_assets:
            raise BundleValidationError(f"\u7f3a\u5c11\u8d44\u6e90\u6587\u4ef6\uff1a{', '.join(missing_assets)}")

    return ReportJobBundle(
        job_dir=job_path,
        request=request,
        simulation_result=simulation_result,
        report_context=report_context,
    )
