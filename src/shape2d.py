import json
import torch
from typing import Optional, List, Tuple

class Shape2D:
    """
    Represents a 2D shape defined by vertices and edges.
    Vertices: torch.Tensor of shape (n, 2), dtype=torch.float32.
    Edges: List of (start_idx, end_idx) tuples (indices into vertices).
    """
    def __init__(self, vertices: Optional[torch.Tensor] = None, edges: Optional[List[Tuple[int, int]]] = None):
        if vertices is None:
            self.vertices = torch.empty((0, 2), dtype=torch.float32)
        elif isinstance(vertices, list):
            self.vertices = torch.tensor(vertices, dtype=torch.float32)
        else:
            self.vertices = vertices  # assume torch.Tensor
        self.edges = edges if edges is not None else []

    @staticmethod
    def load_from_json(path: str) -> 'Shape2D':
        """Load shape from a JSON file."""
        with open(path, 'r') as f:
            data = json.load(f)
        vertices = torch.tensor(data['vertices'], dtype=torch.float32)
        edges = [tuple(e) for e in data['edges']]
        return Shape2D(vertices, edges)

    def save_to_json(self, path: str):
        """Save shape to a JSON file."""
        data = {
            'vertices': self.vertices.tolist(),
            'edges': self.edges
        }
        with open(path, 'w') as f:
            json.dump(data, f, indent=2)
