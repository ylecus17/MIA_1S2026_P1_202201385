import { useEffect, useState } from "react";
import "./Discos.css";

function Discos() {
  const [discos, setDiscos] = useState([]);
  const [vista, setVista] = useState("discos"); // discos | particiones
  const [discoSeleccionado, setDiscoSeleccionado] = useState(null);
  const [particionSeleccionada, setParticionSeleccionada] = useState(null);

  useEffect(() => {
    fetch("http://localhost:5300/disks")
      .then((res) => res.json())
      .then((data) => {
        setDiscos(data.disks);
      })
      .catch((error) => {
        console.error("Error al obtener discos:", error);
      });
  }, []);

  return (
    <div className="contenedor-discos">
      <h2>Visualizador de sistema de archivos</h2>

      {vista === "discos" && (
        <>
          <p>Haz doble clic en un disco para ver sus particiones.</p>
          <div className="grid-discos">
            {discos.map((d, idx) => (
              <div
                key={idx}
                className="tarjeta-disco"
                onClick={() => setDiscoSeleccionado(d)}
                onDoubleClick={() => {
                  setDiscoSeleccionado(d);
                  setVista("particiones");
                }}
              >
                <img
                  src="https://cdn-icons-png.flaticon.com/512/1110/1110837.png"
                  alt="Disco"
                  className="imagen-disco"
                />
                <p className="nombre-disco">{d.name}</p>
              </div>
            ))}
          </div>

          {discoSeleccionado && (
            <div className="propiedades-disco">
              <h3>Propiedades del disco</h3>
              <p><strong>Nombre:</strong> {discoSeleccionado.name}</p>
              <p><strong>Fit:</strong> {discoSeleccionado.fit}</p>
              <p><strong>Tamaño:</strong> {discoSeleccionado.sizeBytes} bytes</p>
              <p><strong>Path:</strong> {discoSeleccionado.path}</p>
            </div>
          )}
        </>
      )}

      {vista === "particiones" && discoSeleccionado && (
        <>
          <button onClick={() => { setVista("discos"); setParticionSeleccionada(null); }}>
            ← Volver a discos
          </button>
          <h3>Particiones de {discoSeleccionado.name}</h3>
          <div className="grid-discos">
            {discoSeleccionado.particionesMontadas.map((p, idx) => (
              <div
                key={idx}
                className="tarjeta-disco"
                onClick={() => setParticionSeleccionada(p)}
              >
                <img
                  src="https://cdn-icons-png.flaticon.com/512/2729/2729129.png"
                  alt="Partición"
                  className="imagen-disco"
                />
                <p className="nombre-disco">{p.name}</p>
              </div>
            ))}
          </div>

          {particionSeleccionada && (
            <div className="propiedades-disco">
              <h3>Propiedades de la partición</h3>
              <p><strong>Nombre:</strong> {particionSeleccionada.name}</p>
              <p><strong>ID:</strong> {particionSeleccionada.id}</p>
              <p><strong>Fit:</strong> {particionSeleccionada.fit}</p>
              <p><strong>Tamaño:</strong> {particionSeleccionada.size} bytes</p>
              <p><strong>Path:</strong> {particionSeleccionada.path}</p>
            </div>
          )}
        </>
      )}
    </div>
  );
}

export default Discos;
