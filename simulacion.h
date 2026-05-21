#ifndef SIMULACION_H
#define SIMULACION_H

#include "estado.h"
#include "layout.h"

// ============================================================
//  BW REAL: velocidad y capacidad proporcionales al ancho de banda
// ============================================================
inline int ticksPorSalto(int a, int b)
{
    Enlace *e = grafo.buscarEnlace(a, b);
    if (!e)
        return 30;
    int mbps = e->bandwidthBps / 1000000;
    if (mbps >= 100)
        return 8;
    if (mbps >= 50)
        return 12;
    if (mbps >= 10)
        return 20;
    if (mbps >= 5)
        return 35;
    return 80;
}

inline int colaMaxEnlace(int a, int b)
{
    Enlace *e = grafo.buscarEnlace(a, b);
    if (!e)
        return MAX_QUEUE;
    int mbps = e->bandwidthBps / 1000000;
    if (mbps >= 100)
        return 20;
    if (mbps >= 10)
        return 10;
    return 4;
}

// ============================================================
//  ENVIO DE TRAMAS
// ============================================================
inline void enviarTrama(int orig, int dest)
{
    if (orig < 0 || dest < 0 || orig >= grafo.numNodos || dest >= grafo.numNodos || orig == dest)
        return;
    ResultadoDijkstra res = grafo.dijkstra(orig);
    vector<int> ruta = grafo.reconstruirRuta(res, dest);
    if ((int)ruta.size() < 2)
        return;
    int pesoTotal = 0;
    for (int i = 0; i + 1 < (int)ruta.size(); i++)
    {
        Enlace *e = grafo.buscarEnlace(ruta[i], ruta[i + 1]);
        if (e)
            pesoTotal += e->costoOSPF;
    }
    TramaVis t;
    t.origen = orig;
    t.destino = dest;
    t.ruta = ruta;
    t.pos = 0;
    t.entregada = false;
    t.ticksVida = 0;
    t.ticksTotales = 0;
    t.saltos = 0;
    t.pesoRuta = pesoTotal;
    t.ticksEspera = 0;
    t.enEspera = false;
    t.vx = centroX(orig);
    t.vy = centroY(orig);
    tramas.push_back(t);
    simPausada = false;
}

inline void enviarTramaAuto()
{
    if (grafo.numNodos < 2)
        return;
    int orig = rand() % grafo.numNodos, dest;
    do
    {
        dest = rand() % grafo.numNodos;
    } while (dest == orig);
    enviarTrama(orig, dest);
}

// ============================================================
//  ACTUALIZAR TRAMAS (logica de movimiento y colas)
// ============================================================
inline void actualizarTramas()
{
    for (auto &kv : colaEnlace)
        kv.second = 0;

    for (auto &t : tramas)
    {
        if (t.entregada)
            continue;
        if ((int)t.ruta.size() < 2)
        {
            t.entregada = true;
            continue;
        }
        if (t.pos >= (int)t.ruta.size() - 1)
        {
            metricas.registrar(t);
            t.entregada = true;
            grafo.nodos[t.destino].tramasRecibidas++;
            continue;
        }
        int pA = t.ruta[t.pos], pB = t.ruta[t.pos + 1];
        if (pA >= (int)elementos.size() || pB >= (int)elementos.size())
        {
            t.entregada = true;
            continue;
        }
        auto ck = claveEnlace(pA, pB);
        int ocup = colaEnlace.count(ck) ? colaEnlace[ck] : 0;
        if (ocup >= colaMaxEnlace(pA, pB))
        {
            t.enEspera = true;
            t.ticksEspera++;
            continue;
        }
        t.enEspera = false;
        colaEnlace[ck] = ocup + 1;
        metricas.maxCola = max(metricas.maxCola, ocup + 1);

        float ax = centroX(pA), ay = centroY(pA);
        float bx = centroX(pB), by = centroY(pB);
        int tpS = ticksPorSalto(pA, pB);
        t.ticksVida++;
        t.ticksTotales++;
        float prog = min(1.f, (float)t.ticksVida / (float)tpS);
        t.vx = ax + (bx - ax) * prog;
        t.vy = ay + (by - ay) * prog;
        if (t.ticksVida >= tpS)
        {
            t.pos++;
            t.ticksVida = 0;
            t.saltos++;
            grafo.nodos[pA].tramasEnviadas++;
            if (t.pos >= (int)t.ruta.size() - 1)
            {
                metricas.registrar(t);
                t.entregada = true;
                grafo.nodos[t.destino].tramasRecibidas++;
            }
        }
    }
}

// ============================================================
//  CREACION Y ELIMINACION DE NODOS/ENLACES
// ============================================================
inline void crearNodo(int x, int y, TipoElemento tipo)
{
    string p = (tipo == PC) ? "PC" : (tipo == ROUTER) ? "R"
                                                      : "SW";
    grafo.agregarNodo(p + to_string(grafo.numNodos));
    ElementoVisual ev;
    ev.tipo = tipo;
    int hw = (tipo == SWITCH) ? TAM_SW_W / 2 : TAM_NODO / 2;
    int hh = (tipo == SWITCH) ? TAM_SW_H / 2 : TAM_NODO / 2;
    ev.x = max(0, min(x - hw, RED_W - (tipo == SWITCH ? TAM_SW_W : TAM_NODO)));
    ev.y = max(0, min(y - hh, AH - (tipo == SWITCH ? TAM_SW_H : TAM_NODO)));
    elementos.push_back(ev);
}

inline void confirmarEnlace()
{
    int bw = max(1, pesoEnlaceInput) * 1000000;
    grafo.agregarEnlace(nodoEnlaceA, nodoEnlaceB, bw, 1.0);
    enlacesVis.push_back({nodoEnlaceA, nodoEnlaceB});
    colaEnlace[claveEnlace(nodoEnlaceA, nodoEnlaceB)] = 0;
    if (dijkstraOrigen >= 0)
    {
        dijkstraRes = grafo.dijkstra(dijkstraOrigen);
        if (dijkstraDestino >= 0)
            dijkstraRuta = grafo.reconstruirRuta(dijkstraRes, dijkstraDestino);
    }
    nodoEnlaceA = nodoEnlaceB = -1;
    pesoEnlaceInput = 10;
    modoActual = MODO_NORMAL;
}

inline void eliminarEnlace(int a, int b)
{
    for (int i = 0; i < (int)enlacesVis.size(); i++)
    {
        auto &e = enlacesVis[i];
        if ((e.a == a && e.b == b) || (e.a == b && e.b == a))
        {
            enlacesVis.erase(enlacesVis.begin() + i);
            break;
        }
    }
    auto elimAdj = [&](int u, int v)
    {
        auto &adj = grafo.nodos[u].adyacentes;
        for (int i = 0; i < (int)adj.size(); i++)
            if (adj[i].destino == v)
            {
                adj.erase(adj.begin() + i);
                break;
            }
    };
    elimAdj(a, b);
    elimAdj(b, a);
    colaEnlace.erase(claveEnlace(a, b));
    for (auto &t : tramas)
        t.entregada = true;
    if (dijkstraOrigen >= 0)
    {
        dijkstraRes = grafo.dijkstra(dijkstraOrigen);
        if (dijkstraDestino >= 0)
            dijkstraRuta = grafo.reconstruirRuta(dijkstraRes, dijkstraDestino);
    }
}

inline void eliminarNodo(int idx)
{
    if (idx < 0 || idx >= (int)elementos.size())
        return;
    for (int i = (int)enlacesVis.size() - 1; i >= 0; i--)
        if (enlacesVis[i].a == idx || enlacesVis[i].b == idx)
            enlacesVis.erase(enlacesVis.begin() + i);
    for (auto &e : enlacesVis)
    {
        if (e.a > idx)
            e.a--;
        if (e.b > idx)
            e.b--;
    }
    for (int i = 0; i < grafo.numNodos; i++)
    {
        auto &adj = grafo.nodos[i].adyacentes;
        for (int j = (int)adj.size() - 1; j >= 0; j--)
        {
            if (adj[j].destino == idx)
                adj.erase(adj.begin() + j);
            else if (adj[j].destino > idx)
                adj[j].destino--;
        }
    }
    grafo.nodos.erase(grafo.nodos.begin() + idx);
    grafo.numNodos--;
    elementos.erase(elementos.begin() + idx);
    tramas.clear();
    colaEnlace.clear();
    nodoSeleccionado = dijkstraOrigen = dijkstraDestino = -1;
    dijkstraRuta.clear();
    dijkstraRes.distancia.clear();
    elimEnlaceA = nodoEnlaceA = nodoEnlaceB = envioOrigen = envioDestino = -1;
}

// ============================================================
//  TICK
// ============================================================
inline void tickSimulacion()
{
    actualizarTramas();
    if (simPausada)
        return;
    tickActual++;
    for (Nodo &n : grafo.nodos)
        n.actualizarSaturacion();
    if (tickActual % 80 == 0 && grafo.numNodos >= 2)
        enviarTramaAuto();
}

#endif