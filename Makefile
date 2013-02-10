all: clone_gui

clean:
	$(RM) -f clone_gui

vix-disklib-sample: clone_gui.cpp
	$(CXX) -o $@ `pkg-config --cflags --libs vix-disklib` `pkg-config --cflags --libs gtk+-3.0` $?
