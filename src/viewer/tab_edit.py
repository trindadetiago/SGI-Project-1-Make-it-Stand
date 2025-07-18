from PyQt5.QtWidgets import QWidget, QHBoxLayout, QVBoxLayout, QLabel, QPushButton
import pyqtgraph as pg
from .custom_viewbox import CustomViewBox

class EditTab(QWidget):
    def __init__(self, main_window):
        super().__init__()
        self.main_window = main_window
        controls_layout = QHBoxLayout()
        self.info_label = QLabel('Selected Vertex: None')
        self.deselect_btn = QPushButton('Deselect')
        self.deselect_btn.clicked.connect(self.deselect_vertex)
        self.save_overwrite_btn = QPushButton('Save (Overwrite)')
        self.save_overwrite_btn.clicked.connect(self.save_overwrite)
        self.save_new_btn = QPushButton('Save As New')
        self.save_new_btn.clicked.connect(self.save_as_new)
        self.make_ground_btn = QPushButton('Make it Ground')
        self.make_ground_btn.clicked.connect(self.make_selected_edge_ground)
        self.make_ground_btn.setEnabled(False)
        controls_layout.addWidget(self.info_label)
        controls_layout.addWidget(self.deselect_btn)
        controls_layout.addWidget(self.save_overwrite_btn)
        controls_layout.addWidget(self.save_new_btn)
        controls_layout.addWidget(self.make_ground_btn)
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
        self.selected_vertex = None
        self.selected_edge = None
        self.update_save_button()
        self.update_info()

    def set_selected_vertex(self, idx):
        self.selected_vertex = idx
        self.selected_edge = None
        self.update_info()
        self.make_ground_btn.setEnabled(False)

    def set_selected_edge(self, edge):
        self.selected_edge = edge
        self.selected_vertex = None
        self.update_info()
        self.make_ground_btn.setEnabled(True)

    def clear_selected_edge(self):
        self.selected_edge = None
        self.make_ground_btn.setEnabled(False)
        self.update_info()

    def deselect_vertex(self):
        self.set_selected_vertex(None)
        self.clear_selected_edge()
        self.main_window.selected_vertex = None
        self.main_window.update_plot()

    def make_selected_edge_ground(self):
        if self.selected_edge is not None:
            shape = self.main_window.shape
            if shape is not None:
                i, j = self.selected_edge
                # Set both vertices' y to 0
                shape.vertices[i][1] = 0.0
                shape.vertices[j][1] = 0.0
                self.main_window.update_plot()

    def save_overwrite(self):
        self.main_window.save_current_shape()
        self.update_save_button()

    def save_as_new(self):
        self.main_window.save_shape_dialog()
        self.update_save_button()

    def update_save_button(self):
        self.save_overwrite_btn.setEnabled(self.main_window.current_shape_path is not None)
        self.save_new_btn.setEnabled(True)

    def update_info(self):
        if self.selected_vertex is not None:
            self.info_label.setText(f'Selected Vertex: v{self.selected_vertex+1}')
        elif self.selected_edge is not None:
            self.info_label.setText(f'Selected Edge: v{self.selected_edge[0]+1} - v{self.selected_edge[1]+1}')
        else:
            self.info_label.setText('Selected: None') 