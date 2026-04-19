# Compilador y flags
CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

# Carpetas
SRC_DIRS = . api commands common parser structures
BUILD_DIR = build

# Buscar todos los .cpp
SRC = $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))

# Convertir rutas a .o dentro de build/
OBJ = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRC))

# Ejecutable
TARGET = server

# Regla principal
all: $(TARGET)

# Crear ejecutable
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Regla para compilar .cpp → .o en build/
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpiar
clean:
	rm -rf $(BUILD_DIR) $(TARGET)