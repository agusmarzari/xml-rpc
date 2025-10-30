DROP TABLE IF EXISTS usuarios;

CREATE TABLE usuarios (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    nombre TEXT NOT NULL UNIQUE,
    clave TEXT NOT NULL,
    privilegio TEXT NOT NULL
);

INSERT INTO usuarios (nombre, clave, privilegio) VALUES
('juan', '1234', 'operario'),
('camila', 'abcd', 'operario'),
('julieta', 'pass1', 'operario'),
('agustina', '4321', 'operario'),
('cesar', 'admin', 'admin');

