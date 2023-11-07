import sys
from PyQt5.QtWidgets import QApplication, QMainWindow, QMenuBar, QAction
from PyQt5.QtOpenGL import QGLWidget
from OpenGL.GL import *

class MyOpenGLWidget(QGLWidget):
    def initializeGL(self):
        glEnable(GL_DEPTH_TEST)
        glClearColor(0.0, 0.0, 0.0, 1.0)

    def paintGL(self):
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        # Add your 3D model rendering code here

app = QApplication(sys.argv)

mainWin = QMainWindow()
mainWin.setWindowTitle("3D Modeling Software")

menuBar = mainWin.menuBar()

fileMenu = menuBar.addMenu("File")
openAction = QAction("Open", mainWin)
saveAction = QAction("Save", mainWin)
exitAction = QAction("Exit", mainWin)
fileMenu.addAction(openAction)
fileMenu.addAction(saveAction)
fileMenu.addSeparator()
fileMenu.addAction(exitAction)

editMenu = menuBar.addMenu("Edit")
createCubeAction = QAction("Create Cube", mainWin)
editMenu.addAction(createCubeAction)

exitAction.triggered.connect(app.quit)

mainWin.setCentralWidget(MyOpenGLWidget(mainWin))

mainWin.show()
sys.exit(app.exec_())
