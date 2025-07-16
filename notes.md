# 1st: What shape 
We define the shape we want to stabilize by 2 objects:
 - The vetrex `V=[[x0, y0],[x1, y1],...,[xn-1, yn-1]]`
 - The edges `E=[[0, 1],[1, 2],...]`


# 2nd: What function to minimize
We need to define a function `f` that if we minimize it we achieved diferent objectives. These are:
 1. Stabilization thanks to carving (aka moving the center of mass)
 2. Smoothing the surface
 3. Similarity to the initial surface

Let's get to it.

## Stabilization thanks to carving



## Smoothing the surface



## Similarity to the initial surface




```python
>>> x = x / np.linalg.norm(x)
```
