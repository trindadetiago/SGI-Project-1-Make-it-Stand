"""
Simple PyQtGraph-based visualizer to load and display 2D meshes with boundaries and holes.
"""

import sys
import os
import json
import numpy as np
from PyQt5 import QtWidgets
import pyqtgraph as pg
from pyqtgraph.Qt import QtCore
from shapely.geometry import Polygon
from support_analyzer import SupportAnalyzer

MESH_DIR = 'meshes'

class MeshVisualizer(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle('2D Mesh Visualizer (PyQtGraph)')
        self.resize(800, 800)
        self.central_widget = QtWidgets.QWidget()
        self.setCentralWidget(self.central_widget)
        self.layout = QtWidgets.QVBoxLayout(self.central_widget)
        self.support_analyzer = SupportAnalyzer()


        # Dropdown to select mesh
        self.mesh_selector = QtWidgets.QComboBox()
        self.layout.addWidget(self.mesh_selector)
        self.mesh_selector.currentIndexChanged.connect(self.load_selected_mesh)

        # Graphics view
        self.plot_widget = pg.PlotWidget()
        self.layout.addWidget(self.plot_widget, stretch=1)
        self.plot_widget.setAspectLocked(True)
        self.plot_widget.showGrid(x=True, y=True, alpha=0.3)

        # Load mesh files
        self.mesh_files = [f for f in os.listdir(MESH_DIR) if f.endswith('.json')]
        self.mesh_files.sort()
        for f in self.mesh_files:
            self.mesh_selector.addItem(f)

        self.mesh_data = None
        self.analysis = None
        if self.mesh_files:
            self.load_selected_mesh(0)

    def load_selected_mesh(self, idx):
        if idx < 0 or idx >= len(self.mesh_files):
            return
        mesh_path = os.path.join(MESH_DIR, self.mesh_files[idx])
        with open(mesh_path, 'r') as f:
            mesh = json.load(f)
        self.mesh_data = mesh
        self.analysis = self.support_analyzer.analyze_mesh(mesh_path)
        
        # Use the rotated mesh for visualization
        if self.analysis and 'rotated_mesh' in self.analysis:
            self.mesh_data = self.analysis['rotated_mesh']
        
        self.plot_mesh()

    def plot_mesh(self):
        self.plot_widget.clear()
        mesh = self.mesh_data
        if mesh is None:
            return
        vertices = np.array(mesh['vertices'])
        edges = mesh['edges']
        boundaries = mesh['boundaries']

        # Draw filled polygons - only fill outer boundary, holes stay transparent
        if len(boundaries) > 0:
            # Fill only the outer boundary (first boundary)
            outer_boundary = boundaries[0]
            outer_vertices = []
            for edge_idx in outer_boundary:
                v1, v2 = edges[edge_idx]
                outer_vertices.append(vertices[v1])
            if len(outer_vertices) > 2:
                # Create polygon points (close the loop)
                outer_vertices = np.array(outer_vertices)
                if not np.allclose(outer_vertices[0], outer_vertices[-1]):
                    outer_vertices = np.vstack([outer_vertices, outer_vertices[0]])
                # Create filled polygon for outer boundary only
                x = outer_vertices[:, 0]
                y = outer_vertices[:, 1]
                self.plot_widget.plot(x, y, 
                                    pen=pg.mkPen('blue', width=2),
                                    fillLevel=0, 
                                    fillBrush=pg.mkBrush('lightblue', alpha=0.3))
        # Draw hole boundaries with black fill to "cut out" the holes
        for i, boundary in enumerate(boundaries[1:], 1):  # Skip first (outer) boundary
            # Get vertices for this hole boundary
            hole_vertices = []
            for edge_idx in boundary:
                v1, v2 = edges[edge_idx]
                hole_vertices.append(vertices[v1])
            if len(hole_vertices) > 2:
                # Create polygon points (close the loop)
                hole_vertices = np.array(hole_vertices)
                if not np.allclose(hole_vertices[0], hole_vertices[-1]):
                    hole_vertices = np.vstack([hole_vertices, hole_vertices[0]])
                # Draw hole boundary with black fill to create "cutout" effect
                x = hole_vertices[:, 0]
                y = hole_vertices[:, 1]
                self.plot_widget.plot(x, y, 
                                    pen=pg.mkPen('red', width=2),
                                    fillLevel=0, 
                                    fillBrush=pg.mkBrush('black', alpha=1.0))  # Black fill to "cut out"
        # Draw all edges with black color for clarity
        for edge in edges:
            v1, v2 = edge
            x = [vertices[v1][0], vertices[v2][0]]
            y = [vertices[v1][1], vertices[v2][1]]
            self.plot_widget.plot(x, y, pen=pg.mkPen('k', width=1))
        # Draw vertices
        self.plot_widget.plot(vertices[:, 0], vertices[:, 1], 
                             pen=None, symbol='o', symbolBrush='w', 
                             symbolPen='k', symbolSize=10)
        # Draw center of mass
        if self.analysis is not None:
            cx, cy = self.analysis['center_of_mass']
            # Draw center of mass as a large red cross
            self.plot_widget.plot([cx], [cy], 
                                 pen=None, symbol='+', symbolBrush='red', 
                                 symbolPen='red', symbolSize=15)
            # Add text label
            text = pg.TextItem(text=f'CoM: ({cx:.2f}, {cy:.2f})', 
                              color='red', anchor=(0.5, 1.5))
            self.plot_widget.addItem(text)
            text.setPos(cx, cy)
        # Draw support region (convex hull of lowest vertices)
        if self.analysis is not None and self.analysis['support_vertices'] is not None and len(self.analysis['support_vertices']) >= 2:
            sp = np.array(self.analysis['support_vertices'])
            sp_closed = np.vstack([sp, sp[0]])  # Close the polygon
            color = 'orange' if self.analysis['is_balanced'] else 'red'
            # Draw filled support polygon
            self.plot_widget.plot(sp_closed[:, 0], sp_closed[:, 1], 
                                pen=pg.mkPen(color, width=5),
                                fillLevel=0, 
                                fillBrush=pg.mkBrush(color, alpha=0.3))
        # Draw balance status
        if self.analysis is not None and self.analysis['is_balanced'] is not None:
            status = 'BALANCED' if self.analysis['is_balanced'] else 'WILL FALL'
            color = 'green' if self.analysis['is_balanced'] else 'red'
            status_text = pg.TextItem(text=status, color=color, anchor=(0.5, 0))
            self.plot_widget.addItem(status_text)
            # Place status at bottom center
            if vertices.shape[0] > 0:
                minx, maxx = np.min(vertices[:, 0]), np.max(vertices[:, 0])
                miny = np.min(vertices[:, 1])
                status_text.setPos((minx + maxx) / 2, miny - 0.2 * (np.max(vertices[:, 1]) - miny + 1))
        # Draw mesh name
        if 'name' in mesh:
            self.plot_widget.setTitle(mesh['name'])
        else:
            self.plot_widget.setTitle('')

if __name__ == '__main__':
    app = QtWidgets.QApplication(sys.argv)
    win = MeshVisualizer()
    win.show()
    sys.exit(app.exec_()) 