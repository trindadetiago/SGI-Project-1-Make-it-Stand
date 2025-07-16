from PyQt5.QtWidgets import QWidget, QHBoxLayout, QComboBox, QPushButton, QCheckBox
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
        self.label_checkbox = QCheckBox('Show Vertex Labels')
        self.label_checkbox.stateChanged.connect(self.on_option_change)
        self.fill_checkbox = QCheckBox('Fill Shape')
        self.fill_checkbox.stateChanged.connect(self.on_option_change)
        self.grid_checkbox = QCheckBox('Show Grid')
        self.grid_checkbox.stateChanged.connect(self.on_option_change)
        self.com_checkbox = QCheckBox('Show Center of Mass')
        self.com_checkbox.setChecked(True)
        self.com_checkbox.stateChanged.connect(self.on_option_change)
        layout.addWidget(self.shape_dropdown)
        layout.addWidget(self.load_btn)
        layout.addWidget(self.label_checkbox)
        layout.addWidget(self.fill_checkbox)
        layout.addWidget(self.grid_checkbox)
        layout.addWidget(self.com_checkbox)
        self.setLayout(layout)
        self.refresh_shape_dropdown()
        self.label_checkbox.setChecked(False)
        self.fill_checkbox.setChecked(False)
        self.grid_checkbox.setChecked(False)

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

    def on_option_change(self):
        self.main_window.update_plot() 