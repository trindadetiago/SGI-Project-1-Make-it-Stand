from PyQt5.QtWidgets import QWidget, QHBoxLayout, QPushButton, QInputDialog
from shape_processing import scale_shape

class ProcessTab(QWidget):
    def __init__(self, main_window):
        super().__init__()
        self.main_window = main_window
        layout = QHBoxLayout()
        self.scale_btn = QPushButton('Scale Shape')
        self.scale_btn.clicked.connect(self.scale_shape)
        layout.addWidget(self.scale_btn)
        self.setLayout(layout)

    def scale_shape(self):
        if self.main_window.shape is None:
            return
        factor, ok = QInputDialog.getDouble(self, 'Scale Shape', 'Scale factor:', 1.0, 0.01, 100.0, 2)
        if ok:
            self.main_window.shape = scale_shape(self.main_window.shape, factor)
            self.main_window.update_plot() 