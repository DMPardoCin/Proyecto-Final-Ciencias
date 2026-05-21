#include "estado.h"
#include "layout.h"
#include "simulacion.h"
#include "dibujo.h"
#include "panel.h"
#include "interaccion.h"
#include <windows.h>
#include <ctime>
using namespace miniwin;

int main()
{
    srand((unsigned)time(nullptr));
    calcularLayout();
    vredimensiona(AW, AH);
    inicializarBotones();

    while (true)
    {
        // Fondo
        color_rgb(14, 14, 24);
        rectangulo_lleno(0, 0, AW, AH);

        // Canvas de red
        dibujarSeparador();
        dibujarEnlaces();
        dibujarMarcadoresDijkstra();
        dibujarElementos();
        dibujarTramas();

        // Panel lateral: botones

        btnDibujar(btnCrearPC, modoActual == MODO_CREAR_PC);

        btnDibujar(btnCrearRouter, modoActual == MODO_CREAR_ROUTER);

        btnDibujar(btnCrearSwitch, modoActual == MODO_CREAR_SWITCH);

        btnDibujar(btnEnlazar, modoActual == MODO_ENLAZAR ||
                                   modoActual == MODO_ENLAZAR_B ||
                                   modoActual == MODO_ENLAZAR_PESO);

        btnDibujar(btnElimEnlace, modoActual == MODO_ELIMINAR_ENLACE ||
                                      modoActual == MODO_ELIMINAR_ENLACE_B);

        btnDibujar(btnElimNodo, modoActual == MODO_ELIMINAR_NODO);

        btnDibujar(btnDijkstra, modoActual == MODO_VER_DIJKSTRA ||
                                    modoActual == MODO_DIJKSTRA_DEST ||
                                    dijkstraOrigen >= 0);
        btnDibujar(btnEnviarManual, modoActual == MODO_ENVIAR_MANUAL ||
                                        modoActual == MODO_ENVIAR_MANUAL_B);
        btnDibujar(btnEnviarAuto);
        btnDibujar(btnSimular, !simPausada);
        btnDibujar(btnPausar, simPausada && tickActual > 0);
        btnDibujar(btnReset);

        // Panel lateral: datos
        dibujarModo();
        dibujarPopupPeso();
        dibujarPanelDatos();

        // Interaccion
        manejarBotones();
        manejarClicRed();
        manejarArrastre();
        manejarTeclado();

        // Logica
        for (int i = 0; i < 3; i++)
            tickSimulacion();

        refresca();
        espera(16);
    }

    vcierra();
    return 0;
}