import sys
import os
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QTabWidget
from PyQt5.QtGui import QIcon
import pyqtgraph as pg
from PyQt5.QtCore import Qt
from shape2d import Shape2D
from .tab_visualize import VisualizeTab
from .tab_process import ProcessTab
from .tab_new import NewTab
from .tab_edit import EditTab
from .tab_compare import CompareTab
from PyQt5.QtGui import QPolygonF, QBrush, QColor
from PyQt5.QtCore import QPointF
from shape_processing import scale_shape
from shape_mass_center import calculate_center_of_mass
import torch
from .tab_optimization import OptimizationTab

class DataPolygonItem(pg.GraphicsObject):
    def __init__(self, vertices, viewbox, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.vertices = vertices
        self.viewbox = viewbox
        self.brush = QBrush(QColor(100, 100, 255, 100))
        self.setZValue(-10)  # Draw behind points/edges
        self._polygon = QPolygonF()
        self.update_polygon()
        self.viewbox.sigResized.connect(self.update_polygon)
        self.viewbox.sigRangeChanged.connect(self.update_polygon)

    def update_polygon(self, *args):
        if not self.vertices:
            return
        pts = [self.viewbox.mapViewToScene(QPointF(x, y)) for x, y in self.vertices]
        self._polygon = QPolygonF(pts)
        self.update()

    def paint(self, p, *args):
        if not self._polygon.isEmpty():
            p.setBrush(self.brush)
            p.setPen(QColor(0,0,0,0))
            p.drawPolygon(self._polygon)

    def boundingRect(self):
        return self._polygon.boundingRect()

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

class ShapeGUI(QWidget):
    SHAPES_DIR = os.path.join(os.path.dirname(__file__), '..', '..', 'shapes')

    def __init__(self):
        super().__init__()
        self.setWindowTitle('2D Shape Framework')
        self.shape = None
        self.drawing_shape = None  # Used in draw mode
        self.draw_mode = False
        self.add_edge_mode = False
        self.selected_vertices = []  # For edge creation
        self.selected_vertex = None  # For edit mode
        self._last_tab_index = 0
        self.current_shape_path = None  # Track the current file path
        self.init_ui()

    def init_ui(self):
        main_layout = QVBoxLayout()
        self.tabs = QTabWidget()
        self.visualize_tab = VisualizeTab(self)
        self.process_tab = ProcessTab(self)
        self.new_tab = NewTab(self)
        self.edit_tab = EditTab(self)
        self.compare_tab = CompareTab(self)
        self.optimization_tab = OptimizationTab(self)
        self.tabs.addTab(self.visualize_tab, 'Visualize')
        self.tabs.addTab(self.process_tab, 'Process')
        self.tabs.addTab(self.new_tab, 'New')
        self.tabs.addTab(self.edit_tab, 'Edit')
        self.tabs.addTab(self.compare_tab, 'Compare')
        self.tabs.addTab(self.optimization_tab, 'Optimization')
        self.tabs.currentChanged.connect(self.on_tab_changed)
        main_layout.addWidget(self.tabs)
        # --- Plot ---
        self.plot_widget = pg.PlotWidget(viewBox=CustomViewBox(self))
        self.plot_widget.setBackground('w')
        self.plot_widget.setAspectLocked(True)  # Lock aspect ratio
        self.plot_widget.scene().sigMouseClicked.connect(self.on_plot_click)
        # Removed sigMouseMoved connection
        main_layout.addWidget(self.plot_widget)
        self.setLayout(main_layout)
        self._dragging_vertex = False

    def showEvent(self, event):
        self.visualize_tab.refresh_shape_dropdown()
        self.compare_tab.refresh_shape_dropdowns()
        # Automatically select and load the first shape if available
        if self.visualize_tab.shape_dropdown.count() > 1:
            self.visualize_tab.shape_dropdown.setCurrentIndex(1)
            self.visualize_tab.on_dropdown_change(1)
        super().showEvent(event)

    def load_shape_from_path(self, path):
        self.shape = Shape2D.load_from_json(path)
        self.current_shape_path = path
        self.drawing_shape = None
        self.selected_vertices = []
        self.draw_mode = False
        self.add_edge_mode = False
        self.new_tab.draw_mode_btn.setChecked(False)
        self.new_tab.add_edge_mode_btn.setChecked(False)
        self.update_plot()
        self.edit_tab.update_save_button()

    def load_shape_dialog(self):
        from PyQt5.QtWidgets import QFileDialog
        path, _ = QFileDialog.getOpenFileName(self, 'Open Shape JSON', '', 'JSON Files (*.json)')
        if path:
            self.load_shape_from_path(path)
            self.visualize_tab.refresh_shape_dropdown()

    def save_shape_dialog(self):
        from PyQt5.QtWidgets import QFileDialog
        in_new_tab = self.tabs.tabText(self.tabs.currentIndex()) == 'New'
        shape_to_save = self.drawing_shape if in_new_tab else self.shape
        if shape_to_save is None or len(shape_to_save.vertices) == 0:
            return
        path, _ = QFileDialog.getSaveFileName(self, 'Save Shape JSON', self.SHAPES_DIR, 'JSON Files (*.json)')
        if path:
            shape_to_save.save_to_json(path)
            self.current_shape_path = path
            self.visualize_tab.refresh_shape_dropdown()
            if self.visualize_tab.shape_dropdown.count() > 1 and self.visualize_tab.shape_dropdown.currentIndex() == 0:
                self.visualize_tab.shape_dropdown.setCurrentIndex(1)
                self.visualize_tab.on_dropdown_change(1)
            self.edit_tab.update_save_button()

    def save_current_shape(self):
        # Overwrite the currently loaded file
        if self.current_shape_path and self.shape and len(self.shape.vertices) > 0:
            self.shape.save_to_json(self.current_shape_path)
            self.visualize_tab.refresh_shape_dropdown()

    def toggle_draw_mode(self):
        # Only toggle the mode, do not overwrite or clear drawing_shape
        self.draw_mode = not self.draw_mode
        if self.draw_mode:
            self.add_edge_mode = False
            self.new_tab.add_edge_mode_btn.setChecked(False)
            if self.drawing_shape is None:
                self.drawing_shape = Shape2D()
            self.selected_vertices = []
        else:
            self.selected_vertices = []
        self.update_plot()
        self.new_tab.draw_mode_btn.setChecked(self.draw_mode)

    def toggle_add_edge_mode(self):
        # Only toggle the mode, do not overwrite or clear drawing_shape
        self.add_edge_mode = not self.add_edge_mode
        if self.add_edge_mode:
            self.draw_mode = False
            self.new_tab.draw_mode_btn.setChecked(False)
            if self.drawing_shape is None:
                self.drawing_shape = Shape2D()
            self.selected_vertices = []
        else:
            self.selected_vertices = []
        self.update_plot()
        self.new_tab.add_edge_mode_btn.setChecked(self.add_edge_mode)

    def on_tab_changed(self, idx):
        # If switching to New tab, clear canvas and set default view
        if self.tabs.tabText(idx) == 'New':
            self.drawing_shape = Shape2D()
            self.shape = None
            self.selected_vertices = []
            self.new_tab.clear_canvas_and_reset_view()
        elif self.tabs.tabText(idx) == 'Compare':
            self.compare_tab.refresh_shape_dropdowns()
        else:
            # When leaving New tab, optionally set self.shape = self.drawing_shape if you want to keep the new shape
            if self.drawing_shape and len(self.drawing_shape.vertices) > 0:
                self.shape = self.drawing_shape
            self.drawing_shape = None
        if self.tabs.tabText(idx) == 'Edit':
            self.selected_vertex = None
            self.edit_tab.set_selected_vertex(None)
        self._last_tab_index = idx

    def on_plot_click(self, event):
        pos = event.scenePos()
        vb = self.plot_widget.getViewBox()
        mouse_point = vb.mapSceneToView(pos)
        x, y = mouse_point.x(), mouse_point.y()
        in_new_tab = self.tabs.tabText(self.tabs.currentIndex()) == 'New'
        in_edit_tab = self.tabs.tabText(self.tabs.currentIndex()) == 'Edit'
        shape = self.drawing_shape if in_new_tab else self.shape
        if self.draw_mode and in_new_tab:
            if self.drawing_shape is None:
                self.drawing_shape = Shape2D()
            new_vertex = torch.tensor([[x, y]], dtype=self.drawing_shape.vertices.dtype)
            self.drawing_shape.vertices = torch.cat([self.drawing_shape.vertices, new_vertex], dim=0)
            self.update_plot()
        elif self.add_edge_mode and in_new_tab and shape is not None:
            if len(shape.vertices) == 0:
                return
            min_dist = float('inf')
            min_idx = -1
            for i, (vx, vy) in enumerate(shape.vertices):
                dist = (vx - x) ** 2 + (vy - y) ** 2
                if dist < min_dist:
                    min_dist = dist
                    min_idx = i
            if min_dist < 0.25:
                if min_idx not in self.selected_vertices:
                    self.selected_vertices.append(min_idx)
                if len(self.selected_vertices) == 2:
                    v0, v1 = self.selected_vertices
                    if v0 != v1 and (v0, v1) not in shape.edges and (v1, v0) not in shape.edges:
                        shape.edges.append((v0, v1))
                    self.selected_vertices = []
                self.update_plot()
        elif in_edit_tab and shape is not None:
            # Select a vertex for dragging
            min_dist = float('inf')
            min_idx = -1
            for i, (vx, vy) in enumerate(shape.vertices):
                dist = (vx - x) ** 2 + (vy - y) ** 2
                if dist < min_dist:
                    min_dist = dist
                    min_idx = i
            if min_dist < 0.25:
                self.selected_vertex = min_idx
                self.edit_tab.set_selected_vertex(min_idx)
                self._dragging_vertex = True
            else:
                self.selected_vertex = None
                self.edit_tab.set_selected_vertex(None)
            self.update_plot()
            self._dragging_vertex = False  # End dragging on click
        elif not in_new_tab:
            if self.draw_mode:
                new_vertex = torch.tensor([[x, y]], dtype=self.drawing_shape.vertices.dtype)
                self.drawing_shape.vertices = torch.cat([self.drawing_shape.vertices, new_vertex], dim=0)
                self.update_plot()
            elif self.add_edge_mode and self.shape is not None:
                if len(self.shape.vertices) == 0:
                    return
                min_dist = float('inf')
                min_idx = -1
                for i, (vx, vy) in enumerate(self.shape.vertices):
                    dist = (vx - x) ** 2 + (vy - y) ** 2
                    if dist < min_dist:
                        min_dist = dist
                        min_idx = i
                if min_dist < 0.1:
                    if min_idx not in self.selected_vertices:
                        self.selected_vertices.append(min_idx)
                    if len(self.selected_vertices) == 2:
                        v0, v1 = self.selected_vertices
                        if v0 != v1 and (v0, v1) not in self.shape.edges and (v1, v0) not in self.shape.edges:
                            self.shape.edges.append((v0, v1))
                        self.selected_vertices = []
                    self.update_plot()

    def update_plot(self):
        self.plot_widget.clear()
        show_grid = False
        show_com = True
        if hasattr(self, 'visualize_tab'):
            show_grid = self.visualize_tab.grid_checkbox.isChecked()
            show_com = self.visualize_tab.com_checkbox.isChecked()
        self.plot_widget.showGrid(x=show_grid, y=show_grid, alpha=0.3 if show_grid else 0)
        if self.tabs.tabText(self.tabs.currentIndex()) == 'New':
            shape = self.drawing_shape
        else:
            shape = self.shape
        if shape is None or len(shape.vertices) == 0:
            self.new_tab.update_info()
            return
        fill_shape = False
        if hasattr(self, 'visualize_tab'):
            fill_shape = self.visualize_tab.fill_checkbox.isChecked()
        if fill_shape and len(shape.vertices) >= 3:
            closed = all((i, (i+1)%len(shape.vertices)) in shape.edges or ((i+1)%len(shape.vertices), i) in shape.edges for i in range(len(shape.vertices)))
            if closed:
                import numpy as np
                pts = np.array(shape.vertices)
                if not np.allclose(pts[0], pts[-1]):
                    pts = np.vstack([pts, pts[0]])
                x = pts[:, 0]
                y = pts[:, 1]
                self.plot_widget.plot(x, y, pen=pg.mkPen('blue', width=2), fillLevel=0, fillBrush=pg.mkBrush('lightblue', alpha=80))
        for edge in shape.edges:
            v0 = shape.vertices[edge[0]]
            v1 = shape.vertices[edge[1]]
            self.plot_widget.plot([v0[0], v1[0]], [v0[1], v1[1]], pen=pg.mkPen('b', width=2))
        xs, ys = zip(*shape.vertices)
        self.plot_widget.plot(xs, ys, pen=None, symbol='o', symbolBrush='r', symbolSize=12)
        show_labels = False
        if hasattr(self, 'visualize_tab'):
            show_labels = self.visualize_tab.label_checkbox.isChecked()
        if show_labels:
            for i, (x, y) in enumerate(shape.vertices):
                label = f"v{i+1}"
                text = pg.TextItem(label, anchor=(0.5, 1.5), color='k')
                text.setPos(x, y)
                self.plot_widget.addItem(text)
        if show_com:
            area, center_of_mass = calculate_center_of_mass(shape)
            if center_of_mass is not None:
                cx, cy = center_of_mass.tolist()
                cx = float(cx)
                cy = float(cy)
                self.plot_widget.plot([cx], [cy], pen=None, symbol='+', symbolBrush='red', symbolPen='red', symbolSize=20)
                text = pg.TextItem(text=f'CoM: ({cx:.2f}, {cy:.2f})', color='red', anchor=(0.5, 1.5))
                text.setPos(cx, cy)
                self.plot_widget.addItem(text)
        if self.add_edge_mode and self.selected_vertices:
            sel_xs = [shape.vertices[i][0] for i in self.selected_vertices]
            sel_ys = [shape.vertices[i][1] for i in self.selected_vertices]
            self.plot_widget.plot(sel_xs, sel_ys, pen=None, symbol='o', symbolBrush='g', symbolSize=16)
        if self.tabs.tabText(self.tabs.currentIndex()) == 'Edit' and self.selected_vertex is not None:
            vx, vy = shape.vertices[self.selected_vertex]
            self.plot_widget.plot([vx], [vy], pen=None, symbol='o', symbolBrush='g', symbolSize=18)
        self.new_tab.update_info() 