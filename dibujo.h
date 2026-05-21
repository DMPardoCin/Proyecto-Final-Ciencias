#ifndef DIBUJO_H
#define DIBUJO_H

#include "estado.h"
#include "layout.h"
#include "simulacion.h"
#include "miniwin.h"
#include "pc.h"
#include "router.h"
#include "switch.h"
using namespace miniwin;

// ============================================================
//  SPRITES
// ============================================================
inline void dibujarSprite40(int x, int y, int esc, pixelRGB img[][40],
                            bool res, int tR, int tG, int tB)
{
    for (int i = 0; i < 40; i++)
        for (int j = 0; j < 40; j++)
        {
            int r = img[i][j].R, g = img[i][j].G, b = img[i][j].B;
            if (res)
            {
                r = min(255, r + tR);
                g = min(255, g + tG);
                b = min(255, b + tB);
            }
            color_rgb(r, g, b);
            rectangulo_lleno(x + j * esc, y + i * esc, x + j * esc + esc - 1, y + i * esc + esc - 1);
        }
}
inline void dibujarRouter(int x, int y, bool res = false)
{
    int e = ESCALA_NODO;
    for (int i = 0; i < 23; i++)
        for (int j = 0; j < 40; j++)
        {
            int r = router[i][j].R, g = router[i][j].G, b = router[i][j].B;
            if (res)
                g = min(255, g + 80);
            color_rgb(r, g, b);
            rectangulo_lleno(x + j * e, y + i * e, x + j * e + e - 1, y + i * e + e - 1);
        }
}
inline void dibujarSwitch(int x, int y, bool res = false)
{
    int e = ESCALA_SW;
    for (int i = 0; i < 19; i++)
        for (int j = 0; j < 60; j++)
        {
            int r = switche[i][j].R, g = switche[i][j].G, b = switche[i][j].B;
            if (res)
            {
                r = min(255, r + 40);
                b = min(255, b + 80);
            }
            color_rgb(r, g, b);
            rectangulo_lleno(x + j * e, y + i * e, x + j * e + e - 1, y + i * e + e - 1);
        }
}

// ============================================================
//  DIJKSTRA: solo ruta optima visible en pantalla
// ============================================================
inline bool esEnlaceEnRuta(int a, int b)
{
    if ((int)dijkstraRuta.size() < 2)
        return false;
    for (int i = 0; i < (int)dijkstraRuta.size() - 1; i++)
    {
        int u = dijkstraRuta[i], v = dijkstraRuta[i + 1];
        if ((u == a && v == b) || (u == b && v == a))
            return true;
    }
    return false;
}

inline void lineaGruesa(int ax, int ay, int bx, int by, int g)
{
    for (int d = -(g / 2); d <= g / 2; d++)
    {
        linea(ax + d, ay, bx + d, by);
        linea(ax, ay + d, bx, by + d);
    }
}

// Marcadores animados de origen/destino + costo total sobre el canvas
inline void dibujarMarcadoresDijkstra()
{
    if (dijkstraOrigen < 0 || dijkstraOrigen >= (int)elementos.size())
        return;
    int ox = centroX(dijkstraOrigen), oy = centroY(dijkstraOrigen);
    int radio = 14 + (tickActual % 20 < 10 ? tickActual % 10 : 10 - tickActual % 10);

    color_rgb(0, 180, 180);
    circulo(ox, oy, radio);
    color_rgb(0, 230, 230);
    circulo(ox, oy, radio - 2);
    texto(ox - 16, max(2, oy - radio - 16), "ORIGEN");

    if (dijkstraDestino >= 0 && dijkstraDestino < (int)elementos.size() && dijkstraDestino != dijkstraOrigen)
    {
        int dx = centroX(dijkstraDestino), dy = centroY(dijkstraDestino);
        color_rgb(200, 200, 0);
        circulo(dx, dy, radio);
        circulo(dx, dy, radio - 2);
        color_rgb(255, 255, 80);
        texto(dx - 20, max(2, dy - radio - 16), "DESTINO");

        // Costo total y ruta optima en texto flotante entre los dos nodos
        if (!dijkstraRuta.empty() && (int)dijkstraRes.distancia.size() > dijkstraDestino)
        {
            int cTotal = dijkstraRes.distancia[dijkstraDestino];
            if (cTotal != INF)
            {
                // Construir string de ruta optima
                string rutaStr = "";
                for (int k = 0; k < (int)dijkstraRuta.size(); k++)
                {
                    if (k)
                        rutaStr += "->";
                    rutaStr += grafo.nodos[dijkstraRuta[k]].nombre;
                }
                int lx = (ox + dx) / 2 - 60, ly = (oy + dy) / 2 - 36;
                lx = max(4, min(lx, RED_W - 200));
                ly = max(4, min(ly, AH - 40));
                // Fondo
                color_rgb(15, 15, 30);
                rectangulo_lleno(lx - 2, ly - 2, lx + 196, ly + 30);
                color_rgb(0, 180, 180);
                rectangulo(lx - 2, ly - 2, lx + 196, ly + 30);
                // Texto
                color_rgb(0, 230, 230);
                int cb = 0;
                for (int k = 0; k + 1 < (int)dijkstraRuta.size(); k++)
                {
                    Enlace *ev = grafo.buscarEnlace(dijkstraRuta[k], dijkstraRuta[k + 1]);
                    if (ev)
                        cb += ev->costoOSPF;
                }
                color_rgb(0, 230, 230);
                string cStr = "OSPF:" + to_string(cb);
                if (cTotal != cb)
                    cStr += " (+" + to_string(cTotal - cb) + " sat)";
                texto(lx, ly, cStr);
                color_rgb(200, 200, 200);
                texto(lx, ly + 14, rutaStr);
            }
        }
    }
}

// ============================================================
//  ENLACES
// ============================================================
inline void dibujarEnlaces()
{
    for (auto &e : enlacesVis)
    {
        if (e.a >= (int)elementos.size() || e.b >= (int)elementos.size())
            continue;
        int ax = centroX(e.a), ay = centroY(e.a);
        int bx = centroX(e.b), by = centroY(e.b);
        bool enRuta = esEnlaceEnRuta(e.a, e.b);
        Enlace *eg = grafo.buscarEnlace(e.a, e.b);

        if (enRuta)
        {
            color_rgb(0, 230, 230);
            lineaGruesa(ax, ay, bx, by, 3);
        }
        else
        {
            if (eg)
            {
                int c = eg->costoOSPF;
                if (c <= 1)
                    color_rgb(60, 200, 60);
                else if (c <= 10)
                    color_rgb(220, 200, 50);
                else
                    color_rgb(220, 70, 70);
            }
            else
                color_rgb(100, 100, 100);
            linea(ax, ay, bx, by);
        }
        if (eg)
        {
            int mx2 = (ax + bx) / 2, my2 = (ay + by) / 2;
            int bwM = eg->bandwidthBps / 1000000;
            color_rgb(12, 12, 22);
            rectangulo_lleno(mx2 - 2, my2 - 24, mx2 + 72, my2 + 2);
            color_rgb(enRuta ? 0 : 220, enRuta ? 230 : 220, enRuta ? 230 : 80);
            texto(mx2 + 2, my2 - 22, to_string(bwM) + "Mbps");
            color_rgb(160, 160, 160);
            texto(mx2 + 2, my2 - 8, "cost:" + to_string(eg->costoOSPF));
            auto ck = claveEnlace(e.a, e.b);
            int ocup = colaEnlace.count(ck) ? colaEnlace[ck] : 0;
            if (ocup > 0)
            {
                color_rgb(220, 100, 40);
                texto(mx2 + 2, my2 + 6, "cola:" + to_string(ocup));
            }
        }
    }
    // Linea guia enlazar
    if ((modoActual == MODO_ENLAZAR_B || modoActual == MODO_ENLAZAR_PESO) && nodoEnlaceA >= 0 && nodoEnlaceA < (int)elementos.size())
    {
        color_rgb(80, 150, 255);
        linea(centroX(nodoEnlaceA), centroY(nodoEnlaceA), (int)raton_x(), (int)raton_y());
    }
    // Linea guia trama manual
    if (modoActual == MODO_ENVIAR_MANUAL_B && envioOrigen >= 0 && envioOrigen < (int)elementos.size())
    {
        color_rgb(255, 220, 40);
        linea(centroX(envioOrigen), centroY(envioOrigen), (int)raton_x(), (int)raton_y());
    }
}

// ============================================================
//  NODOS
// ============================================================
inline void dibujarElementos()
{
    for (int i = 0; i < (int)elementos.size() && i < grafo.numNodos; i++)
    {
        bool res = (i == nodoSeleccionado) || (i == nodoEnlaceA) || (i == elimEnlaceA) ||
                   (i == dijkstraOrigen) || (i == dijkstraDestino) || (i == envioOrigen);
        int ex = elementos[i].x, ey = elementos[i].y;
        if (elementos[i].tipo == PC)
            dibujarSprite40(ex, ey, ESCALA_NODO, pc, res, 60, 60, 0);
        else if (elementos[i].tipo == ROUTER)
            dibujarRouter(ex, ey, res);
        else
            dibujarSwitch(ex, ey, res);

        color(BLANCO);
        texto(ex + 4, max(2, ey - 16), grafo.nodos[i].nombre);
        color_rgb(130, 130, 130);
        texto(ex + ancho_elem(i) - 22, ey + 4, "[" + to_string(i) + "]");

        // Barra de saturacion
        float sat = grafo.nodos[i].saturacion;
        int bx = ex, by = ey + alto_elem(i) + 4, bw = ancho_elem(i);
        color_rgb(20, 20, 20);
        rectangulo_lleno(bx, by, bx + bw, by + 6);
        int rell = (int)(sat * bw);
        if (sat < 0.5)
            color_rgb(60, 200, 60);
        else if (sat < 0.85)
            color_rgb(220, 180, 40);
        else
            color_rgb(220, 50, 50);
        rectangulo_lleno(bx, by, bx + rell, by + 6);
        color_rgb(60, 60, 60);
        rectangulo(bx, by, bx + bw, by + 6);

        if (modoActual == MODO_ELIMINAR_NODO && i == nodoSeleccionado)
        {
            color_rgb(220, 40, 40);
            rectangulo(ex - 2, ey - 2, ex + ancho_elem(i) + 2, ey + alto_elem(i) + 2);
        }
        if (i == dijkstraOrigen)
        {
            color_rgb(0, 200, 200);
            rectangulo(ex - 3, ey - 3, ex + ancho_elem(i) + 3, ey + alto_elem(i) + 3);
        }
        if (i == dijkstraDestino && dijkstraDestino != dijkstraOrigen)
        {
            color_rgb(200, 200, 0);
            rectangulo(ex - 3, ey - 3, ex + ancho_elem(i) + 3, ey + alto_elem(i) + 3);
        }
        if (i == envioOrigen)
        {
            color_rgb(255, 200, 0);
            rectangulo(ex - 3, ey - 3, ex + ancho_elem(i) + 3, ey + alto_elem(i) + 3);
        }
    }
}

// ============================================================
//  TRAMAS
// ============================================================
inline void dibujarTramas()
{
    for (auto &t : tramas)
    {
        if (t.entregada)
            continue;
        if (t.enEspera)
        {
            color_rgb(180, 80, 80);
            circulo_lleno(t.vx, t.vy, 6);
        }
        else
        {
            color_rgb(255, 220, 40);
            circulo_lleno(t.vx, t.vy, 7);
            color_rgb(255, 120, 0);
            circulo(t.vx, t.vy, 8);
        }
        if (t.origen < grafo.numNodos && t.destino < grafo.numNodos)
        {
            color_rgb(200, 200, 200);
            texto((int)t.vx + 10, (int)t.vy - 10,
                  grafo.nodos[t.origen].nombre + "->" + grafo.nodos[t.destino].nombre);
        }
    }
}

#endif