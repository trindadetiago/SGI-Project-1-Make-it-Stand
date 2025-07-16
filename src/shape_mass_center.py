from shape2d import Shape2D

def calculate_center_of_mass(shape: Shape2D):
    """Mock: Returns a fixed center of mass (e.g., average of vertices, or just (0.5, 0.5))."""
    if not shape or not shape.vertices:
        return None
    # Mock: just return the average of all vertices
    xs, ys = zip(*shape.vertices)
    return (sum(xs) / len(xs), sum(ys) / len(ys)) 