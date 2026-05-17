#include "grafo.h"
#include "elementoVisual.h"

#include "miniwin.h"
#include "elementos.h"
#include "pc.h"
#include "router.h"
using namespace miniwin;

void dibujarPc(int x, int y);
void dibujarRouter(int x, int y);
void dibujarBoton(const Boton& btn);
void botonInicializar(Boton& btn, int x, int y, int ancho, int alto, const char* texto) ;
bool botonRatonEncima(const Boton& btn);
void crearPc(int x,int y);
void manejarArrastre();
void detectarArrastre();
void dibujarElementos();
bool botonClick(Boton& btn);

//variables globales
Grafo grafo;
bool arrastrando = false;
int indiceArrastrado = -1;
int offsetX = 0, offsetY = 0; 

int main() {
    

    vredimensiona(1500, 1500);
    
    Boton boton;
    botonInicializar(boton,20,950,200,50,"xd");

    while (true) {
        color(NEGRO);
        rectangulo_lleno(0,0,1500,1500);

        dibujarElementos();
        dibujarBoton(boton);
        manejarArrastre();

        if (!arrastrando && botonClick(boton)) {
            crearPc(0,0);  // o posición predefinida
        }
        refresca();
        espera(10);
    }

    vcierra();
    return 0;
}

void crearPc(int x,int y){
grafo.agregarNodo("pc");

ElementoVisual elemento;
elemento.tipo=PC;
elemento.x=x;
elemento.y=y;
elementos.push_back(elemento);


}
void dibujarElementos(){
    for (const auto& elem : elementos) {
        if (elem.tipo == PC) {
            dibujarPc(elem.x, elem.y);
        } else if (elem.tipo == ROUTER) {
            dibujarRouter(elem.x, elem.y);
        }
    }
}

void manejarArrastre() {
    // Inicio
    if (raton_boton_izq() && !arrastrando) {
        float mx = raton_x();
        float my = raton_y();
        for (size_t i = 0; i < elementos.size(); i++) {
            if (elementos[i].tipo == PC) {
                int x = elementos[i].x, y = elementos[i].y;
                if (mx >= x && mx <= x+160 && my >= y && my <= y+160) {
                    arrastrando = true;
                    indiceArrastrado = i;
                    offsetX = mx - x;
                    offsetY = my - y;
                    break;
                }
            }
        }
    }
    // Actualización
    if (arrastrando && raton_boton_izq()) {
        int nuevaX = raton_x() - offsetX;
        int nuevaY = raton_y() - offsetY;
        // restricciones...
        elementos[indiceArrastrado].x = nuevaX;
        elementos[indiceArrastrado].y = nuevaY;
    }
    // Finalización
    if (!raton_boton_izq()) {
        arrastrando = false;
        indiceArrastrado = -1;
    }
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

void dibujarRouter(int x, int y) {
    int escala = 4;
    for (int i = 0; i < 40; i++) {
        for (int j = 0; j < 40; j++) {
            int r = router[i][j].R;
            int g = router[i][j].G;
            int b = router[i][j].B;
            color_rgb(r, g, b);
            rectangulo_lleno(x + j * escala,
                             y + i * escala,
                             x + j * escala + escala - 1,
                             y + i * escala + escala - 1);
        }
    }
}

void botonInicializar(Boton& btn, int x, int y, int ancho, int alto, const char* texto) {
    btn.x = x;
    btn.y = y;
    btn.ancho = ancho;
    btn.alto = alto;
    btn.texto = texto;
    btn.clic_procesado = false;
}

void dibujarBoton(const Boton& btn) {
    // Fondo: gris si se acaba de hacer clic, blanco normalmente
    if (btn.clic_procesado)
        color_rgb(186,186,186);
    else
        color(BLANCO);
    rectangulo_lleno(btn.x, btn.y, btn.x + btn.ancho, btn.y + btn.alto);
    
    // Borde negro
    color(NEGRO);
    rectangulo(btn.x, btn.y, btn.x + btn.ancho, btn.y + btn.alto);
    
    
}

bool botonRatonEncima(const Boton& btn) {
    float mx = raton_x();
    float my = raton_y();
    return (mx >= btn.x && mx <= btn.x + btn.ancho &&
            my >= btn.y && my <= btn.y + btn.alto);
}

// Devuelve true si se ha hecho clic en el botón en este instante
// (solo una vez por presión, gracias al flag interno)
bool botonClick(Boton& btn) {
    if (raton_boton_izq()) {
        if (!btn.clic_procesado && botonRatonEncima(btn)) {
            btn.clic_procesado = true;
            return true;
        }
    } else {
        // Cuando se suelta el botón del ratón, se reinicia el flag
        btn.clic_procesado = false;
    }
    return false;
}