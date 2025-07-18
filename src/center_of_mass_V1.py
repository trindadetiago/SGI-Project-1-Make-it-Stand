import torch
from collections import defaultdict

def _find_all_loops(edges, num_vertices):
    """Traces edges to find all closed loops in the mesh."""
    adj = defaultdict(list)
    for i, j in edges:
        adj[i].append(j)
        adj[j].append(i)

    used_edges = set()
    all_loops = []

    for start_node in range(num_vertices):
        # Find a starting edge that hasn't been used yet
        for neighbor in adj[start_node]:
            edge = tuple(sorted((start_node, neighbor)))
            if edge not in used_edges:
                # Start tracing a new loop
                current_loop = [start_node]
                prev_node = start_node
                current_node = neighbor
                
                while current_node != start_node:
                    used_edges.add(tuple(sorted((prev_node, current_node))))
                    current_loop.append(current_node)
                    
                    # Find the next node in the path
                    next_node = None
                    for next_neighbor in adj[current_node]:
                        if next_neighbor != prev_node:
                            next_node = next_neighbor
                            break
                    
                    prev_node = current_node
                    current_node = next_node

                used_edges.add(tuple(sorted((prev_node, current_node))))
                all_loops.append(current_loop)
    
    return all_loops

def get_center_of_mass_from_mesh_torch(vertices, edges, device=None):
    """
    Calculates the center of mass for a shape defined by vertices and edges
    using PyTorch, correctly handling shapes with holes.

    Args:
        vertices (list or torch.Tensor): A list of [x, y] coordinates.
        edges (list or torch.Tensor): A list of [v1, v2] vertex indices.
        device (str, optional): The device to run the computations on ('cpu' or 'cuda').
                                  Defaults to None.

    Returns:
        tuple: (area, center_of_mass_coordinates) as torch.Tensors.
    """
    vertices = torch.tensor(vertices, dtype=torch.float32, device=device)
    num_vertices = len(vertices)
    loops = _find_all_loops(edges, num_vertices)

    if not loops:
        print("Warning: No closed loops found in the provided edges.")
        com = torch.mean(vertices, dim=0) if num_vertices > 0 else torch.tensor([0.0, 0.0], device=device)
        return torch.tensor(0.0, device=device), com
    
    # Initialize sums as 0-dimensional tensors
    total_area_sum = torch.tensor(0.0, device=device)
    total_cx_sum = torch.tensor(0.0, device=device)
    total_cy_sum = torch.tensor(0.0, device=device)

    # Calculate properties for each loop and add them to the totals
    for loop_indices in loops:
        loop_verts = vertices[loop_indices]
        
        # Close the loop for calculation
        verts_closed = torch.vstack([loop_verts, loop_verts[0].unsqueeze(0)])
        
        v1 = verts_closed[:-1]
        v2 = verts_closed[1:]

        cross_product = v1[:, 0] * v2[:, 1] - v1[:, 1] * v2[:, 0]

        total_area_sum += torch.sum(cross_product)
        total_cx_sum += torch.sum((v1[:, 0] + v2[:, 0]) * cross_product)
        total_cy_sum += torch.sum((v1[:, 1] + v2[:, 1]) * cross_product)
        
    final_signed_area = 0.5 * total_area_sum

    if torch.abs(final_signed_area) < 1e-9:
        return torch.tensor(0.0, device=device), torch.mean(vertices, dim=0)

    cx_cy_sum_vec = torch.stack([total_cx_sum, total_cy_sum])
    center_of_mass = (1 / (6 * final_signed_area)) * cx_cy_sum_vec

    # Return the absolute area and the calculated center of mass
    return torch.abs(final_signed_area), center_of_mass

# --- Example Usage ---

# 1. A square with a hole (a "donut")

print("## Testing with a Donut Shape ##")
donut_vertices = [
    [0, 0], [4, 0], [4, 4], [0, 4],
    [1, 1], [1, 3], [3, 3], [3, 1]
]

donut_edges = [
    [0, 1], [1, 2], [2, 3], [3, 0],
    [4, 5], [5, 6], [6, 7], [7, 4]
]

area, com = get_center_of_mass_from_mesh_torch(donut_vertices, donut_edges)
print(f"Donut Area: {area:.4f}")
print(f"Donut Center of Mass: {com.round(decimals=4)}")

# 2. Hour glass case
print("\n## Testing with an Hourglass Shape ##")
hourglass_vertices = [
    [-2, 0], [-2, 2], [0, 2], [0, 0],
    [2, 0], [2, -2], [0, -2]
]

hourglass_edges = [
    [0, 1], [1, 2], [2, 3], [3, 0],
    [3, 4], [4, 5], [5, 6], [6, 3]
]

area, com = get_center_of_mass_from_mesh_torch(hourglass_vertices, hourglass_edges)
print(f"Hourglass Area: {area:.4f}")
print(f"Hourglass Center of Mass: {com.round(decimals=4)}")
