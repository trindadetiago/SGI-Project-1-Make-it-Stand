"""
Center of Mass Calculator for 2D Shapes
Calculates the center of mass for 2D polygonal shapes with holes.
"""

import json
import numpy as np
from shapely.geometry import Polygon
from shapely.ops import unary_union

class CenterOfMassCalculator:
    def __init__(self):
        pass
    
    def load_mesh(self, mesh_path):
        """Load a mesh from JSON file."""
        with open(mesh_path, 'r') as f:
            mesh = json.load(f)
        return mesh
    
    def calculate_center_of_mass(self, mesh):
        """
        Calculate the center of mass of a 2D shape with holes.
        
        Args:
            mesh: Dictionary containing vertices, edges, and boundaries
            
        Returns:
            tuple: (center_x, center_y, area)
        """
        vertices = np.array(mesh['vertices'])
        edges = mesh['edges']
        boundaries = mesh['boundaries']
        
        if not boundaries:
            return (0, 0, 0)
        
        # Create Shapely polygon from outer boundary
        outer_boundary = boundaries[0]
        outer_coords = []
        for edge_idx in outer_boundary:
            v1, v2 = edges[edge_idx]
            outer_coords.append(vertices[v1])
        
        # Close the polygon if needed
        if len(outer_coords) > 0 and not np.allclose(outer_coords[0], outer_coords[-1]):
            outer_coords.append(outer_coords[0])
        
        # Create outer polygon
        outer_polygon = Polygon(outer_coords)
        
        # Create holes
        holes = []
        for boundary in boundaries[1:]:
            hole_coords = []
            for edge_idx in boundary:
                v1, v2 = edges[edge_idx]
                hole_coords.append(vertices[v1])
            
            # Close the hole if needed
            if len(hole_coords) > 0 and not np.allclose(hole_coords[0], hole_coords[-1]):
                hole_coords.append(hole_coords[0])
            
            if len(hole_coords) >= 3:  # Need at least 3 points for a valid polygon
                hole_polygon = Polygon(hole_coords)
                if hole_polygon.is_valid:
                    holes.append(hole_polygon)
        
        # Create final polygon with holes
        if holes:
            # Subtract holes from outer polygon
            final_polygon = outer_polygon
            for hole in holes:
                final_polygon = final_polygon.difference(hole)
        else:
            final_polygon = outer_polygon
        
        # Calculate center of mass and area
        if final_polygon.is_valid and not final_polygon.is_empty:
            centroid = final_polygon.centroid
            area = final_polygon.area
            return (centroid.x, centroid.y, area)
        else:
            return (0, 0, 0)
    
    def analyze_mesh(self, mesh_path):
        """Load mesh and calculate center of mass."""
        mesh = self.load_mesh(mesh_path)
        center_x, center_y, area = self.calculate_center_of_mass(mesh)
        
        print(f"Mesh: {mesh.get('name', 'Unknown')}")
        print(f"Center of Mass: ({center_x:.3f}, {center_y:.3f})")
        print(f"Area: {area:.3f}")
        print(f"Number of vertices: {len(mesh['vertices'])}")
        print(f"Number of edges: {len(mesh['edges'])}")
        print(f"Number of boundaries: {len(mesh['boundaries'])}")
        if len(mesh['boundaries']) > 1:
            print(f"Number of holes: {len(mesh['boundaries']) - 1}")
        print("-" * 40)
        
        return center_x, center_y, area

def main():
    """Test the center of mass calculator with all available meshes."""
    calculator = CenterOfMassCalculator()
    
    import os
    mesh_dir = 'meshes'
    mesh_files = [f for f in os.listdir(mesh_dir) if f.endswith('.json')]
    mesh_files.sort()
    
    print("Center of Mass Analysis")
    print("=" * 40)
    
    for mesh_file in mesh_files:
        mesh_path = os.path.join(mesh_dir, mesh_file)
        try:
            calculator.analyze_mesh(mesh_path)
        except Exception as e:
            print(f"Error processing {mesh_file}: {e}")
            print("-" * 40)

if __name__ == '__main__':
    main() 