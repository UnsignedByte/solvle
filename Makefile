SRC_DIR := src
LIB_DIR := lib
OBJ_DIR := obj
BIN_DIR := bin

EXE := $(BIN_DIR)/exe
INIT := $(BIN_DIR)/init

LIB := $(wildcard $(LIB_DIR)/*.cpp)
OBJ := $(patsubst $(LIB_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(LIB)) # Convert .cpp files to .o

CXXFLAGS := -pthread -MMD -MP -Ilib -g -std=c++17
LDLIBS := -lsfml-graphics -lsfml-window -lsfml-system -lGL -lpthread

.PHONY: build dev clean

build: $(EXE) $(INIT)

$(INIT): $(OBJ) $(OBJ_DIR)/init.o | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(EXE): $(OBJ) $(OBJ_DIR)/main.o | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LDLIBS)

$(OBJ_DIR)/%.o: $(LIB_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LDLIBS)

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR)

-include $(OBJ:.o=.d)