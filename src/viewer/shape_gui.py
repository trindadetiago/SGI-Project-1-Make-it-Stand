import sys
import os
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QTabWidget
from PyQt5.QtGui import QIcon
import pyqtgraph as pg
from shape2d import Shape2D
from .tab_visualize import VisualizeTab
from .tab_process import ProcessTab
from .tab_new import NewTab

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
        main_layout.addWidget(self.tabs)
        # --- Plot ---
        self.plot_widget = pg.PlotWidget()
        self.plot_widget.setBackground('w')
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
        if self.shape is None:
            return
        path, _ = QFileDialog.getSaveFileName(self, 'Save Shape JSON', self.SHAPES_DIR, 'JSON Files (*.json)')
        if path:
            self.shape.save_to_json(path)
            self.visualize_tab.refresh_shape_dropdown()
            # If nothing is loaded, select the first shape
            if self.visualize_tab.shape_dropdown.count() > 1 and self.visualize_tab.shape_dropdown.currentIndex() == 0:
                self.visualize_tab.shape_dropdown.setCurrentIndex(1)
                self.visualize_tab.on_dropdown_change(1)

    def toggle_draw_mode(self):
        self.draw_mode = not self.draw_mode
        if self.draw_mode:
            self.add_edge_mode = False
            self.new_tab.add_edge_mode_btn.setChecked(False)
            self.drawing_shape = Shape2D()
            self.selected_vertices = []
        else:
            if self.drawing_shape and len(self.drawing_shape.vertices) > 0:
                self.shape = self.drawing_shape
            self.drawing_shape = None
        self.update_plot()
        self.new_tab.draw_mode_btn.setChecked(self.draw_mode)

    def toggle_add_edge_mode(self):
        self.add_edge_mode = not self.add_edge_mode
        if self.add_edge_mode:
            self.draw_mode = False
            self.new_tab.draw_mode_btn.setChecked(False)
            if self.shape is None:
                self.shape = Shape2D()
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
        if self.draw_mode:
            # Add vertex
            self.drawing_shape.vertices.append((x, y))
            self.update_plot()
        elif self.add_edge_mode and self.shape is not None:
            # Find nearest vertex
            if len(self.shape.vertices) == 0:
                return
            min_dist = float('inf')
            min_idx = -1
            for i, (vx, vy) in enumerate(self.shape.vertices):
                dist = (vx - x) ** 2 + (vy - y) ** 2
                if dist < min_dist:
                    min_dist = dist
                    min_idx = i
            # Only select if close enough (e.g., within 10 pixels in plot coordinates)
            if min_dist < 0.1:  # Adjust threshold as needed
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
        shape = self.drawing_shape if self.draw_mode else self.shape
        if shape is None or len(shape.vertices) == 0:
            return
        # Draw edges
        for edge in shape.edges:
            v0 = shape.vertices[edge[0]]
            v1 = shape.vertices[edge[1]]
            self.plot_widget.plot([v0[0], v1[0]], [v0[1], v1[1]], pen=pg.mkPen('b', width=2))
        # Draw vertices
        xs, ys = zip(*shape.vertices)
        self.plot_widget.plot(xs, ys, pen=None, symbol='o', symbolBrush='r', symbolSize=8)
        # Highlight selected vertices in add edge mode
        if self.add_edge_mode and self.selected_vertices:
            sel_xs = [shape.vertices[i][0] for i in self.selected_vertices]
            sel_ys = [shape.vertices[i][1] for i in self.selected_vertices]
            self.plot_widget.plot(sel_xs, sel_ys, pen=None, symbol='o', symbolBrush='g', symbolSize=14) 