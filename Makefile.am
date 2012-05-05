ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = frobnicator

frobnicator_CXXFLAGS = -Wall -I ${top_srcdir}/src
frobnicator_LDADD = -lyaml -lSDL -lSDL_image -lGL -lGLU
frobnicator_SOURCES = \
	src/main.cpp src/common.cpp \
	src/backend.cpp src/backend.hpp \
	src/backend_sdl.cpp \
	src/game.cpp src/game.hpp \
	src/entity.cpp src/entity.hpp \
	src/level.cpp src/level.hpp \
	src/region.cpp src/region.hpp \
	src/tilemap.cpp src/tilemap.hpp \
	src/waypoint.cpp src/waypoint.hpp
