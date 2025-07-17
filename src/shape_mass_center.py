from shape2d import Shape2D
import torch

def calculate_center_of_mass(shape: Shape2D):
    """Returns the average of all vertices as the center of mass."""
    if shape is None or shape.vertices is None or shape.vertices.shape[0] == 0:
        return None
    xs = shape.vertices[:, 0]
    ys = shape.vertices[:, 1]
    return (float(xs.mean()), float(ys.mean())) 