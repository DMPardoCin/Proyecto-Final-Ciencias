#ifndef ESTADO_H
#define ESTADO_H

#include "grafo.h"
#include "elementoVisual.h"
#include "elementos.h"
#include <map>
using namespace std;

// ============================================================
//  LAYOUT
// ============================================================
int AW, AH, PANEL_W, RED_W;
int ESCALA_NODO, TAM_NODO;
int ESCALA_SW, TAM_SW_H, TAM_SW_W;
int PANEL_BAR_W;

// ============================================================
//  MODOS
// ============================================================
enum Modo
{
    MODO_NORMAL,
    MODO_CREAR_PC,
    MODO_CREAR_ROUTER,
    MODO_CREAR_SWITCH,
    MODO_ENLAZAR,
    MODO_ENLAZAR_B,
    MODO_ENLAZAR_PESO,
    MODO_ELIMINAR_ENLACE,
    MODO_ELIMINAR_ENLACE_B,
    MODO_ELIMINAR_NODO,
    MODO_VER_DIJKSTRA,
    MODO_DIJKSTRA_DEST,
    MODO_ENVIAR_MANUAL,
    MODO_ENVIAR_MANUAL_B
};

// ============================================================
//  TRAMA VISUAL
// ============================================================
struct TramaVis
{
    int origen, destino;
    vector<int> ruta;
    int pos, ticksVida, ticksTotales, saltos, pesoRuta, ticksEspera;
    float vx, vy;
    bool entregada, enEspera;
};

// ============================================================
//  METRICAS
// ============================================================
struct Metricas
{
    int totalEntregadas = 0, totalDescartadas = 0, maxCola = 0;
    double sumaTiempos = 0, sumaSaltos = 0, sumaEspera = 0;
    double tiempoPromedio() const { return totalEntregadas > 0 ? sumaTiempos / totalEntregadas : 0; }
    double saltosPromedio() const { return totalEntregadas > 0 ? sumaSaltos / totalEntregadas : 0; }
    double esperaPromedio() const { return totalEntregadas > 0 ? sumaEspera / totalEntregadas : 0; }
    void registrar(const TramaVis &t)
    {
        totalEntregadas++;
        sumaTiempos += t.ticksTotales;
        sumaSaltos += t.saltos;
        sumaEspera += t.ticksEspera;
    }
};

// ============================================================
//  ENLACE VISUAL
// ============================================================
struct EnlaceVis
{
    int a, b;
};

// ============================================================
//  VARIABLES GLOBALES
// ============================================================
Grafo grafo;
vector<EnlaceVis> enlacesVis;
map<pair<int, int>, int> colaEnlace;

Modo modoActual = MODO_NORMAL;
bool arrastrando = false;
int indiceArrastrado = -1, offsetX = 0, offsetY = 0;
int nodoEnlaceA = -1, nodoEnlaceB = -1, pesoEnlaceInput = 10;
int elimEnlaceA = -1, nodoSeleccionado = -1;
int envioOrigen = -1, envioDestino = -1;

int dijkstraOrigen = -1, dijkstraDestino = -1;
vector<int> dijkstraRuta;
ResultadoDijkstra dijkstraRes;

vector<TramaVis> tramas;
int tickActual = 0;
bool simPausada = true;
Metricas metricas;

// ============================================================
//  HELPERS INLINE
// ============================================================
inline pair<int, int> claveEnlace(int a, int b) { return {min(a, b), max(a, b)}; }
inline bool enlaceExiste(int a, int b)
{
    for (auto &e : enlacesVis)
        if ((e.a == a && e.b == b) || (e.a == b && e.b == a))
            return true;
    return false;
}

#endif