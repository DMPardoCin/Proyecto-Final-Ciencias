using namespace std;
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <string>
#include <limits>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <functional>
#include <iomanip>
#include <sstream>


//  Constantes globales
const int    INF             = numeric_limits<int>::max();
const int    REF_BW          = 100000000;   // 100 Mbps  (referencia OSPF)
const int    MAX_QUEUE       = 10;          // capacidad maxima de cola en enlace
const double SAT_UMBRAL      = 0.85;        // saturacion critica


//  Estructura: Enlace (arista)

struct Enlace {
    int destino;
    int bandwidthBps;   // ancho de banda en bps
    int costoOSPF;      // 10^8 / bandwidth  (metrica OSPF)
    double latenciaMs;     // latencia base en ms
    queue<int> cola;   // tramas esperando en este enlace (id de trama)(quiero queque :v)

    Enlace(int d, int bw, double lat)
        : destino(d), bandwidthBps(bw), latenciaMs(lat) {
        costoOSPF = REF_BW / bw;
        if (costoOSPF < 1) costoOSPF = 1;
    }
};


//  Estructura: Nodo
struct Nodo {
    int id;
    string nombre;
    vector<Enlace> adyacentes;

    // Variables aleatorias de saturacion
    double  cargaCPU;       // % uso CPU   [0.0 - 1.0]
    double  cargaMemoria;   // % uso RAM   [0.0 - 1.0]
    double  saturacion;     // combinacion ponderada

    // Estadisticas
    int tramasRecibidas;
    int tramasEnviadas;

    Nodo(int i, const string& n)
        : id(i), nombre(n),
          cargaCPU(0), cargaMemoria(0), saturacion(0),
          tramasRecibidas(0), tramasEnviadas(0) {}

    // Actualiza las variables aleatorias y calcula saturacion
    void actualizarSaturacion() {
        // Simula lecturas de sensores: fluctuacion aleatoria +/-10 %
        double dCPU = ((rand() % 21) - 10) / 100.0;
        double dMem = ((rand() % 21) - 10) / 100.0;
        cargaCPU    = max(0.0, min(1.0, cargaCPU + dCPU));
        cargaMemoria= max(0.0, min(1.0, cargaMemoria + dMem));
        // Saturacion = 60 % CPU + 40 % RAM  (ponderacion arbitraria OSPF-like)
        saturacion  = 0.6 * cargaCPU + 0.4 * cargaMemoria;
    }

    bool estaSaturado() const { return saturacion >= SAT_UMBRAL; }

    // Anade un enlace bidireccional (solo registra la mitad saliente aqui)
    void agregarEnlace(int dest, int bw, double lat) {
        adyacentes.emplace_back(dest, bw, lat);
    }
};


//  Estructura: Trama
struct Trama {
    int id;
    int origen;
    int destino;
    vector<int> ruta;          // secuencia de nodos calculada por Dijkstra
    int posicionEnRuta;         // indice actual dentro de ruta
    int tickCreacion;
    int tickEntrega;
    bool entregada;
    bool descartada;
    string motivo;             // motivo si fue descartada

    Trama(int i, int o, int d, int tickCreado)
        : id(i), origen(o), destino(d),
          posicionEnRuta(0), tickCreacion(tickCreado),
          tickEntrega(-1), entregada(false), descartada(false) {}

    int nodoActual() const {
        if (posicionEnRuta < (int)ruta.size()) return ruta[posicionEnRuta];
        return -1;
    }
    int nodoSiguiente() const {
        if (posicionEnRuta + 1 < (int)ruta.size()) return ruta[posicionEnRuta + 1];
        return -1;
    }
    bool llego() const { return posicionEnRuta == (int)ruta.size() - 1; }
};


//  Resultado de Dijkstra

struct ResultadoDijkstra {
    vector<int> distancia;
    vector<int> predecesor;
};
