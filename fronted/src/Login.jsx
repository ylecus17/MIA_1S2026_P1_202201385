import { useState } from "react";
import "./Login.css";

function Login({ setSalida, setLoginStatus }) {
  const [user, setUser] = useState("");
  const [pass, setPass] = useState("");
  const [id, setId] = useState("");
  const [loginError, setLoginError] = useState(""); // estado para errores de login

  const manejarLogin = async () => {
    try {
      const body = `user=${user};pass=${pass};id=${id}`;

      const res = await fetch("http://localhost:5300/login", {
        method: "POST",
        headers: { "Content-Type": "text/plain" },
        body
      });

      const data = await res.json();

      if (data.success) {
        // Mensaje general en salida
        setSalida([{ level: "info", text: `Login correcto: ${data.user}` }]);
        setLoginStatus({ user: data.user, token: data.token });
        if (data.token) {
          localStorage.setItem("token", data.token);
        }
        setLoginError(""); // limpiar error si fue exitoso
      } else {
        // Mostrar error en la pestaña de login
        setLoginError(data.error);
      }
    } catch (error) {
      setLoginError(`Error al conectar con backend: ${error.message}`);
    }
  };

  return (
    <div className="login-page">
      <div className="login-box">
        <h2 className="login-title">Iniciar Sesión</h2>
        <div className="login-form">
          <input
            type="text"
            placeholder="ID Partición"
            value={id}
            onChange={(e) => setId(e.target.value)}
            className="login-input"
          />
          <input
            type="text"
            placeholder="Usuario"
            value={user}
            onChange={(e) => setUser(e.target.value)}
            className="login-input"
          />
          <input
            type="password"
            placeholder="Contraseña"
            value={pass}
            onChange={(e) => setPass(e.target.value)}
            className="login-input"
          />
          <button onClick={manejarLogin} className="login-button">Login</button>

          {/* Mostrar error de login aquí */}
          {loginError && (
            <div className="login-error">
              {loginError}
            </div>
          )}
        </div>
      </div>
    </div>
  );
}

export default Login;
