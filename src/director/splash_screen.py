"""Splash screen helpers with import-time progress calibration."""

import sys
import time
from collections.abc import Callable
from pathlib import Path

BASELINE_MODULES = set(sys.modules.keys())

from qtpy import QtCore, QtGui, QtWidgets

DEFAULT_CALIBRATION_GROUP = "splash_import_calibration"


def _parse_splash_settings_name(value: str) -> tuple[str, str]:
    parts = value.split("/")
    if len(parts) != 2 or not all(parts):
        raise RuntimeError("splash_settings_name must be in the format 'Organization/Application'")
    return parts[0], parts[1]


def _application_instance(app_name: str) -> QtWidgets.QApplication:
    app = QtWidgets.QApplication.instance()
    if app is None:
        app = QtWidgets.QApplication(sys.argv[:1] + ["-name", app_name])
    return app


class SplashWidget(QtWidgets.QSplashScreen):
    """Internal widget that lays out splash UI elements."""

    def __init__(
        self,
        pixmap: QtGui.QPixmap,
        title: str,
        show_module_list: bool,
        image_pixmap: QtGui.QPixmap | None,
    ) -> None:
        super().__init__(pixmap)
        self.setWindowFlag(QtCore.Qt.FramelessWindowHint, True)
        self.setWindowFlag(QtCore.Qt.WindowStaysOnTopHint, True)

        self._title_label = QtWidgets.QLabel(title, self)
        self._title_label.setAlignment(QtCore.Qt.AlignHCenter | QtCore.Qt.AlignVCenter)
        self._title_label.setStyleSheet("color: white; font-size: 18px; font-weight: 600;")

        self._status_label = QtWidgets.QLabel("", self)
        self._status_label.setAlignment(QtCore.Qt.AlignHCenter | QtCore.Qt.AlignVCenter)
        self._status_label.setStyleSheet("color: white;")

        self._progress_bar = QtWidgets.QProgressBar(self)
        self._progress_bar.setTextVisible(False)
        self._progress_bar.setRange(0, 0)
        self._progress_bar.setFixedHeight(8)
        self._progress_bar.setStyleSheet(
            "QProgressBar {"
            "  border: 0;"
            "  background: rgba(255, 255, 255, 35);"
            "}"
            "QProgressBar::chunk {"
            "  background: rgba(255, 255, 255, 180);"
            "}"
        )

        self._image_pixmap = image_pixmap
        self._image_label = None
        if self._image_pixmap is not None:
            self._image_label = QtWidgets.QLabel(self)
            self._image_label.setAlignment(QtCore.Qt.AlignHCenter | QtCore.Qt.AlignVCenter)

        self._module_list = None
        if show_module_list:
            self._module_list = QtWidgets.QPlainTextEdit(self)
            self._module_list.setReadOnly(True)
            self._module_list.setFrameStyle(QtWidgets.QFrame.NoFrame)
            self._module_list.setLineWrapMode(QtWidgets.QPlainTextEdit.NoWrap)
            self._module_list.setFocusPolicy(QtCore.Qt.NoFocus)
            self._module_list.setStyleSheet("color: white;background: rgba(0, 0, 0, 120);font-size: 11px;")

        self._layout_widgets()

    def set_status(self, text: str) -> None:
        self._status_label.setText(text)

    def set_progress(self, percent: int | None) -> None:
        if percent is None:
            self._progress_bar.setRange(0, 0)
        else:
            self._progress_bar.setRange(0, 100)
            self._progress_bar.setValue(percent)

    def module_list_widget(self) -> QtWidgets.QPlainTextEdit | None:
        return self._module_list

    def resizeEvent(self, event: QtGui.QResizeEvent) -> None:
        super().resizeEvent(event)
        self._layout_widgets()

    def _layout_widgets(self) -> None:
        margin = 16
        bar_margin = 24
        bar_height = self._progress_bar.height()
        status_height = 18
        title_height = 28
        image_padding = 8

        width = self.width()
        height = self.height()

        self._title_label.setGeometry(
            margin,
            margin,
            width - 2 * margin,
            title_height,
        )

        self._progress_bar.setGeometry(
            bar_margin,
            height - bar_margin - bar_height,
            width - 2 * bar_margin,
            bar_height,
        )

        self._status_label.setGeometry(
            margin,
            self._progress_bar.y() - status_height - 8,
            width - 2 * margin,
            status_height,
        )

        if self._module_list is not None:
            if self._image_label is not None:
                self._image_label.hide()
            module_top = self._title_label.y() + title_height + 8
            module_bottom = self._status_label.y() - 8
            module_height = max(0, module_bottom - module_top)
            self._module_list.setGeometry(
                margin,
                module_top,
                width - 2 * margin,
                module_height,
            )
        elif self._image_label is not None and self._image_pixmap is not None:
            module_top = self._title_label.y() + title_height + image_padding
            module_bottom = self._status_label.y() - image_padding
            module_height = max(0, module_bottom - module_top)
            image_width = max(0, width - 2 * margin)
            image_height = max(0, module_height)

            if image_width > 0 and image_height > 0:
                scaled = self._image_pixmap.scaled(
                    image_width,
                    image_height,
                    QtCore.Qt.KeepAspectRatio,
                    QtCore.Qt.SmoothTransformation,
                )
                self._image_label.setPixmap(scaled)
                self._image_label.setGeometry(
                    margin,
                    module_top,
                    width - 2 * margin,
                    module_height,
                )
                self._image_label.show()
            else:
                self._image_label.hide()


class ModuleImportTracker:
    """Track new sys.modules entries and surface them to a text widget."""

    def __init__(
        self,
        text_widget: QtWidgets.QPlainTextEdit | None,
        baseline_modules: set[str],
    ) -> None:
        self._text_widget = text_widget
        self._seen = set(baseline_modules)
        self._imported_module_count = 0

    def imported_module_count(self) -> int:
        return self._imported_module_count

    def reset_progress_baseline(self) -> None:
        self._imported_module_count = 0

    def update_from_sys_modules(self) -> None:
        new_modules: list[str] = []
        for name in sys.modules.keys():
            if name not in self._seen:
                self._seen.add(name)
                self._imported_module_count += 1
                new_modules.append(name)

        if not new_modules:
            return

        if self._text_widget is None:
            return

        self._text_widget.appendPlainText("\n".join(new_modules))


class SplashCalibration:
    """Persist rolling import baselines for progress estimation.

    Each run records how long the import phase took and how many new modules were
    loaded (based on sys.modules growth since a baseline). Those measurements are
    saved via QSettings, then averaged to provide a stable expected duration and
    module count for future launches.
    """

    def __init__(self, settings: QtCore.QSettings, group: str) -> None:
        self._settings = settings
        self._group = group
        self._run_count = 0
        self._total_import_time_s = 0.0
        self._total_import_modules = 0.0
        self._load()

    def has_data(self) -> bool:
        return self._total_import_time_s > 0.0 and self._total_import_modules > 0.0

    def estimate_progress(self, elapsed_s: float, imported_module_count: int) -> int | None:
        if not self.has_data():
            return None

        time_fraction = min(elapsed_s / self._total_import_time_s, 1.0)
        module_fraction = min(imported_module_count / self._total_import_modules, 1.0)
        progress = max(time_fraction, module_fraction)
        return min(int(progress * 100), 99)

    def update(self, import_duration_s: float, imported_module_count: int) -> None:
        if import_duration_s <= 0.0 or imported_module_count <= 0:
            return

        new_run_count = self._run_count + 1
        self._total_import_time_s = (self._total_import_time_s * self._run_count + import_duration_s) / new_run_count
        self._total_import_modules = (
            self._total_import_modules * self._run_count + imported_module_count
        ) / new_run_count
        self._run_count = new_run_count
        self._save(import_duration_s, imported_module_count)

    def _load(self) -> None:
        self._settings.beginGroup(self._group)
        try:
            self._run_count = _read_int(self._settings, "run_count", 0)
            self._total_import_time_s = _read_float(self._settings, "total_import_time_s", 0.0)
            self._total_import_modules = _read_float(self._settings, "total_import_modules", 0.0)
        finally:
            self._settings.endGroup()

    def _save(self, import_duration_s: float, imported_module_count: int) -> None:
        self._settings.beginGroup(self._group)
        try:
            self._settings.setValue("run_count", self._run_count)
            self._settings.setValue("total_import_time_s", self._total_import_time_s)
            self._settings.setValue("total_import_modules", self._total_import_modules)
            self._settings.setValue("last_import_time_s", import_duration_s)
            self._settings.setValue("last_import_modules", imported_module_count)
        finally:
            self._settings.endGroup()
        self._settings.sync()


class ImportProgressController:
    """Compute calibrated progress and notify a progress callback.

    Uses the current elapsed time and the number of newly imported modules to
    estimate completion against the rolling baselines stored by SplashCalibration.
    """

    def __init__(
        self,
        calibration: SplashCalibration,
        tracker: ModuleImportTracker,
        progress_callback: Callable[[int], None],
        start_time: float,
    ) -> None:
        self._calibration = calibration
        self._tracker = tracker
        self._progress_callback = progress_callback
        self._start_time = start_time
        self._last_progress = None

    def update(self) -> None:
        progress = self._calibration.estimate_progress(
            elapsed_s=time.monotonic() - self._start_time,
            imported_module_count=self._tracker.imported_module_count(),
        )
        if progress is None or progress == self._last_progress:
            return

        self._progress_callback(progress)
        self._last_progress = progress


class ProcessEventsImportHook:
    """Import hook that pumps Qt events and updates progress during imports.

    It runs on every import resolution attempt, snapshots sys.modules growth via
    the tracker, then updates the progress controller and processes Qt events so
    the splash stays responsive while heavy imports are happening.
    """

    def __init__(
        self,
        app: QtWidgets.QApplication,
        tracker: ModuleImportTracker | None = None,
        progress_controller: ImportProgressController | None = None,
        min_interval: float = 0.05,
    ) -> None:
        self._app = app
        self._tracker = tracker
        self._progress_controller = progress_controller
        self._min_interval = min_interval
        self._last_process_time = 0.0
        self._processing = False

    def find_spec(self, fullname: str, path: list[str] | None, target=None):
        now = time.monotonic()
        if self._processing or now - self._last_process_time < self._min_interval:
            return None

        self._processing = True
        try:
            if self._tracker is not None:
                self._tracker.update_from_sys_modules()
            if self._progress_controller is not None:
                self._progress_controller.update()
            self._app.processEvents()
        finally:
            self._processing = False
            self._last_process_time = now

        return None


class SplashScreen:
    """Splash screen wrapper for import/construct/start phases.

    Example:
        splash = SplashScreen(
            title="Director",
            splash_settings_name="Director/DirectorSplashScreen",
            app_name="Director_MainWindowApp",
            image_path="path/to/logo.png",
        )
        splash.begin_imports()
        from director import mainwindowapp
        splash.begin_construction()
        fields = mainwindowapp.construct()
        splash.begin_start()
        fields.app.start()
    """

    def __init__(
        self,
        *,
        title: str,
        splash_settings_name: str,
        app_name: str = "Director_MainWindowApp",
        window_size: tuple[int, int] = (700, 420),
        background_color: QtGui.QColor | tuple[int, int, int] = (20, 20, 20),
        image_path: str | Path | None = None,
        show_module_list: bool = False,
        calibration_group: str = DEFAULT_CALIBRATION_GROUP,
        min_import_interval: float = 0.05,
        status_importing: str = "Loading modules...",
        status_constructing: str = "Constructing application...",
        status_starting: str = "Starting application...",
    ) -> None:
        self._app = _application_instance(app_name)
        self._status_importing = status_importing
        self._status_constructing = status_constructing
        self._status_starting = status_starting
        self._min_import_interval = min_import_interval
        self._import_start = None

        module_list_enabled = show_module_list
        settings_org, settings_app = _parse_splash_settings_name(splash_settings_name)

        pixmap = self._create_background_pixmap(
            window_size=window_size,
            background_color=background_color,
        )
        image_pixmap = None
        if image_path is not None and not module_list_enabled:
            image_pixmap = QtGui.QPixmap(str(image_path))
            if image_pixmap.isNull():
                image_pixmap = None

        self._splash = SplashWidget(
            pixmap,
            title,
            show_module_list=module_list_enabled,
            image_pixmap=image_pixmap,
        )

        self._module_tracker = ModuleImportTracker(
            text_widget=self._splash.module_list_widget(),
            baseline_modules=BASELINE_MODULES,
        )
        self._calibration = SplashCalibration(
            QtCore.QSettings(settings_org, settings_app),
            calibration_group,
        )
        self._progress_controller = None
        self._import_hook = None

    def begin_imports(self) -> None:
        self._splash.set_status(self._status_importing)
        self._splash.show()
        self._app.processEvents()

        self._module_tracker.update_from_sys_modules()
        self._module_tracker.reset_progress_baseline()

        if self._calibration.has_data():
            self._splash.set_progress(0)

        self._import_start = time.monotonic()
        if self._calibration.has_data():
            self._progress_controller = ImportProgressController(
                calibration=self._calibration,
                tracker=self._module_tracker,
                progress_callback=self._splash.set_progress,
                start_time=self._import_start,
            )

        self._import_hook = ProcessEventsImportHook(
            self._app,
            tracker=self._module_tracker,
            progress_controller=self._progress_controller,
            min_interval=self._min_import_interval,
        )
        sys.meta_path.insert(0, self._import_hook)

    def begin_construction(self) -> None:
        self._disable_import_hook()
        self._module_tracker.update_from_sys_modules()

        import_duration = None
        if self._import_start is not None:
            import_duration = time.monotonic() - self._import_start
            self._calibration.update(import_duration, self._module_tracker.imported_module_count())

        if self._calibration.has_data():
            self._splash.set_progress(100)

        self._splash.set_status(self._status_constructing)
        self._app.processEvents()

    def begin_start(self) -> None:
        self._splash.set_status(self._status_starting)
        self._app.processEvents()
        self._splash.hide()

    def _disable_import_hook(self) -> None:
        if self._import_hook is None:
            return
        if self._import_hook in sys.meta_path:
            sys.meta_path.remove(self._import_hook)
        self._import_hook = None

    def _create_background_pixmap(
        self,
        *,
        window_size: tuple[int, int],
        background_color: QtGui.QColor | tuple[int, int, int],
    ) -> QtGui.QPixmap:
        pixmap = QtGui.QPixmap(*window_size)
        if isinstance(background_color, QtGui.QColor):
            color = background_color
        else:
            color = QtGui.QColor(*background_color)
        pixmap.fill(color)
        return pixmap


def _read_float(settings: QtCore.QSettings, key: str, default: float) -> float:
    value = settings.value(key, default)
    try:
        return float(value)
    except (TypeError, ValueError):
        return default


def _read_int(settings: QtCore.QSettings, key: str, default: int) -> int:
    value = settings.value(key, default)
    try:
        return int(value)
    except (TypeError, ValueError):
        return default
