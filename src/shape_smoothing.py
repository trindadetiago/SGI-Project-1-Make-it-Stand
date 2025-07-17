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

def calculate_vertex_smoothing_scores(vertices) -> torch.Tensor:
    V = _get_vertices(vertices)
    n = V.shape[0]
    if n < 3:
        return torch.zeros(n, dtype=V.dtype)
    scores = torch.zeros(n, dtype=V.dtype)
    for i in range(n):
        v0 = V[(i - 1) % n]
        v1 = V[i]
        v2 = V[(i + 1) % n]
        if v1[1] == 0:
            scores[i] = 0.0
            continue
        midpoint = 0.5 * (v0 + v2)
        distance = torch.norm(v1 - midpoint, p=2)
        scores[i] = 0.5 * (distance ** 2)
    return scores

def find_roughest_vertices(vertices, num_vertices: int = 5) -> List[Tuple[int, float]]:
    scores = calculate_vertex_smoothing_scores(vertices)
    vertex_scores = [(i, float(scores[i].item())) for i in range(len(scores))]
    vertex_scores.sort(key=lambda x: x[1], reverse=True)
    return vertex_scores[:num_vertices]

def adaptive_smooth_shape(vertices, target_smoothness: float = 0.1, max_iterations: int = 10, edges=None) -> torch.Tensor:
    V = _get_vertices(vertices).clone()
    for iteration in range(max_iterations):
        current_score = calculate_smoothing_score(V)
        if current_score.item() <= target_smoothness:
            break
        V = smooth_shape(V, iterations=1, strength=0.5)
    return V

def test_smoothing_functions():
    vertices = torch.tensor([
        (0.0, 0.0),   # Base
        (1.0, 0.0),   # Base
        (1.0, 1.0),   # Normal
        (1.2, 1.5),   # Spike
        (1.0, 2.0),   # Normal
        (0.0, 2.0),   # Normal
        (0.0, 1.0),   # Normal
    ], dtype=torch.float32)
    print("=== Shape Smoothing Demo ===\n")
    initial_score = calculate_smoothing_score(vertices).item()
    print(f"Initial smoothing score: {initial_score:.6f}")
    roughest = find_roughest_vertices(vertices, 3)
    print(f"Roughest vertices: {roughest}")
    smoothed_vertices = smooth_shape(vertices, iterations=3, strength=0.5)
    smoothed_score = calculate_smoothing_score(smoothed_vertices).item()
    print(f"Smoothed score: {smoothed_score:.6f}")
    print(f"Improvement: {initial_score - smoothed_score:.6f}")
    adaptive_vertices = adaptive_smooth_shape(vertices, target_smoothness=0.1)
    adaptive_score = calculate_smoothing_score(adaptive_vertices).item()
    print(f"Adaptive smoothed score: {adaptive_score:.6f}")

if __name__ == "__main__":
    test_smoothing_functions() 