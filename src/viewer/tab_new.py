from PyQt5.QtWidgets import QWidget, QHBoxLayout, QVBoxLayout, QPushButton, QFileDialog, QLabel
import pyqtgraph as pg
from .custom_viewbox import CustomViewBox
import os

class NewTab(QWidget):
    def __init__(self, main_window):
        super().__init__()
        self.main_window = main_window
        controls_layout = QHBoxLayout()
        self.draw_mode_btn = QPushButton('Add Vertices')
        self.add_edge_mode_btn = QPushButton('Add Edges')
        self.save_btn = QPushButton('Save Shape')
        self.info_label = QLabel('Vertices: 0   Edges: 0')
        self.draw_mode_btn.setCheckable(True)
        self.add_edge_mode_btn.setCheckable(True)
        self.draw_mode_btn.clicked.connect(self.toggle_draw_mode)
        self.add_edge_mode_btn.clicked.connect(self.toggle_add_edge_mode)
        self.save_btn.clicked.connect(self.save_shape)
        controls_layout.addWidget(self.draw_mode_btn)
        controls_layout.addWidget(self.add_edge_mode_btn)
        controls_layout.addWidget(self.save_btn)
        controls_layout.addWidget(self.info_label)
        # Plot widget
        self.plot_widget = pg.PlotWidget(viewBox=CustomViewBox(self.main_window))
        self.plot_widget.setBackground('w')
        self.plot_widget.setAspectLocked(True)
        self.plot_widget.setMinimumHeight(600)
        self.plot_widget.setMinimumWidth(900)
        self.plot_widget.scene().sigMouseClicked.connect(self.main_window.on_plot_click)
        # Main layout
        main_layout = QVBoxLayout()
        main_layout.addLayout(controls_layout)
        main_layout.addWidget(self.plot_widget, stretch=1)
        self.setLayout(main_layout)
        self.update_info()

    def toggle_draw_mode(self):
        self.main_window.toggle_draw_mode()
        self.update_info()

    def toggle_add_edge_mode(self):
        self.main_window.toggle_add_edge_mode()
        self.update_info()

    def save_shape(self):
        self.main_window.save_shape_dialog()
        self.update_info()

    def update_info(self):
        # Always use drawing_shape for the New tab
        shape = self.main_window.drawing_shape
        n_vertices = len(shape.vertices) if shape else 0
        n_edges = len(shape.edges) if shape else 0
        self.info_label.setText(f'Vertices: {n_vertices}   Edges: {n_edges}')

    def clear_canvas_and_reset_view(self):
        self.main_window.drawing_shape = None
        self.main_window.shape = None
        self.main_window.selected_vertices = []
        self.main_window.plot_widget.clear()
        vb = self.main_window.plot_widget.getViewBox()
        vb.setRange(xRange=[-1, 1], yRange=[-1, 1], padding=0.1)
        self.update_info() 