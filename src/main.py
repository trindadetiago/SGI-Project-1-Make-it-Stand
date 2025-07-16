import sys
import os
from PyQt5.QtWidgets import QApplication
from PyQt5.QtGui import QIcon
from shape_gui import ShapeGUI

def main():
    app = QApplication(sys.argv)
    icon_path = os.path.join(os.path.dirname(__file__), '..', 'assets', 'mit.png')
    app.setWindowIcon(QIcon(icon_path))
    gui = ShapeGUI()
    gui.show()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main() 