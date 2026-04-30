"""Command line entry point for the Reportflow MVP."""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Sequence

from .loaders import load_bundle
from .renderers import render_html, render_markdown
from .utils.errors import ReportflowError
from .utils.fs import append_log_line, ensure_directory, write_status_file, write_text_file


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="\u62a5\u544a\u751f\u6210\u5668")
    parser.add_argument("--job-dir", required=True, help="\u4efb\u52a1\u76ee\u5f55\u8def\u5f84")
    return parser


def run_job(job_dir: str | Path) -> int:
    job_path = Path(job_dir).expanduser().resolve()
    if not job_path.exists() or not job_path.is_dir():
        return 1

    ensure_directory(job_path / "outputs")
    ensure_directory(job_path / "logs")

    current_stage = "validate_bundle"
    write_status_file(job_path, state="running", stage=current_stage)
    append_log_line(job_path, f"\u5f00\u59cb\u5904\u7406\u4efb\u52a1\uff1a{job_path}")

    try:
        bundle = load_bundle(job_path)
        append_log_line(job_path, "bundle \u8bfb\u53d6\u6210\u529f")

        current_stage = "render_markdown"
        write_status_file(job_path, state="running", stage=current_stage)
        markdown_text = render_markdown(bundle)
        write_text_file(job_path / "outputs" / "report.md", markdown_text)
        append_log_line(job_path, "report.md \u5df2\u751f\u6210")

        current_stage = "render_html"
        write_status_file(job_path, state="running", stage=current_stage)
        html_text = render_html(bundle)
        write_text_file(job_path / "outputs" / "report.html", html_text)
        append_log_line(job_path, "report.html \u5df2\u751f\u6210")

        current_stage = "completed"
        write_status_file(job_path, state="succeeded", stage=current_stage)
        append_log_line(job_path, "\u4efb\u52a1\u5904\u7406\u5b8c\u6210")
        return 0
    except ReportflowError as exc:
        append_log_line(job_path, f"\u4efb\u52a1\u5931\u8d25\uff1a{exc}")
        write_status_file(job_path, state="failed", stage=current_stage, error_message=str(exc))
        return 1
    except Exception as exc:  # pragma: no cover - defensive fallback
        append_log_line(job_path, f"\u672a\u9884\u671f\u5f02\u5e38\uff1a{exc}")
        write_status_file(job_path, state="failed", stage=current_stage, error_message=str(exc))
        return 1


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    return run_job(args.job_dir)


if __name__ == "__main__":  # pragma: no cover
    raise SystemExit(main())
