import json
from typing import List, Tuple

class Shape2D:
    """
    Represents a 2D shape defined by vertices and edges.
    Vertices: List of (x, y) tuples.
    Edges: List of (start_idx, end_idx) tuples (indices into vertices).
    """
    def __init__(self, vertices: List[Tuple[float, float]] = None, edges: List[Tuple[int, int]] = None):
        self.vertices = vertices if vertices is not None else []
        self.edges = edges if edges is not None else []

    @staticmethod
    def load_from_json(path: str) -> 'Shape2D':
        """Load shape from a JSON file."""
        with open(path, 'r') as f:
            data = json.load(f)
        vertices = [tuple(v) for v in data['vertices']]
        edges = [tuple(e) for e in data['edges']]
        return Shape2D(vertices, edges)

    def save_to_json(self, path: str):
        """Save shape to a JSON file."""
        data = {
            'vertices': self.vertices,
            'edges': self.edges
        }
        with open(path, 'w') as f:
            json.dump(data, f, indent=2)
