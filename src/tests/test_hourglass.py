import torch
import sys
import os

sys.path.append(r"C:\Users\Hello\Desktop\Make-it-Stand-with-Quadrics-\src")

from shape_mass_center import calculate_center_of_mass, Shape2D

def test_hourglass_shape():
    """Tests the center of mass calculation for an hourglass shape."""
    hourglass_vertices_data = [
        [-2, 0], [-2, 2], [0, 2], [0, 0],
        [2, 0], [2, -2], [0, -2]
    ]
    hourglass_edges_data = [
        [0, 1], [1, 2], [2, 3], [3, 0],
        [3, 4], [4, 5], [5, 6], [6, 3]
    ]

    # --- Test 1: Using the Shape2D object API ---
    print("## Testing Hourglass with a Shape2D object ##")
    shape = Shape2D(
        vertices=torch.tensor(hourglass_vertices_data, dtype=torch.float32),
        edges=hourglass_edges_data
    )
    
    area, com = calculate_center_of_mass(shape)
    
    print(f"Hourglass Area: {area:.4f}")
    print(f"Hourglass Center of Mass: {com.round(decimals=4).tolist()}")

    # --- Test 2: Using separate vertex and edge tensors ---
    print("\n## Testing Hourglass with separate tensors ##")
    vertices_tensor = torch.tensor(hourglass_vertices_data, dtype=torch.float32)
    edges_tensor = torch.tensor(hourglass_edges_data, dtype=torch.long)

    area_t, com_t = calculate_center_of_mass(vertices_tensor, edges_tensor)

    print(f"Hourglass Area: {area_t:.4f}")
    print(f"Hourglass Center of Mass: {com_t.round(decimals=4).tolist()}")

if __name__ == "__main__":
    test_hourglass_shape()
