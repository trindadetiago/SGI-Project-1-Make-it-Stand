"""
Support Polygon Analyzer for 2D Shapes
Determines the support polygon (convex hull) and balance state of 2D shapes.
"""

import json
import numpy as np
from shapely.geometry import Polygon, Point
from scipy.spatial import ConvexHull
import matplotlib.pyplot as plt
import matplotlib.patches as patches
from matplotlib.path import Path

class SupportAnalyzer:
    def __init__(self, gravity_direction=(0, -1)):
        """
        Initialize the support analyzer.
        
        Args:
            gravity_direction: Direction of gravity as (dx, dy) vector
        """
        self.gravity_direction = np.array(gravity_direction)
        self.gravity_direction = self.gravity_direction / np.linalg.norm(self.gravity_direction)
    
    def load_mesh(self, mesh_path):
        """Load a mesh from JSON file."""
        with open(mesh_path, 'r') as f:
            mesh = json.load(f)
        return mesh
    
    def auto_rotate_to_ground(self, mesh):
        """
        Auto-rotate the mesh so the lowest edge is horizontal (parallel to x-axis).
        Returns the rotated mesh and the rotation angle.
        """
        vertices = np.array(mesh['vertices'])
        edges = mesh['edges']
        boundaries = mesh['boundaries']
        
        if not boundaries or len(vertices) == 0:
            return mesh, 0.0
        
        # Find the lowest edge (edge with minimum average y-coordinate)
        min_y_avg = float('inf')
        lowest_edge = None
        
        for edge in edges:
            v1, v2 = edge
            y_avg = (vertices[v1][1] + vertices[v2][1]) / 2
            if y_avg < min_y_avg:
                min_y_avg = y_avg
                lowest_edge = edge
        
        if lowest_edge is None:
            return mesh, 0.0
        
        # Calculate the angle of the lowest edge
        v1, v2 = lowest_edge
        p1 = vertices[v1]
        p2 = vertices[v2]
        
        # Vector of the edge
        edge_vector = np.array([p2[0] - p1[0], p2[1] - p1[1]])
        
        # Calculate angle to make it horizontal
        angle = -np.arctan2(edge_vector[1], edge_vector[0])
        
        # Rotation matrix
        cos_a = np.cos(angle)
        sin_a = np.sin(angle)
        rotation_matrix = np.array([[cos_a, -sin_a], [sin_a, cos_a]])
        
        # Rotate all vertices
        rotated_vertices = []
        for vertex in vertices:
            rotated_vertex = rotation_matrix @ vertex
            rotated_vertices.append(rotated_vertex.tolist())
        
        # Create rotated mesh
        rotated_mesh = {
            'name': mesh.get('name', '') + ' (rotated)',
            'description': mesh.get('description', ''),
            'vertices': rotated_vertices,
            'edges': edges,
            'boundaries': boundaries
        }
        
        return rotated_mesh, angle

    def calculate_support_polygon(self, mesh):
        """
        Calculate the support region as convex hull of lowest vertices.
        """
        vertices = np.array(mesh['vertices'])
        boundaries = mesh['boundaries']
        
        if not boundaries or len(vertices) == 0:
            return [], 0, 0
        
        # Find lowest y-coordinate
        lowest_y = np.min(vertices[:, 1])
        tolerance = 1e-6
        
        # Find all vertices at the lowest level
        lowest_vertices = []
        for i, vertex in enumerate(vertices):
            if abs(vertex[1] - lowest_y) < tolerance:
                lowest_vertices.append(vertex)
        
        if len(lowest_vertices) < 2:
            return [], 0, lowest_y
        
        # In 2D, the "convex hull" of lowest vertices is just the line segment
        # from leftmost to rightmost vertex at the lowest y
        lowest_vertices = np.array(lowest_vertices)
        
        # Sort by x-coordinate to get leftmost and rightmost
        sorted_indices = np.argsort(lowest_vertices[:, 0])
        leftmost = lowest_vertices[sorted_indices[0]]
        rightmost = lowest_vertices[sorted_indices[-1]]
        
        # Create support region as line segment
        support_vertices = np.array([leftmost, rightmost])
        support_length = np.linalg.norm(rightmost - leftmost)
        
        return support_vertices, support_length, lowest_y

    def is_balanced(self, center_of_mass, support_vertices):
        """
        Check if the center of mass is above the support region (line segment in 2D).
        """
        if len(support_vertices) < 2:
            return False
        
        # For 2D, support_vertices is a line segment [leftmost, rightmost]
        leftmost = support_vertices[0]
        rightmost = support_vertices[1]
        
        # Check if center of mass x is between leftmost and rightmost x
        x = center_of_mass[0]
        x_min = min(leftmost[0], rightmost[0])
        x_max = max(leftmost[0], rightmost[0])
        
        return x_min - 1e-8 <= x <= x_max + 1e-8

    def analyze_mesh(self, mesh_path):
        """Complete analysis of a mesh's support and balance with auto-rotation."""
        mesh = self.load_mesh(mesh_path)
        
        # Auto-rotate to align with ground
        rotated_mesh, rotation_angle = self.auto_rotate_to_ground(mesh)
        
        # Calculate center of mass of rotated mesh
        from center_of_mass import CenterOfMassCalculator
        com_calc = CenterOfMassCalculator()
        center_x, center_y, area = com_calc.calculate_center_of_mass(rotated_mesh)
        center_of_mass = (center_x, center_y)
        
        # Calculate support region (convex hull of lowest vertices)
        support_vertices, support_length, lowest_y = self.calculate_support_polygon(rotated_mesh)
        
        # Check balance
        is_balanced = self.is_balanced(center_of_mass, support_vertices)
        
        # Print results
        print(f"Mesh: {mesh.get('name', 'Unknown')}")
        print(f"Rotation angle: {np.degrees(rotation_angle):.1f}°")
        print(f"Center of Mass: ({center_x:.3f}, {center_y:.3f})")
        print(f"Shape Area: {area:.3f}")
        print(f"Support Region Length: {support_length:.3f}")
        print(f"Lowest Y: {lowest_y:.3f}")
        print(f"Number of Support Vertices: {len(support_vertices)}")
        print(f"Balanced: {'YES' if is_balanced else 'NO'}")
        
        if is_balanced:
            print("Status: STABLE (green)")
        else:
            print("Status: WILL FALL (red)")
        
        print("-" * 40)
        
        return {
            'center_of_mass': center_of_mass,
            'support_vertices': support_vertices,
            'support_length': support_length,
            'lowest_y': lowest_y,
            'is_balanced': is_balanced,
            'shape_area': area,
            'rotation_angle': rotation_angle,
            'rotated_mesh': rotated_mesh
        }

def main():
    """Test the support analyzer with all available meshes."""
    analyzer = SupportAnalyzer()
    
    import os
    mesh_dir = 'meshes'
    mesh_files = [f for f in os.listdir(mesh_dir) if f.endswith('.json')]
    mesh_files.sort()
    
    print("Support Polygon and Balance Analysis")
    print("=" * 50)
    
    for mesh_file in mesh_files:
        mesh_path = os.path.join(mesh_dir, mesh_file)
        try:
            analyzer.analyze_mesh(mesh_path)
        except Exception as e:
            print(f"Error processing {mesh_file}: {e}")
            print("-" * 40)

if __name__ == '__main__':
    main() 