import torch
import sys
import os

sys.path.append(r"C:\Users\Hello\Desktop\Make-it-Stand-with-Quadrics-\src")

from shape_mass_center import calculate_center_of_mass, Shape2D

def test_donut_shape():
    """Tests the center of mass calculation for a donut shape."""
    donut_vertices_data = [
        [0, 0], [4, 0], [4, 4], [0, 4],  
        [1, 1], [1, 3], [3, 3], [3, 1]   
    ]
    donut_edges_data = [
        [0, 1], [1, 2], [2, 3], [3, 0],  
        [4, 5], [5, 6], [6, 7], [7, 4]   
    ]

    # --- Test 1: Using the Shape2D object API ---
    print("## Testing Donut with a Shape2D object ##")
    shape = Shape2D(
        vertices=torch.tensor(donut_vertices_data, dtype=torch.float32),
        edges=donut_edges_data
    )
    
    area, com = calculate_center_of_mass(shape)
    
    print(f"Donut Area: {area:.4f}")
    print(f"Donut Center of Mass: {com.round(decimals=4).tolist()}")
    
    # --- Test 2: Using separate vertex and edge tensors ---
    print("\n## Testing Donut with separate tensors ##")
    vertices_tensor = torch.tensor(donut_vertices_data, dtype=torch.float32)
    edges_tensor = torch.tensor(donut_edges_data, dtype=torch.long)

    area_t, com_t = calculate_center_of_mass(vertices_tensor, edges_tensor)

    print(f"Donut Area: {area_t:.4f}")
    print(f"Donut Center of Mass: {com_t.round(decimals=4).tolist()}")

if __name__ == "__main__":
    test_donut_shape()
