from PyQt5.QtWidgets import QWidget, QHBoxLayout, QPushButton, QFileDialog
import os

class NewTab(QWidget):
    def __init__(self, main_window):
        super().__init__()
        self.main_window = main_window
        layout = QHBoxLayout()
        self.draw_mode_btn = QPushButton('Draw Mode')
        self.add_edge_mode_btn = QPushButton('Add Edge Mode')
        self.save_btn = QPushButton('Save Shape')
        self.draw_mode_btn.setCheckable(True)
        self.add_edge_mode_btn.setCheckable(True)
        self.draw_mode_btn.clicked.connect(self.toggle_draw_mode)
        self.add_edge_mode_btn.clicked.connect(self.toggle_add_edge_mode)
        self.save_btn.clicked.connect(self.save_shape)
        layout.addWidget(self.draw_mode_btn)
        layout.addWidget(self.add_edge_mode_btn)
        layout.addWidget(self.save_btn)
        self.setLayout(layout)

    def toggle_draw_mode(self):
        self.main_window.toggle_draw_mode()

    def toggle_add_edge_mode(self):
        self.main_window.toggle_add_edge_mode()

    def save_shape(self):
        self.main_window.save_shape_dialog() 