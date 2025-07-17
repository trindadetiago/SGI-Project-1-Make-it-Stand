import torch
from typing import List, Tuple
from shape2d import Shape2D


def calculate_shape_similarity(modified_shape: Shape2D, original_shape: Shape2D) -> float:
    """
    Calculate the similarity between a modified shape and the original shape.
    Implements the f₃ function: f₃(V') = (1/2) * ||V' - V||²
    """
    if modified_shape.vertices.shape != original_shape.vertices.shape:
        raise ValueError("Shapes must have the same number of vertices for similarity calculation")
    V_prime = modified_shape.vertices
    V_original = original_shape.vertices
    D = V_prime - V_original
    norm_D = torch.norm(D, p=2)
    f3 = 0.5 * (norm_D ** 2)
    return float(f3)


def calculate_shape_similarity_weighted(modified_shape: Shape2D, 
                                      original_shape: Shape2D, 
                                      lambda_weight: float = 1.0,
                                      mu_scale: float = 1.0) -> float:
    f3 = calculate_shape_similarity(modified_shape, original_shape)
    return lambda_weight * mu_scale * f3


def calculate_vertex_distances(modified_shape: Shape2D, 
                             original_shape: Shape2D) -> List[float]:
    if modified_shape.vertices.shape != original_shape.vertices.shape:
        raise ValueError("Shapes must have the same number of vertices")
    D = modified_shape.vertices - original_shape.vertices
    distances = torch.norm(D, dim=1).tolist()
    return distances


def calculate_max_vertex_displacement(modified_shape: Shape2D, 
                                    original_shape: Shape2D) -> Tuple[float, int]:
    distances = calculate_vertex_distances(modified_shape, original_shape)
    max_distance = max(distances)
    max_index = distances.index(max_distance)
    return max_distance, max_index


def calculate_average_vertex_displacement(modified_shape: Shape2D, 
                                        original_shape: Shape2D) -> float:
    distances = calculate_vertex_distances(modified_shape, original_shape)
    return float(torch.tensor(distances).mean())


def normalize_shape_for_comparison(shape: Shape2D) -> Shape2D:
    if shape.vertices.shape[0] == 0:
        return shape
    vertices = shape.vertices
    min_coords = torch.min(vertices, dim=0).values
    max_coords = torch.max(vertices, dim=0).values
    current_area = (max_coords[0] - min_coords[0]) * (max_coords[1] - min_coords[1])
    if current_area == 0:
        return shape
    scale_factor = 1.0 / torch.sqrt(current_area)
    normalized_vertices = vertices * scale_factor
    return Shape2D(normalized_vertices, shape.edges.copy())


def calculate_similarity_with_normalization(modified_shape: Shape2D, 
                                          original_shape: Shape2D) -> float:
    norm_modified = normalize_shape_for_comparison(modified_shape)
    norm_original = normalize_shape_for_comparison(original_shape)
    return calculate_shape_similarity(norm_modified, norm_original)
