import torch
from shape2d import Shape2D
from shape_smoothing import calculate_smoothing_score
from shape_similarity import calculate_shape_similarity
from shape_mass_center import calculate_center_of_mass
from constants import SUPPORT_TOL

def f1(V: torch.Tensor, E: torch.Tensor, support_y=None) -> torch.Tensor:
    # Use the lowest y as the support if not specified
    if support_y is None:
        min_y = V[:, 1].min()
        tol = SUPPORT_TOL
        support_mask = torch.abs(V[:, 1] - min_y) < tol
    else:
        tol = SUPPORT_TOL
        support_mask = torch.isclose(V[:, 1], torch.tensor(support_y, dtype=V.dtype, device=V.device), atol=tol)
    if support_mask.sum() == 0:
        # If still no support, just use the lowest y vertex
        support_mask = (V[:, 1] == V[:, 1].min())
    c_star_x = V[support_mask, 0].mean()
    area, com = calculate_center_of_mass(V, E)
    return 0.5 * (com[0] - c_star_x) ** 2

def f2(V: torch.Tensor) -> torch.Tensor:
    return calculate_smoothing_score(V)

def f3(V: torch.Tensor, V_og: torch.Tensor) -> torch.Tensor:
    return calculate_shape_similarity(V, V_og)

# --- Total loss function using the shared modules ---
def total_loss(V: torch.Tensor, E: torch.Tensor, V_og: torch.Tensor, lambda1=0.33, lambda2=0.33, lambda3=0.34, mu1=1.0, mu2=1.0, mu3=1.0) -> torch.Tensor:
    return lambda1 * mu1 * f1(V, E) + lambda2 * mu2 * f2(V) + lambda3 * mu3 * f3(V, V_og)

# --- Simple gradient descent optimizer for V only ---
def gradient_descent(f, V0, lr=0.01, tol=SUPPORT_TOL, max_iters=1000, verbose=False):
    """
    Simple gradient descent optimizer for V only.
    Args:
        f: loss function taking V
        V0: initial vertices (torch.Tensor)
        lr: learning rate
        tol: loss tolerance for early stopping
        max_iters: maximum number of iterations
        verbose: if True, prints loss every 100 steps and stopping reason
    Returns:
        Optimized vertices (torch.Tensor)
    """
    V = V0.clone().detach().requires_grad_(True)
    
    # Identify support vertices (those at y=0 or very close to it)
    support_mask = torch.abs(V0[:, 1]) < SUPPORT_TOL
    
    for i in range(max_iters):
        if V.grad is not None:
            V.grad.zero_()
        loss = f(V)
        if verbose and i % 100 == 0:
            print(f"Iter {i}: loss = {loss.item():.6f}")
        if loss.item() < tol:
            if verbose:
                print(f"Stopping: loss {loss.item():.6g} < tol {tol}")
            break
        loss.backward()
        with torch.no_grad():
            # Zero out gradients for support vertices to keep them fixed
            V.grad[support_mask] = 0.0
            V -= lr * V.grad
    else:
        if verbose:
            print(f"Stopping: reached max_iters = {max_iters}")
    if verbose:
        print(f"Final loss: {loss.item():.6f}")
        try:
            from shape_stability import is_shape_stable
            # Try to infer E from closure if possible
            E = None
            if hasattr(f, '__closure__') and f.__closure__ is not None:
                for cell in f.__closure__:
                    val = cell.cell_contents
                    if isinstance(val, torch.Tensor) and val.dtype == torch.long and val.dim() == 2:
                        E = val
            if E is not None:
                is_stable, _, _, _ = is_shape_stable(V.detach(), E)
                print(f"Shape stable at end? {'YES' if is_stable else 'NO'}")
        except Exception as e:
            pass
    return V.detach()
