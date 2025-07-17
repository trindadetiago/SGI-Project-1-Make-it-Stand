FUNCTION Calculate_Center_of_Mass(shape_input)

  // Step 1: Get the list of vertices for the shape's boundary
  DECLARE vertices
  IF shape_input IS A CURVE_FUNCTION THEN
    // If input is a function that defines a shape, sample it to get points
    vertices = sample_points_from_curve(shape_input, resolution_points)
  ELSE IF shape_input IS A LIST_OF_POINTS THEN
    // If input is already a list of vertices, use it directly
    vertices = shape_input
  ELSE
    THROW ERROR "Invalid input type. Must be a curve function or list of vertices."
  END IF
  // Step 2: Calculate area and centroid sums using the polygon formula
  DECLARE area_sum = 0
  DECLARE cx_sum = 0
  DECLARE cy_sum = 0
  // Loop through each edge of the polygon (from vertex i to i+1)
  FOR i FROM 0 TO (number_of_vertices - 1)
    p1 = vertices[i]
    p2 = vertices[(i + 1) % number_of_vertices] // Wrap around for the last edge
    // The 2D cross product gives twice the signed area of the triangle (Origin, p1, p2)
    cross_product = p1.x * p2.y - p2.x * p1.y
    area_sum = area_sum + cross_product
    cx_sum = cx_sum + (p1.x + p2.x) * cross_product
    cy_sum = cy_sum + (p1.y + p2.y) * cross_product
  END LOOP
  // Step 3: Calculate final area and centroid
  DECLARE total_area = 0.5 * area_sum
  IF absolute(total_area) < 1e-9 THEN
    // For a line or point, the centroid is the average of the vertices
    RETURN 0, average_of(vertices)
  END IF
  DECLARE final_cx = cx_sum / (6 * total_area)
  DECLARE final_cy = cy_sum / (6 * total_area)
  RETURN total_area, (final_cx, final_cy)
END FUNCTION