"""Custom errors for the Reportflow MVP."""

from __future__ import annotations


class ReportflowError(Exception):
    """Base exception for Reportflow failures."""


class BundleValidationError(ReportflowError):
    """Raised when the bundle structure is invalid."""


class RenderError(ReportflowError):
    """Raised when rendering fails."""
