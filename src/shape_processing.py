"""
This module contains functions for processing shapes.
"""
from shape2d import Shape2D

def scale_shape(shape: Shape2D, factor: float) -> Shape2D:
    """Returns a new Shape2D scaled by the given factor."""
    new_vertices = [(x * factor, y * factor) for (x, y) in shape.vertices]
    return Shape2D(new_vertices, shape.edges.copy()) 