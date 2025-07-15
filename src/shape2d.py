"""
2D Shape representation following 3D mesh structure.
- vertices: list of (x,y) coordinates
- edges: list of edge indices (pairs of vertex indices)
- boundaries: list of boundary edge sequences
"""

import numpy as np
import matplotlib.pyplot as plt
from shapely.geometry import Polygon
import matplotlib.patches as patches


class Shape2D:
    """
    A 2D shape represented using vertices, edges, and boundaries (like 3D meshes).
    """
    
    def __init__(self, vertices, edges=None, boundaries=None):
        """
        Initialize a 2D shape.
        
        Args:
            vertices: Array of (x, y) vertex coordinates
            edges: List of (v1, v2) edge indices (optional, will be inferred from boundaries)
            boundaries: List of boundary edge sequences (optional, will be inferred from edges)
        """
        self.vertices = np.array(vertices, dtype=float)
        self.nb_vertices = len(self.vertices)
        
        if edges is not None:
            self.edges = edges
        else:
            self.edges = []
            
        if boundaries is not None:
            self.boundaries = boundaries
        else:
            self.boundaries = []
            
        # If we have vertices but no edges/boundaries, create a simple boundary
        if len(self.edges) == 0 and len(self.boundaries) == 0:
            self._create_simple_boundary()
    
    def _create_simple_boundary(self):
        """Create a simple boundary from vertices (assumes they form a closed loop)."""
        # Create edges connecting consecutive vertices
        self.edges = []
        for i in range(self.nb_vertices):
            v1 = i
            v2 = (i + 1) % self.nb_vertices
            self.edges.append((v1, v2))
        
        # Create boundary as sequence of edge indices
        self.boundaries = [list(range(len(self.edges)))]
    
    def add_hole(self, hole_vertices):
        """
        Add a hole by creating a new boundary.
        
        Args:
            hole_vertices: Array of (x, y) vertices for the hole
        """
        # Add hole vertices to the vertex list
        start_idx = self.nb_vertices
        hole_vertices = np.array(hole_vertices, dtype=float)
        
        # Append hole vertices
        self.vertices = np.vstack([self.vertices, hole_vertices])
        
        # Create edges for the hole
        hole_edges = []
        for i in range(len(hole_vertices)):
            v1 = start_idx + i
            v2 = start_idx + ((i + 1) % len(hole_vertices))
            hole_edges.append((v1, v2))
        
        # Add hole edges to edge list
        edge_start_idx = len(self.edges)
        self.edges.extend(hole_edges)
        
        # Create boundary for the hole
        hole_boundary = list(range(edge_start_idx, edge_start_idx + len(hole_edges)))
        self.boundaries.append(hole_boundary)
        
        # Update vertex count
        self.nb_vertices = len(self.vertices)
    
    def get_boundary_vertices(self, boundary_idx=0):
        """
        Get vertices for a specific boundary.
        
        Args:
            boundary_idx: Index of the boundary (0 = outer, 1+ = holes)
            
        Returns:
            Array of vertex coordinates for the boundary
        """
        if boundary_idx >= len(self.boundaries):
            return np.array([])
        
        boundary_edges = self.boundaries[boundary_idx]
        boundary_vertices = []
        
        for edge_idx in boundary_edges:
            if edge_idx < len(self.edges):
                v1_idx = self.edges[edge_idx][0]
                boundary_vertices.append(self.vertices[v1_idx])
        
        return np.array(boundary_vertices)
    
    def center_of_mass(self):
        """
        Calculate the center of mass using Shapely.
        """
        polygon = self._create_shapely_polygon()
        return np.array(polygon.centroid.coords[0])
    
    def area(self):
        """
        Calculate the area using Shapely.
        """
        polygon = self._create_shapely_polygon()
        return polygon.area
    
    def _create_shapely_polygon(self):
        """
        Create a Shapely polygon from boundaries.
        """
        if len(self.boundaries) == 0:
            return Polygon([])
        
        # Get outer boundary
        outer_vertices = self.get_boundary_vertices(0)
        if len(outer_vertices) == 0:
            return Polygon([])
        
        # Get holes
        holes = []
        for i in range(1, len(self.boundaries)):
            hole_vertices = self.get_boundary_vertices(i)
            if len(hole_vertices) > 0:
                holes.append(hole_vertices)
        
        if holes:
            return Polygon(outer_vertices, holes=holes)
        else:
            return Polygon(outer_vertices)
    
    def visualize(self, show_vertices=True, show_com=True, show_support=True, 
                  title="2D Shape with Vertices and Edges"):
        """
        Visualize the shape with all vertices and edges clearly shown.
        """
        fig, ax = plt.subplots(figsize=(12, 10))
        
        # Colors for different boundaries
        colors = ['blue', 'red', 'green', 'orange', 'purple', 'brown']
        
        # Plot each boundary
        for boundary_idx, boundary in enumerate(self.boundaries):
            color = colors[boundary_idx % len(colors)]
            label = 'Outer Boundary' if boundary_idx == 0 else f'Hole {boundary_idx}'
            
            # Get vertices for this boundary
            boundary_vertices = self.get_boundary_vertices(boundary_idx)
            if len(boundary_vertices) == 0:
                continue
            
            # Plot boundary line
            ax.plot(boundary_vertices[:, 0], boundary_vertices[:, 1], 
                   color=color, linewidth=3, label=label)
            
            # Plot vertices for this boundary
            if show_vertices:
                ax.plot(boundary_vertices[:, 0], boundary_vertices[:, 1], 
                       color=color, marker='o', markersize=8, 
                       linestyle='none', alpha=0.8)
            
            # Plot edges for this boundary
            for edge_idx in boundary:
                if edge_idx < len(self.edges):
                    v1_idx, v2_idx = self.edges[edge_idx]
                    v1 = self.vertices[v1_idx]
                    v2 = self.vertices[v2_idx]
                    ax.plot([v1[0], v2[0]], [v1[1], v2[1]], 
                           color=color, linewidth=2, alpha=0.8)
        
        # Fill the shape using PathPatch for proper hole rendering
        from matplotlib.patches import PathPatch
        from matplotlib.path import Path
        from shapely.geometry import Polygon as ShapelyPolygon
        import matplotlib

        polygon = self._create_shapely_polygon()
        if not polygon.is_empty and polygon.is_valid:
            # Build the compound path (outer + holes)
            vertices = []
            codes = []

            # Outer boundary
            ext = np.asarray(polygon.exterior.coords)
            vertices.extend(ext)
            codes.extend([Path.MOVETO] + [Path.LINETO]*(len(ext)-2) + [Path.CLOSEPOLY])

            # Holes
            for interior in polygon.interiors:
                hole = np.asarray(interior.coords)
                vertices.extend(hole)
                codes.extend([Path.MOVETO] + [Path.LINETO]*(len(hole)-2) + [Path.CLOSEPOLY])

            path = Path(vertices, codes)
            patch = PathPatch(path, facecolor='lightblue', edgecolor='none', alpha=0.3)
            ax.add_patch(patch)
        else:
            # Simple fill for shapes without holes
            if len(self.boundaries) > 0:
                outer_vertices = self.get_boundary_vertices(0)
                if len(outer_vertices) > 0:
                    ax.fill(outer_vertices[:, 0], outer_vertices[:, 1], 
                           alpha=0.3, color='lightblue')
        
        # Plot center of mass
        if show_com:
            com = self.center_of_mass()
            ax.plot(com[0], com[1], 'go', markersize=12, label='Center of Mass')
            
            # Add text label
            ax.annotate(f'COM: ({com[0]:.2f}, {com[1]:.2f})', 
                       (com[0], com[1]), xytext=(10, 10), 
                       textcoords='offset points', fontsize=10,
                       bbox=dict(boxstyle='round,pad=0.3', facecolor='yellow', alpha=0.7))
        
        # Add support line (ground)
        if show_support:
            ax.axhline(y=0, color='k', linestyle='--', alpha=0.5, label='Ground')
        
        ax.set_aspect('equal')
        ax.grid(True, alpha=0.3)
        ax.legend()
        ax.set_title(title)
        ax.set_xlabel('X')
        ax.set_ylabel('Y')
        
        plt.tight_layout()
        plt.show()
    
    def get_bounding_box(self):
        """Get the bounding box of the shape."""
        if self.nb_vertices == 0:
            return 0, 0, 0, 0
        
        x_min, y_min = np.min(self.vertices, axis=0)
        x_max, y_max = np.max(self.vertices, axis=0)
        return x_min, y_min, x_max, y_max
    
    def __str__(self):
        """String representation of the shape."""
        if self.nb_vertices == 0:
            return "Shape2D (empty)"
        
        com = self.center_of_mass()
        area = self.area()
        hole_info = f", {len(self.boundaries)-1} holes" if len(self.boundaries) > 1 else ""
        return f"Shape2D with {self.nb_vertices} vertices, {len(self.edges)} edges{hole_info}, COM: ({com[0]:.2f}, {com[1]:.2f}), Area: {area:.2f}" 