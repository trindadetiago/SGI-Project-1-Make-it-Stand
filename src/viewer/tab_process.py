from PyQt5.QtWidgets import QWidget, QHBoxLayout, QVBoxLayout, QPushButton, QInputDialog, QMessageBox
import pyqtgraph as pg
from .custom_viewbox import CustomViewBox
from shape_processing import scale_shape
from shape_smoothing import smooth_shape, calculate_smoothing_score
from shape2d import Shape2D

class ProcessTab(QWidget):
    def __init__(self, main_window):
        super().__init__()
        self.main_window = main_window
        controls_layout = QHBoxLayout()
        self.scale_btn = QPushButton('Scale Shape')
        self.scale_btn.clicked.connect(self.scale_shape)
        self.smooth_btn = QPushButton('Smooth Shape')
        self.smooth_btn.clicked.connect(self.smooth_shape)
        self.smoothing_score_btn = QPushButton('Show Smoothing Score')
        self.smoothing_score_btn.clicked.connect(self.show_smoothing_score)
        self.save_btn = QPushButton('Save Shape')
        self.save_btn.clicked.connect(self.save_shape)
        controls_layout.addWidget(self.scale_btn)
        controls_layout.addWidget(self.smooth_btn)
        controls_layout.addWidget(self.smoothing_score_btn)
        controls_layout.addWidget(self.save_btn)
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

    def scale_shape(self):
        if self.main_window.shape is None:
            return
        factor, ok = QInputDialog.getDouble(self, 'Scale Shape', 'Scale factor:', 1.0, 0.01, 100.0, 2)
        if ok:
            self.main_window.shape = scale_shape(self.main_window.shape, factor)
            self.main_window.update_plot()

    def smooth_shape(self):
        if self.main_window.shape is None:
            return
        iterations, ok = QInputDialog.getInt(self, 'Smooth Shape', 'Number of iterations:', 1, 1, 10, 1)
        if ok:
            strength, ok2 = QInputDialog.getDouble(self, 'Smooth Shape', 'Smoothing strength (0.0-1.0):', 0.5, 0.0, 1.0, 2)
            if ok2:
                # Calculate initial score
                initial_score = calculate_smoothing_score(self.main_window.shape).item()
                # Apply smoothing
                new_vertices = smooth_shape(self.main_window.shape, iterations, strength)
                self.main_window.shape = Shape2D(new_vertices, self.main_window.shape.edges.copy())
                self.main_window.update_plot()
                # Calculate final score
                final_score = calculate_smoothing_score(self.main_window.shape).item()
                # Show improvement
                QMessageBox.information(self, 'Smoothing Complete', 
                    f'Initial smoothing score: {initial_score:.6f}\n'
                    f'Final smoothing score: {final_score:.6f}\n'
                    f'Improvement: {initial_score - final_score:.6f}')

    def show_smoothing_score(self):
        if self.main_window.shape is None:
            return
        score = calculate_smoothing_score(self.main_window.shape).item()
        QMessageBox.information(self, 'Smoothing Score', 
            f'Current smoothing score: {score:.6f}\n'
            f'(Lower values indicate smoother shapes)')

    def save_shape(self):
        self.main_window.save_shape_dialog() 