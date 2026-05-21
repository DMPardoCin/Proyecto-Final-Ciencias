#include "grafo.h"
#include "elementoVisual.h"
#include "miniwin.h"
#include "elementos.h"
#include "pc.h"
#include "router.h"
#include "switch.h"
#include <windows.h>
using namespace miniwin;

// ============================================================
//  LAYOUT ADAPTATIVO
// ============================================================
int AW, AH;
int PANEL_W, RED_W;
int ESCALA_NODO, TAM_NODO;
int ESCALA_SW, TAM_SW_H, TAM_SW_W;
int PANEL_BAR_W;

void calcularLayout()
{
    AW = GetSystemMetrics(SM_CXSCREEN);
    AH = GetSystemMetrics(SM_CYSCREEN);
    PANEL_W = AW * 26 / 100;
    RED_W = AW - PANEL_W;
    ESCALA_NODO = max(2, AH / 225);
    TAM_NODO = ESCALA_NODO * 40;
    ESCALA_SW = max(2, AH / 300);
    TAM_SW_H = ESCALA_SW * 19;
    TAM_SW_W = ESCALA_SW * 60;
    PANEL_BAR_W = PANEL_W - 20;
}

// ============================================================
//  MODOS
// ============================================================
enum Modo
{
    MODO_NORMAL,
    MODO_CREAR_PC,
    MODO_CREAR_ROUTER,
    MODO_CREAR_SWITCH,
    MODO_ENLAZAR,      // pide nodo A, nodo B, luego peso
    MODO_ENLAZAR_B,    // ya tiene nodo A, esperando B
    MODO_ENLAZAR_PESO, // tiene A y B, mostrando input de peso
    MODO_ELIMINAR_ENLACE,
    MODO_ELIMINAR_ENLACE_B,
    MODO_ELIMINAR_NODO,
    MODO_VER_DIJKSTRA,   // click en origen
    MODO_DIJKSTRA_DEST,  // tiene origen, click en destino
    MODO_ENVIAR_MANUAL,  // click en origen de trama
    MODO_ENVIAR_MANUAL_B // tiene origen, click en destino
};

// ============================================================
//  TRAMA VISUAL  (con metricas de rendimiento)
// ============================================================
struct TramaVis
{
    int origen, destino;
    vector<int> ruta;
    int pos;
    bool entregada;
    int ticksVida;    // ticks dentro del salto actual
    int ticksTotales; // ticks desde creacion
    float vx, vy;
    int saltos;   // cuantos saltos dio
    int pesoRuta; // costo total de la ruta (suma de pesos)
    // Cola: simula espera en enlace ocupado
    int ticksEspera; // ticks acumulados esperando en colas
    bool enEspera;
};

// ============================================================
//  METRICAS GLOBALES
// ============================================================
struct Metricas
{
    int totalEntregadas = 0;
    int totalDescartadas = 0;
    double sumaTiempos = 0; // suma de ticksTotales de tramas entregadas
    double sumaSaltos = 0;
    double sumaEspera = 0;
    int maxCola = 0; // ocupacion maxima de cola observada

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
} metricas;

// ============================================================
//  ESTADO GLOBAL
// ============================================================
Grafo grafo;
Modo modoActual = MODO_NORMAL;
bool arrastrando = false;
int indiceArrastrado = -1;
int offsetX = 0, offsetY = 0;

int nodoEnlaceA = -1;     // enlazar: primer nodo
int nodoEnlaceB = -1;     // enlazar: segundo nodo
int pesoEnlaceInput = 10; // peso mientras se edita (Mbps)

int elimEnlaceA = -1;
int nodoSeleccionado = -1;

// Dijkstra
int dijkstraOrigen = -1;
int dijkstraDestino = -1;
vector<int> dijkstraRuta;
ResultadoDijkstra dijkstraRes;

// Envio manual de tramas
int envioOrigen = -1;
int envioDestino = -1;

// Simulacion
vector<TramaVis> tramas;
int tickActual = 0;
bool simPausada = true;

// Colas de enlace: mapa (min(a,b), max(a,b)) -> ocupacion actual
map<pair<int, int>, int> colaEnlace;

// ============================================================
//  BOTON
// ============================================================
void btnInit(Boton &b, int x, int y, int w, int h, const char *t)
{
    b.x = x;
    b.y = y;
    b.ancho = w;
    b.alto = h;
    b.texto = t;
    b.clic_procesado = false;
}
void btnDibujar(const Boton &b, bool activo = false)
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
bool btnClick(Boton &b)
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
//  BOTONES
// ============================================================
Boton btnCrearPC, btnCrearRouter, btnCrearSwitch;
Boton btnEnlazar, btnElimEnlace, btnElimNodo;
Boton btnDijkstra;
Boton btnSimular, btnPausar, btnReset;
Boton btnEnviarManual, btnEnviarAuto;
// Popup de peso: botones posicionados dinamicamente en el canvas
Boton popPesoMenos, popPesoMas, popPesoD10, popPesoX10, popPesoOK;

void inicializarBotones()
{
    int px = RED_W + 8, bw = PANEL_W - 16;
    int bh = max(26, AH / 33), gap = bh + 5;
    int y = 8;

    btnInit(btnCrearPC, px, y, bw, bh, "[+] Agregar PC");
    y += gap;
    btnInit(btnCrearRouter, px, y, bw, bh, "[+] Agregar Router");
    y += gap;
    btnInit(btnCrearSwitch, px, y, bw, bh, "[+] Agregar Switch");
    y += gap + 4;

    btnInit(btnEnlazar, px, y, bw, bh, "[~] Enlazar (con peso)");
    y += gap;
    btnInit(btnElimEnlace, px, y, bw, bh, "[-] Eliminar enlace");
    y += gap;
    btnInit(btnElimNodo, px, y, bw, bh, "[x] Eliminar nodo");
    y += gap + 4;

    btnInit(btnDijkstra, px, y, bw, bh, "[?] Ver Dijkstra");
    y += gap + 4;

    // Envio de tramas
    int hw = bw / 2 - 2;
    btnInit(btnEnviarManual, px, y, hw, bh, "[>>] Manual");
    btnInit(btnEnviarAuto, px + hw + 4, y, hw, bh, "[>>] Auto");
    y += gap;

    btnInit(btnSimular, px, y, hw, bh, "[>] Simular");
    btnInit(btnPausar, px + hw + 4, y, hw, bh, "[||] Pausar");
    y += gap;
    btnInit(btnReset, px, y, bw, bh, "[R] Reset sim.");

    // popup de peso: botones se inicializan dinamicamente en dibujarPopupPeso()
}

// ============================================================
//  HELPERS GEOMETRICOS
// ============================================================
int ancho_elem(int idx) { return elementos[idx].tipo == SWITCH ? TAM_SW_W : TAM_NODO; }
int alto_elem(int idx)
{
    if (elementos[idx].tipo == SWITCH)
        return TAM_SW_H;
    if (elementos[idx].tipo == ROUTER)
        return ESCALA_NODO * 23;
    return TAM_NODO; // PC: 40 filas
}
int centroX(int idx) { return elementos[idx].x + ancho_elem(idx) / 2; }
int centroY(int idx) { return elementos[idx].y + alto_elem(idx) / 2; }

int elementoBajoRaton(float mx, float my)
{
    for (int i = (int)elementos.size() - 1; i >= 0; i--)
    {
        int x = elementos[i].x, y = elementos[i].y;
        if (mx >= x && mx <= x + ancho_elem(i) && my >= y && my <= y + alto_elem(i))
            return i;
    }
    return -1;
}

pair<int, int> claveEnlace(int a, int b) { return {min(a, b), max(a, b)}; }

// ============================================================
//  SPRITES
// ============================================================
void dibujarSprite40(int x, int y, int esc, pixelRGB img[][40],
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
void dibujarRouter(int x, int y, bool res = false)
{
    int e = ESCALA_NODO;
    for (int i = 0; i < 23; i++)
        for (int j = 0; j < 40; j++)
        {
            int r = router[i][j].R, g = router[i][j].G, b = router[i][j].B;
            if (res)
            {
                g = min(255, g + 80);
            }
            color_rgb(r, g, b);
            rectangulo_lleno(x + j * e, y + i * e, x + j * e + e - 1, y + i * e + e - 1);
        }
}
void dibujarSwitch(int x, int y, bool res = false)
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
//  ENLACES VISUALES
// ============================================================
struct EnlaceVis
{
    int a, b;
};
vector<EnlaceVis> enlacesVis;

bool enlaceExiste(int a, int b)
{
    for (auto &e : enlacesVis)
        if ((e.a == a && e.b == b) || (e.a == b && e.b == a))
            return true;
    return false;
}

bool esEnlaceEnRuta(int a, int b)
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

// Dibuja marcadores visuales de origen y destino Dijkstra
// Se llama DESPUES de dibujarEnlaces para quedar encima
void dibujarMarcadoresDijkstra()
{
    if (dijkstraOrigen < 0 || dijkstraOrigen >= (int)elementos.size())
        return;

    // Pulso animado en el origen (circulo que parpadea con el tick)
    int ox = centroX(dijkstraOrigen), oy = centroY(dijkstraOrigen);
    int radio = 14 + (tickActual % 20 < 10 ? tickActual % 10 : 10 - tickActual % 10);
    color_rgb(0, 180, 180);
    circulo(ox, oy, radio);
    color_rgb(0, 230, 230);
    circulo(ox, oy, radio - 2);
    // Etiqueta origen
    color_rgb(0, 230, 230);
    texto(ox - 16, oy - radio - 16, "ORIGEN");

    // Destino si esta seleccionado
    if (dijkstraDestino >= 0 && dijkstraDestino < (int)elementos.size() && dijkstraDestino != dijkstraOrigen)
    {
        int dx = centroX(dijkstraDestino), dy = centroY(dijkstraDestino);
        color_rgb(200, 200, 0);
        circulo(dx, dy, radio);
        circulo(dx, dy, radio - 2);
        color_rgb(255, 255, 80);
        texto(dx - 16, dy - radio - 16, "DESTINO");

        // Costo total de la ruta sobre el punto medio
        if (!dijkstraRuta.empty() && dijkstraRes.distancia.size() > (size_t)dijkstraDestino)
        {
            int cTotal = dijkstraRes.distancia[dijkstraDestino];
            if (cTotal != INF)
            {
                int lx = (ox + dx) / 2, ly = (oy + dy) / 2 - 30;
                color_rgb(20, 20, 30);
                rectangulo_lleno(lx - 2, ly - 2, lx + 90, ly + 14);
                color_rgb(0, 230, 230);
                texto(lx, ly, "Costo total: " + to_string(cTotal));
            }
        }
    }
}

// Dibuja una linea con grosor N repitiendo offsets
void lineaGruesa(int ax, int ay, int bx, int by, int grosor)
{
    for (int d = -(grosor / 2); d <= grosor / 2; d++)
    {
        linea(ax + d, ay, bx + d, by);
        linea(ax, ay + d, bx, by + d);
    }
}

void dibujarEnlaces()
{
    for (auto &e : enlacesVis)
    {
        if (e.a >= (int)elementos.size() || e.b >= (int)elementos.size())
            continue;
        int ax = centroX(e.a), ay = centroY(e.a);
        int bx = centroX(e.b), by = centroY(e.b);
        bool enRuta = esEnlaceEnRuta(e.a, e.b);

        Enlace *eg = grafo.buscarEnlace(e.a, e.b);

        // Color del enlace segun costo
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

        // ── ETIQUETA DE PESO ──────────────────────────
        // Siempre visible: BW en Mbps y costo OSPF
        if (eg)
        {
            int mx2 = (ax + bx) / 2, my2 = (ay + by) / 2;
            int bwM = eg->bandwidthBps / 1000000;

            // Fondo semitransparente para la etiqueta
            color_rgb(15, 15, 25);
            rectangulo_lleno(mx2 - 2, my2 - 24, mx2 + 72, my2 + 2);

            // BW grande y visible
            if (enRuta)
                color_rgb(0, 230, 230);
            else
                color_rgb(220, 220, 80);
            texto(mx2 + 2, my2 - 22, to_string(bwM) + "Mbps");

            // Costo OSPF debajo, mas pequeno
            color_rgb(160, 160, 160);
            texto(mx2 + 2, my2 - 8, "cost:" + to_string(eg->costoOSPF));

            // Ocupacion de cola sobre el enlace
            auto ck = claveEnlace(e.a, e.b);
            int ocup = colaEnlace.count(ck) ? colaEnlace[ck] : 0;
            if (ocup > 0)
            {
                color_rgb(220, 100, 40);
                texto(mx2 + 2, my2 + 6, "cola:" + to_string(ocup));
            }
        }
    }

    // Linea guia mientras se enlaza
    if ((modoActual == MODO_ENLAZAR_B || modoActual == MODO_ENLAZAR_PESO) && nodoEnlaceA >= 0 && nodoEnlaceA < (int)elementos.size())
    {
        color_rgb(80, 150, 255);
        linea(centroX(nodoEnlaceA), centroY(nodoEnlaceA),
              (int)raton_x(), (int)raton_y());
    }
    // Linea guia envio manual
    if (modoActual == MODO_ENVIAR_MANUAL_B && envioOrigen >= 0 && envioOrigen < (int)elementos.size())
    {
        color_rgb(255, 220, 40);
        linea(centroX(envioOrigen), centroY(envioOrigen),
              (int)raton_x(), (int)raton_y());
    }
}

// ============================================================
//  DIBUJAR NODOS
// ============================================================
void dibujarElementos()
{
    for (int i = 0; i < (int)elementos.size() && i < grafo.numNodos; i++)
    {
        bool res = (i == nodoSeleccionado) ||
                   (i == nodoEnlaceA) ||
                   (i == elimEnlaceA) ||
                   (i == dijkstraOrigen) ||
                   (i == dijkstraDestino) ||
                   (i == envioOrigen);

        int ex = elementos[i].x, ey = elementos[i].y;

        if (elementos[i].tipo == PC)
            dibujarSprite40(ex, ey, ESCALA_NODO, pc, res, 60, 60, 0);
        else if (elementos[i].tipo == ROUTER)
            dibujarRouter(ex, ey, res);
        else
            dibujarSwitch(ex, ey, res);

        // Nombre
        color(BLANCO);
        texto(ex + 4, max(2, ey - 16), grafo.nodos[i].nombre);

        // Indice
        color_rgb(130, 130, 130);
        texto(ex + ancho_elem(i) - 22, ey + 4, "[" + to_string(i) + "]");

        // Barra de saturacion
        if (i < grafo.numNodos)
        {
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
        }

        // Marca eliminacion
        if (modoActual == MODO_ELIMINAR_NODO && i == nodoSeleccionado)
        {
            color_rgb(220, 40, 40);
            rectangulo(ex - 2, ey - 2, ex + ancho_elem(i) + 2, ey + alto_elem(i) + 2);
        }

        // Marca origen Dijkstra
        if (i == dijkstraOrigen)
        {
            color_rgb(0, 200, 200);
            rectangulo(ex - 3, ey - 3, ex + ancho_elem(i) + 3, ey + alto_elem(i) + 3);
        }
        // Marca destino Dijkstra
        if (i == dijkstraDestino && dijkstraDestino != dijkstraOrigen)
        {
            color_rgb(200, 200, 0);
            rectangulo(ex - 3, ey - 3, ex + ancho_elem(i) + 3, ey + alto_elem(i) + 3);
        }
        // Marca origen trama manual
        if (i == envioOrigen)
        {
            color_rgb(255, 200, 0);
            rectangulo(ex - 3, ey - 3, ex + ancho_elem(i) + 3, ey + alto_elem(i) + 3);
        }
    }
}

// ============================================================
//  TRAMAS ANIMADAS  (con colas de enlace)
// ============================================================
// ── BW real: cuantas tramas puede drenar el enlace por tick ──
// 100 Mbps -> 5 tramas/tick  |  10 Mbps -> 1 trama/tick  |  1 Mbps -> 1 trama cada 5 ticks
int capacidadPorTick(int a, int b)
{
    Enlace *e = grafo.buscarEnlace(a, b);
    if (!e)
        return 1;
    int mbps = e->bandwidthBps / 1000000;
    if (mbps >= 100)
        return 5;
    if (mbps >= 50)
        return 3;
    if (mbps >= 10)
        return 1;
    return 1; // 1 Mbps: 1 trama pero tarda mas ticks
}

// Ticks necesarios para cruzar un enlace segun BW real
// 100 Mbps=8 ticks | 10 Mbps=20 ticks | 1 Mbps=80 ticks
int ticksPorSalto(int a, int b)
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
    return 80; // 1 Mbps: muy lento
}

// Cola maxima proporcional al BW: mas BW -> mas buffer
int colaMaxEnlace(int a, int b)
{
    Enlace *e = grafo.buscarEnlace(a, b);
    if (!e)
        return MAX_QUEUE;
    int mbps = e->bandwidthBps / 1000000;
    if (mbps >= 100)
        return 20;
    if (mbps >= 10)
        return 10;
    return 4; // 1 Mbps: buffer muy pequeno
}

void actualizarTramas()
{
    // Limpiar contadores de cola
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

        // Cola maxima depende del BW real del enlace
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
        float prog = (float)t.ticksVida / (float)tpS;
        if (prog > 1.f)
            prog = 1.f;
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

void dibujarTramas()
{
    for (auto &t : tramas)
    {
        if (t.entregada)
            continue;
        // Color diferente si esta en espera
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
        // Etiqueta O->D
        color_rgb(200, 200, 200);
        texto((int)t.vx + 10, (int)t.vy - 10,
              grafo.nodos[t.origen].nombre + "->" + grafo.nodos[t.destino].nombre);
    }
}

// ============================================================
//  SEPARADOR Y PANEL
// ============================================================
void dibujarSeparador()
{
    color_rgb(18, 18, 30);
    rectangulo_lleno(RED_W, 0, AW, AH);
    color_rgb(55, 95, 195);
    linea(RED_W, 0, RED_W, AH);
}

void barraH(int px, int &py, float val,
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

// ──────── Subpanel: indicador de modo ────────────────────────
void dibujarModo()
{
    int px = RED_W + 8;
    int py = btnReset.y + btnReset.alto + 10;
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
        texto(px + 4, py + 4, "Ajusta peso y confirma [ENTER]");
        break;
    case MODO_ELIMINAR_ENLACE:
        texto(px + 4, py + 4, "Elim.enlace: nodo A");
        break;
    case MODO_ELIMINAR_ENLACE_B:
        texto(px + 4, py + 4, "Elim.enlace: nodo B");
        break;
    case MODO_ELIMINAR_NODO:
        texto(px + 4, py + 4, "Elim.nodo: clic en nodo");
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

// ──────── Subpanel: input de peso de enlace ──────────────────
void dibujarPopupPeso()
{
    if (modoActual != MODO_ENLAZAR_PESO)
        return;
    if (nodoEnlaceA < 0 || nodoEnlaceB < 0)
        return;
    if (nodoEnlaceA >= (int)elementos.size() || nodoEnlaceB >= (int)elementos.size())
        return;

    // Punto medio entre los dos nodos
    int mx2 = (centroX(nodoEnlaceA) + centroX(nodoEnlaceB)) / 2;
    int my2 = (centroY(nodoEnlaceA) + centroY(nodoEnlaceB)) / 2;

    // Dimensiones del popup
    const int PW = 220, PH = 130;
    int px = mx2 - PW / 2;
    int py = my2 - PH / 2;

    // Mantener dentro del canvas
    px = max(4, min(px, RED_W - PW - 4));
    py = max(4, min(py, AH - PH - 4));

    // Fondo con sombra
    color_rgb(8, 8, 18);
    rectangulo_lleno(px + 3, py + 3, px + PW + 3, py + PH + 3);

    // Cuerpo
    color_rgb(28, 28, 48);
    rectangulo_lleno(px, py, px + PW, py + PH);
    color_rgb(80, 140, 255);
    rectangulo(px, py, px + PW, py + PH);

    // Linea de titulo
    color_rgb(50, 50, 80);
    rectangulo_lleno(px + 1, py + 1, px + PW - 1, py + 22);
    color_rgb(255, 220, 60);
    texto(px + 8, py + 6, "Peso del enlace");

    // Valor actual grande y centrado
    int bw2 = max(1, pesoEnlaceInput) * 1000000;
    int costo = max(1, REF_BW / bw2);
    string valStr = to_string(pesoEnlaceInput) + " Mbps";
    color(BLANCO);
    texto(px + PW / 2 - (int)valStr.size() * 4, py + 30, valStr);

    // Costo OSPF
    color_rgb(120, 200, 120);
    texto(px + 8, py + 48, "Costo OSPF: " + to_string(costo));

    // Botones -10 / +10 / /10 / x10
    int bw = 44, bh = 22, gap = 48;
    int by = py + 70;
    int bx = px + 6;
    btnInit(popPesoMenos, bx, by, bw, bh, "-10");
    btnInit(popPesoMas, bx + gap, by, bw, bh, "+10");
    btnInit(popPesoD10, bx + gap * 2, by, bw, bh, "/10");
    btnInit(popPesoX10, bx + gap * 3, by, bw, bh, "x10");

    btnDibujar(popPesoMenos);
    btnDibujar(popPesoMas);
    btnDibujar(popPesoD10);
    btnDibujar(popPesoX10);

    // Boton OK
    btnInit(popPesoOK, px + 8, py + PH - 28, PW - 16, 22, "[ENTER] Confirmar");
    btnDibujar(popPesoOK, true);

    // Hint ESC
    color_rgb(100, 100, 100);
    texto(px + 8, py + PH + 6, "ESC para cancelar");
}

// ──────── Panel principal de datos ───────────────────────────
void dibujarPanelDatos()
{
    int px = RED_W + 8;
    int py = btnReset.y + btnReset.alto + 38;

    // ── Simulacion ──
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
    texto(px, py, "Nodos:   " + to_string(grafo.numNodos) + "  Enl:" + to_string((int)enlacesVis.size()));
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

    // ── Metricas ──
    color_rgb(70, 140, 255);
    texto(px, py, "-- METRICAS --");
    py += 19;
    color_rgb(160, 160, 160);
    texto(px, py, "Entregadas: " + to_string(metricas.totalEntregadas));
    py += 16;

    // Tiempo promedio de entrega
    double tp = metricas.tiempoPromedio();
    texto(px, py, "T.entrega prom: " + to_string((int)tp) + " ticks");
    py += 16;

    // Saltos promedio
    double sp = metricas.saltosPromedio();
    // Barra de saltos (max 10)
    texto(px, py, "Saltos prom:  " + to_string((int)(sp * 10) / 10.0f).substr(0, 4));
    py += 14;
    barraH(px, py, min(1.f, (float)(sp / 8.f)), 60, 200, 60, 220, 50, 50);

    // Espera en cola promedio
    double ep = metricas.esperaPromedio();
    texto(px, py, "Espera cola prom: " + to_string((int)ep));
    py += 14;
    barraH(px, py, min(1.f, (float)(ep / 20.f)), 60, 150, 220, 220, 50, 50);

    // Max ocupacion de cola
    color_rgb(160, 160, 160);
    texto(px, py, "Max cola obs: " + to_string(metricas.maxCola) + "/" + to_string(MAX_QUEUE));
    py += 14;
    barraH(px, py, (float)metricas.maxCola / MAX_QUEUE, 60, 200, 60, 220, 50, 50);
    py += 6;

    // ── Nodo seleccionado ──
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

    // ── Dijkstra resultado ──
    if (dijkstraOrigen >= 0 && !dijkstraRes.distancia.empty())
    {
        if (py > AH - 120)
            return; // no hay espacio
        color_rgb(0, 200, 200);
        if (dijkstraOrigen < grafo.numNodos)
            texto(px, py, "-- DIJKSTRA desde " + grafo.nodos[dijkstraOrigen].nombre + " --");
        py += 18;

        for (int i = 0; i < (int)dijkstraRes.distancia.size() && i < grafo.numNodos && py < AH - 18; i++)
        {
            if (i == dijkstraOrigen)
                continue;
            bool esDest = (i == dijkstraDestino);
            color_rgb(esDest ? 0 : 110, esDest ? 220 : 110, esDest ? 220 : 110);

            string dist = dijkstraRes.distancia[i] == INF ? "INF" : to_string(dijkstraRes.distancia[i]);

            // Calcular BW efectivo de la ruta (cuello de botella)
            vector<int> r = grafo.reconstruirRuta(dijkstraRes, i);
            int minBW = 999999;
            for (int k = 0; k + 1 < (int)r.size(); k++)
            {
                Enlace *e = grafo.buscarEnlace(r[k], r[k + 1]);
                if (e)
                    minBW = min(minBW, e->bandwidthBps / 1000000);
            }
            string bwStr = (minBW == 999999) ? "" : (" BW>=" + to_string(minBW) + "M");

            // Ruta
            string rutaStr = "";
            for (int k = 0; k < (int)r.size(); k++)
            {
                if (k)
                    rutaStr += "->";
                rutaStr += grafo.nodos[r[k]].nombre;
            }
            if (rutaStr.empty())
                rutaStr = "SIN RUTA";

            texto(px, py, grafo.nodos[i].nombre + " C:" + dist + bwStr);
            py += 14;
            color_rgb(esDest ? 0 : 80, esDest ? 180 : 80, esDest ? 180 : 80);
            texto(px + 6, py, " " + rutaStr);
            py += 14;
        }
    }
    else if (py < AH - 30)
    {
        color_rgb(60, 60, 80);
        texto(px, py, "[?] Dijkstra | [>>] Enviar trama");
    }

    // ── Leyenda ──
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
    texto(px + 155, ly, "Ruta");
}

// ============================================================
//  CREACION Y ELIMINACION
// ============================================================
void crearNodo(int x, int y, TipoElemento tipo)
{
    string p = (tipo == PC) ? "PC" : (tipo == ROUTER) ? "R"
                                                      : "SW";
    grafo.agregarNodo(p + to_string(grafo.numNodos));
    ElementoVisual ev;
    ev.tipo = tipo;
    int hw = (tipo == SWITCH) ? TAM_SW_W / 2 : (tipo == ROUTER ? ESCALA_NODO * 23 / 2 : TAM_NODO / 2); // sin acceso fuera de rango
    hw = (tipo == SWITCH) ? TAM_SW_W / 2 : TAM_NODO / 2;
    int hh = (tipo == SWITCH) ? TAM_SW_H / 2 : TAM_NODO / 2;
    ev.x = max(0, min(x - hw, RED_W - (tipo == SWITCH ? TAM_SW_W : TAM_NODO)));
    ev.y = max(0, min(y - hh, AH - (tipo == SWITCH ? TAM_SW_H : TAM_NODO)));
    elementos.push_back(ev);
}

void confirmarEnlace()
{
    // pesoEnlaceInput en Mbps -> convertir a bps
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

void eliminarEnlace(int a, int b)
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

void eliminarNodo(int idx)
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
//  ENVIO DE TRAMAS
// ============================================================
void enviarTrama(int orig, int dest)
{
    if (orig < 0 || dest < 0 || orig >= grafo.numNodos || dest >= grafo.numNodos)
        return;
    if (orig == dest)
        return;
    ResultadoDijkstra res = grafo.dijkstra(orig);
    vector<int> ruta = grafo.reconstruirRuta(res, dest);
    if ((int)ruta.size() < 2)
        return;

    // Calcular peso total de la ruta
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

void enviarTramaAuto()
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
//  ARRASTRE
// ============================================================
void manejarArrastre()
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

void manejarClicRed()
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
        // No limpiar dijkstraRuta al clicar en vacio: la ruta permanece visible
        break;
    default:
        break;
    }
}

// ============================================================
//  INPUT DE TECLADO (peso y teclas generales)
// ============================================================
void manejarTeclado()
{
    int t = tecla();
    if (t == ESCAPE)
    {
        modoActual = MODO_NORMAL;
        nodoEnlaceA = nodoEnlaceB = elimEnlaceA = envioOrigen = envioDestino = -1;
        return;
    }
    if (modoActual == MODO_ENLAZAR_PESO)
    {
        if (t == RETURN)
        {
            confirmarEnlace();
            return;
        }
    }
}

// ============================================================
//  TICK SIMULACION
// ============================================================
void tickSimulacion()
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

// ============================================================
//  MAIN
// ============================================================
void inicializarVentana()
{
    srand((unsigned)time(nullptr));
    calcularLayout();
    vredimensiona(AW, AH);
    inicializarBotones();

    while (true)
    {
        color_rgb(14, 14, 24);
        rectangulo_lleno(0, 0, AW, AH);

        dibujarSeparador();
        dibujarEnlaces();
        dibujarMarcadoresDijkstra(); // encima de enlaces, debajo de nodos
        dibujarElementos();
        dibujarTramas();

        // Panel
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

        dibujarModo();
        dibujarPopupPeso();
        dibujarPanelDatos();

        // Acciones botones
        if (btnClick(btnCrearPC))
        {
            modoActual = MODO_CREAR_PC;
            nodoEnlaceA = -1;
            elimEnlaceA = -1;
        }
        if (btnClick(btnCrearRouter))
        {
            modoActual = MODO_CREAR_ROUTER;
            nodoEnlaceA = -1;
            elimEnlaceA = -1;
        }
        if (btnClick(btnCrearSwitch))
        {
            modoActual = MODO_CREAR_SWITCH;
            nodoEnlaceA = -1;
            elimEnlaceA = -1;
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

        // Botones de ajuste de peso
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

        manejarClicRed();
        manejarArrastre();
        manejarTeclado();

        for (int i = 0; i < 3; i++)
            tickSimulacion();

        refresca();
        espera(16);
    }
    vcierra();
}