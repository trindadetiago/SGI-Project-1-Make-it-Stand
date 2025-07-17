import numpy as np
from typing import List, Tuple
from shape2d import Shape2D


def calculate_shape_similarity(modified_shape: Shape2D, original_shape: Shape2D) -> float:
    """
    Calculate the similarity between a modified shape and the original shape.
    
    This implements the f₃ function from the notes:
    f₃(V') = (1/2) * ||V' - V||²
    
    Where V' is the modified shape and V is the original shape.
    
    Args:
        modified_shape: The shape to compare (V')
        original_shape: The reference original shape (V)
        
    Returns:
        float: The similarity score (lower values indicate more similar shapes)
        
    Raises:
        ValueError: If shapes have different numbers of vertices
    """
    if len(modified_shape.vertices) != len(original_shape.vertices):
        raise ValueError("Shapes must have the same number of vertices for similarity calculation")
    
    # Convert vertices to numpy arrays for efficient computation
    V_prime = np.array(modified_shape.vertices)
    V_original = np.array(original_shape.vertices)
    
    # Calculate the difference D = V' - V
    D = V_prime - V_original
    
    # Compute the L2 norm: ||D||₂
    norm_D = np.linalg.norm(D, ord=2)
    
    # f₃ = (1/2) * ||D||²
    f3 = 0.5 * (norm_D ** 2)
    
    return float(f3)


def calculate_shape_similarity_weighted(modified_shape: Shape2D, 
                                      original_shape: Shape2D, 
                                      lambda_weight: float = 1.0,
                                      mu_scale: float = 1.0) -> float:
    """
    Calculate the weighted similarity between shapes with scaling factors.
    
    This implements the weighted version: λ₃ * μ₃ * f₃(V')
    
    Args:
        modified_shape: The shape to compare (V')
        original_shape: The reference original shape (V)
        lambda_weight: Weight factor λ₃ (default: 1.0)
        mu_scale: Scaling factor μ₃ (default: 1.0)
        
    Returns:
        float: The weighted similarity score
    """
    f3 = calculate_shape_similarity(modified_shape, original_shape)
    return lambda_weight * mu_scale * f3


def calculate_vertex_distances(modified_shape: Shape2D, 
                             original_shape: Shape2D) -> List[float]:
    """
    Calculate the distance between each pair of corresponding vertices.
    
    Args:
        modified_shape: The shape to compare (V')
        original_shape: The reference original shape (V)
        
    Returns:
        List[float]: List of distances between corresponding vertices
        
    Raises:
        ValueError: If shapes have different numbers of vertices
    """
    if len(modified_shape.vertices) != len(original_shape.vertices):
        raise ValueError("Shapes must have the same number of vertices")
    
    distances = []
    for v_prime, v_original in zip(modified_shape.vertices, original_shape.vertices):
        # Calculate Euclidean distance between corresponding vertices
        distance = np.sqrt((v_prime[0] - v_original[0])**2 + (v_prime[1] - v_original[1])**2)
        distances.append(distance)
    
    return distances


def calculate_max_vertex_displacement(modified_shape: Shape2D, 
                                    original_shape: Shape2D) -> Tuple[float, int]:
    """
    Find the maximum displacement of any vertex and its index.
    
    Args:
        modified_shape: The shape to compare (V')
        original_shape: The reference original shape (V)
        
    Returns:
        Tuple[float, int]: (maximum_displacement, vertex_index)
    """
    distances = calculate_vertex_distances(modified_shape, original_shape)
    max_distance = max(distances)
    max_index = distances.index(max_distance)
    return max_distance, max_index


def calculate_average_vertex_displacement(modified_shape: Shape2D, 
                                        original_shape: Shape2D) -> float:
    """
    Calculate the average displacement of all vertices.
    
    Args:
        modified_shape: The shape to compare (V')
        original_shape: The reference original shape (V)
        
    Returns:
        float: Average displacement
    """
    distances = calculate_vertex_distances(modified_shape, original_shape)
    return float(np.mean(distances))


def normalize_shape_for_comparison(shape: Shape2D) -> Shape2D:
    """
    Normalize a shape to have unit area for fair comparison.
    
    This is useful when comparing shapes of different sizes.
    
    Args:
        shape: The shape to normalize
        
    Returns:
        Shape2D: Normalized shape
    """
    if not shape.vertices:
        return shape
    
    # Calculate current area (simplified as bounding box area)
    vertices = np.array(shape.vertices)
    min_coords = np.min(vertices, axis=0)
    max_coords = np.max(vertices, axis=0)
    current_area = (max_coords[0] - min_coords[0]) * (max_coords[1] - min_coords[1])
    
    if current_area == 0:
        return shape
    
    # Scale factor to achieve unit area
    scale_factor = 1.0 / np.sqrt(current_area)
    
    # Apply scaling
    normalized_vertices = [(v[0] * scale_factor, v[1] * scale_factor) for v in shape.vertices]
    
    return Shape2D(normalized_vertices, shape.edges.copy())


def calculate_similarity_with_normalization(modified_shape: Shape2D, 
                                          original_shape: Shape2D) -> float:
    """
    Calculate similarity between shapes after normalizing them to the same scale.
    
    Args:
        modified_shape: The shape to compare (V')
        original_shape: The reference original shape (V)
        
    Returns:
        float: The similarity score after normalization
    """
    # Normalize both shapes
    norm_modified = normalize_shape_for_comparison(modified_shape)
    norm_original = normalize_shape_for_comparison(original_shape)
    
    return calculate_shape_similarity(norm_modified, norm_original)
