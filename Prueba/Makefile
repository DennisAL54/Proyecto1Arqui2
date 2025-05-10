# Compilador y flags
CXX := g++
CXXFLAGS := -std=c++17 -pthread
TARGET := sim

# Archivos fuente y objetos
SRCS := main.cpp PE.cpp Interconnect.cpp
OBJS := $(SRCS:.cpp=.o)

# Regla principal
all: $(TARGET)

# Enlazado
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Dependencias actualizadas
main.o: main.cpp PE.h Interconnect.h Message.h
PE.o: PE.cpp PE.h Interconnect.h Message.h
Interconnect.o: Interconnect.cpp Interconnect.h PE.h Message.h

# Regla genérica para .cpp -> .o
%.o: %..cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpiar
clean:
	rm -f $(OBJS) $(TARGET)

# Ejecutar
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
