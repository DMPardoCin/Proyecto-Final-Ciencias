#include "miniwin.h"
#include "pc.h"
using namespace miniwin;

void dibujarPc(int x, int y);

int main() {
    vredimensiona(800, 600);
    
    color(BLANCO);
    rectangulo_lleno(0, 0, 800, 600);  // Fondo blanco

    dibujarPc(50, 50);
    refresca();
    

    while (true) {
        refresca();                    // Necesario dentro del bucle
        espera(30); 
    }

    vcierra();
    return 0;
}

void dibujarPc(int x, int y) {
    int escala = 4;
    for (int i = 0; i < 40; i++) {
        for (int j = 0; j < 40; j++) {
            int r = pc[i][j].R;
            int g = pc[i][j].G;
            int b = pc[i][j].B;
            color_rgb(r, g, b);
            rectangulo_lleno(x + j * escala,
                             y + i * escala,
                             x + j * escala + escala - 1,
                             y + i * escala + escala - 1);
        }
    }
}