import sys
import os
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QTabWidget
from PyQt5.QtGui import QIcon
import pyqtgraph as pg
from shape2d import Shape2D
from .tab_visualize import VisualizeTab
from .tab_process import ProcessTab
from .tab_new import NewTab
from PyQt5.QtGui import QPolygonF, QBrush, QColor
from PyQt5.QtCore import QPointF

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
        self._last_tab_index = 0
        self.init_ui()

    def init_ui(self):
        main_layout = QVBoxLayout()
        self.tabs = QTabWidget()
        self.visualize_tab = VisualizeTab(self)
        self.process_tab = ProcessTab(self)
        self.new_tab = NewTab(self)
        self.tabs.addTab(self.visualize_tab, 'Visualize')
        self.tabs.addTab(self.process_tab, 'Process')
        self.tabs.addTab(self.new_tab, 'New')
        self.tabs.currentChanged.connect(self.on_tab_changed)
        main_layout.addWidget(self.tabs)
        # --- Plot ---
        self.plot_widget = pg.PlotWidget()
        self.plot_widget.setBackground('w')
        self.plot_widget.setAspectLocked(True)  # Lock aspect ratio
        self.plot_widget.scene().sigMouseClicked.connect(self.on_plot_click)
        main_layout.addWidget(self.plot_widget)
        self.setLayout(main_layout)

    def showEvent(self, event):
        self.visualize_tab.refresh_shape_dropdown()
        # Automatically select and load the first shape if available
        if self.visualize_tab.shape_dropdown.count() > 1:
            self.visualize_tab.shape_dropdown.setCurrentIndex(1)
            self.visualize_tab.on_dropdown_change(1)
        super().showEvent(event)

    def load_shape_from_path(self, path):
        self.shape = Shape2D.load_from_json(path)
        self.drawing_shape = None
        self.selected_vertices = []
        self.draw_mode = False
        self.add_edge_mode = False
        self.new_tab.draw_mode_btn.setChecked(False)
        self.new_tab.add_edge_mode_btn.setChecked(False)
        self.update_plot()

    def load_shape_dialog(self):
        from PyQt5.QtWidgets import QFileDialog
        path, _ = QFileDialog.getOpenFileName(self, 'Open Shape JSON', '', 'JSON Files (*.json)')
        if path:
            self.load_shape_from_path(path)
            self.visualize_tab.refresh_shape_dropdown()

    def save_shape_dialog(self):
        from PyQt5.QtWidgets import QFileDialog
        # Save drawing_shape if in New tab, else save shape
        in_new_tab = self.tabs.tabText(self.tabs.currentIndex()) == 'New'
        shape_to_save = self.drawing_shape if in_new_tab else self.shape
        if shape_to_save is None or len(shape_to_save.vertices) == 0:
            return
        path, _ = QFileDialog.getSaveFileName(self, 'Save Shape JSON', self.SHAPES_DIR, 'JSON Files (*.json)')
        if path:
            shape_to_save.save_to_json(path)
            self.visualize_tab.refresh_shape_dropdown()
            # If nothing is loaded, select the first shape
            if self.visualize_tab.shape_dropdown.count() > 1 and self.visualize_tab.shape_dropdown.currentIndex() == 0:
                self.visualize_tab.shape_dropdown.setCurrentIndex(1)
                self.visualize_tab.on_dropdown_change(1)

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

    def on_plot_click(self, event):
        pos = event.scenePos()
        vb = self.plot_widget.getViewBox()
        mouse_point = vb.mapSceneToView(pos)
        x, y = mouse_point.x(), mouse_point.y()
        # Use drawing_shape in New tab
        in_new_tab = self.tabs.tabText(self.tabs.currentIndex()) == 'New'
        shape = self.drawing_shape if in_new_tab else self.shape
        if self.draw_mode and in_new_tab:
            # Add vertex
            if self.drawing_shape is None:
                self.drawing_shape = Shape2D()
            self.drawing_shape.vertices.append((x, y))
            self.update_plot()
        elif self.add_edge_mode and in_new_tab and shape is not None:
            # Find nearest vertex (use larger threshold for easier selection)
            if len(shape.vertices) == 0:
                return
            min_dist = float('inf')
            min_idx = -1
            for i, (vx, vy) in enumerate(shape.vertices):
                dist = (vx - x) ** 2 + (vy - y) ** 2
                if dist < min_dist:
                    min_dist = dist
                    min_idx = i
            # Use a larger threshold (e.g., 0.25 in data coordinates)
            if min_dist < 0.25:
                if min_idx not in self.selected_vertices:
                    self.selected_vertices.append(min_idx)
                if len(self.selected_vertices) == 2:
                    v0, v1 = self.selected_vertices
                    if v0 != v1 and (v0, v1) not in shape.edges and (v1, v0) not in shape.edges:
                        shape.edges.append((v0, v1))
                    self.selected_vertices = []
                self.update_plot()
        elif not in_new_tab:
            # Existing logic for other tabs (if any)
            if self.draw_mode:
                self.drawing_shape.vertices.append((x, y))
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

    def on_tab_changed(self, idx):
        # If switching to New tab, clear canvas and set default view
        if self.tabs.tabText(idx) == 'New':
            self.drawing_shape = Shape2D()
            self.shape = None
            self.selected_vertices = []
            self.new_tab.clear_canvas_and_reset_view()
        else:
            # When leaving New tab, optionally set self.shape = self.drawing_shape if you want to keep the new shape
            if self.drawing_shape and len(self.drawing_shape.vertices) > 0:
                self.shape = self.drawing_shape
            self.drawing_shape = None
        self._last_tab_index = idx

    def update_plot(self):
        self.plot_widget.clear()
        # Set grid visibility based on checkbox
        show_grid = False
        if hasattr(self, 'visualize_tab'):
            show_grid = self.visualize_tab.grid_checkbox.isChecked()
        self.plot_widget.showGrid(x=show_grid, y=show_grid, alpha=0.3 if show_grid else 0)
        # Use drawing_shape in New tab, otherwise use shape
        if self.tabs.tabText(self.tabs.currentIndex()) == 'New':
            shape = self.drawing_shape
        else:
            shape = self.shape
        if shape is None or len(shape.vertices) == 0:
            self.new_tab.update_info()
            return
        # Draw filled polygon for simple closed shapes if option is enabled
        fill_shape = False
        if hasattr(self, 'visualize_tab'):
            fill_shape = self.visualize_tab.fill_checkbox.isChecked()
        if fill_shape and len(shape.vertices) >= 3:
            closed = all((i, (i+1)%len(shape.vertices)) in shape.edges or ((i+1)%len(shape.vertices), i) in shape.edges for i in range(len(shape.vertices)))
            if closed:
                import numpy as np
                pts = np.array(shape.vertices)
                # Ensure the polygon is closed
                if not np.allclose(pts[0], pts[-1]):
                    pts = np.vstack([pts, pts[0]])
                x = pts[:, 0]
                y = pts[:, 1]
                self.plot_widget.plot(x, y, pen=pg.mkPen('blue', width=2), fillLevel=0, fillBrush=pg.mkBrush('lightblue', alpha=80))
        # Draw edges
        for edge in shape.edges:
            v0 = shape.vertices[edge[0]]
            v1 = shape.vertices[edge[1]]
            self.plot_widget.plot([v0[0], v1[0]], [v0[1], v1[1]], pen=pg.mkPen('b', width=2))
        # Draw vertices (always large and red)
        xs, ys = zip(*shape.vertices)
        self.plot_widget.plot(xs, ys, pen=None, symbol='o', symbolBrush='r', symbolSize=12)
        # Draw vertex labels if requested
        show_labels = False
        if hasattr(self, 'visualize_tab'):
            show_labels = self.visualize_tab.label_checkbox.isChecked()
        if show_labels:
            for i, (x, y) in enumerate(shape.vertices):
                label = f"v{i+1}"
                text = pg.TextItem(label, anchor=(0.5, 1.5), color='k')
                text.setPos(x, y)
                self.plot_widget.addItem(text)
        # Highlight selected vertices in add edge mode
        if self.add_edge_mode and self.selected_vertices:
            sel_xs = [shape.vertices[i][0] for i in self.selected_vertices]
            sel_ys = [shape.vertices[i][1] for i in self.selected_vertices]
            self.plot_widget.plot(sel_xs, sel_ys, pen=None, symbol='o', symbolBrush='g', symbolSize=16)
        # Update info label in New tab
        self.new_tab.update_info() 