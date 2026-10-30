# MotorVJ - Motor 3D - E1 - Visualizador de Geometría

**MotorVJ** es un motor y visualizador de geometría 3D en tiempo real desarrollado en C++ como proyecto para la asignatura de **Motores de Videojuegos** en el CITM UPC

* **Repositorio oficial:** [https://github.com/MarcPladellorensPerez/Game-Engine-3D-Group-5](https://github.com/MarcPladellorensPerez/Game-Engine-3D-Group-5)

---

## 👥 Integrantes del Grupo 5

* **Marc Pladellorens Pérez**
  * GitHub: [@MarcPladellorensPerez](https://github.com/MarcPladellorensPerez)
  * Rol: Interfaz de usuario (Dear ImGui), sistema de ventanas del editor, consola y flujo de trabajo.
* **Montserrat Medina Chávez**
  * GitHub: [@montse4](https://github.com/montse4)
  * Rol: Sistema de cámara orbital estilo Unity, transformaciones matemáticas y matrices View/Projection.
* **Aidan Albero García**
  * GitHub: [@IDK-oss52](https://github.com/IDK-oss52)
  * Rol: Arquitectura de GameObjects y Componentes, importación de mallas con Assimp y texturas con DevIL.

---

## 🚀 Instalación y Puesta en Marcha

El repositorio está preparado para configurarse y compilarse automáticamente en cualquier ordenador con Windows:

1. Clona el repositorio:
   ```bash
   git clone https://github.com/MarcPladellorensPerez/Game-Engine-3D-Group-5.git
   ```
2. Ejecuta el script **`pou.bat`**:
   * Si el equipo no dispone de `vcpkg`, el script lo descargará e inicializará automáticamente en tu carpeta de usuario.
   * Generará la solución de Visual Studio mediante CMake y descargará las dependencias necesarias (`vcpkg.json`).
   * Abrirá Visual Studio fijando automáticamente **MotorVJ** como proyecto de inicio y configurando el directorio de trabajo en la raíz.
3. En Visual Studio, selecciona la configuración **Release** o **Debug** (x64) y pulsa **F5 / Iniciar depuración**.

---

## 🎮 Controles y Uso del Motor

### 1. Navegación de Cámara (Estilo Unity)
* **Clic derecho (RMB) + WASD:** Movimiento libre en primera persona (modo vuelo).
* **Clic derecho (RMB) + Q / E:** Desplazamiento vertical (bajar / subir).
* **Shift (manteniendo RMB):** Duplica la velocidad de movimiento (sprint).
* **Alt + Clic izquierdo (LMB) y arrastrar:** Orbitar la cámara alrededor del objeto o punto de referencia.
* **Rueda del ratón** (o **Alt + RMB y arrastrar**): Zoom in / Zoom out.
* **Botón central del ratón (MMB) y arrastrar:** Desplazamiento lateral y vertical de la vista.
* **Tecla `F`:** Centra y enfoca la cámara (*Focus*) sobre el objeto seleccionado en la jerarquía (o sobre el origen si no hay ninguno).

### 2. Carga de Modelos y Texturas (Drag & Drop)
* **Carga automática inicial:** Al arrancar, el motor carga automáticamente el modelo `assets/BakerHouse.fbx` y su textura difusa asociada.
* **Arrastrar modelos 3D (`.fbx`, `.obj`):** Arrastra cualquier archivo 3D desde el explorador de archivos directamente a la ventana del motor. Assimp importará la geometría y la añadirá como un nuevo GameObject con sus sub-mallas a la jerarquía de la escena.
* **Arrastrar texturas (`.png`, `.dds`, `.jpg`, `.tga`):** Arrastra una textura a la ventana del motor para aplicársela de forma inmediata al GameObject seleccionado en la Jerarquía.

### 3. Ventanas del Editor (Dear ImGui)
* **Barra de menú superior:**
  * **File:** Opción para salir del motor de manera limpia.
  * **Create:** Instancia geometrías básicas primitivas en la escena (Cubo, Esfera, Plano, Pirámide, Cilindro y Triángulo).
  * **View:** Activa o desactiva la visibilidad de cada una de las ventanas del editor.
  * **Help:** Enlaces directos a la documentación en GitHub, repositorio de incidencias (*Issues*), página de *Releases* y ventana *About*.
* **Jerarquía (Hierarchy):**
  * Lista todos los GameObjects presentes en la escena en una vista en árbol.
  * Permite seleccionar cualquier objeto haciendo clic sobre su nombre.
* **Inspector:**
  * Muestra y permite editar las propiedades del GameObject seleccionado:
    * **Transform:** Inspección y modificación interactiva de la Posición, Rotación y Escala.
    * **Mesh:** Información técnica (conteo de vértices, triángulos e índices). Incluye selectores para visualizar las **normales por cara** (amarillo) y **normales por vértice** (cian).
    * **Texture:** Ruta del archivo, resolución (ancho × alto) y canales. Incluye un conmutador para sustituir la textura activa por una textura de damero / ajedrezado (*Checkerboard*) blanco y negro generada proceduralmente.
* **Configuración (Configuration):**
  * **Gráficas de rendimiento:** Histogramas en tiempo real de FPS y milisegundos por fotograma.
  * **Window:** Modo pantalla completa (*Fullscreen*), ventana redimensionable y ajuste manual de resolución.
  * **Renderer:** Control de sincronización vertical (VSync), renderizado en modo alambre (*Wireframe*), Depth Test, Cull Face y selector del color de fondo (*Clear Color*).
  * **Camera:** Ajuste de FOV, planos de recorte (*Near Plane* y *Far Plane*), velocidad y sensibilidad.
  * **Hardware & Software:** Detección de versiones de SDL3, DevIL, especificaciones de la CPU, número de núcleos, memoria RAM total del sistema, consumo de RAM del proceso en ejecución (Working Set), fabricante de la GPU, modelo de tarjeta gráfica, versión de OpenGL y versión de GLSL.
* **Consola (Console):**
  * Registro en vivo de todos los eventos del motor mediante la macro `LOG()`.
  * Filtro de búsqueda textual, botón para limpiar (*Clear*), botón para copiar al portapapeles (*Copy*), auto-scroll y resaltado cromático de advertencias (*Warnings* en amarillo) y fallos (*Errors* en rojo).
* **Acerca de (About):**
  * Resumen del motor, integrantes del grupo, bibliotecas de terceros utilizadas y licencia del proyecto.

---

## ⭐ Funcionalidades Extra (Bonus)

* **Script de automatización `pou.bat`:** Inicialización desatendida del entorno de desarrollo (instalación automática de `vcpkg`, resolución de dependencias, generación de proyectos de Visual Studio y fijación del proyecto de inicio).
* **Menú de primitivas procedimentales:** Generación desde código de 6 geometrías básicas (Cubo, Plano, Esfera, Cilindro, Pirámide y Triángulo) calculando posiciones, normales y coordenadas UV.
* **Doble modo de visualización de normales:** Posibilidad de activar de forma independiente normales por vértice (en cian) y normales por cara (en amarillo) para inspección de mallas.
* **Monitoreo de memoria del proceso:** Medición en tiempo real del uso de memoria RAM privada del ejecutable mediante la API nativa de Windows (`psapi.h`).
* **Soporte de doble backend para imágenes:** Carga principal mediante DevIL (con soporte para DDS, PNG, JPG, TGA) y capa de compatibilidad fallback con `stb_image`.

---

## 📝 Comentarios Técnicos para el Profesor

* **Arquitectura de GameObjects y Componentes:**
  Siguiendo las directrices explicadas en clase, la clase `GameObject` gestiona sus componentes desacoplados mediante un enumerado de tipos (`ComponentType`). Se ha prescindido deliberadamente del uso de RTTI y `dynamic_cast` en `GetComponent()`, garantizando mayor velocidad de acceso y respetando el patrón de componentes enseñado en la asignatura.
* **Carga de texturas con DevIL:**
  El motor emplea la biblioteca oficial de la asignatura (**DevIL**) para la carga y descompresión de texturas y formatos de compresión como DDS, administrando los identificadores de imagen de DevIL (`ilGenImages`, `ilLoadImage`) y volcando los buffers correspondientes a texturas 2D de OpenGL.
* **Separación de código Motor vs. Editor:**
  Toda la lógica de dibujo e interacción de la interfaz se encuentra estrictamente aislada dentro del módulo `ModuleEditor`, sin interferir en los módulos troncales (`ModuleRender`, `ModuleCamera`, `ModuleWindow`, etc.).

---

## 📄 Licencia

Este proyecto está bajo la **Licencia MIT**. Consulta el archivo `LICENSE` para más información.
