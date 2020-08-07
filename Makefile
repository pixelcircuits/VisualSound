# Target Name
TARGET=VisualSound

# Include/Lib Directories
INCDIR=-I/usr/include/dbus-1.0 -I/usr/lib/arm-linux-gnueabihf/dbus-1.0/include -I/home/pi/rpi-rgb-led-matrix/include -I/home/pi/rpi_ws281x
LIBDIR=-L/home/pi/rpi-rgb-led-matrix/lib -L/home/pi/rpi_ws281x

# Code Directories
BUILDDIR=build
SOURCEDIR=source
VISUALIZERDIR=visualizers
BASEDIR=core

# Objects to Build
OBJECTS=$(BUILDDIR)/main.o $(BUILDDIR)/CVideoDriver.o $(BUILDDIR)/CSoundAnalyzer.o $(BUILDDIR)/CSettingsManager.o $(BUILDDIR)/CSongDataManager.o \
		$(BUILDDIR)/CRoundVisualizer.o $(BUILDDIR)/CStraightVisualizer.o \
        $(BUILDDIR)/led.o $(BUILDDIR)/bt.o $(BUILDDIR)/snd.o $(BUILDDIR)/dbs.o $(BUILDDIR)/inp.o $(BUILDDIR)/pair.o \

# Libraries to Include
LIBRARIES=-lasound -lpthread -ldbus-1 -lrgbmatrix -lws2811

# Compiler
CC=gcc
CXX=g++

# Flags
CFLAGS=-lrt -lm -lstdc++
CXXFLAGS=$(CFLAGS)

#===============================================================================

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(CFLAGS) $(LIBDIR) $(LIBRARIES)

$(BUILDDIR)/%.o : $(SOURCEDIR)/%.cpp
	$(CXX) $(INCDIR) -I$(SOURCEDIR)/$(BASEDIR) $(CFLAGS) -c -o $@ $<
	
$(BUILDDIR)/%.o : $(SOURCEDIR)/$(VISUALIZERDIR)/%.cpp
	$(CXX) $(INCDIR) $(CFLAGS) -c -o $@ $<
	
$(BUILDDIR)/%.o : $(SOURCEDIR)/$(BASEDIR)/%.c
	$(CXX) $(INCDIR) -I$(SOURCEDIR)/$(BASEDIR) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET)
	rm -f $(BUILDDIR)/*.o

.PHONY: FORCE
