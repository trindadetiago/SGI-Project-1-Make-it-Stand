import torch
from typing import List, Tuple

def _get_vertices(shape_or_vertices):
    if hasattr(shape_or_vertices, 'vertices'):
        return shape_or_vertices.vertices
    return shape_or_vertices

def calculate_shape_similarity(modified_shape, original_shape) -> torch.Tensor:
    V_prime = _get_vertices(modified_shape)
    V_original = _get_vertices(original_shape)
    if V_prime.shape != V_original.shape:
        raise ValueError("Shapes must have the same number of vertices for similarity calculation")
    D = V_prime - V_original
    norm_D = torch.norm(D, p=2)
    f3 = 0.5 * (norm_D ** 2)
    return f3
