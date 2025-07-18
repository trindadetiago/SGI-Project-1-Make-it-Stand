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
        verts_closed = np.vstack([loop_verts, loop_verts[0]])
        x = verts_closed[:, 0]
        y = verts_closed[:, 1]
        
        cross_product = np.cross(verts_closed[:-1], verts_closed[1:])
        
        total_area_sum += np.sum(cross_product)
        total_cx_sum += np.sum((x[:-1] + x[1:]) * cross_product)
        total_cy_sum += np.sum((y[:-1] + y[1:]) * cross_product)
        
    final_signed_area = 0.5 * total_area_sum

    if abs(final_signed_area) < 1e-9:
        return 0, np.mean(vertices, axis=0)

    # Use the total sums to calculate the final centroid
    center_of_mass = (1 / (6 * final_signed_area)) * np.array([total_cx_sum, total_cy_sum])
    
    # Return the absolute area and the calculated center of mass
    return abs(final_signed_area), center_of_mass

# --- Example Usage ---

# 1. A square with a hole (a "donut")
# Note: The outer loop is CCW, the inner loop (hole) is CW.
print("## Testing with a Donut Shape ##")
donut_vertices = [
    # Outer square (CCW)
    [0, 0], [4, 0], [4, 4], [0, 4],
    # Inner square (CW)
    [1, 1], [1, 3], [3, 3], [3, 1]
]

donut_edges = [
    # Outer loop
    [0, 1], [1, 2], [2, 3], [3, 0],
    # Inner loop
    [4, 5], [5, 6], [6, 7], [7, 4]
]

area, com = get_center_of_mass_from_mesh(donut_vertices, donut_edges)

# Expected area = 4*4 - 2*2 = 12
# Expected CoM = [2.0, 2.0] due to symmetry
print(f"Donut Area: {area:.4f}")
print(f"Donut Center of Mass: {np.round(com, 4)}")


# 2. Jorge's case
print("\n## Testing with an Hourglass Shape ##")
hourglass_vertices = [
    [-2, 0], [-2, 2], [0, 2], [0, 0],
    [2, 0], [2, -2], [0, -2]
]

hourglass_edges = [
    [0, 1], [1, 2], [2, 3], [3, 0],
    [3, 4], [4, 5], [5, 6], [6, 3]
]

area, com = get_center_of_mass_from_mesh(hourglass_vertices, hourglass_edges)

# Expected area = 2*2 + 2*2 = 8
# Expected CoM = [0.0, 0.0] due to symmetry
print(f"Hourglass Area: {area:.4f}")
print(f"Hourglass Center of Mass: {np.round(com, 4)}")