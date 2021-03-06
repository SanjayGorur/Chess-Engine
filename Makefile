CXX = g++
CXXFLAGS = -O3 -Wall -Werror -Wextra -Wpedantic -Weffc++ -D_FORTIFY_SOURCE=2 -std=c++17 -I $(INCLUDE)
INCLUDE = include
SRCDIR = src
OBJDIR = obj
SRC = $(wildcard $(SRCDIR)/*.cc)
OBJS = $(patsubst $(SRCDIR)/%.cc,$(OBJDIR)/%.o,$(SRC))
LINLOCATION = bin/linux/ce
NAME = ce
MW = x86_64-w64-mingw32-g++
MWFLAGS = -O3 -std=c++17 -I $(INCLUDE) -static-libgcc -static-libstdc++ -static -D__NO_INLINE__ -Wall -Werror
MWLOCATION = bin/win64/ce.exe
DEBUGFLAGS= -g -fsanitize=address
DOX = doxygen
DOXFILE = Doxyfile
DOCDIR = doc


all: ce

ce: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(NAME)

linux: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(LINLOCATION) $(OBJS)

windows:
	$(MW) $(MWFLAGS) -o $(MWLOCATION) $(SRC)

$(OBJDIR)/%.o: $(SRCDIR)/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

docs: 
	(cd $(DOCDIR); $(DOX) $(DOXFILE))

depend: .depend

.depend: $(SRC)
	rm -f ./.depend
	$(CXX) $(CXXFLAGS) -MM $^>>./.depend;

dist-clean: clean
	rm -f *~ .depend

include .depend
