from PyQt5.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QLabel, QComboBox
import pyqtgraph as pg
from .custom_viewbox import CustomViewBox
from shape2d import Shape2D
from shape_similarity import calculate_shape_similarity
import os

class CompareTab(QWidget):
    def __init__(self, main_window):
        super().__init__()
        self.main_window = main_window
        self.shape1 = None
        self.shape2 = None
        self.init_ui()

    def init_ui(self):
        layout = QVBoxLayout()
        # Shape 1 dropdown
        h1 = QHBoxLayout()
        self.shape1_dropdown = QComboBox()
        self.shape1_dropdown.addItem('Select shape 1...')
        self.shape1_dropdown.currentIndexChanged.connect(self.on_shape1_change)
        h1.addWidget(QLabel('Shape 1:'))
        h1.addWidget(self.shape1_dropdown)
        layout.addLayout(h1)
        # Shape 2 dropdown
        h2 = QHBoxLayout()
        self.shape2_dropdown = QComboBox()
        self.shape2_dropdown.addItem('Select shape 2...')
        self.shape2_dropdown.currentIndexChanged.connect(self.on_shape2_change)
        h2.addWidget(QLabel('Shape 2:'))
        h2.addWidget(self.shape2_dropdown)
        layout.addLayout(h2)
        # Compare button
        self.compare_btn = QPushButton('Compare Shapes')
        self.compare_btn.clicked.connect(self.compare_shapes)
        layout.addWidget(self.compare_btn)
        # Result label
        self.result_label = QLabel('Similarity: -')
        layout.addWidget(self.result_label)
        # Plot widget
        self.plot_widget = pg.PlotWidget(viewBox=CustomViewBox(self.main_window))
        self.plot_widget.setBackground('w')
        self.plot_widget.setAspectLocked(True)
        self.plot_widget.setMinimumHeight(600)
        self.plot_widget.setMinimumWidth(900)
        self.plot_widget.scene().sigMouseClicked.connect(self.main_window.on_plot_click)
        layout.addWidget(self.plot_widget, stretch=1)
        self.setLayout(layout)
        self.refresh_shape_dropdowns()

    def refresh_shape_dropdowns(self):
        SHAPES_DIR = self.main_window.SHAPES_DIR
        # Shape 1
        self.shape1_dropdown.blockSignals(True)
        self.shape1_dropdown.clear()
        self.shape1_dropdown.addItem('Select shape 1...')
        # Shape 2
        self.shape2_dropdown.blockSignals(True)
        self.shape2_dropdown.clear()
        self.shape2_dropdown.addItem('Select shape 2...')
        if os.path.isdir(SHAPES_DIR):
            for fname in sorted(os.listdir(SHAPES_DIR)):
                if fname.endswith('.json'):
                    self.shape1_dropdown.addItem(fname)
                    self.shape2_dropdown.addItem(fname)
        self.shape1_dropdown.blockSignals(False)
        self.shape2_dropdown.blockSignals(False)
        self.shape1 = None
        self.shape2 = None
        self.result_label.setText('Similarity: -')

    def on_shape1_change(self, idx):
        if idx == 0:
            self.shape1 = None
            return
        fname = self.shape1_dropdown.currentText()
        path = os.path.join(self.main_window.SHAPES_DIR, fname)
        if os.path.isfile(path):
            self.shape1 = Shape2D.load_from_json(path)
            self.result_label.setText('Similarity: -')

    def on_shape2_change(self, idx):
        if idx == 0:
            self.shape2 = None
            return
        fname = self.shape2_dropdown.currentText()
        path = os.path.join(self.main_window.SHAPES_DIR, fname)
        if os.path.isfile(path):
            self.shape2 = Shape2D.load_from_json(path)
            self.result_label.setText('Similarity: -')

    def compare_shapes(self):
        if self.shape1 is None or self.shape2 is None:
            self.result_label.setText('Select both shapes first!')
            return
        try:
            similarity = calculate_shape_similarity(self.shape1, self.shape2)
            self.result_label.setText(f'Similarity: {similarity:.6f} (lower is more similar)')
        except Exception as e:
            self.result_label.setText(f'Error: {str(e)}') 