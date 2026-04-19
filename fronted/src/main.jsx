import { StrictMode, useState } from "react";
import { createRoot } from "react-dom/client";
import { BrowserRouter, Routes, Route } from "react-router-dom";
import Home from "./Home";
import Discos from "./Discos";
import Navbar from "./navbar";

function App() {
  // Estado levantado al padre
  const [textoEntrada, setTextoEntrada] = useState("");
  const [salida, setSalida] = useState([]);

  return (
    <>
      <Navbar />
      <Routes>
        <Route
          path="/"
          element={
            <Home
              textoEntrada={textoEntrada}
              setTextoEntrada={setTextoEntrada}
              salida={salida}
              setSalida={setSalida}
            />
          }
        />
        <Route path="/discos" element={<Discos />} />
      </Routes>
    </>
  );
}

createRoot(document.getElementById("root")).render(
  <StrictMode>
    <BrowserRouter>
      <App />
    </BrowserRouter>
  </StrictMode>
);
