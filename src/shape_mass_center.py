import torch
from typing import Union, List, Tuple
from collections import defaultdict
from dataclasses import dataclass
from shape2d import Shape2D

## --- Core Helper and Calculation Functions ---

def _find_all_loops(edges: List[List[int]], num_vertices: int) -> List[List[int]]:
    """
    Traces edges to find all closed loops in the mesh using standard Python.
    This is the clean version without redundant logic.
    """
    adj = defaultdict(list)
    for i, j in edges:
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
                    
                    # If the path is broken (should not happen in a valid mesh)
                    if next_node is None:
                        break 
                    prev_node = current_node
                    current_node = next_node
                
                if current_node == start_node:
                    used_edges.add(tuple(sorted((prev_node, current_node))))
                    all_loops.append(current_loop)

    return all_loops


def _get_com_from_loops(vertices: torch.Tensor, loops: List[List[int]]) -> Tuple[torch.Tensor, torch.Tensor]:
    """Calculates the CoM from a pre-computed list of loops."""
    total_area_sum = torch.tensor(0.0, dtype=vertices.dtype, device=vertices.device)
    total_cx_sum = torch.tensor(0.0, dtype=vertices.dtype, device=vertices.device)
    total_cy_sum = torch.tensor(0.0, dtype=vertices.dtype, device=vertices.device)

    for loop_indices in loops:
        loop_verts = vertices[loop_indices]
        
        verts_closed = torch.cat([loop_verts, loop_verts[:1]], dim=0)
        v1 = verts_closed[:-1]
        v2 = verts_closed[1:]
        
        cross_product = v1[:, 0] * v2[:, 1] - v1[:, 1] * v2[:, 0]
        
        total_area_sum += torch.sum(cross_product)
        total_cx_sum += torch.sum((v1[:, 0] + v2[:, 0]) * cross_product)
        total_cy_sum += torch.sum((v1[:, 1] + v2[:, 1]) * cross_product)
            
    final_signed_area = 0.5 * total_area_sum

    if torch.abs(final_signed_area) < 1e-9:
        return torch.tensor(0.0, device=vertices.device), torch.mean(vertices, dim=0)

    cx_cy_sum_vec = torch.stack([total_cx_sum, total_cy_sum])
    center_of_mass = (1.0 / (6.0 * final_signed_area)) * cx_cy_sum_vec
    
    return torch.abs(final_signed_area), center_of_mass


## --- Public API ---

def calculate_center_of_mass(
    shape_or_vertices: Union[Shape2D, torch.Tensor],
    edges: Union[List[List[int]], torch.Tensor] = None
) -> Tuple[torch.Tensor, torch.Tensor]:
    """
    Calculates the area and center of mass for a 2D mesh.

    This function serves as a robust wrapper and can be called in two ways:
    1. With a Shape2D object: `calculate_center_of_mass(my_shape)`
    2. With vertices and edges: `calculate_center_of_mass(vertices_tensor, edges_tensor)`

    Returns:
        A tuple of (area, center_of_mass_coordinates) as torch.Tensors.
    """
    if isinstance(shape_or_vertices, Shape2D):
        vertices = shape_or_vertices.vertices
        edge_list = shape_or_vertices.edges
    elif isinstance(shape_or_vertices, torch.Tensor) and edges is not None:
        vertices = shape_or_vertices
        edge_list = edges.tolist() if isinstance(edges, torch.Tensor) else edges
    else:
        raise TypeError("Invalid input. Provide a Shape2D object or separate vertex and edge tensors.")

    if vertices.shape[0] == 0:
        return torch.tensor(0.0, device=vertices.device), torch.zeros(2, device=vertices.device)
        
    loops = _find_all_loops(edge_list, vertices.shape[0])

    if not loops:
        print("Warning: No closed loops found in the provided edges.")
        com = torch.mean(vertices, dim=0) if vertices.shape[0] > 0 else torch.zeros(2, device=vertices.device)
        return torch.tensor(0.0, device=vertices.device), com

    return _get_com_from_loops(vertices, loops)

