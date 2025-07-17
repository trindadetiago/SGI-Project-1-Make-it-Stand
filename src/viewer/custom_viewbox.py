import pyqtgraph as pg
from PyQt5.QtCore import Qt
import torch

class CustomViewBox(pg.ViewBox):
    def __init__(self, main_window, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.main_window = main_window
        self.setMouseMode(self.PanMode)
        self._dragging_vertex = False

    def mouseClickEvent(self, ev):
        # Let the main window handle selection
        super().mouseClickEvent(ev)

    def mouseDragEvent(self, ev, axis=None):
        if self.main_window.tabs.tabText(self.main_window.tabs.currentIndex()) == 'Edit' and \
           self.main_window.selected_vertex is not None:
            if ev.button() == Qt.LeftButton:
                pos = ev.pos()
                x, y = self.mapSceneToView(pos).x(), self.mapSceneToView(pos).y()
                shape = self.main_window.shape
                idx = self.main_window.selected_vertex
                if shape and 0 <= idx < len(shape.vertices):
                    shape.vertices[idx] = torch.tensor([x, y], dtype=shape.vertices.dtype)
                    self.main_window.update_plot()
                ev.accept()
                return
        # Otherwise, default panning/zooming
        super().mouseDragEvent(ev, axis=axis) 