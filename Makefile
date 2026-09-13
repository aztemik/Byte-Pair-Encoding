CXX = clang++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -MMD -MP

# Lista de todos los archivos .cpp que componen el proyecto
SOURCES = main.cpp \
		manipulateBinaryFiles/BinaryFileAHandler.cpp \
		manipulateBinaryFiles/BinaryFileBHandler.cpp \
		manipulateBinaryFiles/BinaryFileCHandler.cpp \
		manipulateBinaryFiles/BinaryFileDHandler.cpp \
		manipulateBinaryFiles/GeneralBinaryFileHandler.cpp \
		tokenizer/tokenizer.cpp \
		manipulateBinaryFiles/specialTokens.cpp

# Genera los nombres de los archivos objeto (.o) y de dependencias (.d)
OBJECTS = $(SOURCES:.cpp=.o)
DEPS = $(OBJECTS:.o=.d)

# Nombre del ejecutable final
EXECUTABLE = main

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Reglas de dependencia generadas por -MMD: si cambia un .h, se recompila
# el .o que lo incluye. Sin esto, editar un header no dispara recompilacion.
-include $(DEPS)

clean:
	rm -f $(EXECUTABLE) $(OBJECTS) $(DEPS)

.PHONY: all clean
