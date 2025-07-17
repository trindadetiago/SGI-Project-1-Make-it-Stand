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

def calculate_shape_similarity_weighted(modified_shape, original_shape, lambda_weight: float = 1.0, mu_scale: float = 1.0) -> torch.Tensor:
    f3 = calculate_shape_similarity(modified_shape, original_shape)
    return lambda_weight * mu_scale * f3

def calculate_vertex_distances(modified_shape, original_shape) -> List[float]:
    V_prime = _get_vertices(modified_shape)
    V_original = _get_vertices(original_shape)
    if V_prime.shape != V_original.shape:
        raise ValueError("Shapes must have the same number of vertices")
    D = V_prime - V_original
    distances = torch.norm(D, dim=1).tolist()
    return distances

def calculate_max_vertex_displacement(modified_shape, original_shape) -> Tuple[float, int]:
    distances = calculate_vertex_distances(modified_shape, original_shape)
    max_distance = max(distances)
    max_index = distances.index(max_distance)
    return max_distance, max_index

def calculate_average_vertex_displacement(modified_shape, original_shape) -> float:
    distances = calculate_vertex_distances(modified_shape, original_shape)
    return float(torch.tensor(distances).mean())

def normalize_shape_for_comparison(shape) -> torch.Tensor:
    V = _get_vertices(shape)
    if V.shape[0] == 0:
        return V
    min_coords = torch.min(V, dim=0).values
    max_coords = torch.max(V, dim=0).values
    current_area = (max_coords[0] - min_coords[0]) * (max_coords[1] - min_coords[1])
    if current_area == 0:
        return V
    scale_factor = 1.0 / torch.sqrt(current_area)
    normalized_vertices = V * scale_factor
    return normalized_vertices

def calculate_similarity_with_normalization(modified_shape, original_shape) -> torch.Tensor:
    norm_modified = normalize_shape_for_comparison(modified_shape)
    norm_original = normalize_shape_for_comparison(original_shape)
    return calculate_shape_similarity(norm_modified, norm_original)
