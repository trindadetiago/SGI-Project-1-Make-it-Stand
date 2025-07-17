from PyQt5.QtWidgets import QWidget, QVBoxLayout, QPushButton, QLabel, QHBoxLayout
import pyqtgraph as pg
import torch
from shape2d import Shape2D
from optimization import total_loss, gradient_descent

class OptimizationTab(QWidget):
    def __init__(self, main_window):
        super().__init__()
        self.main_window = main_window
        self.init_ui()

    def init_ui(self):
        layout = QVBoxLayout()
        self.info_label = QLabel("Run optimization to improve stability, smoothness, and similarity.")
        layout.addWidget(self.info_label)
        self.run_btn = QPushButton("Run Optimizer on Current Shape")
        self.run_btn.clicked.connect(self.run_optimization)
        layout.addWidget(self.run_btn)
        plot_layout = QHBoxLayout()
        self.before_plot = pg.PlotWidget()
        self.after_plot = pg.PlotWidget()
        self.before_plot.setBackground('w')
        self.after_plot.setBackground('w')
        self.before_plot.setAspectLocked(True)
        self.after_plot.setAspectLocked(True)
        plot_layout.addWidget(self.before_plot)
        plot_layout.addWidget(self.after_plot)
        layout.addLayout(plot_layout)
        self.setLayout(layout)

    def run_optimization(self):
        shape = self.main_window.shape
        if shape is None or len(shape.vertices) < 3 or len(shape.edges) < 3:
            self.info_label.setText("No valid shape loaded.")
            return
        V_og = shape.vertices
        E = torch.tensor(shape.edges, dtype=torch.long, device=V_og.device)
        V0 = V_og.clone()  # No perturbation
        def loss_fn(V):
            return total_loss(V, E, V_og, lambda1=0.33, lambda2=0.33, lambda3=0.34)
        V_opt = gradient_descent(loss_fn, V0, lr=0.05, max_iters=1000, verbose=False)
        # Logging for debugging
        print("V_opt after optimization:", V_opt)
        print("Any NaN in V_opt?", torch.isnan(V_opt).any().item())
        final_loss = loss_fn(V_opt)
        print("Final loss:", final_loss.item())
        if torch.isnan(V_opt).any():
            self.info_label.setText("Optimization failed: NaN encountered in result.")
            return
        # Plot before (edges)
        self.before_plot.clear()
        for i, j in shape.edges:
            self.before_plot.plot(
                [V0[i, 0].item(), V0[j, 0].item()],
                [V0[i, 1].item(), V0[j, 1].item()],
                pen=pg.mkPen('b', width=2)
            )
        self.before_plot.plot(V0[:,0].cpu().numpy(), V0[:,1].cpu().numpy(), pen=None, symbol='o', symbolBrush='r', symbolSize=10)
        self.before_plot.setTitle("Before Optimization")
        # Plot after (edges)
        self.after_plot.clear()
        for i, j in shape.edges:
            self.after_plot.plot(
                [V_opt[i, 0].item(), V_opt[j, 0].item()],
                [V_opt[i, 1].item(), V_opt[j, 1].item()],
                pen=pg.mkPen('g', width=2)
            )
        self.after_plot.plot(V_opt[:,0].cpu().numpy(), V_opt[:,1].cpu().numpy(), pen=None, symbol='o', symbolBrush='r', symbolSize=10)
        self.after_plot.setTitle("After Optimization")
        self.info_label.setText("Optimization complete!") 