"""
Interactive Shape Drawing Tool
Draw 2D shapes by clicking to add vertices and creating edges.
"""

import sys
import json
import numpy as np
from PyQt5 import QtWidgets, QtCore
import pyqtgraph as pg
from pyqtgraph.Qt import QtCore

class ShapeDrawer(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle('2D Shape Drawer')
        self.resize(1000, 800)
        
        # Central widget and layout
        self.central_widget = QtWidgets.QWidget()
        self.setCentralWidget(self.central_widget)
        self.layout = QtWidgets.QHBoxLayout(self.central_widget)
        
        # Left panel for controls
        self.control_panel = QtWidgets.QVBoxLayout()
        self.layout.addLayout(self.control_panel)
        
        # Drawing area
        self.plot_widget = pg.PlotWidget()
        self.layout.addWidget(self.plot_widget, stretch=1)
        self.plot_widget.setAspectLocked(True)
        self.plot_widget.showGrid(x=True, y=True, alpha=0.3)
        
        # Initialize data
        self.vertices = []
        self.edges = []
        self.boundaries = []
        self.current_boundary = []
        self.mode = 'add_vertex'  # 'add_vertex', 'add_edge', 'create_boundary'
        
        # Setup controls
        self.setup_controls()
        
        # Connect mouse events
        self.plot_widget.scene().sigMouseClicked.connect(self.mouse_clicked)
        
        # Update display
        self.update_display()
    
    def setup_controls(self):
        """Setup the control panel."""
        # Mode selection
        mode_group = QtWidgets.QGroupBox("Drawing Mode")
        mode_layout = QtWidgets.QVBoxLayout()
        
        self.add_vertex_btn = QtWidgets.QPushButton("Add Vertex")
        self.add_vertex_btn.setCheckable(True)
        self.add_vertex_btn.setChecked(True)
        self.add_vertex_btn.clicked.connect(lambda: self.set_mode('add_vertex'))
        
        self.add_edge_btn = QtWidgets.QPushButton("Add Edge")
        self.add_edge_btn.setCheckable(True)
        self.add_edge_btn.clicked.connect(lambda: self.set_mode('add_edge'))
        
        self.create_boundary_btn = QtWidgets.QPushButton("Create Boundary")
        self.create_boundary_btn.setCheckable(True)
        self.create_boundary_btn.clicked.connect(lambda: self.set_mode('create_boundary'))
        
        mode_layout.addWidget(self.add_vertex_btn)
        mode_layout.addWidget(self.add_edge_btn)
        mode_layout.addWidget(self.create_boundary_btn)
        mode_group.setLayout(mode_layout)
        self.control_panel.addWidget(mode_group)
        
        # Boundary controls
        boundary_group = QtWidgets.QGroupBox("Boundary Controls")
        boundary_layout = QtWidgets.QVBoxLayout()
        
        self.finish_boundary_btn = QtWidgets.QPushButton("Finish Current Boundary")
        self.finish_boundary_btn.clicked.connect(self.finish_boundary)
        
        self.clear_boundary_btn = QtWidgets.QPushButton("Clear Current Boundary")
        self.clear_boundary_btn.clicked.connect(self.clear_current_boundary)
        
        boundary_layout.addWidget(self.finish_boundary_btn)
        boundary_layout.addWidget(self.clear_boundary_btn)
        boundary_group.setLayout(boundary_layout)
        self.control_panel.addWidget(boundary_group)
        
        # Shape controls
        shape_group = QtWidgets.QGroupBox("Shape Controls")
        shape_layout = QtWidgets.QVBoxLayout()
        
        self.clear_all_btn = QtWidgets.QPushButton("Clear All")
        self.clear_all_btn.clicked.connect(self.clear_all)
        
        self.save_btn = QtWidgets.QPushButton("Save Shape")
        self.save_btn.clicked.connect(self.save_shape)
        
        shape_layout.addWidget(self.clear_all_btn)
        shape_layout.addWidget(self.save_btn)
        shape_group.setLayout(shape_layout)
        self.control_panel.addWidget(shape_group)
        
        # Status display
        self.status_label = QtWidgets.QLabel("Status: Ready to add vertices")
        self.control_panel.addWidget(self.status_label)
        
        # Info display
        info_group = QtWidgets.QGroupBox("Shape Info")
        info_layout = QtWidgets.QVBoxLayout()
        self.info_label = QtWidgets.QLabel("Vertices: 0\nEdges: 0\nBoundaries: 0")
        info_layout.addWidget(self.info_label)
        info_group.setLayout(info_layout)
        self.control_panel.addWidget(info_group)
    
    def set_mode(self, mode):
        """Set the current drawing mode."""
        self.mode = mode
        
        # Update button states
        self.add_vertex_btn.setChecked(mode == 'add_vertex')
        self.add_edge_btn.setChecked(mode == 'add_edge')
        self.create_boundary_btn.setChecked(mode == 'create_boundary')
        
        # Update status
        if mode == 'add_vertex':
            self.status_label.setText("Status: Click to add vertices")
        elif mode == 'add_edge':
            self.status_label.setText("Status: Click two vertices to create an edge")
        elif mode == 'create_boundary':
            self.status_label.setText("Status: Click vertices in order to create boundary")
    
    def mouse_clicked(self, event):
        """Handle mouse clicks."""
        if event.button() == QtCore.Qt.LeftButton:
            pos = self.plot_widget.getViewBox().mapSceneToView(event.scenePos())
            x, y = pos.x(), pos.y()
            
            if self.mode == 'add_vertex':
                self.add_vertex(x, y)
            elif self.mode == 'add_edge':
                self.handle_edge_click(x, y)
            elif self.mode == 'create_boundary':
                self.handle_boundary_click(x, y)
    
    def add_vertex(self, x, y):
        """Add a new vertex at the given position."""
        self.vertices.append([x, y])
        self.update_display()
        self.update_info()
    
    def handle_edge_click(self, x, y):
        """Handle clicks in edge mode."""
        # Find closest vertex
        closest_vertex = self.find_closest_vertex(x, y)
        if closest_vertex is not None:
            if not hasattr(self, 'edge_start_vertex'):
                self.edge_start_vertex = closest_vertex
                self.status_label.setText(f"Status: Edge started at vertex {closest_vertex}. Click second vertex.")
            else:
                # Create edge
                if self.edge_start_vertex != closest_vertex:
                    self.edges.append([self.edge_start_vertex, closest_vertex])
                    self.update_display()
                    self.update_info()
                delattr(self, 'edge_start_vertex')
                self.status_label.setText("Status: Edge created. Click to start new edge.")
    
    def handle_boundary_click(self, x, y):
        """Handle clicks in boundary mode."""
        closest_vertex = self.find_closest_vertex(x, y)
        if closest_vertex is not None:
            if closest_vertex not in self.current_boundary:
                self.current_boundary.append(closest_vertex)
                self.status_label.setText(f"Status: Boundary has {len(self.current_boundary)} vertices. Click 'Finish' when done.")
                self.update_display()
            else:
                self.status_label.setText("Status: Vertex already in boundary. Choose different vertex.")
    
    def find_closest_vertex(self, x, y, max_distance=0.5):
        """Find the closest vertex to the given position."""
        if not self.vertices:
            return None
        
        min_distance = float('inf')
        closest_vertex = None
        
        for i, vertex in enumerate(self.vertices):
            distance = np.sqrt((vertex[0] - x)**2 + (vertex[1] - y)**2)
            if distance < min_distance and distance < max_distance:
                min_distance = distance
                closest_vertex = i
        
        return closest_vertex
    
    def finish_boundary(self):
        """Finish the current boundary."""
        if len(self.current_boundary) >= 3:
            self.boundaries.append(self.current_boundary.copy())
            self.current_boundary = []
            self.update_display()
            self.update_info()
            self.status_label.setText("Status: Boundary finished. Start new boundary or save shape.")
        else:
            self.status_label.setText("Status: Need at least 3 vertices for a boundary.")
    
    def clear_current_boundary(self):
        """Clear the current boundary."""
        self.current_boundary = []
        self.update_display()
        self.status_label.setText("Status: Current boundary cleared.")
    
    def clear_all(self):
        """Clear all vertices, edges, and boundaries."""
        self.vertices = []
        self.edges = []
        self.boundaries = []
        self.current_boundary = []
        if hasattr(self, 'edge_start_vertex'):
            delattr(self, 'edge_start_vertex')
        self.update_display()
        self.update_info()
        self.status_label.setText("Status: All cleared. Ready to start new shape.")
    
    def update_display(self):
        """Update the visual display."""
        self.plot_widget.clear()
        
        # Draw vertices
        if self.vertices:
            vertices = np.array(self.vertices)
            self.plot_widget.plot(vertices[:, 0], vertices[:, 1], 
                                pen=None, symbol='o', symbolBrush='blue', 
                                symbolPen='black', symbolSize=10)
            
            # Draw vertex numbers
            for i, vertex in enumerate(self.vertices):
                text = pg.TextItem(text=str(i), color='black')
                self.plot_widget.addItem(text)
                text.setPos(vertex[0] + 0.1, vertex[1] + 0.1)
        
        # Draw edges
        for edge in self.edges:
            v1, v2 = edge
            x = [self.vertices[v1][0], self.vertices[v2][0]]
            y = [self.vertices[v1][1], self.vertices[v2][1]]
            self.plot_widget.plot(x, y, pen=pg.mkPen('red', width=3))
        
        # Draw completed boundaries
        for i, boundary in enumerate(self.boundaries):
            if len(boundary) >= 3:
                boundary_vertices = [self.vertices[j] for j in boundary]
                boundary_vertices.append(boundary_vertices[0])  # Close the loop
                boundary_vertices = np.array(boundary_vertices)
                color = 'green' if i == 0 else 'red'  # Outer boundary green, holes red
                self.plot_widget.plot(boundary_vertices[:, 0], boundary_vertices[:, 1], 
                                    pen=pg.mkPen(color, width=3))
        
        # Draw current boundary
        if self.current_boundary:
            current_vertices = [self.vertices[j] for j in self.current_boundary]
            if len(current_vertices) >= 2:
                current_vertices = np.array(current_vertices)
                self.plot_widget.plot(current_vertices[:, 0], current_vertices[:, 1], 
                                    pen=pg.mkPen('orange', width=2, style=QtCore.Qt.DashLine))
    
    def update_info(self):
        """Update the info display."""
        self.info_label.setText(f"Vertices: {len(self.vertices)}\nEdges: {len(self.edges)}\nBoundaries: {len(self.boundaries)}")
    
    def save_shape(self):
        """Save the current shape as a JSON file."""
        if not self.boundaries:
            QtWidgets.QMessageBox.warning(self, "Warning", "No boundaries defined. Cannot save shape.")
            return
        
        # Create mesh data
        mesh_data = {
            "name": "Drawn Shape",
            "description": "Shape drawn with the interactive tool",
            "vertices": self.vertices,
            "edges": self.edges,
            "boundaries": self.boundaries
        }
        
        # Get filename from user
        filename, _ = QtWidgets.QFileDialog.getSaveFileName(
            self, "Save Shape", "meshes/drawn_shape.json", "JSON files (*.json)")
        
        if filename:
            try:
                with open(filename, 'w') as f:
                    json.dump(mesh_data, f, indent=2)
                QtWidgets.QMessageBox.information(self, "Success", f"Shape saved to {filename}")
            except Exception as e:
                QtWidgets.QMessageBox.critical(self, "Error", f"Failed to save shape: {str(e)}")

def main():
    app = QtWidgets.QApplication(sys.argv)
    win = ShapeDrawer()
    win.show()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main() 