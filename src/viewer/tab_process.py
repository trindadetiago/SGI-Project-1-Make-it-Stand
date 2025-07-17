from PyQt5.QtWidgets import QWidget, QHBoxLayout, QPushButton, QInputDialog, QMessageBox
from shape_processing import scale_shape
from shape_smoothing import smooth_shape, calculate_smoothing_score

class ProcessTab(QWidget):
    def __init__(self, main_window):
        super().__init__()
        self.main_window = main_window
        layout = QHBoxLayout()
        self.scale_btn = QPushButton('Scale Shape')
        self.scale_btn.clicked.connect(self.scale_shape)
        self.smooth_btn = QPushButton('Smooth Shape')
        self.smooth_btn.clicked.connect(self.smooth_shape)
        self.smoothing_score_btn = QPushButton('Show Smoothing Score')
        self.smoothing_score_btn.clicked.connect(self.show_smoothing_score)
        self.save_btn = QPushButton('Save Shape')
        self.save_btn.clicked.connect(self.save_shape)
        layout.addWidget(self.scale_btn)
        layout.addWidget(self.smooth_btn)
        layout.addWidget(self.smoothing_score_btn)
        layout.addWidget(self.save_btn)
        self.setLayout(layout)

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
                initial_score = calculate_smoothing_score(self.main_window.shape)
                
                # Apply smoothing
                self.main_window.shape = smooth_shape(self.main_window.shape, iterations, strength)
                self.main_window.update_plot()
                
                # Calculate final score
                final_score = calculate_smoothing_score(self.main_window.shape)
                
                # Show improvement
                QMessageBox.information(self, 'Smoothing Complete', 
                    f'Initial smoothing score: {initial_score:.6f}\n'
                    f'Final smoothing score: {final_score:.6f}\n'
                    f'Improvement: {initial_score - final_score:.6f}')

    def show_smoothing_score(self):
        if self.main_window.shape is None:
            return
        score = calculate_smoothing_score(self.main_window.shape)
        QMessageBox.information(self, 'Smoothing Score', 
            f'Current smoothing score: {score:.6f}\n'
            f'(Lower values indicate smoother shapes)')

    def save_shape(self):
        self.main_window.save_shape_dialog() 