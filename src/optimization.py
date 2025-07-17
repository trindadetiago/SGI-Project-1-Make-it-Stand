import torch
from shape2d import Shape2D
from shape_smoothing import calculate_smoothing_score
from shape_similarity import calculate_shape_similarity

# --- Stability (center of mass) objective ---
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
                        # No valid next node, break to avoid appending None
                        break
                    prev_node = current_node
                    current_node = next_node
                used_edges.add(tuple(sorted((prev_node, current_node))))
                # Only add loop if it is closed (last node is start_node)
                if current_loop[0] == current_loop[-1] or start_node in current_loop:
                    all_loops.append(current_loop)
    return all_loops

def get_center_of_mass_from_mesh_torch(vertices, edges):
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

def f1(V: torch.Tensor, E: torch.Tensor, support_y=0.0) -> torch.Tensor:
    support_mask = torch.isclose(V[:, 1], torch.tensor(support_y, dtype=V.dtype, device=V.device))
    c_star_x = V[support_mask, 0].mean()
    _, com = get_center_of_mass_from_mesh_torch(V, E)
    return 0.5 * (com[0] - c_star_x) ** 2

def f2(V: torch.Tensor) -> torch.Tensor:
    return calculate_smoothing_score(V)

def f3(V: torch.Tensor, V_og: torch.Tensor) -> torch.Tensor:
    return calculate_shape_similarity(V, V_og)

# --- Total loss function using the shared modules ---
def total_loss(V: torch.Tensor, E: torch.Tensor, V_og: torch.Tensor, lambda1=0.33, lambda2=0.33, lambda3=0.34, mu1=1.0, mu2=1.0, mu3=1.0) -> torch.Tensor:
    return lambda1 * mu1 * f1(V, E) + lambda2 * mu2 * f2(V) + lambda3 * mu3 * f3(V, V_og)

# --- Simple gradient descent optimizer for V only ---
def gradient_descent(f, V0, lr=0.01, tol=1e-6, max_iters=1000, verbose=False):
    V = V0.clone().detach().requires_grad_(True)
    for i in range(max_iters):
        if V.grad is not None:
            V.grad.zero_()
        loss = f(V)
        if verbose and i % 100 == 0:
            print(f"Iter {i}: loss = {loss.item():.6f}")
        if loss.item() < tol:
            break
        loss.backward()
        with torch.no_grad():
            V -= lr * V.grad
    return V.detach()

# --- Example usage/test function ---
def test_shape_optimization():
    import os
    shape_path = os.path.join(os.path.dirname(__file__), '..', 'shapes', 'square.json')
    shape = Shape2D.load_from_json(shape_path)
    V_og = shape.vertices
    E = torch.tensor(shape.edges, dtype=torch.long, device=V_og.device)
    torch.manual_seed(42)
    V0 = V_og + 0.2 * torch.randn_like(V_og)
    def loss_fn(V):
        return total_loss(V, E, V_og, lambda1=0.33, lambda2=0.33, lambda3=0.34)
    V_opt = gradient_descent(loss_fn, V0, lr=0.05, max_iters=1000, verbose=True)
    print("Original vertices:\n", V_og)
    print("Initial (perturbed) vertices:\n", V0)
    print("Optimized vertices:\n", V_opt)
    optimized_shape = Shape2D(V_opt, shape.edges.copy())
    out_path = os.path.join(os.path.dirname(__file__), '..', 'shapes', 'square_optimized.json')
    optimized_shape.save_to_json(out_path)
    print(f"Optimized shape saved to {out_path}")
    return V_og, V0, V_opt

if __name__ == "__main__":
    test_shape_optimization() 