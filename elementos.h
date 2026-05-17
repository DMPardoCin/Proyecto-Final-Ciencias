using namespace std;
#include <string>


struct Boton {
    int x, y;           // esquina superior izquierda
    int ancho, alto;
    const char* texto;
    bool clic_procesado; // evita múltiples acciones mientras se mantiene presionado
};

