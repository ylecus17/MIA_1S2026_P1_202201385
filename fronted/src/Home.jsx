import { useState, useRef, useEffect } from 'react';
import './Home.css';
import { useNavigate } from "react-router-dom";

function Home({ textoEntrada, setTextoEntrada, salida, setSalida, loginStatus, setLoginStatus }) {
  const [nombreArchivo, setNombreArchivo] = useState('');
  const refInputArchivo = useRef(null);
  const refSalida = useRef(null);
  const navigate = useNavigate();

  useEffect(() => {
    if (refSalida.current) {
      refSalida.current.scrollTop = refSalida.current.scrollHeight;
    }
  }, [salida]);

  const manejarLogout = async () => {
    try {
      const token = loginStatus?.token || localStorage.getItem("token");
      const res = await fetch("http://localhost:5300/api/logout", {
        method: "POST",
        headers: { "Authorization": "Bearer " + token }
      });

      const data = await res.json();

      if (data.success) {
        setSalida([{ level: "info", text: "Sesión cerrada correctamente" }]);
        setLoginStatus(null);
        localStorage.removeItem("token");
      } else {
        setSalida([{ level: "error", text: `Error: ${data.error}` }]);
      }
    } catch (error) {
      setSalida([{ level: "error", text: `Error al conectar con backend: ${error.message}` }]);
    }
  };

  const manejarCambioArchivo = async (e) => {
    const archivo = e.target.files[0];
    if (archivo) {
      setNombreArchivo(archivo.name);
      try {
        const contenido = await archivo.text();
        setTextoEntrada(contenido);
      } catch (error) {
        setSalida([{ level: 'error', text: `Error al leer el archivo: ${error.message}` }]);
      }
    } else {
      setNombreArchivo('');
      setTextoEntrada('');
    }
  };

  const manejarEjecucion = async () => {
    if (textoEntrada.trim()) {
      try {
        const res = await fetch("http://localhost:5300/execute", {
          method: "POST",
          headers: { "Content-Type": "text/plain" },
          body: textoEntrada
        });

        const data = await res.json();
        setSalida(data.output);
      } catch (error) {
        setSalida([{ level: 'error', text: `Error al conectar con backend: ${error.message}` }]);
      }
    } else {
      setSalida([{ level: 'info', text: 'Por favor ingresa texto o selecciona un archivo primero' }]);
    }
  };

  const manejarLimpiar = () => {
    setNombreArchivo('');
    setTextoEntrada('');
    setSalida([]);
    if (refInputArchivo.current) {
      refInputArchivo.current.value = '';
    }
  };

  return (
    <div className="contenedor-inicio">
      <header className="encabezado">
        <div className="grupo-botones">
          <div className="contenedor-input-archivo">
            <input
              type="file"
              id="inputArchivo"
              ref={refInputArchivo}
              onChange={manejarCambioArchivo}
              className="input-archivo"
              accept=".smia"
            />
            <label htmlFor="inputArchivo" className="boton-archivo">
              <span className="icono-archivo">📄</span>
              Elegir archivo
            </label>
            {nombreArchivo && (
              <span className="nombre-archivo">{nombreArchivo}</span>
            )}
          </div>

          <div className="botones-accion">
            <button onClick={manejarEjecucion} className="boton-ejecutar">Ejecutar</button>
            <button onClick={manejarLimpiar} className="boton-limpiar">Limpiar</button>

            {loginStatus ? (
              <button onClick={manejarLogout} className="boton-logout">Logout</button>
            ) : (
              <button onClick={() => navigate("/login")} className="boton-login">Login</button>
            )}
          </div>
        </div>
      </header>

      <main className="contenedor-areas-texto">
        <div className="contenedor-area">
          <label htmlFor="areaEntrada" className="etiqueta-area">Área de Entrada</label>
          <textarea
            id="areaEntrada"
            className="area-texto area-entrada"
            value={textoEntrada}
            onChange={(e) => setTextoEntrada(e.target.value)}
            placeholder="Selecciona un archivo o escribe texto aquí..."
          />
        </div>

        <div className="contenedor-area">
          <label className="etiqueta-area">Área de Salida</label>
          <div ref={refSalida} className="area-salida-consola">
            {salida.map((msg, idx) => (
              <div key={idx} className={`log-${msg.level}`}>
                {msg.text}
              </div>
            ))}
          </div>
        </div>
      </main>
    </div>
  );
}

export default Home;
