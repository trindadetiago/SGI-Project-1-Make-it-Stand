import torch
from typing import Tuple
from shape_mass_center import calculate_center_of_mass

def is_shape_stable(vertices: torch.Tensor, edges) -> Tuple[bool, float, float, float]:
    """
    Determines if the shape is stable (will not fall) based on its center of mass and support base.
    Args:
        vertices: torch.Tensor of shape (N, 2)
        edges: list or torch.LongTensor of shape (M, 2)
    Returns:
        is_stable (bool): True if stable, False if will fall
        x_cm (float): x-coordinate of center of mass
        x_left (float): leftmost x of support
        x_right (float): rightmost x of support
    """
    if isinstance(edges, list):
        edges = torch.tensor(edges, dtype=torch.long, device=vertices.device)
    y_min = vertices[:, 1].min()
    tol = 1e-3
    support_mask = torch.abs(vertices[:, 1] - y_min) < tol
    support_x = vertices[support_mask, 0]
    if support_x.numel() < 2:
        # Not enough support, treat as unstable
        x_left = x_right = vertices[:, 0].mean().item()
        is_stable = False
    else:
        x_left = support_x.min().item()
        x_right = support_x.max().item()
    _, com = calculate_center_of_mass(vertices, edges)
    x_cm = com[0].item()
    is_stable = (x_left <= x_cm <= x_right) and (support_x.numel() >= 2)
    return is_stable, x_cm, x_left, x_right 