from PyQt5.QtWidgets import QWidget, QVBoxLayout, QPushButton, QLabel, QHBoxLayout, QSpinBox
import pyqtgraph as pg
import torch
from shape2d import Shape2D
from optimization import total_loss, gradient_descent

class OptimizationTab(QWidget):
    def __init__(self, main_window):
        super().__init__()
        self.main_window = main_window
        self.last_optimized_vertices = None  # Store last optimized result
        self.init_ui()
        self.plot_current_shape()  # Show the current shape on tab open

    def init_ui(self):
        layout = QVBoxLayout()
        self.info_label = QLabel("Run optimization to improve stability, smoothness, and similarity.")
        layout.addWidget(self.info_label)
        # Add spinbox for iterations
        iter_layout = QHBoxLayout()
        iter_label = QLabel("Iterations:")
        self.iter_spinbox = QSpinBox()
        self.iter_spinbox.setMinimum(1)
        self.iter_spinbox.setMaximum(100000)
        self.iter_spinbox.setValue(1000)
        iter_layout.addWidget(iter_label)
        iter_layout.addWidget(self.iter_spinbox)
        layout.addLayout(iter_layout)
        # Add Reset button
        self.reset_btn = QPushButton("Reset to Original Shape")
        self.reset_btn.clicked.connect(self.reset_to_original)
        layout.addWidget(self.reset_btn)
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
        # Show grid on both plots
        self.before_plot.showGrid(x=True, y=True, alpha=0.3)
        self.after_plot.showGrid(x=True, y=True, alpha=0.3)
        plot_layout.addWidget(self.before_plot, stretch=1)
        plot_layout.addWidget(self.after_plot, stretch=1)
        layout.addLayout(plot_layout)
        
        # Add save button for optimized shape
        self.save_optimized_btn = QPushButton("Save Optimized Shape")
        self.save_optimized_btn.clicked.connect(self.save_optimized_shape)
        self.save_optimized_btn.setEnabled(False)  # Initially disabled until optimization is run
        layout.addWidget(self.save_optimized_btn)
        
        self.setLayout(layout)

    def plot_current_shape(self, use_last_optimized=False):
        shape = self.main_window.shape
        self.before_plot.clear()
        self.before_plot.showGrid(x=True, y=True, alpha=0.3)
        if shape is None:
            self.before_plot.setTitle("Before Optimization")
            return
        if use_last_optimized and self.last_optimized_vertices is not None:
            V = self.last_optimized_vertices
        else:
            V = shape.vertices
        E = shape.edges
        if V is None or len(V) < 2:
            self.before_plot.setTitle("Before Optimization")
            return
        # Plot edges
        for i, j in E:
            self.before_plot.plot(
                [V[i, 0].item(), V[j, 0].item()],
                [V[i, 1].item(), V[j, 1].item()],
                pen=pg.mkPen('b', width=2)
            )
        self.before_plot.plot(V[:,0].cpu().numpy(), V[:,1].cpu().numpy(), pen=None, symbol='o', symbolBrush='r', symbolSize=10)
        self.before_plot.setTitle("Before Optimization")
        # Center of Mass and Stability
        try:
            from shape_mass_center import calculate_center_of_mass
            from shape_stability import is_shape_stable
            E_tensor = torch.tensor(E, dtype=torch.long, device=V.device)
            area_b, com_b = calculate_center_of_mass(V, E_tensor)
            cx_b, cy_b = float(com_b[0].item()), float(com_b[1].item())
            self.before_plot.plot([cx_b], [cy_b], pen=None, symbol='+', symbolBrush='red', symbolPen='red', symbolSize=20)
            is_stable_b, _, _, _ = is_shape_stable(V, E_tensor)
            if is_stable_b:
                stability_text_b = pg.TextItem(text='Stable', color='green', anchor=(0.5, -0.5))
            else:
                stability_text_b = pg.TextItem(text='Will fall!', color='red', anchor=(0.5, -0.5))
            stability_text_b.setPos(cx_b, cy_b)
            self.before_plot.addItem(stability_text_b)
        except Exception as e:
            pass

    def run_optimization(self):
        shape = self.main_window.shape
        # Use last optimized vertices if available, else use original
        if self.last_optimized_vertices is not None:
            V_og = self.last_optimized_vertices
        else:
            V_og = shape.vertices
        if V_og is None or len(V_og) < 3 or len(shape.edges) < 3:
            self.info_label.setText("No valid shape loaded.")
            return
        E = torch.tensor(shape.edges, dtype=torch.long, device=V_og.device)
        V0 = V_og.clone()  # No perturbation
        def loss_fn(V):
            return total_loss(V, E, V_og, lambda1=0.33, lambda2=0.33, lambda3=0.34)
        max_iters = self.iter_spinbox.value()
        V_opt = gradient_descent(loss_fn, V0, lr=0.05, max_iters=max_iters, verbose=True)
        # Logging for debugging
        print("V_opt after optimization:", V_opt)
        print("Any NaN in V_opt?", torch.isnan(V_opt).any().item())
        final_loss = loss_fn(V_opt)
        print("Final loss:", final_loss.item())
        if torch.isnan(V_opt).any():
            self.info_label.setText("Optimization failed: NaN encountered in result.")
            return
        # Store the optimized vertices for possible re-optimization
        self.last_optimized_vertices = V_opt.detach()
        # Plot before (edges) -- show the previous input
        self.plot_current_shape(use_last_optimized=False)
        # Plot after (edges)
        self.after_plot.clear()
        self.after_plot.showGrid(x=True, y=True, alpha=0.3)
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
            from shape_mass_center import calculate_center_of_mass
            from shape_stability import is_shape_stable
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
        self.info_label.setText("Optimization complete! Run again to further optimize the result.")
        # Enable save button after successful optimization
        self.save_optimized_btn.setEnabled(True)

    def reset_to_original(self):
        self.last_optimized_vertices = None
        self.plot_current_shape(use_last_optimized=False)
        self.after_plot.clear()
        self.info_label.setText("Reset to original shape. Ready to optimize again.")
        # Disable save button when resetting
        self.save_optimized_btn.setEnabled(False)

    def save_optimized_shape(self):
        """Save the optimized shape to a JSON file."""
        if self.last_optimized_vertices is None:
            return
            
        from PyQt5.QtWidgets import QFileDialog
        import os
        
        # Get the shapes directory
        shapes_dir = os.path.join(os.path.dirname(__file__), '..', '..', 'shapes')
        
        # Create a new shape with optimized vertices
        optimized_shape = Shape2D()
        optimized_shape.vertices = self.last_optimized_vertices
        optimized_shape.edges = self.main_window.shape.edges  # Keep original edges
        
        # Open file dialog for saving
        path, _ = QFileDialog.getSaveFileName(
            self, 
            'Save Optimized Shape JSON', 
            shapes_dir, 
            'JSON Files (*.json)'
        )
        
        if path:
            optimized_shape.save_to_json(path)
            self.info_label.setText(f"Optimized shape saved to: {os.path.basename(path)}")
            # Refresh the shape dropdowns in other tabs
            if hasattr(self.main_window, 'visualize_tab'):
                self.main_window.visualize_tab.refresh_shape_dropdown()
            if hasattr(self.main_window, 'compare_tab'):
                self.main_window.compare_tab.refresh_shape_dropdowns() 