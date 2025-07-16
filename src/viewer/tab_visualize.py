from PyQt5.QtWidgets import QWidget, QHBoxLayout, QComboBox, QPushButton
import os

class VisualizeTab(QWidget):
    def __init__(self, main_window):
        super().__init__()
        self.main_window = main_window
        layout = QHBoxLayout()
        self.shape_dropdown = QComboBox()
        self.shape_dropdown.addItem('Select shape from folder...')
        self.shape_dropdown.currentIndexChanged.connect(self.on_dropdown_change)
        self.load_btn = QPushButton('Load Shape')
        self.load_btn.clicked.connect(self.load_shape)
        layout.addWidget(self.shape_dropdown)
        layout.addWidget(self.load_btn)
        self.setLayout(layout)
        self.refresh_shape_dropdown()

    def refresh_shape_dropdown(self):
        SHAPES_DIR = self.main_window.SHAPES_DIR
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
        path = os.path.join(self.main_window.SHAPES_DIR, fname)
        if os.path.isfile(path):
            self.main_window.load_shape_from_path(path)

    def load_shape(self):
        self.main_window.load_shape_dialog() 