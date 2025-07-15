# 2D Shape Balancing Analysis

A Python implementation for analyzing and visualizing the balance of 2D shapes using B-spline representation. This project is inspired by the "Make It Stand" research and provides a simplified 2D version for educational purposes.

## Project Structure

```
python/
├── shape2d.py           # Core Shape2D class with B-spline representation
├── balance_analyzer.py  # Balance analysis and stability checking
├── shape_generator.py   # Predefined shapes for testing
├── demo.py             # Main demonstration script
├── requirements.txt    # Python dependencies
└── README.md          # This file
```

## Features

- **B-spline Shape Representation**: Smooth curve representation using control points
- **Balance Analysis**: Automatic detection of stability and balance
- **Visualization**: Rich visual output with matplotlib
- **Shape Modification**: Interactive control point manipulation
- **Predefined Shapes**: Collection of test shapes for demonstration

## Installation

1. **Install Python dependencies:**
   ```bash
   pip install -r requirements.txt
   ```

2. **Required packages:**
   - `numpy`: Numerical computations
   - `matplotlib`: Visualization
   - `scipy`: B-spline interpolation
   - `shapely`: Geometric operations

## Quick Start

### Basic Usage

```python
from shape2d import Shape2D
from balance_analyzer import BalanceAnalyzer

# Create a shape
control_points = [[0, 0], [2, 0], [3, 1], [2, 2], [1, 2.5], [0, 2], [-1, 1]]
shape = Shape2D(control_points)

# Analyze balance
analyzer = BalanceAnalyzer(shape)
analyzer.print_balance_report()
analyzer.visualize_balance()
```

### Run the Demo

```bash
cd python
python demo.py
```

This will run a comprehensive demonstration showing:
- Basic shape creation and visualization
- Balance analysis
- Shape modification
- Comparison of balanced vs unbalanced shapes
- Interactive shape creation

## Core Classes

### Shape2D

The main class for representing 2D shapes using B-splines.

**Key Methods:**
- `center_of_mass()`: Calculate the center of mass
- `area()`: Calculate the shape's area
- `visualize()`: Display the shape with optional elements
- `modify_control_point()`: Change control point positions
- `add_control_point()`: Add new control points
- `remove_control_point()`: Remove control points

### BalanceAnalyzer

Analyzes the stability and balance of shapes.

**Key Methods:**
- `is_balanced()`: Check if shape is balanced
- `get_balance_info()`: Get detailed balance information
- `visualize_balance()`: Show balance analysis visually
- `print_balance_report()`: Print detailed balance report

## Predefined Shapes

The `shape_generator.py` module provides several test shapes:

- `unbalanced`: Asymmetric shape that will fall over
- `balanced`: Symmetric shape that should be stable
- `triangle`: Simple triangle
- `rectangle`: Simple rectangle
- `circle`: Circle approximation
- `rocking_horse`: Complex curved shape
- `teardrop`: Naturally unbalanced teardrop shape

## Examples

### Creating a Custom Shape

```python
import numpy as np
from shape2d import Shape2D

# Define control points for your shape
control_points = np.array([
    [0, 0], [1, 0], [1.5, 1], [1, 2], [0, 2], [-0.5, 1]
])

# Create the shape
shape = Shape2D(control_points)

# Visualize it
shape.visualize()
```

### Analyzing Balance

```python
from balance_analyzer import BalanceAnalyzer

# Create analyzer
analyzer = BalanceAnalyzer(shape)

# Check if balanced
if analyzer.is_balanced():
    print("Shape is balanced!")
else:
    print("Shape will fall over!")

# Get detailed analysis
info = analyzer.get_balance_info()
print(f"Center of mass: {info['center_of_mass']}")
print(f"Support base: {info['support_base']}")
```

### Modifying Shapes

```python
# Modify a control point to try to balance the shape
shape.modify_control_point(2, [1.2, 1.0])

# Add a new control point
shape.add_control_point([0.5, 1.5])

# Visualize the changes
shape.visualize()
```

## Balance Analysis

The system analyzes balance by:

1. **Finding the support base**: Points where the shape contacts the ground
2. **Calculating center of mass**: Using the shape's geometry
3. **Checking stability**: Center of mass must be within the support base
4. **Computing stability margin**: Distance from center of mass to support edge

## Visualization Features

- **Shape boundary**: Blue curve showing the B-spline
- **Control points**: Red dots with dashed connections
- **Center of mass**: Green dot with coordinates
- **Support base**: Red line showing ground contact
- **Balance status**: Color-coded status indicator
- **Stability margin**: Visual indication of how stable the shape is

## Future Enhancements

- **Optimization algorithms**: Automatic shape modification for balance
- **3D extension**: Extend to 3D shapes
- **Interactive GUI**: Real-time shape manipulation
- **Export capabilities**: Save shapes in various formats
- **Advanced physics**: More realistic balance calculations

## Contributing

Feel free to extend this project with:
- New balance algorithms
- Additional shape types
- Improved visualization
- Performance optimizations

## License

This project is for educational purposes, inspired by the "Make It Stand" research from ETH Zurich and INRIA. 