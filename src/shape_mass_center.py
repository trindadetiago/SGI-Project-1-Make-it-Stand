import torch
from typing import Union
from shape2d import Shape2D

def _find_all_loops_torch(edges, num_vertices):
    from collections import defaultdict
    adj = defaultdict(list)
    for i, j in edges.tolist():
        adj[i].append(j)
        adj[j].append(i)
    used_edges = set()
    all_loops = []
    for start_node in range(num_vertices):
        for neighbor in adj[start_node]:
            edge = tuple(sorted((start_node, neighbor)))
            if edge not in used_edges:
                current_loop = [start_node]
                prev_node = start_node
                current_node = neighbor
                while current_node != start_node:
                    used_edges.add(tuple(sorted((prev_node, current_node))))
                    current_loop.append(current_node)
                    next_node = None
                    for next_neighbor in adj[current_node]:
                        if next_neighbor != prev_node:
                            next_node = next_neighbor
                            break
                    if next_node is None:
                        break
                    prev_node = current_node
                    current_node = next_node
                used_edges.add(tuple(sorted((prev_node, current_node))))
                if current_loop[0] == current_loop[-1] or start_node in current_loop:
                    all_loops.append(current_loop)
    return all_loops

def get_center_of_mass_from_mesh_torch(vertices: torch.Tensor, edges: torch.Tensor):
    loops = _find_all_loops_torch(edges, vertices.shape[0])
    if not loops:
        return torch.tensor(0.0, dtype=vertices.dtype, device=vertices.device), vertices.mean(dim=0) if vertices.shape[0] > 0 else torch.zeros(2, dtype=vertices.dtype, device=vertices.device)
    total_area_sum = torch.tensor(0.0, dtype=vertices.dtype, device=vertices.device)
    total_cx_sum = torch.tensor(0.0, dtype=vertices.dtype, device=vertices.device)
    total_cy_sum = torch.tensor(0.0, dtype=vertices.dtype, device=vertices.device)
    for loop_indices in loops:
        loop_verts = vertices[loop_indices]
        verts_closed = torch.cat([loop_verts, loop_verts[:1]], dim=0)
        x = verts_closed[:, 0]
        y = verts_closed[:, 1]
        cross_product = x[:-1] * y[1:] - x[1:] * y[:-1]
        total_area_sum += cross_product.sum()
        total_cx_sum += ((x[:-1] + x[1:]) * cross_product).sum()
        total_cy_sum += ((y[:-1] + y[1:]) * cross_product).sum()
    final_signed_area = 0.5 * total_area_sum
    if torch.abs(final_signed_area) < 1e-9:
        return torch.tensor(0.0, dtype=vertices.dtype, device=vertices.device), vertices.mean(dim=0)
    center_of_mass = (1.0 / (6.0 * final_signed_area)) * torch.stack([total_cx_sum, total_cy_sum])
    return torch.abs(final_signed_area), center_of_mass

def calculate_center_of_mass(shape_or_vertices: Union[Shape2D, torch.Tensor], edges: torch.Tensor = None):
    """
    Returns (area, center_of_mass) for a Shape2D or (vertices, edges) pair.
    Center of mass is a torch.Tensor of shape (2,).
    """
    if isinstance(shape_or_vertices, Shape2D):
        vertices = shape_or_vertices.vertices
        edges_tensor = torch.tensor(shape_or_vertices.edges, dtype=torch.long, device=vertices.device)
        return get_center_of_mass_from_mesh_torch(vertices, edges_tensor)
    elif isinstance(shape_or_vertices, torch.Tensor) and edges is not None:
        return get_center_of_mass_from_mesh_torch(shape_or_vertices, edges)
    else:
        # Always return tensors, even for empty input
        return torch.tensor(0.0), torch.zeros(2) 