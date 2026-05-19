#include "grafo.h"
#include "elementoVisual.h"

#include "miniwin.h"
#include "elementos.h"
#include "pc.h"
#include "router.h"
#include "switch.h"
using namespace miniwin;

void dibujarPc(int x, int y);
void dibujarRouter(int x, int y);
void dibujarSwitch(int x, int y);
void dibujarEnlaces();
void dibujarBoton(const Boton& btn);
void botonInicializar(Boton& btn, int x, int y, int ancho, int alto, const char* texto) ;
bool botonRatonEncima(const Boton& btn);
void crearElemento(int x, int y, TipoElemento tipo);
void eliminarElemento();
void manejarArrastre();
void manejarConexion();
void detectarArrastre();
void dibujarElementos();
bool botonClick(Boton& btn);

//variables globales
Grafo grafo;
bool arrastrando = false;
bool modoConexion=false;
int nodoOrigen=-1;
int indiceArrastrado = -1;
int indiceEliminar=-1;
int offsetX = 0, offsetY = 0; 

int main() {
    

    vredimensiona(1500, 1500);
    
    Boton botonPc, botonRouter, botonSwitch;
    botonInicializar(botonPc,20,950,200,50,"pc");
    botonInicializar(botonRouter,600,950,200,50,"router");
    botonInicializar(botonSwitch,1000,950,200,50,"switch");

    while (true) {
        color(NEGRO);
        rectangulo_lleno(0,0,1500,1500);

        dibujarEnlaces();
        dibujarElementos();
        dibujarBoton(botonPc);
        dibujarBoton(botonRouter);
        dibujarBoton(botonSwitch);
        
        manejarArrastre();  //comprueba si se esta oprimiendo el click izquierdo para arrastrar
        eliminarElemento(); //comprueba si se esta oprimiendo el click derecho para eliminar
        manejarConexion();

        if (!arrastrando && botonClick(botonPc)) {
            crearElemento(0,0,PC);  
        }
        if (!arrastrando && botonClick(botonRouter)) {
            crearElemento(0,0,ROUTER);  
            
        }
        if (!arrastrando && botonClick(botonSwitch)) {
            crearElemento(0,0,SWITCH);  
        }
        
        refresca();
        espera(10);
    }

    vcierra();
    return 0;
}



void crearElemento(int x,int y,TipoElemento tipo){
    ElementoVisual elemento;
    int id;
    if(tipo==PC){
        id=grafo.agregarNodo("pc");
        elemento.tipo=PC;
    }else if(tipo==ROUTER){
        id=grafo.agregarNodo("router");
        elemento.tipo=ROUTER;
    }else{
        id=grafo.agregarNodo("switch");
        elemento.tipo=SWITCH;
    }
    elemento.idNodo=id;
    elemento.x=x;
    elemento.y=y;
    elementos.push_back(elemento);
}

void eliminarElemento(){
    if(raton_boton_der()&& !arrastrando){
        float mx=raton_x();
        float my=raton_y();
        for(int i=0;i<elementos.size();i++){
            int x=elementos[i].x, y=elementos[i].y;
            if(mx >= x && mx <= x+160 && my >= y && my <= y+160){
                arrastrando=true;
                indiceEliminar=i;
                break;
            }
        }
    }
    if(arrastrando && raton_boton_der()){
        elementos.erase(elementos.begin()+indiceEliminar);

    }
}

void manejarConexion() {
    // Activar/desactivar modo conexión con tecla 'c'
    if (tecla() == int('C')) {
        modoConexion = !modoConexion;
        if (modoConexion) {
            mensaje("Modo Conexion Haz clic en el nodo ORIGEN");
            nodoOrigen = -1;
        } else {
            mensaje("Modo Conexion Desactivado");
        }
        espera(100); // para evitar rebote
        return;
    }

    if (!modoConexion) return;

    // Si estamos en modo conexión y se presiona el botón izquierdo (y no se está arrastrando)
    if (raton_boton_izq()) {
        float mx = raton_x(), my = raton_y();

        // Buscar qué elemento se ha clicado
        for (size_t i = 0; i < elementos.size(); i++) {
            int x = elementos[i].x, y = elementos[i].y;
            if (mx >= x && mx <= x + 160 && my >= y && my <= y + 160) {
                int idActual = elementos[i].idNodo;

                if (nodoOrigen == -1) {
                    // Primer clic: establecer origen
                    nodoOrigen = idActual;
                    mensaje("Modo Conexión Origen seleccionado. Ahora haz clic en el DESTINO");
                } else {
                    // Segundo clic: crear enlace si origen != destino
                    if (nodoOrigen != idActual) {
                        grafo.agregarEnlace(nodoOrigen, idActual, 1000000, 2.0);
                        grafo.agregarEnlace(idActual, nodoOrigen, 1000000, 2.0);
                        mensaje("Conexión Enlace creado correctamente");
                        modoConexion = false; // salir del modo
                    } else {
                        mensaje("Error No se puede conectar un nodo consigo mismo");
                    }
                    nodoOrigen = -1;
                }
                break; // solo procesar un elemento por clic
            }
        }
        espera(200); // evitar múltiples detecciones por un solo clic
    }
}

bool obtenerPosicionPorId(int idNodo, int& x, int& y) {
    //esta funcion devielve tres valores: x, y y un booleano
    for (const auto& elem : elementos) {
        if (elem.idNodo == idNodo) {
            x = elem.x;
            y = elem.y;
            return true;
        }
    }
    return false;
}

void dibujarElementos(){
    //Recorre el arreglo de elementos y los dibuja segun su tipo
    for (const auto& elem : elementos) {
        if (elem.tipo == PC) {
            dibujarPc(elem.x, elem.y);
        } else if (elem.tipo == ROUTER) {
            dibujarRouter(elem.x, elem.y);
        }else if(elem.tipo==SWITCH){
            dibujarSwitch(elem.x, elem.y);
        }
    }
}
void dibujarEnlaces() {
    // Configurar color y grosor de línea
    color(BLANCO);
    
    for (int u = 0; u < grafo.numNodos; ++u) {
        int x1, y1;
        if (!obtenerPosicionPorId(u, x1, y1)) continue;  // este nodo no está en pantalla (quizás no debería ocurrir)
        // Calcular centro del elemento (suponiendo que todos los elementos tienen el mismo tamaño)
        int cx1 = x1+90;
        int cy1 = y1+20;
        
        for (const Enlace& e : grafo.nodos[u].adyacentes) {
            int v = e.destino;
            if (u < v) {   // Para no duplicar
                int x2, y2;
                if (obtenerPosicionPorId(v, x2, y2)) {
                    int cx2 = x2+40;
                    int cy2 = y2+80;
                    linea(cx1, cy1, cx2, cy2);
                }
            }
        }
    }
}

void manejarArrastre() {
    // Inicio
    if (raton_boton_izq() && !arrastrando) {
        float mx = raton_x();
        float my = raton_y();
        for (size_t i = 0; i < elementos.size(); i++) {
            
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
    int escala = 6;
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
void dibujarSwitch(int x, int y) {
    int escala = 6;
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 40; j++) {
            int r = switche[i][j].R;
            int g = switche[i][j].G;
            int b = switche[i][j].B;
            color_rgb(r, g, b);
            rectangulo_lleno(x + j * escala,
                             y + i * escala,
                             x + j * escala + escala - 1,
                             y + i * escala + escala - 1);
        }
    }
}

void botonInicializar(Boton& btn, int x, int y, int ancho, int alto, const char* texto) {
    //funcion para inicializar un boton con sus atributos
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