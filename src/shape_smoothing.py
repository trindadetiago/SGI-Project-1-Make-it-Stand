import torch
from typing import List, Tuple, Optional
from shape2d import Shape2D


def calculate_smoothing_score(shape: Shape2D) -> float:
    if shape.vertices.shape[0] < 3:
        return 0.0
    f2 = 0.0
    n = shape.vertices.shape[0]
    for i in range(n):
        v0 = shape.vertices[i]
        v1 = shape.vertices[(i + 1) % n]
        v2 = shape.vertices[(i + 2) % n]
        if v1[1] == 0 and v0[1] == 0:
            continue
        if v2[1] == 0 and v1[1] == 0:
            continue
        v02 = 0.5 * (v0 + v2)
        distance = torch.norm(v1 - v02, p=2)
        f2 += 0.5 * (distance ** 2)
    return float(f2)


def smooth_shape(shape: Shape2D, iterations: int = 1, strength: float = 0.5) -> Shape2D:
    if shape.vertices.shape[0] < 3:
        return Shape2D(shape.vertices.clone(), shape.edges.copy())
    smoothed_vertices = shape.vertices.clone()
    n = smoothed_vertices.shape[0]
    for _ in range(iterations):
        new_vertices = smoothed_vertices.clone()
        for i in range(n):
            v0 = smoothed_vertices[(i - 1) % n]
            v1 = smoothed_vertices[i]
            v2 = smoothed_vertices[(i + 1) % n]
            if v1[1] == 0:
                new_vertices[i] = v1
                continue
            midpoint = 0.5 * (v0 + v2)
            new_vertices[i] = v1 + strength * (midpoint - v1)
        smoothed_vertices = new_vertices
    return Shape2D(smoothed_vertices, shape.edges.copy())


def calculate_vertex_smoothing_scores(shape: Shape2D) -> List[float]:
    if shape.vertices.shape[0] < 3:
        return [0.0] * shape.vertices.shape[0]
    n = shape.vertices.shape[0]
    scores = []
    for i in range(n):
        v0 = shape.vertices[(i - 1) % n]
        v1 = shape.vertices[i]
        v2 = shape.vertices[(i + 1) % n]
        if v1[1] == 0:
            scores.append(0.0)
            continue
        midpoint = 0.5 * (v0 + v2)
        distance = torch.norm(v1 - midpoint, p=2)
        score = 0.5 * (distance ** 2)
        scores.append(float(score))
    return scores


def find_roughest_vertices(shape: Shape2D, num_vertices: int = 5) -> List[Tuple[int, float]]:
    scores = calculate_vertex_smoothing_scores(shape)
    vertex_scores = [(i, scores[i]) for i in range(len(scores))]
    vertex_scores.sort(key=lambda x: x[1], reverse=True)
    return vertex_scores[:num_vertices]


def adaptive_smooth_shape(shape: Shape2D, target_smoothness: float = 0.1, max_iterations: int = 10) -> Shape2D:
    current_shape = Shape2D(shape.vertices.clone(), shape.edges.copy())
    for iteration in range(max_iterations):
        current_score = calculate_smoothing_score(current_shape)
        if current_score <= target_smoothness:
            break
        current_shape = smooth_shape(current_shape, iterations=1, strength=0.5)
    return current_shape


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
    edges = [(0, 1), (1, 2), (2, 3), (3, 4), (4, 5), (5, 6), (6, 0)]
    original_shape = Shape2D(vertices, edges)
    print("=== Shape Smoothing Demo ===\n")
    initial_score = calculate_smoothing_score(original_shape)
    print(f"Initial smoothing score: {initial_score:.6f}")
    roughest = find_roughest_vertices(original_shape, 3)
    print(f"Roughest vertices: {roughest}")
    smoothed_shape = smooth_shape(original_shape, iterations=3, strength=0.5)
    smoothed_score = calculate_smoothing_score(smoothed_shape)
    print(f"Smoothed score: {smoothed_score:.6f}")
    print(f"Improvement: {initial_score - smoothed_score:.6f}")
    adaptive_shape = adaptive_smooth_shape(original_shape, target_smoothness=0.1)
    adaptive_score = calculate_smoothing_score(adaptive_shape)
    print(f"Adaptive smoothed score: {adaptive_score:.6f}")

if __name__ == "__main__":
    test_smoothing_functions() 