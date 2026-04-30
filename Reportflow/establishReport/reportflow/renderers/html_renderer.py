"""Render HTML output for a report job."""

from __future__ import annotations

import html
import json
from string import Template

from ..contracts import ReportJobBundle, build_report_payload
from ..utils.fs import load_template_text


def _json_block(data: object) -> str:
    return json.dumps(data, ensure_ascii=False, indent=2, sort_keys=True)


def _render_list(items: list[dict[str, object]], empty_text: str) -> str:
    if not items:
        return f"<p>{html.escape(empty_text)}</p>"

    lines = ["<ul>"]
    for item in items:
        key = html.escape(str(item.get("key", "")))
        path = html.escape(str(item.get("relativePath", "")))
        state = "\u5df2\u627e\u5230" if item.get("exists") else "\u7f3a\u5931"
        lines.append(f"<li><code>{key}</code>: <code>{path}</code>（{html.escape(state)}）</li>")
    lines.append("</ul>")
    return "\n".join(lines)


def _render_metrics(items: list[dict[str, object]], empty_text: str) -> str:
    if not items:
        return f"<p>{html.escape(empty_text)}</p>"

    lines = ["<ul>"]
    for item in items:
        key = html.escape(str(item.get("key", "")))
        value = html.escape(str(item.get("value", "")))
        lines.append(f"<li><code>{key}</code>: <code>{value}</code></li>")
    lines.append("</ul>")
    return "\n".join(lines)


def _render_charts(items: list[dict[str, object]], empty_text: str) -> str:
    if not items:
        return f"<p>{html.escape(empty_text)}</p>"

    lines = ["<ul>"]
    for item in items:
        chart_id = html.escape(str(item.get("chartId", "")))
        title = html.escape(str(item.get("title", "")))
        subtitle = html.escape(str(item.get("subtitle", "")))
        detail_summary = html.escape(str(item.get("detailSummary", "")))
        asset_file = html.escape(str(item.get("assetFile", "")))
        state = "\u53ef\u7528" if item.get("available") else "\u4e0d\u53ef\u7528"
        lines.append(
            f"<li><code>{chart_id}</code> <strong>{title}</strong> | {subtitle} | {detail_summary} | <code>{asset_file}</code> | {html.escape(state)}</li>"
        )
    lines.append("</ul>")
    return "\n".join(lines)


def render_html(bundle: ReportJobBundle) -> str:
    payload = build_report_payload(bundle)
    template_text = load_template_text("report.html.j2")
    template = Template(template_text)
    return template.safe_substitute(
        {
            "title": html.escape(str(payload["title"])),
            "task_id": html.escape(str(payload["task_id"])),
            "mode": html.escape(str(payload["mode"])),
            "language": html.escape(str(payload["language"])),
            "template_id": html.escape(str(payload["template_id"])),
            "report_bundle_version": html.escape(str(payload["report_bundle_version"])),
            "report_context_version": html.escape(str(payload["report_context_version"])),
            "result_schema_version": html.escape(str(payload["result_schema_version"])),
            "summary_text": html.escape(str(payload["summary_text"])).replace("\n", "<br />"),
            "asset_section": _render_list(payload["asset_items"], "\u6682\u65e0\u8d44\u6e90\u6587\u4ef6\u3002"),
            "chart_section": _render_charts(payload["chart_items"], "\u6682\u65e0\u56fe\u8868\u6e05\u5355\u3002"),
            "metric_section": _render_metrics(payload["metric_items"], "\u6682\u65e0\u6d3e\u751f\u6307\u6807\u6458\u8981\u3002"),
            "request_json": html.escape(_json_block(payload["request_json"])),
            "simulation_result_json": html.escape(_json_block(payload["simulation_result_json"])),
            "report_context_json": html.escape(_json_block(payload["report_context_json"])),
        }
    )
