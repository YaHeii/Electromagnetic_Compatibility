"""Render markdown output for a report job."""

from __future__ import annotations

import json
from string import Template

from ..contracts import ReportJobBundle, build_report_payload
from ..utils.fs import load_template_text


def _json_block(data: object) -> str:
    return json.dumps(data, ensure_ascii=False, indent=2, sort_keys=True)


def _render_asset_section(payload: dict[str, object]) -> str:
    asset_items = payload.get("asset_items", [])
    if not asset_items:
        return "\u6682\u65e0\u8d44\u6e90\u6587\u4ef6\u3002"

    lines: list[str] = []
    for item in asset_items:
        key = str(item["key"])
        path = str(item["relativePath"])
        exists = "\u5df2\u627e\u5230" if item["exists"] else "\u7f3a\u5931"
        lines.append(f"- `{key}`: `{path}`\uff08{exists}\uff09")
    return "\n".join(lines)


def _render_metric_section(payload: dict[str, object]) -> str:
    metric_items = payload.get("metric_items", [])
    if not metric_items:
        return "\u6682\u65e0\u6d3e\u751f\u6307\u6807\u6458\u8981\u3002"

    lines: list[str] = []
    for item in metric_items:
        lines.append(f"- `{item['key']}`: `{item['value']}`")
    return "\n".join(lines)


def _render_chart_section(payload: dict[str, object]) -> str:
    chart_items = payload.get("chart_items", [])
    if not chart_items:
        return "\u6682\u65e0\u56fe\u8868\u6e05\u5355\u3002"

    lines: list[str] = []
    for item in chart_items:
        title = item["title"]
        subtitle = item["subtitle"]
        detail_summary = item["detailSummary"]
        asset_file = item["assetFile"]
        state = "\u53ef\u7528" if item["available"] else "\u4e0d\u53ef\u7528"
        lines.append(
            f"- `{item['chartId']}`: **{title}** | {subtitle} | {detail_summary} | `{asset_file}` | {state}"
        )
    return "\n".join(lines)


def render_markdown(bundle: ReportJobBundle) -> str:
    payload = build_report_payload(bundle)
    template_text = load_template_text("report.md.j2")
    template = Template(template_text)
    return template.safe_substitute(
        {
            "title": payload["title"],
            "task_id": payload["task_id"],
            "mode": payload["mode"],
            "language": payload["language"],
            "template_id": payload["template_id"],
            "report_bundle_version": payload["report_bundle_version"],
            "report_context_version": payload["report_context_version"],
            "result_schema_version": payload["result_schema_version"],
            "summary_text": payload["summary_text"],
            "asset_section": _render_asset_section(payload),
            "chart_section": _render_chart_section(payload),
            "metric_section": _render_metric_section(payload),
            "request_json": _json_block(payload["request_json"]),
            "simulation_result_json": _json_block(payload["simulation_result_json"]),
            "report_context_json": _json_block(payload["report_context_json"]),
        }
    )
