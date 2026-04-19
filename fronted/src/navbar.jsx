import { Link } from "react-router-dom";
import "./navbar.css";

function Navbar() {
  return (
    <nav className="navbar">
      <h1 className="navbar-logo">Proyecto 2</h1>
      <ul className="navbar-links">
        <li>
          <Link to="/">Consola</Link>
        </li>
        <li>
          <Link to="/discos">Discos</Link>
        </li>
      </ul>
    </nav>
  );
}

export default Navbar;
