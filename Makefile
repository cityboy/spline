CC = c++
RM = rm

PKGDIR = $(HOME)/Packages
GLEWDIR = $(PKGDIR)/glew-1.12.0
GLFWDIR = $(PKGDIR)/glfw-3.1.1
GLMDIR = $(PKGDIR)/glm-0.9.6.3
JPGDIR = $(PKGDIR)/jpeg-9a
X11DIR = /usr/X11R6
CFLAGS =  
CDEFS = 
IPATH = -I$(HOME)/Projects -I$(GLFWDIR)/include -I$(GLEWDIR)/include -I$(GLMDIR) -I$(JPGDIR)
LPATH = -L$(GLFWDIR)/lib -L$(GLEWDIR)/lib -L$(JPGDIR)
LFLAGS = -framework OpenGL -framework Cocoa -framework CoreVideo -framework IOKit
#LDPATH = -Wl,-R/usr/pkg/lib
#-- have to link the same libraries multiple times because for the interdependency
LIBS = -lglfw3 -lGLEWs -ljpeg $(HOME)/Projects/common/shader.o $(HOME)/Projects/common/Image.o

all: register

register: register.o
	$(CC) -o $@ $^ $(LFLAGS) $(LPATH) $(LIBS)
	
.cpp.o: 
	$(CC) $(CDEFS) $(CFLAGS) -c -o $@ $^ $(IPATH)
	
clean:
	$(RM) -f *.o *.gch warp



