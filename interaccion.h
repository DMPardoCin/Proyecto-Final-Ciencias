#ifndef INTERACCION_H
#define INTERACCION_H

#include "estado.h"
#include "layout.h"
#include "simulacion.h"
#include "panel.h"
#include "miniwin.h"
using namespace miniwin;

// ============================================================
//  ARRASTRE DE NODOS
// ============================================================
inline void manejarArrastre()
{
    if (modoActual != MODO_NORMAL)
        return;
    float mx = raton_x(), my = raton_y();
    if (mx >= RED_W)
        return;
    if (raton_boton_izq() && !arrastrando)
    {
        int idx = elementoBajoRaton(mx, my);
        if (idx >= 0)
        {
            arrastrando = true;
            indiceArrastrado = idx;
            offsetX = (int)mx - elementos[idx].x;
            offsetY = (int)my - elementos[idx].y;
            nodoSeleccionado = idx;
            if (dijkstraOrigen >= 0 && idx != dijkstraOrigen && !dijkstraRes.distancia.empty())
            {
                dijkstraDestino = idx;
                dijkstraRuta = grafo.reconstruirRuta(dijkstraRes, idx);
            }
        }
    }
    if (arrastrando && raton_boton_izq() && indiceArrastrado >= 0 && indiceArrastrado < (int)elementos.size())
    {
        int nx = (int)mx - offsetX, ny = (int)my - offsetY;
        int w = ancho_elem(indiceArrastrado), h = alto_elem(indiceArrastrado);
        elementos[indiceArrastrado].x = max(0, min(nx, RED_W - w));
        elementos[indiceArrastrado].y = max(0, min(ny, AH - h));
    }
    if (!raton_boton_izq())
    {
        arrastrando = false;
        indiceArrastrado = -1;
    }
}

// ============================================================
//  CLIC EN AREA DE RED
// ============================================================
static bool clicPrev = false;

inline void manejarClicRed()
{
    float mx = raton_x(), my = raton_y();
    bool clic = raton_boton_izq() && mx < RED_W;
    bool nuevo = clic && !clicPrev;
    clicPrev = clic;
    if (!nuevo)
        return;
    int idx = elementoBajoRaton(mx, my);
    switch (modoActual)
    {
    case MODO_CREAR_PC:
        crearNodo((int)mx, (int)my, PC);
        modoActual = MODO_NORMAL;
        break;
    case MODO_CREAR_ROUTER:
        crearNodo((int)mx, (int)my, ROUTER);
        modoActual = MODO_NORMAL;
        break;
    case MODO_CREAR_SWITCH:
        crearNodo((int)mx, (int)my, SWITCH);
        modoActual = MODO_NORMAL;
        break;
    case MODO_ENLAZAR:
        if (idx >= 0)
        {
            nodoEnlaceA = idx;
            modoActual = MODO_ENLAZAR_B;
        }
        break;
    case MODO_ENLAZAR_B:
        if (idx >= 0 && idx != nodoEnlaceA)
        {
            nodoEnlaceB = idx;
            pesoEnlaceInput = 10;
            modoActual = MODO_ENLAZAR_PESO;
        }
        break;
    case MODO_ELIMINAR_ENLACE:
        if (idx >= 0)
        {
            elimEnlaceA = idx;
            modoActual = MODO_ELIMINAR_ENLACE_B;
        }
        break;
    case MODO_ELIMINAR_ENLACE_B:
        if (idx >= 0)
        {
            eliminarEnlace(elimEnlaceA, idx);
            elimEnlaceA = -1;
            modoActual = MODO_NORMAL;
        }
        break;
    case MODO_ELIMINAR_NODO:
        if (idx >= 0)
        {
            eliminarNodo(idx);
            modoActual = MODO_NORMAL;
        }
        break;
    case MODO_VER_DIJKSTRA:
        if (idx >= 0)
        {
            dijkstraOrigen = idx;
            dijkstraDestino = -1;
            dijkstraRes = grafo.dijkstra(idx);
            dijkstraRuta.clear();
            nodoSeleccionado = idx;
            modoActual = MODO_DIJKSTRA_DEST;
        }
        break;
    case MODO_DIJKSTRA_DEST:
        if (idx >= 0 && idx != dijkstraOrigen)
        {
            dijkstraDestino = idx;
            dijkstraRuta = grafo.reconstruirRuta(dijkstraRes, idx);
            nodoSeleccionado = idx;
            modoActual = MODO_NORMAL;
        }
        break;
    case MODO_ENVIAR_MANUAL:
        if (idx >= 0)
        {
            envioOrigen = idx;
            modoActual = MODO_ENVIAR_MANUAL_B;
        }
        break;
    case MODO_ENVIAR_MANUAL_B:
        if (idx >= 0 && idx != envioOrigen)
        {
            enviarTrama(envioOrigen, idx);
            envioOrigen = envioDestino = -1;
            modoActual = MODO_NORMAL;
        }
        break;
    case MODO_NORMAL:
        nodoSeleccionado = idx;
        if (dijkstraOrigen >= 0 && idx >= 0 && idx != dijkstraOrigen && !dijkstraRes.distancia.empty())
        {
            dijkstraDestino = idx;
            dijkstraRuta = grafo.reconstruirRuta(dijkstraRes, idx);
        }
        break;
    default:
        break;
    }
}

// ============================================================
//  TECLADO
// ============================================================
inline void manejarTeclado()
{
    int t = tecla();
    if (t == ESCAPE)
    {
        modoActual = MODO_NORMAL;
        nodoEnlaceA = nodoEnlaceB = elimEnlaceA = envioOrigen = envioDestino = -1;
        return;
    }
    if (modoActual == MODO_ENLAZAR_PESO && t == RETURN)
        confirmarEnlace();
}

// ============================================================
//  ACCIONES DE BOTONES (llamar desde main loop)
// ============================================================
inline void manejarBotones()
{
    if (btnClick(btnCrearPC))
    {
        modoActual = MODO_CREAR_PC;
        nodoEnlaceA = elimEnlaceA = -1;
    }
    if (btnClick(btnCrearRouter))
    {
        modoActual = MODO_CREAR_ROUTER;
        nodoEnlaceA = elimEnlaceA = -1;
    }
    if (btnClick(btnCrearSwitch))
    {
        modoActual = MODO_CREAR_SWITCH;
        nodoEnlaceA = elimEnlaceA = -1;
    }
    if (btnClick(btnEnlazar))
    {
        modoActual = MODO_ENLAZAR;
        nodoEnlaceA = nodoEnlaceB = -1;
    }
    if (btnClick(btnElimEnlace))
    {
        modoActual = MODO_ELIMINAR_ENLACE;
        elimEnlaceA = -1;
    }
    if (btnClick(btnElimNodo))
    {
        modoActual = MODO_ELIMINAR_NODO;
    }
    if (btnClick(btnDijkstra))
    {
        modoActual = MODO_VER_DIJKSTRA;
        dijkstraOrigen = dijkstraDestino = -1;
        dijkstraRuta.clear();
        dijkstraRes.distancia.clear();
    }
    if (btnClick(btnEnviarManual))
    {
        modoActual = MODO_ENVIAR_MANUAL;
        envioOrigen = -1;
    }
    if (btnClick(btnEnviarAuto))
    {
        enviarTramaAuto();
    }
    if (btnClick(btnSimular))
    {
        simPausada = false;
        modoActual = MODO_NORMAL;
    }
    if (btnClick(btnPausar))
    {
        simPausada = true;
    }
    if (btnClick(btnReset))
    {
        tramas.clear();
        colaEnlace.clear();
        tickActual = 0;
        simPausada = true;
        metricas = {};
        for (Nodo &n : grafo.nodos)
        {
            n.tramasRecibidas = 0;
            n.tramasEnviadas = 0;
        }
    }
    if (modoActual == MODO_ENLAZAR_PESO)
    {
        if (btnClick(popPesoMenos))
            pesoEnlaceInput = max(1, pesoEnlaceInput - 10);
        if (btnClick(popPesoMas))
            pesoEnlaceInput = min(10000, pesoEnlaceInput + 10);
        if (btnClick(popPesoD10))
            pesoEnlaceInput = max(1, pesoEnlaceInput / 10);
        if (btnClick(popPesoX10))
            pesoEnlaceInput = min(10000, pesoEnlaceInput * 10);
        if (btnClick(popPesoOK))
            confirmarEnlace();
    }
}

#endif