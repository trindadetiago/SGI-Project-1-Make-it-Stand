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
        # Make plots bigger
        self.before_plot.setMinimumSize(500, 500)
        self.after_plot.setMinimumSize(500, 500)
        plot_layout.addWidget(self.before_plot, stretch=1)
        plot_layout.addWidget(self.after_plot, stretch=1)
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
        V_opt = gradient_descent(loss_fn, V0, lr=0.05, max_iters=1000, verbose=True)
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
        # --- Center of Mass and Stability (Before) ---
        try:
            from shape_mass_center import calculate_center_of_mass
            from shape_stability import is_shape_stable
            area_b, com_b = calculate_center_of_mass(V0, E)
            cx_b, cy_b = float(com_b[0].item()), float(com_b[1].item())
            self.before_plot.plot([cx_b], [cy_b], pen=None, symbol='+', symbolBrush='red', symbolPen='red', symbolSize=20)
            is_stable_b, _, _, _ = is_shape_stable(V0, E)
            if is_stable_b:
                stability_text_b = pg.TextItem(text='Stable', color='green', anchor=(0.5, -0.5))
            else:
                stability_text_b = pg.TextItem(text='Will fall!', color='red', anchor=(0.5, -0.5))
            stability_text_b.setPos(cx_b, cy_b)
            self.before_plot.addItem(stability_text_b)
        except Exception as e:
            pass
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
        # --- Center of Mass and Stability (After) ---
        try:
            area_a, com_a = calculate_center_of_mass(V_opt, E)
            cx_a, cy_a = float(com_a[0].item()), float(com_a[1].item())
            self.after_plot.plot([cx_a], [cy_a], pen=None, symbol='+', symbolBrush='red', symbolPen='red', symbolSize=20)
            is_stable_a, _, _, _ = is_shape_stable(V_opt, E)
            if is_stable_a:
                stability_text_a = pg.TextItem(text='Stable', color='green', anchor=(0.5, -0.5))
            else:
                stability_text_a = pg.TextItem(text='Will fall!', color='red', anchor=(0.5, -0.5))
            stability_text_a.setPos(cx_a, cy_a)
            self.after_plot.addItem(stability_text_a)
        except Exception as e:
            pass
        self.info_label.setText("Optimization complete!") 