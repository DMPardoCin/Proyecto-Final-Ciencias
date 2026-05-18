#include <vector>

enum TipoElemento { PC, ROUTER,SWITCH };

struct ElementoVisual {
    TipoElemento tipo;
    int x, y;           // coordenadas esquina superior izquierda (píxeles reales)
     
};

std::vector<ElementoVisual> elementos;