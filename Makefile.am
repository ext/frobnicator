ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = frobnicator

frobnicator_CXXFLAGS = -Wall -I ${top_srcdir}/src
frobnicator_LDADD = -lyaml -lSDL
frobnicator_SOURCES = \
	src/main.cpp \
	src/backend.cpp src/backend.hpp \
	src/backend_sdl.cpp \
	src/game.cpp src/game.hpp \
	src/level.cpp src/level.hpp 
