from PyQt5.QtWidgets import QWidget, QHBoxLayout, QLabel, QPushButton

class EditTab(QWidget):
    def __init__(self, main_window):
        super().__init__()
        self.main_window = main_window
        layout = QHBoxLayout()
        self.info_label = QLabel('Selected Vertex: None')
        self.deselect_btn = QPushButton('Deselect')
        self.deselect_btn.clicked.connect(self.deselect_vertex)
        self.save_overwrite_btn = QPushButton('Save (Overwrite)')
        self.save_overwrite_btn.clicked.connect(self.save_overwrite)
        layout.addWidget(self.info_label)
        layout.addWidget(self.deselect_btn)
        layout.addWidget(self.save_overwrite_btn)
        self.setLayout(layout)
        self.selected_vertex = None
        self.update_save_button()

    def set_selected_vertex(self, idx):
        self.selected_vertex = idx
        if idx is not None:
            self.info_label.setText(f'Selected Vertex: v{idx+1}')
        else:
            self.info_label.setText('Selected Vertex: None')

    def deselect_vertex(self):
        self.set_selected_vertex(None)
        self.main_window.selected_vertex = None
        self.main_window.update_plot()

    def save_overwrite(self):
        self.main_window.save_current_shape()
        self.update_save_button()

    def update_save_button(self):
        self.save_overwrite_btn.setEnabled(self.main_window.current_shape_path is not None) 