# Make-it-Stand-with-Quadrics

How do we ensure a 3D printed object can stand in a desired orientation but still closely approximate the original target shape?
This results in a cool shape (and density) optimization problem with a surprisingly simple class of solutions. Exploring how shape and
density affect stability will involve learning many fundamentals in geometric processing and optimization!

---

## Getting Started (For Beginners)

This project includes a graphical tool to create, visualize, and edit 2D shapes (meshes). You do **not** need to be a Python expert to use it!

### 1. Install Python

- Download and install Python 3.8 or newer from [python.org](https://www.python.org/downloads/).
- During installation, check the box that says **"Add Python to PATH"**.

### 2. Install Dependencies

- Open a terminal (Command Prompt or PowerShell on Windows).
- Navigate to the project folder (where this README is).
- Run:
  ```sh
  pip install -r src/requirements.txt
  ```

### 3. Start the GUI

- In the terminal, run:
  ```sh
  python src/main.py
  ```
- The graphical interface will open.

---

## Features & How to Use the GUI

The GUI is organized into tabs at the top:

### **Visualize**

- Load and view existing shapes from the `src/shapes/` folder.
- Show/hide vertex labels, fill the shape, and toggle a grid.

### **Process**

- Apply simple processing (e.g., scale the shape).
- Save the processed shape.

### **New**

- Draw a new shape from scratch:
  - **Add Vertices:** Click to add points.
  - **Add Edges:** Click two vertices to connect them.
  - **Save Shape:** Save your creation to a file.
  - **Clear:** Start over with a blank canvas.
- See the live count of vertices and edges.

### **Edit**

- Select a vertex by clicking it.
- Drag to move the selected vertex.
- Deselect with the button.
- Save (overwrite) the current shape file.

---

## File Structure (For Advanced Users)

- `src/` — Main source folder
  - `main.py` — Entry point to launch the GUI
  - `viewer/` — All GUI and visualization code
    - `shape_gui.py` — Main GUI logic
    - `tab_visualize.py`, `tab_process.py`, `tab_new.py`, `tab_edit.py` — GUI tabs
  - `shape2d.py` — Shape data structure and I/O
  - `shape_processing.py` — Shape processing functions
  - `shapes/` — Example and saved shape files (JSON)
  - `requirements.txt` — Python dependencies

---

## FAQ

- **I get an error about missing packages?**
  - Make sure you ran `pip install -r src/requirements.txt`.
- **How do I add my own shapes?**
  - Save them in the `src/shapes/` folder as `.json` files.
- **Can I use this on Mac or Linux?**
  - Yes! Just follow the same steps in your terminal.

---

## Need Help?

If you get stuck, ask for help!
