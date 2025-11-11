# ===============================
#  Proyecto Integrador XML-RPC
# ===============================

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Iinclude
SRC_DIR := src
APP_DIR := app
BIN_DIR := bin

LIBS := -lpthread -ldl -lsqlite3

# ===============================
#  Objetos comunes
# ===============================

COMMON_OBJS := \
  $(SRC_DIR)/Mensaje.o \
  $(SRC_DIR)/Usuario.o \
  $(SRC_DIR)/ValidadorUsuario.o \
  $(SRC_DIR)/PALogger.o \
  $(SRC_DIR)/InterpreteDeComandos.o \
  $(SRC_DIR)/Reporte.o \
  $(SRC_DIR)/Archivo.o \
  $(SRC_DIR)/Controlador.o

XMLRPC_OBJS := \
  $(SRC_DIR)/XmlRpcClient.o \
  $(SRC_DIR)/XmlRpcDispatch.o \
  $(SRC_DIR)/XmlRpcServer.o \
  $(SRC_DIR)/XmlRpcServerConnection.o \
  $(SRC_DIR)/XmlRpcServerMethod.o \
  $(SRC_DIR)/XmlRpcSocket.o \
  $(SRC_DIR)/XmlRpcSource.o \
  $(SRC_DIR)/XmlRpcUtil.o \
  $(SRC_DIR)/XmlRpcValue.o

SERVER := $(BIN_DIR)/server
CLIENT := $(BIN_DIR)/client
CLIENTCMD := $(BIN_DIR)/clientcmd

# ===============================
#  Reglas de compilación
# ===============================

all: $(SERVER) $(CLIENT) $(CLIENTCMD)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(APP_DIR)/%.o: $(APP_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# ===============================
#  Ejecutables principales
# ===============================

$(SERVER): $(APP_DIR)/server.o $(COMMON_OBJS) $(XMLRPC_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $^ $(LIBS)

$(CLIENT): $(APP_DIR)/client.o $(SRC_DIR)/Mensaje.o $(XMLRPC_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $^ $(LIBS)

$(CLIENTCMD): $(APP_DIR)/clientcmd.o $(SRC_DIR)/Mensaje.o $(XMLRPC_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $^ $(LIBS)

# ===============================
#  Clientes Python
# ===============================

client_py:
	python3 $(APP_DIR)/client_rpc.py 127.0.0.1 8080

client_py_cli:
	python3 $(APP_DIR)/client_cli.py 127.0.0.1 8080

client_gui:
	python3 $(APP_DIR)/interfaz_cl.py

client_pkg:
	python3 $(APP_DIR)/client_pkg_main.py 127.0.0.1 8080

# ===============================
#  Tests C++
# ===============================

TESTS := $(BIN_DIR)/tests

test: $(TESTS)
	./$(TESTS)

$(TESTS): $(wildcard $(SRC_DIR)/*.cpp) tests/tests.cpp
	@mkdir -p $(BIN_DIR)
	$(CXX) -std=c++17 -Wall -Iinclude $^ -o $@ $(LIBS)

# ===============================
#  Tests Python (cliente)
# ===============================

# Si los tests están dentro de app/tests_py/
test_py:
	PYTHONPATH=$(APP_DIR):. python3 -m unittest -v \
		app.tests_py.test_mensaje \
		app.tests_py.test_interfaz \
		app.tests_py.test_cliente

# ===============================
#  Utilidades
# ===============================

clean:
	rm -f $(SRC_DIR)/*.o $(APP_DIR)/*.o
	rm -rf $(BIN_DIR)

rebuild: clean all

.PHONY: all clean rebuild client_py client_py_cli client_pkg client_gui test test_py


