"""Core contracts used by the Reportflow MVP."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any


@dataclass(slots=True)
class ReportJobBundle:
    """In-memory view of a report job bundle."""

    job_dir: Path
    request: dict[str, Any]
    simulation_result: dict[str, Any]
    report_context: dict[str, Any]

    @property
    def task_id(self) -> str:
        value = self.request.get("taskId")
        return str(value) if value is not None else ""

    @property
    def title(self) -> str:
        value = self.report_context.get("title")
        if value:
            return str(value)
        if self.task_id:
            return f"\u4efb\u52a1 {self.task_id}"
        return "Reportflow \u62a5\u544a"


def _stringify(value: Any, fallback: str = "") -> str:
    if value is None:
        return fallback
    text = str(value).strip()
    return text if text else fallback


def build_report_payload(bundle: ReportJobBundle) -> dict[str, Any]:
    request = bundle.request
    context = bundle.report_context
    result = bundle.simulation_result

    asset_files = request.get("assetFiles")
    asset_items: list[dict[str, Any]] = []
    if isinstance(asset_files, dict):
        for key, relative_path in asset_files.items():
            asset_path = bundle.job_dir / str(relative_path)
            asset_items.append(
                {
                    "key": str(key),
                    "label": str(key),
                    "relativePath": str(relative_path),
                    "exists": asset_path.exists(),
                }
            )

    chart_items: list[dict[str, Any]] = []
    charts = context.get("charts")
    if isinstance(charts, list):
        for item in charts:
            if not isinstance(item, dict):
                continue
            chart_items.append(
                {
                    "chartId": _stringify(item.get("chartId")),
                    "title": _stringify(item.get("title"), "\u672a\u547d\u540d\u56fe\u8868"),
                    "subtitle": _stringify(item.get("subtitle")),
                    "detailSummary": _stringify(item.get("detailSummary")),
                    "assetFile": _stringify(item.get("assetFile")),
                    "available": bool(item.get("available", False)),
                }
            )

    metrics_summary = context.get("metricsSummary")
    if not isinstance(metrics_summary, dict):
        metrics_summary = {}

    metric_items: list[dict[str, Any]] = []
    for key in ("scf", "s3i", "tElev", "dDesense"):
        if key in metrics_summary and key != "available":
            metric_items.append({"key": key, "value": metrics_summary[key]})

    summary_text = _stringify(context.get("summaryText"), "\u6682\u65e0\u6458\u8981\u3002")
    if summary_text == "\u6682\u65e0\u6458\u8981\u3002":
        summary_text = _stringify(result.get("summaryText"), summary_text)

    return {
        "title": _stringify(context.get("title"), bundle.title),
        "task_id": bundle.task_id,
        "mode": _stringify(request.get("mode"), "template-only"),
        "language": _stringify(request.get("language"), "zh-CN"),
        "template_id": _stringify(request.get("templateId"), "default"),
        "report_bundle_version": _stringify(request.get("reportBundleVersion"), "1.0.0"),
        "report_context_version": _stringify(context.get("reportContextVersion"), "1.0.0"),
        "result_schema_version": _stringify(result.get("resultSchemaVersion"), "1.0.0"),
        "summary_text": summary_text,
        "request_json": request,
        "simulation_result_json": result,
        "report_context_json": context,
        "asset_items": asset_items,
        "chart_items": chart_items,
        "metric_items": metric_items,
    }
