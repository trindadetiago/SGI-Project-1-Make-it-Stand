import numpy as np
from typing import List, Tuple, Optional
from shape2d import Shape2D


def calculate_smoothing_score(shape: Shape2D) -> float:
    """
    Calculate the smoothing score for a shape.
    
    This implements the f₂ function from the notes:
    f₂(V) = Σ (1/2) * ||v₁ - (1/2)(v₀ + v₂)||²
    
    Where we sum over all consecutive triplets of vertices (v₀, v₁, v₂).
    
    Args:
        shape: The shape to calculate smoothing score for
        
    Returns:
        float: The smoothing score (lower values indicate smoother shapes)
    """
    if len(shape.vertices) < 3:
        return 0.0
    
    f2 = 0.0
    
    # Iterate through all vertices to find consecutive triplets
    for i in range(len(shape.vertices)):
        v0_idx = i
        v1_idx = (i + 1) % len(shape.vertices)  # Wrap around for closed shapes
        v2_idx = (i + 2) % len(shape.vertices)
        
        v0 = np.array(shape.vertices[v0_idx])
        v1 = np.array(shape.vertices[v1_idx])
        v2 = np.array(shape.vertices[v2_idx])
        
        # Check if vertices are on support plane (y=0)
        if v1[1] == 0 and v0[1] == 0:
            continue  # Skip triplets on support plane
        if v2[1] == 0 and v1[1] == 0:
            continue  # Skip triplets on support plane
        
        # Calculate midpoint of v0 and v2
        v02 = 0.5 * (v0 + v2)
        
        # Calculate distance from v1 to midpoint
        distance = np.linalg.norm(v1 - v02)
        
        # Add to smoothing score
        f2 += 0.5 * (distance ** 2)
    
    return float(f2)


def smooth_shape(shape: Shape2D, iterations: int = 1, strength: float = 0.5) -> Shape2D:
    """
    Apply smoothing to a shape by moving vertices toward the midpoint of their neighbors.
    
    Args:
        shape: The shape to smooth
        iterations: Number of smoothing iterations to apply
        strength: Smoothing strength (0.0 = no change, 1.0 = full smoothing)
        
    Returns:
        Shape2D: A new smoothed shape
    """
    if len(shape.vertices) < 3:
        return Shape2D(shape.vertices.copy(), shape.edges.copy())
    
    # Start with a copy of the original vertices
    smoothed_vertices = [list(v) for v in shape.vertices]
    
    for _ in range(iterations):
        new_vertices = []
        
        for i in range(len(smoothed_vertices)):
            v0_idx = (i - 1) % len(smoothed_vertices)
            v1_idx = i
            v2_idx = (i + 1) % len(smoothed_vertices)
            
            v0 = np.array(smoothed_vertices[v0_idx])
            v1 = np.array(smoothed_vertices[v1_idx])
            v2 = np.array(smoothed_vertices[v2_idx])
            
            # Check if vertex is on support plane (y=0)
            if v1[1] == 0:
                # Don't smooth vertices on support plane
                new_vertices.append(list(v1))
                continue
            
            # Calculate midpoint of neighbors
            midpoint = 0.5 * (v0 + v2)
            
            # Move vertex toward midpoint
            smoothed_v1 = v1 + strength * (midpoint - v1)
            
            new_vertices.append(list(smoothed_v1))
        
        smoothed_vertices = new_vertices
    
    return Shape2D([(float(v[0]), float(v[1])) for v in smoothed_vertices], shape.edges.copy())


def calculate_vertex_smoothing_scores(shape: Shape2D) -> List[float]:
    """
    Calculate smoothing scores for each vertex individually.
    
    Args:
        shape: The shape to analyze
        
    Returns:
        List[float]: Smoothing score for each vertex
    """
    if len(shape.vertices) < 3:
        return [0.0] * len(shape.vertices)
    
    scores = []
    
    for i in range(len(shape.vertices)):
        v0_idx = (i - 1) % len(shape.vertices)
        v1_idx = i
        v2_idx = (i + 1) % len(shape.vertices)
        
        v0 = np.array(shape.vertices[v0_idx])
        v1 = np.array(shape.vertices[v1_idx])
        v2 = np.array(shape.vertices[v2_idx])
        
        # Check if vertex is on support plane
        if v1[1] == 0:
            scores.append(0.0)
            continue
        
        # Calculate midpoint of neighbors
        midpoint = 0.5 * (v0 + v2)
        
        # Calculate distance from vertex to midpoint
        distance = np.linalg.norm(v1 - midpoint)
        score = 0.5 * (distance ** 2)
        
        scores.append(float(score))
    
    return scores


def find_roughest_vertices(shape: Shape2D, num_vertices: int = 5) -> List[Tuple[int, float]]:
    """
    Find the vertices with the highest smoothing scores (roughest areas).
    
    Args:
        shape: The shape to analyze
        num_vertices: Number of roughest vertices to return
        
    Returns:
        List[Tuple[int, float]]: List of (vertex_index, smoothing_score) tuples
    """
    scores = calculate_vertex_smoothing_scores(shape)
    
    # Create list of (index, score) pairs
    vertex_scores = [(i, scores[i]) for i in range(len(scores))]
    
    # Sort by score (highest first)
    vertex_scores.sort(key=lambda x: x[1], reverse=True)
    
    return vertex_scores[:num_vertices]


def adaptive_smooth_shape(shape: Shape2D, target_smoothness: float = 0.1, max_iterations: int = 10) -> Shape2D:
    """
    Apply adaptive smoothing until the shape reaches a target smoothness level.
    
    Args:
        shape: The shape to smooth
        target_smoothness: Target smoothing score to achieve
        max_iterations: Maximum number of iterations to apply
        
    Returns:
        Shape2D: Smoothed shape
    """
    current_shape = Shape2D(shape.vertices.copy(), shape.edges.copy())
    
    for iteration in range(max_iterations):
        current_score = calculate_smoothing_score(current_shape)
        
        if current_score <= target_smoothness:
            break
        
        # Apply one iteration of smoothing
        current_shape = smooth_shape(current_shape, iterations=1, strength=0.5)
    
    return current_shape


# Example usage and testing functions
def test_smoothing_functions():
    """Test the smoothing functions with example shapes."""
    
    # Create a shape with some spikes
    vertices = [
        (0.0, 0.0),   # Base
        (1.0, 0.0),   # Base
        (1.0, 1.0),   # Normal
        (1.2, 1.5),   # Spike
        (1.0, 2.0),   # Normal
        (0.0, 2.0),   # Normal
        (0.0, 1.0),   # Normal
    ]
    edges = [(0, 1), (1, 2), (2, 3), (3, 4), (4, 5), (5, 6), (6, 0)]
    
    original_shape = Shape2D(vertices, edges)
    
    print("=== Shape Smoothing Demo ===\n")
    
    # Calculate initial smoothing score
    initial_score = calculate_smoothing_score(original_shape)
    print(f"Initial smoothing score: {initial_score:.6f}")
    
    # Find roughest vertices
    roughest = find_roughest_vertices(original_shape, 3)
    print(f"Roughest vertices: {roughest}")
    
    # Apply smoothing
    smoothed_shape = smooth_shape(original_shape, iterations=3, strength=0.5)
    smoothed_score = calculate_smoothing_score(smoothed_shape)
    print(f"Smoothed score: {smoothed_score:.6f}")
    print(f"Improvement: {initial_score - smoothed_score:.6f}")
    
    # Test adaptive smoothing
    adaptive_shape = adaptive_smooth_shape(original_shape, target_smoothness=0.1)
    adaptive_score = calculate_smoothing_score(adaptive_shape)
    print(f"Adaptive smoothed score: {adaptive_score:.6f}")


if __name__ == "__main__":
    test_smoothing_functions() 