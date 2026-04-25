import { StrictMode, useState } from "react";
import { createRoot } from "react-dom/client";
import { BrowserRouter, Routes, Route } from "react-router-dom";
import Home from "./Home";
import Discos from "./Discos";
import Navbar from "./navbar";
import Login from "./Login";

function App() {
  const [textoEntrada, setTextoEntrada] = useState("");
  const [salida, setSalida] = useState([]);
  const [loginStatus, setLoginStatus] = useState(null); // null | { user, token }

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
              loginStatus={loginStatus}
              setLoginStatus={setLoginStatus}
            />
          }
        />
        <Route path="/discos" element={<Discos />} />
        <Route
          path="/login"
          element={<Login setSalida={setSalida} setLoginStatus={setLoginStatus} />}
        />
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
