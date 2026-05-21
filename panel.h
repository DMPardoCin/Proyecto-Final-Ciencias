#ifndef PANEL_H
#define PANEL_H

#include "estado.h"
#include "layout.h"
#include "simulacion.h"
#include "miniwin.h"
using namespace miniwin;

// ============================================================
//  BOTON
// ============================================================
inline void btnInit(Boton &b, int x, int y, int w, int h, const char *t)
{
    b.x = x;
    b.y = y;
    b.ancho = w;
    b.alto = h;
    b.texto = t;
    b.clic_procesado = false;
}
inline void btnDibujar(const Boton &b, bool activo = false)
{
    if (activo)
        color_rgb(45, 100, 180);
    else if (b.clic_procesado)
        color_rgb(80, 80, 80);
    else
        color_rgb(30, 30, 46);
    rectangulo_lleno(b.x, b.y, b.x + b.ancho, b.y + b.alto);
    color_rgb(activo ? 110 : 70, activo ? 170 : 120, 240);
    rectangulo(b.x, b.y, b.x + b.ancho, b.y + b.alto);
    color(BLANCO);
    texto(b.x + 7, b.y + b.alto / 2 - 7, b.texto);
}
inline bool btnClick(Boton &b)
{
    float mx = raton_x(), my = raton_y();
    bool enc = mx >= b.x && mx <= b.x + b.ancho && my >= b.y && my <= b.y + b.alto;
    if (raton_boton_izq())
    {
        if (!b.clic_procesado && enc)
        {
            b.clic_procesado = true;
            return true;
        }
    }
    else
        b.clic_procesado = false;
    return false;
}

// ============================================================
//  DECLARACION DE BOTONES
// ============================================================
Boton btnCrearPC, btnCrearRouter, btnCrearSwitch;
Boton btnEnlazar, btnElimEnlace, btnElimNodo;
Boton btnDijkstra;
Boton btnSimular, btnPausar, btnReset;
Boton btnEnviarManual, btnEnviarAuto;
Boton popPesoMenos, popPesoMas, popPesoD10, popPesoX10, popPesoOK;

inline void inicializarBotones()
{
    int px = RED_W + 8, bw = PANEL_W - 16;
    int bh = max(26, AH / 33), gap = bh + 5, y = 8;
    btnInit(btnCrearPC, px, y, bw, bh, "[+] Agregar PC");
    y += gap;
    btnInit(btnCrearRouter, px, y, bw, bh, "[+] Agregar Router");
    y += gap;
    btnInit(btnCrearSwitch, px, y, bw, bh, "[+] Agregar Switch");
    y += gap + 4;
    btnInit(btnEnlazar, px, y, bw, bh, "[~] Enlazar (peso)");
    y += gap;
    btnInit(btnElimEnlace, px, y, bw, bh, "[-] Eliminar enlace");
    y += gap;
    btnInit(btnElimNodo, px, y, bw, bh, "[x] Eliminar nodo");
    y += gap + 4;
    btnInit(btnDijkstra, px, y, bw, bh, "[?] Ver Dijkstra");
    y += gap + 4;
    int hw = bw / 2 - 2;
    btnInit(btnEnviarManual, px, y, hw, bh, "[>>] Manual");
    btnInit(btnEnviarAuto, px + hw + 4, y, hw, bh, "[>>] Auto");
    y += gap;
    btnInit(btnSimular, px, y, hw, bh, "[>] Simular");
    btnInit(btnPausar, px + hw + 4, y, hw, bh, "[||] Pausar");
    y += gap;
    btnInit(btnReset, px, y, bw, bh, "[R] Reset sim.");
}

// ============================================================
//  BARRA HORIZONTAL
// ============================================================
inline void barraH(int px, int &py, float val,
                   int rL, int gL, int bL, int rH, int gH, int bH, int w = -1)
{
    if (w < 0)
        w = PANEL_BAR_W;
    color_rgb(20, 20, 20);
    rectangulo_lleno(px, py, px + w, py + 8);
    int rell = (int)(val * w);
    if (val < 0.5)
        color_rgb(rL, gL, bL);
    else if (val < 0.85)
        color_rgb(220, 180, 40);
    else
        color_rgb(rH, gH, bH);
    rectangulo_lleno(px, py, px + rell, py + 8);
    color_rgb(60, 60, 60);
    rectangulo(px, py, px + w, py + 8);
    py += 11;
}

// ============================================================
//  SEPARADOR
// ============================================================
inline void dibujarSeparador()
{
    color_rgb(18, 18, 30);
    rectangulo_lleno(RED_W, 0, AW, AH);
    color_rgb(55, 95, 195);
    linea(RED_W, 0, RED_W, AH);
}

// ============================================================
//  INDICADOR DE MODO
// ============================================================
inline void dibujarModo()
{
    int px = RED_W + 8, py = btnReset.y + btnReset.alto + 10;
    color_rgb(25, 25, 42);
    rectangulo_lleno(px, py, px + PANEL_W - 16, py + 22);
    color_rgb(255, 200, 50);
    switch (modoActual)
    {
    case MODO_NORMAL:
        texto(px + 4, py + 4, "Modo: Normal");
        break;
    case MODO_CREAR_PC:
        texto(px + 4, py + 4, "Clic -> crear PC");
        break;
    case MODO_CREAR_ROUTER:
        texto(px + 4, py + 4, "Clic -> crear Router");
        break;
    case MODO_CREAR_SWITCH:
        texto(px + 4, py + 4, "Clic -> crear Switch");
        break;
    case MODO_ENLAZAR:
        texto(px + 4, py + 4, "Enlazar: elige nodo A");
        break;
    case MODO_ENLAZAR_B:
        texto(px + 4, py + 4, "Enlazar: elige nodo B");
        break;
    case MODO_ENLAZAR_PESO:
        texto(px + 4, py + 4, "Ajusta peso [ENTER]");
        break;
    case MODO_ELIMINAR_ENLACE:
        texto(px + 4, py + 4, "Elim.enlace: nodo A");
        break;
    case MODO_ELIMINAR_ENLACE_B:
        texto(px + 4, py + 4, "Elim.enlace: nodo B");
        break;
    case MODO_ELIMINAR_NODO:
        texto(px + 4, py + 4, "Elim.nodo: clic");
        break;
    case MODO_VER_DIJKSTRA:
        texto(px + 4, py + 4, "Dijkstra: elige origen");
        break;
    case MODO_DIJKSTRA_DEST:
        texto(px + 4, py + 4, "Dijkstra: elige destino");
        break;
    case MODO_ENVIAR_MANUAL:
        texto(px + 4, py + 4, "Trama: elige origen");
        break;
    case MODO_ENVIAR_MANUAL_B:
        texto(px + 4, py + 4, "Trama: elige destino");
        break;
    }
}

// ============================================================
//  POPUP DE PESO (flotante en canvas)
// ============================================================
inline void dibujarPopupPeso()
{
    if (modoActual != MODO_ENLAZAR_PESO)
        return;
    if (nodoEnlaceA < 0 || nodoEnlaceB < 0)
        return;
    if (nodoEnlaceA >= (int)elementos.size() || nodoEnlaceB >= (int)elementos.size())
        return;
    int mx2 = (centroX(nodoEnlaceA) + centroX(nodoEnlaceB)) / 2;
    int my2 = (centroY(nodoEnlaceA) + centroY(nodoEnlaceB)) / 2;
    const int PW = 220, PH = 130;
    int px = max(4, min(mx2 - PW / 2, RED_W - PW - 4));
    int py = max(4, min(my2 - PH / 2, AH - PH - 4));
    color_rgb(8, 8, 18);
    rectangulo_lleno(px + 3, py + 3, px + PW + 3, py + PH + 3);
    color_rgb(28, 28, 48);
    rectangulo_lleno(px, py, px + PW, py + PH);
    color_rgb(80, 140, 255);
    rectangulo(px, py, px + PW, py + PH);
    color_rgb(50, 50, 80);
    rectangulo_lleno(px + 1, py + 1, px + PW - 1, py + 22);
    color_rgb(255, 220, 60);
    texto(px + 8, py + 6, "Peso del enlace");
    int bw2 = max(1, pesoEnlaceInput) * 1000000;
    int costo = max(1, REF_BW / bw2);
    string valStr = to_string(pesoEnlaceInput) + " Mbps";
    color(BLANCO);
    texto(px + PW / 2 - (int)valStr.size() * 4, py + 30, valStr);
    color_rgb(120, 200, 120);
    texto(px + 8, py + 48, "Costo OSPF: " + to_string(costo));
    int bwb = 44, bh = 22, gap = 48, by = py + 70, bx = px + 6;
    btnInit(popPesoMenos, bx, by, bwb, bh, "-10");
    btnInit(popPesoMas, bx + gap, by, bwb, bh, "+10");
    btnInit(popPesoD10, bx + gap * 2, by, bwb, bh, "/10");
    btnInit(popPesoX10, bx + gap * 3, by, bwb, bh, "x10");
    btnDibujar(popPesoMenos);
    btnDibujar(popPesoMas);
    btnDibujar(popPesoD10);
    btnDibujar(popPesoX10);
    btnInit(popPesoOK, px + 8, py + PH - 28, PW - 16, 22, "[ENTER] Confirmar");
    btnDibujar(popPesoOK, true);
    color_rgb(100, 100, 100);
    texto(px + 8, py + PH + 6, "ESC para cancelar");
}

// ============================================================
//  PANEL PRINCIPAL DE DATOS
// ============================================================
inline void dibujarPanelDatos()
{
    int px = RED_W + 8, py = btnReset.y + btnReset.alto + 38;

    // Simulacion
    color_rgb(70, 140, 255);
    texto(px, py, "-- SIMULACION --");
    py += 19;
    color_rgb(160, 160, 160);
    int act = 0;
    for (auto &t : tramas)
        if (!t.entregada)
            act++;
    texto(px, py, "Tick:    " + to_string(tickActual));
    py += 16;
    texto(px, py, "Activas: " + to_string(act));
    py += 16;
    texto(px, py, "Nodos:" + to_string(grafo.numNodos) + " Enl:" + to_string((int)enlacesVis.size()));
    py += 16;
    if (simPausada)
    {
        color_rgb(220, 180, 40);
        texto(px, py, "[PAUSADA]");
    }
    else
    {
        color_rgb(60, 220, 60);
        texto(px, py, "[CORRIENDO]");
    }
    py += 20;

    // Metricas
    color_rgb(70, 140, 255);
    texto(px, py, "-- METRICAS --");
    py += 19;
    color_rgb(160, 160, 160);
    texto(px, py, "Entregadas: " + to_string(metricas.totalEntregadas));
    py += 16;
    texto(px, py, "T.entrega:  " + to_string((int)metricas.tiempoPromedio()) + " ticks");
    py += 14;
    barraH(px, py, min(1.f, (float)(metricas.tiempoPromedio() / 100.f)), 60, 200, 60, 220, 50, 50);
    texto(px, py, "Saltos prom:" + to_string((int)metricas.saltosPromedio()));
    py += 14;
    barraH(px, py, min(1.f, (float)(metricas.saltosPromedio() / 8.f)), 60, 200, 60, 220, 50, 50);
    texto(px, py, "Espera cola:" + to_string((int)metricas.esperaPromedio()));
    py += 14;
    barraH(px, py, min(1.f, (float)(metricas.esperaPromedio() / 20.f)), 60, 150, 220, 220, 50, 50);
    texto(px, py, "Max cola:   " + to_string(metricas.maxCola) + "/" + to_string(MAX_QUEUE));
    py += 14;
    barraH(px, py, (float)metricas.maxCola / MAX_QUEUE, 60, 200, 60, 220, 50, 50);
    py += 6;

    // Nodo activo
    color_rgb(70, 140, 255);
    texto(px, py, "-- NODO ACTIVO --");
    py += 19;
    if (nodoSeleccionado >= 0 && nodoSeleccionado < grafo.numNodos)
    {
        Nodo &n = grafo.nodos[nodoSeleccionado];
        color_rgb(255, 240, 70);
        texto(px, py, n.nombre);
        py += 16;
        color_rgb(150, 150, 150);
        texto(px, py, "CPU " + to_string((int)(n.cargaCPU * 100)) + "%");
        py += 13;
        barraH(px, py, n.cargaCPU, 60, 200, 60, 220, 50, 50);
        texto(px, py, "RAM " + to_string((int)(n.cargaMemoria * 100)) + "%");
        py += 13;
        barraH(px, py, n.cargaMemoria, 60, 120, 220, 220, 50, 50);
        texto(px, py, "SAT " + to_string((int)(n.saturacion * 100)) + "%");
        py += 13;
        barraH(px, py, n.saturacion, 60, 200, 60, 220, 50, 50);
        if (n.estaSaturado())
        {
            color_rgb(220, 50, 50);
            texto(px, py, "! SATURADO !");
            py += 16;
        }
        color_rgb(130, 130, 130);
        texto(px, py, "RX:" + to_string(n.tramasRecibidas) + " TX:" + to_string(n.tramasEnviadas));
        py += 16;
        color_rgb(70, 140, 255);
        texto(px, py, "Vecinos:");
        py += 14;
        for (auto &e : n.adyacentes)
        {
            if (e.destino < grafo.numNodos && py < AH - 80)
            {
                color_rgb(110, 110, 110);
                texto(px + 6, py, "->" + grafo.nodos[e.destino].nombre + " " + to_string(e.bandwidthBps / 1000000) + "M C:" + to_string(e.costoOSPF));
                py += 14;
            }
        }
        py += 4;
    }
    else
    {
        color_rgb(80, 80, 80);
        texto(px, py, "Clic en nodo para ver datos");
        py += 20;
    }

    // Dijkstra: solo ruta optima al destino elegido
    if (dijkstraOrigen >= 0 && dijkstraDestino >= 0 && !dijkstraRuta.empty())
    {
        if (py > AH - 100)
            return;
        color_rgb(0, 200, 200);
        texto(px, py, "-- RUTA OPTIMA --");
        py += 18;

        // Ruta como texto
        color_rgb(0, 230, 230);
        string rutaStr = "";
        for (int k = 0; k < (int)dijkstraRuta.size(); k++)
        {
            if (k)
                rutaStr += "->";
            if (dijkstraRuta[k] < grafo.numNodos)
                rutaStr += grafo.nodos[dijkstraRuta[k]].nombre;
        }
        texto(px, py, rutaStr);
        py += 18;

        // ── Costo base: suma de costoOSPF de cada enlace (lo que ve el usuario)
        int costoBase = 0;
        for (int k = 0; k + 1 < (int)dijkstraRuta.size(); k++)
        {
            Enlace *e = grafo.buscarEnlace(dijkstraRuta[k], dijkstraRuta[k + 1]);
            if (e)
            {
                costoBase += e->costoOSPF;
                // Mostrar desglose por enlace
                color_rgb(120, 120, 120);
                texto(px + 6, py,
                      grafo.nodos[dijkstraRuta[k]].nombre + "->" +
                          grafo.nodos[dijkstraRuta[k + 1]].nombre +
                          " = " + to_string(e->costoOSPF));
                py += 14;
            }
        }

        // Linea separadora
        color_rgb(60, 60, 80);
        linea(px, py, px + PANEL_BAR_W, py);
        py += 6;

        // Costo base (suma de los enlaces, sin penalidad)
        color_rgb(200, 230, 200);
        texto(px, py, "Costo OSPF: " + to_string(costoBase));
        py += 16;

        // Costo efectivo (con penalidad por saturacion, el que uso Dijkstra)
        int cEfectivo = (dijkstraDestino < (int)dijkstraRes.distancia.size())
                            ? dijkstraRes.distancia[dijkstraDestino]
                            : -1;
        if (cEfectivo != costoBase && cEfectivo >= 0 && cEfectivo != INF)
        {
            color_rgb(180, 180, 100);
            texto(px, py, "+ saturacion: +" + to_string(cEfectivo - costoBase));
            py += 14;
            color_rgb(220, 180, 40);
            texto(px, py, "Total efectivo: " + to_string(cEfectivo));
            py += 16;
        }

        // BW cuello de botella
        int minBW = 999999;
        for (int k = 0; k + 1 < (int)dijkstraRuta.size(); k++)
        {
            Enlace *e = grafo.buscarEnlace(dijkstraRuta[k], dijkstraRuta[k + 1]);
            if (e)
                minBW = min(minBW, e->bandwidthBps / 1000000);
        }
        color_rgb(130, 130, 130);
        if (minBW != 999999)
            texto(px, py, "BW minimo: " + to_string(minBW) + "Mbps  Saltos: " + to_string((int)dijkstraRuta.size() - 1));
        else
            texto(px, py, "Saltos: " + to_string((int)dijkstraRuta.size() - 1));
        py += 16;
    }
    else if (dijkstraOrigen >= 0 && py < AH - 40)
    {
        color_rgb(70, 70, 90);
        texto(px, py, "Elige destino en el canvas");
        py += 16;
    }
    else if (py < AH - 30)
    {
        color_rgb(60, 60, 80);
        texto(px, py, "[?] Dijkstra | [>>] Trama");
    }

    // Leyenda
    int ly = AH - 52;
    color_rgb(60, 90, 160);
    texto(px, ly, "Leyenda:");
    ly += 14;
    color_rgb(60, 200, 60);
    rectangulo_lleno(px, ly + 3, px + 12, ly + 10);
    color_rgb(130, 130, 130);
    texto(px + 15, ly, "100M");
    color_rgb(220, 200, 50);
    rectangulo_lleno(px + 52, ly + 3, px + 64, ly + 10);
    texto(px + 67, ly, "10M");
    color_rgb(220, 70, 70);
    rectangulo_lleno(px + 98, ly + 3, px + 110, ly + 10);
    texto(px + 113, ly, "1M");
    color_rgb(0, 230, 230);
    rectangulo_lleno(px + 140, ly + 3, px + 152, ly + 10);
    texto(px + 155, ly, "Ruta optima");
}

#endif