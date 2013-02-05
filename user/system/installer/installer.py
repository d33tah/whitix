#!/Applications/py

import wgui
import sys

def GuiCreate():
	w = wgui.Window()
	w.setTitle("Whitix installer")
	w.setDimensions(50, 50, 800, 600)
	w.setBackgroundColor(0xD8, 0xD0, 0xC8)
	w.setMinSize(200, 100)
	w.setMaxSize(800, 600)
	
	w.main()

def main():
	GuiCreate()

if __name__ == "__main__":
	sys.exit(main())
