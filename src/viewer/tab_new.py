from PyQt5.QtWidgets import QWidget, QHBoxLayout, QPushButton, QFileDialog, QLabel
import os

class NewTab(QWidget):
    def __init__(self, main_window):
        super().__init__()
        self.main_window = main_window
        layout = QHBoxLayout()
        self.draw_mode_btn = QPushButton('Add Vertices')
        self.add_edge_mode_btn = QPushButton('Add Edges')
        self.save_btn = QPushButton('Save Shape')
        self.info_label = QLabel('Vertices: 0   Edges: 0')
        self.draw_mode_btn.setCheckable(True)
        self.add_edge_mode_btn.setCheckable(True)
        self.draw_mode_btn.clicked.connect(self.toggle_draw_mode)
        self.add_edge_mode_btn.clicked.connect(self.toggle_add_edge_mode)
        self.save_btn.clicked.connect(self.save_shape)
        layout.addWidget(self.draw_mode_btn)
        layout.addWidget(self.add_edge_mode_btn)
        layout.addWidget(self.save_btn)
        layout.addWidget(self.info_label)
        self.setLayout(layout)
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