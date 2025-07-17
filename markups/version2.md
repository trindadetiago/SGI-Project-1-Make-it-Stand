FUNCTION Calculate_Center_of_Mass_From_Mesh(vertices, edges)

  // Step 1: Find all separate, closed loops from the edge list
  all_loops = find_all_closed_loops(edges, number_of_vertices)

  IF no loops were found THEN
    // Handle cases with no closed shapes
    RETURN 0, average_of(vertices)
  END IF

  // Step 2: Initialize total sums for all geometric properties
  DECLARE total_signed_area_sum = 0
  DECLARE total_cx_sum = 0
  DECLARE total_cy_sum = 0

  // Step 3: Iterate over each found loop (outer boundary and holes)
  FOR each loop_indices IN all_loops
    // Get the actual vertex coordinates for the current loop
    loop_vertices = get_vertices_from_indices(vertices, loop_indices)

    // Calculate properties for this single loop
    // Note: The sign of the area depends on the loop's winding order (CW or CCW)
    loop_area_sum = 0
    loop_cx_sum = 0
    loop_cy_sum = 0

    FOR i FROM 0 TO (number_of_vertices_in_loop - 1)
      p1 = loop_vertices[i]
      p2 = loop_vertices[(i + 1) % number_of_vertices_in_loop] // Wrap around

      cross_product = p1.x * p2.y - p2.x * p1.y
      
      loop_area_sum = loop_area_sum + cross_product
      loop_cx_sum = loop_cx_sum + (p1.x + p2.x) * cross_product
      loop_cy_sum = loop_cy_sum + (p1.y + p2.y) * cross_product
    END LOOP

    // Add this loop's properties to the running totals
    total_signed_area_sum = total_signed_area_sum + loop_area_sum
    total_cx_sum = total_cx_sum + loop_cx_sum
    total_cy_sum = total_cy_sum + loop_cy_sum
  END LOOP

  // Step 4: Calculate final results from the combined totals
  final_signed_area = 0.5 * total_signed_area_sum

  IF absolute(final_signed_area) IS VERY_SMALL THEN
    RETURN 0, average_of(vertices) // Avoid division by zero
  END IF

  final_cx = total_cx_sum / (3 * total_signed_area_sum) // Simplified from / (6 * 0.5 * area)
  final_cy = total_cy_sum / (3 * total_signed_area_sum)

  // Return the absolute area and the final centroid
  RETURN absolute(final_signed_area), (final_cx, final_cy)

END FUNCTION