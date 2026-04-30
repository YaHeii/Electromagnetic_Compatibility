"""Filesystem helpers for the Reportflow MVP."""

from __future__ import annotations

import json
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


def ensure_directory(path: str | Path) -> Path:
    directory = Path(path)
    directory.mkdir(parents=True, exist_ok=True)
    return directory


def read_json_file(path: str | Path) -> Any:
    file_path = Path(path)
    with file_path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def write_json_file(path: str | Path, data: Any) -> None:
    file_path = Path(path)
    ensure_directory(file_path.parent)
    with file_path.open("w", encoding="utf-8") as handle:
        json.dump(data, handle, ensure_ascii=False, indent=2, sort_keys=False)
        handle.write("\n")


def write_text_file(path: str | Path, content: str) -> None:
    file_path = Path(path)
    ensure_directory(file_path.parent)
    with file_path.open("w", encoding="utf-8", newline="\n") as handle:
        handle.write(content)


def load_template_text(template_name: str) -> str:
    template_path = Path(__file__).resolve().parents[1] / "templates" / template_name
    with template_path.open("r", encoding="utf-8") as handle:
        return handle.read()


def _now_utc_ms() -> int:
    return int(datetime.now(timezone.utc).timestamp() * 1000)


def write_status_file(
    job_dir: str | Path,
    *,
    state: str,
    stage: str,
    error_message: str | None = None,
) -> None:
    status_path = Path(job_dir) / "status.json"
    previous_status: dict[str, Any] = {}
    if status_path.exists():
        try:
            loaded_status = read_json_file(status_path)
            if isinstance(loaded_status, dict):
                previous_status = loaded_status
        except Exception:
            previous_status = {}

    payload: dict[str, Any] = {
        "taskId": previous_status.get("taskId", Path(job_dir).name),
        "state": state,
        "stage": stage,
        "updatedAtUtcMs": _now_utc_ms(),
        "errors": [],
    }
    if "startedAtUtcMs" in previous_status:
        payload["startedAtUtcMs"] = previous_status["startedAtUtcMs"]
    if state == "running":
        payload["startedAtUtcMs"] = previous_status.get("startedAtUtcMs", _now_utc_ms())
    if state in {"succeeded", "failed", "cancelled"}:
        payload["finishedAtUtcMs"] = _now_utc_ms()
    if error_message:
        payload["errorMessage"] = error_message
        payload["errors"] = [error_message]
    write_json_file(status_path, payload)


def append_log_line(job_dir: str | Path, message: str) -> None:
    log_path = Path(job_dir) / "logs" / "reportflow.log"
    ensure_directory(log_path.parent)
    timestamp = datetime.now(timezone.utc).isoformat()
    with log_path.open("a", encoding="utf-8", newline="\n") as handle:
        handle.write(f"[{timestamp}] {message}\n")
