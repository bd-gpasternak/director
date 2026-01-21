from typing import Any

from qtpy import QtCore, QtGui, QtWidgets


class DictTableWidget(QtWidgets.QWidget):
    """Two-column widget for presenting dictionary entries.

    Supports nested dict/list/tuple values by rendering them as child rows under the parent row.
    """

    def __init__(self, data: dict[str, Any] | None = None, parent: QtWidgets.QWidget | None = None) -> None:
        super().__init__(parent)
        self._tree = QtWidgets.QTreeWidget(self)
        self._tree.setColumnCount(2)
        self._tree.setHeaderLabels(["Key", "Value"])
        self._tree.setAlternatingRowColors(True)
        self._tree.setSelectionMode(QtWidgets.QAbstractItemView.NoSelection)
        self._tree.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)
        self._tree.setVerticalScrollMode(QtWidgets.QAbstractItemView.ScrollPerPixel)
        self._tree.setTextElideMode(QtCore.Qt.ElideNone)
        self._tree.setSortingEnabled(False)

        palette = self._tree.palette()
        palette.setColor(QtGui.QPalette.Base, QtGui.QColor("#ffffff"))
        palette.setColor(QtGui.QPalette.AlternateBase, QtGui.QColor("#f5f5f5"))
        self._tree.setPalette(palette)

        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self._tree)

        self.set_data(data or {})

    def set_data(self, data: dict[str, Any]) -> None:
        """Populate the widget with dictionary entries."""
        self._tree.clear()

        root = self._tree.invisibleRootItem()
        for key, value in data.items():
            self._add_value_item(parent=root, key=str(key), value=value)

        self._tree.expandToDepth(3)
        self._tree.resizeColumnToContents(0)
        self._tree.header().setStretchLastSection(True)

    def _add_value_item(self, parent: QtWidgets.QTreeWidgetItem, *, key: str, value: Any) -> None:
        item = QtWidgets.QTreeWidgetItem([key, ""])
        parent.addChild(item)

        key_font = item.font(0)
        key_font.setBold(True)
        item.setFont(0, key_font)

        if isinstance(value, dict):
            item.setText(1, "")
            for child_key, child_value in value.items():
                self._add_value_item(parent=item, key=str(child_key), value=child_value)
            return

        if isinstance(value, (list, tuple)):
            item.setText(1, f"{type(value).__name__} (len={len(value)})")
            for idx, child_value in enumerate(value):
                self._add_value_item(parent=item, key=str(idx), value=child_value)
            return

        item.setText(1, str(value))
