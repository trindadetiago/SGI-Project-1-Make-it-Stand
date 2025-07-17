import torch
from typing import List, Tuple, Union

# Accepts either Shape2D or torch.Tensor for vertices

def _get_vertices(shape_or_vertices) -> torch.Tensor:
    # Only check hasattr, don't reference Shape2D directly
    if hasattr(shape_or_vertices, 'vertices'):
        return shape_or_vertices.vertices
    return shape_or_vertices

def calculate_smoothing_score(vertices) -> torch.Tensor:
    V = _get_vertices(vertices)
    n = V.shape[0]
    if n < 3:
        return torch.tensor(0.0, dtype=V.dtype)
    f2 = torch.tensor(0.0, dtype=V.dtype)
    for i in range(n):
        v0 = V[(i - 1) % n]
        v1 = V[i]
        v2 = V[(i + 1) % n]
        if v1[1] == 0:
            continue
        midpoint = 0.5 * (v0 + v2)
        distance = torch.norm(v1 - midpoint, p=2)
        f2 = f2 + 0.5 * (distance ** 2)
    return f2

def smooth_shape(vertices, iterations: int = 1, strength: float = 0.5, edges=None) -> torch.Tensor:
    V = _get_vertices(vertices).clone()
    n = V.shape[0]
    for _ in range(iterations):
        new_V = V.clone()
        for i in range(n):
            v0 = V[(i - 1) % n]
            v1 = V[i]
            v2 = V[(i + 1) % n]
            if v1[1] == 0:
                new_V[i] = v1
                continue
            midpoint = 0.5 * (v0 + v2)
            new_V[i] = v1 + strength * (midpoint - v1)
        V = new_V
    return V
