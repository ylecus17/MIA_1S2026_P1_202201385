# Compilador y flags
CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

# Busca todos los .cpp en subcarpetas
SRC = $(wildcard *.cpp api/*.cpp commands/*.cpp common/*.cpp parser/*.cpp structures/*.cpp)

OBJ = $(SRC:.cpp=.o)

# Nombre del ejecutable
TARGET = server

# Regla principal
all: $(TARGET)

# Cómo construir el ejecutable
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Cómo compilar cada .cpp a .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpiar archivos generados
clean:
	rm -f $(OBJ) $(TARGET)
