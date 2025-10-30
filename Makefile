# ===============================
#  Proyecto Integrador XML-RPC
# ===============================

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude
SRC_DIR = src
APP_DIR = app
BIN_DIR = bin

# Archivos fuente principales (C++)
SRCS = $(wildcard $(SRC_DIR)/*.cpp $(APP_DIR)/*.cpp)
OBJS = $(SRCS:.cpp=.o)

# Librerías necesarias
LIBS = -lpthread -ldl -lsqlite3

# Ejecutables
SERVER = $(BIN_DIR)/server
CLIENT = $(BIN_DIR)/client
CLIENTCMD = $(BIN_DIR)/clientcmd

# ===============================
#  Targets principales
# ===============================

all: $(SERVER) $(CLIENT) $(CLIENTCMD)

$(SERVER): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $(filter %server.o,$(OBJS)) $(filter %XmlRpc%.o,$(OBJS)) $(filter %Mensaje.o,$(OBJS)) \
	$(filter %Usuario.o,$(OBJS)) $(filter %ValidadorUsuario.o,$(OBJS)) $(filter %PALogger.o,$(OBJS)) \
	$(filter %InterpreteDeComandos.o,$(OBJS)) $(filter %Reporte.o,$(OBJS)) $(filter %Archivo.o,$(OBJS)) $(LIBS)

$(CLIENT): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $(filter %client.o,$(OBJS)) $(filter %Mensaje.o,$(OBJS)) $(filter %XmlRpc%.o,$(OBJS)) $(LIBS)

$(CLIENTCMD): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $(filter %clientcmd.o,$(OBJS)) $(filter %Mensaje.o,$(OBJS)) $(filter %XmlRpc%.o,$(OBJS)) $(LIBS)

# ===============================
#  Clientes Python
# ===============================

# Ejecuta el cliente Python (versión básica)
client_py:
	python3 $(APP_DIR)/client_rpc.py 127.0.0.1 8080

# Ejecuta el cliente Python con menú CLI
client_py_cli:
	python3 $(APP_DIR)/client_cli.py 127.0.0.1 8080

# ===============================
#  Utilidades
# ===============================

clean:
	rm -f $(SRC_DIR)/*.o $(APP_DIR)/*.o
	rm -rf $(BIN_DIR)

rebuild: clean all

.PHONY: all clean rebuild client_py client_py_cli

