import sys
import os
from PyQt5.QtWidgets import (QApplication, QWidget, QVBoxLayout, QPushButton, QFileDialog, QHBoxLayout, QInputDialog, QComboBox, QTabWidget, QFrame)
import pyqtgraph as pg
from shape2d import Shape2D
from shape_processing import scale_shape

SHAPES_DIR = os.path.join(os.path.dirname(__file__), '..', 'shapes')

class ShapeGUI(QWidget):
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

        # --- Visualize Tab (was Load) ---
        visualize_tab = QWidget()
        visualize_layout = QHBoxLayout()
        self.shape_dropdown = QComboBox()
        self.shape_dropdown.addItem('Select shape from folder...')
        self.shape_dropdown.currentIndexChanged.connect(self.on_dropdown_change)
        self.refresh_shape_dropdown()
        self.load_btn = QPushButton('Load Shape')
        self.load_btn.clicked.connect(self.load_shape)
        visualize_layout.addWidget(self.shape_dropdown)
        visualize_layout.addWidget(self.load_btn)
        visualize_tab.setLayout(visualize_layout)
        self.tabs.addTab(visualize_tab, 'Visualize')

        # --- Process Tab ---
        process_tab = QWidget()
        process_layout = QHBoxLayout()
        self.scale_btn = QPushButton('Scale Shape')
        self.scale_btn.clicked.connect(self.scale_shape)
        process_layout.addWidget(self.scale_btn)
        process_tab.setLayout(process_layout)
        self.tabs.addTab(process_tab, 'Process')

        # --- New Tab ---
        new_tab = QWidget()
        new_layout = QHBoxLayout()
        self.draw_mode_btn = QPushButton('Draw Mode')
        self.add_edge_mode_btn = QPushButton('Add Edge Mode')
        self.save_btn = QPushButton('Save Shape')
        self.draw_mode_btn.setCheckable(True)
        self.add_edge_mode_btn.setCheckable(True)
        self.draw_mode_btn.clicked.connect(self.toggle_draw_mode)
        self.add_edge_mode_btn.clicked.connect(self.toggle_add_edge_mode)
        self.save_btn.clicked.connect(self.save_shape)
        new_layout.addWidget(self.draw_mode_btn)
        new_layout.addWidget(self.add_edge_mode_btn)
        new_layout.addWidget(self.save_btn)
        new_tab.setLayout(new_layout)
        self.tabs.addTab(new_tab, 'New')

        main_layout.addWidget(self.tabs)

        # --- Plot ---
        self.plot_widget = pg.PlotWidget()
        self.plot_widget.setBackground('w')
        self.plot_widget.scene().sigMouseClicked.connect(self.on_plot_click)
        main_layout.addWidget(self.plot_widget)

        self.setLayout(main_layout)

    def refresh_shape_dropdown(self):
        self.shape_dropdown.blockSignals(True)
        self.shape_dropdown.clear()
        self.shape_dropdown.addItem('Select shape from folder...')
        if os.path.isdir(SHAPES_DIR):
            for fname in sorted(os.listdir(SHAPES_DIR)):
                if fname.endswith('.json'):
                    self.shape_dropdown.addItem(fname)
        self.shape_dropdown.blockSignals(False)

    def on_dropdown_change(self, idx):
        if idx == 0:
            return
        fname = self.shape_dropdown.currentText()
        path = os.path.join(SHAPES_DIR, fname)
        if os.path.isfile(path):
            self.shape = Shape2D.load_from_json(path)
            self.drawing_shape = None
            self.selected_vertices = []
            self.draw_mode_btn.setChecked(False)
            self.add_edge_mode_btn.setChecked(False)
            self.draw_mode = False
            self.add_edge_mode = False
            self.update_plot()

    def showEvent(self, event):
        self.refresh_shape_dropdown()
        # Automatically select and load the first shape if available
        if self.shape_dropdown.count() > 1:
            self.shape_dropdown.setCurrentIndex(1)
            self.on_dropdown_change(1)
        super().showEvent(event)

    def load_shape(self):
        path, _ = QFileDialog.getOpenFileName(self, 'Open Shape JSON', '', 'JSON Files (*.json)')
        if path:
            self.shape = Shape2D.load_from_json(path)
            self.drawing_shape = None
            self.selected_vertices = []
            self.draw_mode_btn.setChecked(False)
            self.add_edge_mode_btn.setChecked(False)
            self.draw_mode = False
            self.add_edge_mode = False
            self.update_plot()
            self.refresh_shape_dropdown()

    def save_shape(self):
        if self.shape is None:
            return
        path, _ = QFileDialog.getSaveFileName(self, 'Save Shape JSON', SHAPES_DIR, 'JSON Files (*.json)')
        if path:
            self.shape.save_to_json(path)
            self.refresh_shape_dropdown()
            # If nothing is loaded, select the first shape
            if self.shape_dropdown.count() > 1 and self.shape_dropdown.currentIndex() == 0:
                self.shape_dropdown.setCurrentIndex(1)
                self.on_dropdown_change(1)

    def scale_shape(self):
        if self.shape is None:
            return
        factor, ok = QInputDialog.getDouble(self, 'Scale Shape', 'Scale factor:', 1.0, 0.01, 100.0, 2)
        if ok:
            self.shape = scale_shape(self.shape, factor)
            self.update_plot()

    def toggle_draw_mode(self):
        self.draw_mode = self.draw_mode_btn.isChecked()
        if self.draw_mode:
            self.add_edge_mode = False
            self.add_edge_mode_btn.setChecked(False)
            self.drawing_shape = Shape2D()
            self.selected_vertices = []
        else:
            if self.drawing_shape and len(self.drawing_shape.vertices) > 0:
                self.shape = self.drawing_shape
            self.drawing_shape = None
        self.update_plot()

    def toggle_add_edge_mode(self):
        self.add_edge_mode = self.add_edge_mode_btn.isChecked()
        if self.add_edge_mode:
            self.draw_mode = False
            self.draw_mode_btn.setChecked(False)
            if self.shape is None:
                self.shape = Shape2D()
            self.selected_vertices = []
        else:
            self.selected_vertices = []
        self.update_plot()

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