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
int AW, AH;          // ancho y alto reales de la ventana
int PANEL_W;         // ancho panel derecho
int RED_W;           // ancho area de red
int ESCALA_NODO;     // escala sprite 40x40
int TAM_NODO;        // pixeles del nodo (ESCALA_NODO * 40)
int ESCALA_SW;       // escala sprite switch 19x60
int TAM_SW_H;        // alto switch en pantalla
int TAM_SW_W;        // ancho switch en pantalla
int PANEL_BAR_W;     // ancho de las barras en el panel

void calcularLayout() {
    AW       = GetSystemMetrics(SM_CXSCREEN);
    AH       = GetSystemMetrics(SM_CYSCREEN);
    PANEL_W  = AW * 23 / 100;          // 23% del ancho
    RED_W    = AW - PANEL_W;
    ESCALA_NODO = max(2, AH / 225);    // ~4 en 900p, ~5 en 1080p
    TAM_NODO    = ESCALA_NODO * 40;
    ESCALA_SW   = max(2, AH / 300);
    TAM_SW_H    = ESCALA_SW * 19;
    TAM_SW_W    = ESCALA_SW * 60;
    PANEL_BAR_W = PANEL_W - 30;
}

// ============================================================
//  MODOS
// ============================================================
enum Modo {
    MODO_NORMAL,
    MODO_CREAR_PC,
    MODO_CREAR_ROUTER,
    MODO_CREAR_SWITCH,
    MODO_ENLAZAR,
    MODO_ELIMINAR_ENLACE,
    MODO_ELIMINAR_NODO,
    MODO_VER_DIJKSTRA
};

// ============================================================
//  TRAMA VISUAL
// ============================================================
struct TramaVis {
    int origen, destino;
    vector<int> ruta;
    int pos;
    bool entregada;
    int ticksVida;
    float vx, vy;
};

// ============================================================
//  ESTADO GLOBAL
// ============================================================
Grafo  grafo;
Modo   modoActual       = MODO_NORMAL;
bool   arrastrando      = false;
int    indiceArrastrado = -1;
int    offsetX = 0, offsetY = 0;
int    nodoEnlaceOrigen = -1;

vector<TramaVis> tramas;
int    tickActual      = 0;
int    totalEntregadas = 0;
bool   simPausada      = true;
int    nodoSeleccionado = -1;

// Dijkstra visual
int    dijkstraOrigen  = -1;
vector<int> dijkstraRuta;        // ruta hacia nodoSeleccionado
ResultadoDijkstra dijkstraRes;   // tabla completa

// Eliminar enlace: primer nodo elegido
int    elimEnlaceA = -1;

// ============================================================
//  BOTON
// ============================================================
void btnInit(Boton& b, int x, int y, int w, int h, const char* t) {
    b.x=x; b.y=y; b.ancho=w; b.alto=h;
    b.texto=t; b.clic_procesado=false;
}

void btnDibujar(const Boton& b, bool activo=false) {
    if      (activo)           color_rgb(50,110,190);
    else if (b.clic_procesado) color_rgb(90,90,90);
    else                       color_rgb(35,35,50);
    rectangulo_lleno(b.x,b.y,b.x+b.ancho,b.y+b.alto);
    color_rgb(activo?120:80, activo?180:130, 240);
    rectangulo(b.x,b.y,b.x+b.ancho,b.y+b.alto);
    color(BLANCO);
    texto(b.x+8, b.y+b.alto/2-7, b.texto);
}

bool btnClick(Boton& b) {
    float mx=raton_x(), my=raton_y();
    bool enc=mx>=b.x&&mx<=b.x+b.ancho&&my>=b.y&&my<=b.y+b.alto;
    if (raton_boton_izq()) {
        if (!b.clic_procesado&&enc){ b.clic_procesado=true; return true; }
    } else { b.clic_procesado=false; }
    return false;
}

// ============================================================
//  BOTONES
// ============================================================
// Creacion
Boton btnCrearPC, btnCrearRouter, btnCrearSwitch;
// Enlace/eliminar
Boton btnEnlazar, btnElimEnlace, btnElimNodo;
// Simulacion
Boton btnSimular, btnPausar, btnReset, btnEnviarTrama;
// Dijkstra
Boton btnDijkstra;

void inicializarBotones() {
    int px = RED_W + 8;
    int bw = PANEL_W - 16;
    int bh = max(28, AH/30);
    int gap = bh + 6;
    int y  = 10;

    btnInit(btnCrearPC,     px, y,       bw, bh, "[+] Agregar PC");      y+=gap;
    btnInit(btnCrearRouter, px, y,       bw, bh, "[+] Agregar Router");  y+=gap;
    btnInit(btnCrearSwitch, px, y,       bw, bh, "[+] Agregar Switch");  y+=gap+6;

    btnInit(btnEnlazar,     px, y,       bw, bh, "[~] Enlazar nodos");   y+=gap;
    btnInit(btnElimEnlace,  px, y,       bw, bh, "[-] Eliminar enlace"); y+=gap;
    btnInit(btnElimNodo,    px, y,       bw, bh, "[x] Eliminar nodo");   y+=gap+6;

    btnInit(btnDijkstra,    px, y,       bw, bh, "[?] Ver Dijkstra");    y+=gap+6;

    int hw = bw/2 - 2;
    btnInit(btnSimular,     px,    y,    hw, bh, "[>] Simular");
    btnInit(btnPausar,      px+hw+4, y,  hw, bh, "[||] Pausar");         y+=gap;
    btnInit(btnReset,       px,    y,    bw, bh, "[R] Reiniciar sim.");  y+=gap;
    btnInit(btnEnviarTrama, px,    y,    bw, bh, "[>>] Enviar trama");
}

// ============================================================
//  SPRITES
// ============================================================
void dibujarSprite40(int x,int y,int escala, pixelRGB img[][40],
                     bool resaltado, int tR,int tG,int tB) {
    for(int i=0;i<40;i++) for(int j=0;j<40;j++){
        int r=img[i][j].R, g=img[i][j].G, b=img[i][j].B;
        if(resaltado){r=min(255,r+tR);g=min(255,g+tG);b=min(255,b+tB);}
        color_rgb(r,g,b);
        rectangulo_lleno(x+j*escala,y+i*escala,
                         x+j*escala+escala-1,y+i*escala+escala-1);
    }
}

void dibujarSwitch(int x,int y,bool resaltado=false) {
    int e=ESCALA_SW;
    for(int i=0;i<19;i++) for(int j=0;j<60;j++){
        int r=switche[i][j].R,g=switche[i][j].G,b=switche[i][j].B;
        if(resaltado){r=min(255,r+40);b=min(255,b+80);}
        color_rgb(r,g,b);
        rectangulo_lleno(x+j*e,y+i*e,x+j*e+e-1,y+i*e+e-1);
    }
}

// Devuelve el centro X del elemento i
int centroX(int idx) {
    if(elementos[idx].tipo==SWITCH)
        return elementos[idx].x + TAM_SW_W/2;
    return elementos[idx].x + TAM_NODO/2;
}
int centroY(int idx) {
    if(elementos[idx].tipo==SWITCH)
        return elementos[idx].y + TAM_SW_H/2;
    return elementos[idx].y + TAM_NODO/2;
}

// ============================================================
//  HITBOX BAJO EL RATON
// ============================================================
int elementoBajoRaton(float mx,float my) {
    for(int i=(int)elementos.size()-1;i>=0;i--){
        int x=elementos[i].x, y=elementos[i].y;
        int w=(elementos[i].tipo==SWITCH)?TAM_SW_W:TAM_NODO;
        int h=(elementos[i].tipo==SWITCH)?TAM_SW_H:TAM_NODO;
        if(mx>=x&&mx<=x+w&&my>=y&&my<=y+h) return i;
    }
    return -1;
}

// ============================================================
//  ENLACES VISUALES
// ============================================================
struct EnlaceVis { int a,b; };
vector<EnlaceVis> enlacesVis;

bool enlaceExiste(int a,int b){
    for(auto& e:enlacesVis)
        if((e.a==a&&e.b==b)||(e.a==b&&e.b==a)) return true;
    return false;
}

// Resalta los enlaces que forman la ruta Dijkstra activa
bool esEnlaceEnRuta(int a,int b){
    if(dijkstraRuta.size()<2) return false;
    for(int i=0;i<(int)dijkstraRuta.size()-1;i++){
        int u=dijkstraRuta[i], v=dijkstraRuta[i+1];
        if((u==a&&v==b)||(u==b&&v==a)) return true;
    }
    return false;
}

void dibujarEnlaces(){
    for(auto& e:enlacesVis){
        if(e.a>=(int)elementos.size()||e.b>=(int)elementos.size()) continue;
        int ax=centroX(e.a),ay=centroY(e.a);
        int bx=centroX(e.b),by=centroY(e.b);

        bool enRuta = esEnlaceEnRuta(e.a,e.b);
        if(enRuta){
            // Resaltar ruta Dijkstra con linea mas gruesa y cian
            color_rgb(0,230,230);
            linea(ax-1,ay,bx-1,by);
            linea(ax+1,ay,bx+1,by);
            linea(ax,ay-1,bx,by-1);
            linea(ax,ay+1,bx,by+1);
        }

        Enlace* eg=grafo.buscarEnlace(e.a,e.b);
        if(eg){
            int c=eg->costoOSPF;
            if     (c<=1)  color_rgb(60,200,60);
            else if(c<=10) color_rgb(220,200,50);
            else           color_rgb(220,70,70);
        } else color_rgb(100,100,100);
        linea(ax,ay,bx,by);

        // Etiqueta costo
        if(eg){
            color_rgb(enRuta?0:160, enRuta?230:160, enRuta?230:160);
            texto((ax+bx)/2+4,(ay+by)/2-12,
                  "C:"+to_string(eg->costoOSPF));
        }
    }
}

// ============================================================
//  DIBUJAR NODOS
// ============================================================
void dibujarElementos(){
    for(int i=0;i<(int)elementos.size();i++){
        bool res=(i==nodoSeleccionado)||
                 (modoActual==MODO_ENLAZAR&&i==nodoEnlaceOrigen)||
                 (modoActual==MODO_ELIMINAR_ENLACE&&i==elimEnlaceA)||
                 (modoActual==MODO_VER_DIJKSTRA&&i==dijkstraOrigen);

        int ex=elementos[i].x, ey=elementos[i].y;

        if(elementos[i].tipo==PC)
            dibujarSprite40(ex,ey,ESCALA_NODO,pc,res,60,60,0);
        else if(elementos[i].tipo==ROUTER)
            dibujarSprite40(ex,ey,ESCALA_NODO,router,res,0,80,0);
        else
            dibujarSwitch(ex,ey,res);

        // Nombre
        color(BLANCO);
        texto(ex+4, ey-16, grafo.nodos[i].nombre);

        // Indice
        color_rgb(140,140,140);
        int tw=(elementos[i].tipo==SWITCH)?TAM_SW_W:TAM_NODO;
        texto(ex+tw-22, ey+4, "["+to_string(i)+"]");

        // Barra de saturacion (debajo del sprite)
        if(i<grafo.numNodos){
            float sat=grafo.nodos[i].saturacion;
            int bx=ex, by=ey+((elementos[i].tipo==SWITCH)?TAM_SW_H:TAM_NODO)+4;
            int bw=(elementos[i].tipo==SWITCH)?TAM_SW_W:TAM_NODO;
            color_rgb(25,25,25);
            rectangulo_lleno(bx,by,bx+bw,by+7);
            int rell=(int)(sat*bw);
            if(sat<0.5)       color_rgb(60,200,60);
            else if(sat<0.85) color_rgb(220,180,40);
            else              color_rgb(220,50,50);
            rectangulo_lleno(bx,by,bx+rell,by+7);
            color_rgb(70,70,70);
            rectangulo(bx,by,bx+bw,by+7);
        }

        // Marca roja si va a ser eliminado
        if(modoActual==MODO_ELIMINAR_NODO&&i==nodoSeleccionado){
            color_rgb(220,40,40);
            rectangulo(ex-2,ey-2,
                       ex+(elementos[i].tipo==SWITCH?TAM_SW_W:TAM_NODO)+2,
                       ey+(elementos[i].tipo==SWITCH?TAM_SW_H:TAM_NODO)+2);
        }
    }
}

// ============================================================
//  TRAMAS ANIMADAS
// ============================================================
void actualizarTramas(){
    for(auto& t:tramas){
        if(t.entregada) continue;
        if((int)t.ruta.size()<2){t.entregada=true;continue;}
        int pA=t.ruta[t.pos];
        int pB=t.ruta[min(t.pos+1,(int)t.ruta.size()-1)];
        if(pA>=(int)elementos.size()||pB>=(int)elementos.size()){
            t.entregada=true; continue;
        }
        float ax=centroX(pA),ay=centroY(pA);
        float bx=centroX(pB),by=centroY(pB);
        t.ticksVida++;
        float prog=(t.ticksVida%30)/30.0f;
        t.vx=ax+(bx-ax)*prog;
        t.vy=ay+(by-ay)*prog;
        if(prog>=0.99f){
            t.pos++; t.ticksVida=0;
            if(t.pos>=(int)t.ruta.size()-1){
                t.entregada=true; totalEntregadas++;
            }
        }
    }
}

void dibujarTramas(){
    for(auto& t:tramas){
        if(t.entregada) continue;
        color_rgb(255,220,40);
        circulo_lleno(t.vx,t.vy,7);
        color_rgb(255,120,0);
        circulo(t.vx,t.vy,8);
    }
}

// ============================================================
//  PANEL SEPARADOR
// ============================================================
void dibujarSeparador(){
    color_rgb(20,20,32);
    rectangulo_lleno(RED_W,0,AW,AH);
    color_rgb(60,100,200);
    linea(RED_W,0,RED_W,AH);
}

// ============================================================
//  INDICADOR DE MODO (en panel)
// ============================================================
void dibujarModoActual(){
    int px=RED_W+8;
    int py=btnEnviarTrama.y+btnEnviarTrama.alto+12;
    color_rgb(30,30,48);
    rectangulo_lleno(px,py,px+PANEL_W-16,py+24);
    color_rgb(255,200,60);
    switch(modoActual){
        case MODO_NORMAL:          texto(px+4,py+4,"Modo: Normal (arrastrar)"); break;
        case MODO_CREAR_PC:        texto(px+4,py+4,"Modo: Clic -> crear PC"); break;
        case MODO_CREAR_ROUTER:    texto(px+4,py+4,"Modo: Clic -> crear Router"); break;
        case MODO_CREAR_SWITCH:    texto(px+4,py+4,"Modo: Clic -> crear Switch"); break;
        case MODO_ENLAZAR:
            if(nodoEnlaceOrigen==-1) texto(px+4,py+4,"Enlazar: elige nodo A");
            else texto(px+4,py+4,"Enlazar: elige nodo B");
            break;
        case MODO_ELIMINAR_ENLACE:
            if(elimEnlaceA==-1) texto(px+4,py+4,"Elim.enlace: elige nodo A");
            else texto(px+4,py+4,"Elim.enlace: elige nodo B");
            break;
        case MODO_ELIMINAR_NODO:   texto(px+4,py+4,"Elim.nodo: clic en nodo"); break;
        case MODO_VER_DIJKSTRA:    texto(px+4,py+4,"Dijkstra: elige origen"); break;
    }
}

// ============================================================
//  PANEL DE DATOS (derecha)
// ============================================================
void barraHorizontal(int px,int& py,float val,
                     int rL,int gL,int bL, int rH,int gH,int bH){
    color_rgb(25,25,25);
    rectangulo_lleno(px,py,px+PANEL_BAR_W,py+9);
    int rell=(int)(val*PANEL_BAR_W);
    if(val<0.5)       color_rgb(rL,gL,bL);
    else if(val<0.85) color_rgb(220,180,40);
    else              color_rgb(rH,gH,bH);
    rectangulo_lleno(px,py,px+rell,py+9);
    color_rgb(70,70,70);
    rectangulo(px,py,px+PANEL_BAR_W,py+9);
    py+=12;
}

void dibujarPanelDatos(){
    int px=RED_W+8;
    // Posicion inicio: debajo del indicador de modo
    int py=btnEnviarTrama.y+btnEnviarTrama.alto+44;

    // ---- Simulacion ----
    color_rgb(80,160,255);
    texto(px,py,"--- SIMULACION ---"); py+=20;
    color_rgb(170,170,170);
    texto(px,py,"Tick: "+to_string(tickActual)); py+=18;
    int act=0; for(auto& t:tramas) if(!t.entregada) act++;
    texto(px,py,"Tramas activas: "+to_string(act)); py+=18;
    texto(px,py,"Entregadas:     "+to_string(totalEntregadas)); py+=18;
    texto(px,py,"Nodos: "+to_string(grafo.numNodos)+
          "  Enlaces: "+to_string((int)enlacesVis.size())); py+=18;
    if(simPausada){ color_rgb(220,180,40); texto(px,py,"[PAUSADA]"); }
    else          { color_rgb(60,220,60);  texto(px,py,"[CORRIENDO]"); }
    py+=24;

    // ---- Nodo seleccionado ----
    color_rgb(80,160,255);
    texto(px,py,"--- NODO ACTIVO ---"); py+=20;

    if(nodoSeleccionado>=0&&nodoSeleccionado<grafo.numNodos){
        Nodo& n=grafo.nodos[nodoSeleccionado];
        color_rgb(255,240,80);
        texto(px,py,n.nombre); py+=18;

        color_rgb(160,160,160);
        texto(px,py,"CPU "+to_string((int)(n.cargaCPU*100))+"%"); py+=14;
        barraHorizontal(px,py,n.cargaCPU, 60,200,60, 220,50,50);

        texto(px,py,"RAM "+to_string((int)(n.cargaMemoria*100))+"%"); py+=14;
        barraHorizontal(px,py,n.cargaMemoria, 60,120,220, 220,50,50);

        texto(px,py,"SAT "+to_string((int)(n.saturacion*100))+"%"); py+=14;
        barraHorizontal(px,py,n.saturacion, 60,200,60, 220,50,50);

        if(n.estaSaturado()){ color_rgb(220,50,50); texto(px,py,"! SATURADO !"); py+=18; }

        color_rgb(140,140,140);
        texto(px,py,"RX:"+to_string(n.tramasRecibidas)+
              " TX:"+to_string(n.tramasEnviadas)); py+=18;

        // Vecinos
        color_rgb(80,160,255);
        texto(px,py,"Vecinos:"); py+=16;
        for(auto& e:n.adyacentes){
            if(e.destino<grafo.numNodos&&py<AH-60){
                color_rgb(120,120,120);
                texto(px+6,py,"->"+grafo.nodos[e.destino].nombre+
                      " C:"+to_string(e.costoOSPF)+
                      " "+to_string(e.bandwidthBps/1000000)+"M"); py+=16;
            }
        }
        py+=4;
    } else {
        color_rgb(90,90,90);
        texto(px,py,"Clic en un nodo"); py+=16;
        texto(px,py,"para ver sus datos"); py+=24;
    }

    // ---- Tabla Dijkstra ----
    if(dijkstraOrigen>=0&&!dijkstraRes.distancia.empty()){
        color_rgb(0,210,210);
        texto(px,py,"--- DIJKSTRA desde "+
              grafo.nodos[dijkstraOrigen].nombre+" ---"); py+=20;

        for(int i=0;i<grafo.numNodos&&py<AH-40;i++){
            if(i==dijkstraOrigen) continue;
            // Resaltar la fila si es el destino activo
            bool esDestino=(nodoSeleccionado==i&&modoActual!=MODO_VER_DIJKSTRA);
            if(esDestino) color_rgb(0,230,230);
            else          color_rgb(130,130,130);

            string dist=dijkstraRes.distancia[i]==INF
                        ? "INF" : to_string(dijkstraRes.distancia[i]);

            // Reconstruir ruta corta
            vector<int> r=grafo.reconstruirRuta(dijkstraRes,i);
            string rutaStr="";
            for(int k=0;k<(int)r.size();k++){
                if(k) rutaStr+="->";
                rutaStr+=grafo.nodos[r[k]].nombre;
            }
            if(rutaStr.empty()) rutaStr="SIN RUTA";

            texto(px,py, grafo.nodos[i].nombre+
                  " C:"+dist+" | "+rutaStr); py+=16;
        }
    } else if(py<AH-60){
        color_rgb(70,70,90);
        texto(px,py,"[?] para ver tabla Dijkstra");
    }

    // ---- Leyenda ----
    int ly=AH-68;
    color_rgb(70,100,180);
    texto(px,ly,"Leyenda enlaces:"); ly+=16;
    color_rgb(60,200,60);   rectangulo_lleno(px,ly+3,px+14,ly+10);
    color_rgb(150,150,150); texto(px+18,ly,"100M (C1)");
    color_rgb(220,200,50);  rectangulo_lleno(px+90,ly+3,px+104,ly+10);
    color_rgb(150,150,150); texto(px+108,ly,"10M (C10)"); ly+=18;
    color_rgb(220,70,70);   rectangulo_lleno(px,ly+3,px+14,ly+10);
    color_rgb(150,150,150); texto(px+18,ly,"1M (C100)");
    color_rgb(0,230,230);   rectangulo_lleno(px+90,ly+3,px+104,ly+10);
    color_rgb(150,150,150); texto(px+108,ly,"Ruta activa");
}

// ============================================================
//  CREACION DE NODOS
// ============================================================
void crearNodo(int x,int y,TipoElemento tipo){
    string prefijo=(tipo==PC)?"PC":(tipo==ROUTER)?"R":"SW";
    string nombre=prefijo+to_string(grafo.numNodos);
    grafo.agregarNodo(nombre);
    ElementoVisual ev;
    ev.tipo=tipo;
    int hw=(tipo==SWITCH)?TAM_SW_W/2:TAM_NODO/2;
    int hh=(tipo==SWITCH)?TAM_SW_H/2:TAM_NODO/2;
    ev.x=x-hw; ev.y=y-hh;
    if(ev.x<0) ev.x=0;
    if(ev.y<0) ev.y=0;
    int maxW=(tipo==SWITCH)?TAM_SW_W:TAM_NODO;
    int maxH=(tipo==SWITCH)?TAM_SW_H:TAM_NODO;
    if(ev.x+maxW>RED_W) ev.x=RED_W-maxW;
    if(ev.y+maxH>AH)    ev.y=AH-maxH;
    elementos.push_back(ev);
}

void crearEnlace(int a,int b){
    if(a==b||enlaceExiste(a,b)) return;
    grafo.agregarEnlace(a,b,10000000,1.0);
    enlacesVis.push_back({a,b});
    // Recalcular Dijkstra si habia uno activo
    if(dijkstraOrigen>=0)
        dijkstraRes=grafo.dijkstra(dijkstraOrigen);
}

// ============================================================
//  ELIMINAR ENLACE
// ============================================================
void eliminarEnlace(int a,int b){
    // Del vector visual
    for(int i=0;i<(int)enlacesVis.size();i++){
        auto& e=enlacesVis[i];
        if((e.a==a&&e.b==b)||(e.a==b&&e.b==a)){
            enlacesVis.erase(enlacesVis.begin()+i);
            break;
        }
    }
    // Del grafo logico: eliminar en ambas listas de adyacencia
    auto elimAdj=[&](int u,int v){
        auto& adj=grafo.nodos[u].adyacentes;
        for(int i=0;i<(int)adj.size();i++)
            if(adj[i].destino==v){ adj.erase(adj.begin()+i); break; }
    };
    elimAdj(a,b); elimAdj(b,a);
    // Limpiar tramas que usaban este enlace
    for(auto& t:tramas) t.entregada=true;
    if(dijkstraOrigen>=0) dijkstraRes=grafo.dijkstra(dijkstraOrigen);
}

// ============================================================
//  ELIMINAR NODO
// ============================================================
void eliminarNodo(int idx){
    if(idx<0||idx>=(int)elementos.size()) return;

    // 1. Eliminar todos los enlaces visuales que tocan este nodo
    for(int i=(int)enlacesVis.size()-1;i>=0;i--){
        if(enlacesVis[i].a==idx||enlacesVis[i].b==idx)
            enlacesVis.erase(enlacesVis.begin()+i);
    }
    // Reindexar los enlaces restantes (indices mayores bajan 1)
    for(auto& e:enlacesVis){
        if(e.a>idx) e.a--;
        if(e.b>idx) e.b--;
    }

    // 2. Eliminar nodo del grafo logico
    // Quitar de listas de adyacencia de todos los demas
    for(int i=0;i<grafo.numNodos;i++){
        auto& adj=grafo.nodos[i].adyacentes;
        for(int j=(int)adj.size()-1;j>=0;j--){
            if(adj[j].destino==idx) adj.erase(adj.begin()+j);
            else if(adj[j].destino>idx) adj[j].destino--;
        }
    }
    grafo.nodos.erase(grafo.nodos.begin()+idx);
    grafo.numNodos--;

    // 3. Eliminar elemento visual
    elementos.erase(elementos.begin()+idx);

    // 4. Limpiar estado
    tramas.clear();
    nodoSeleccionado=-1;
    dijkstraOrigen=-1;
    dijkstraRuta.clear();
    dijkstraRes.distancia.clear();
    elimEnlaceA=-1;
    nodoEnlaceOrigen=-1;
}

// ============================================================
//  ENVIAR TRAMA
// ============================================================
void enviarTrama(){
    if(grafo.numNodos<2) return;
    int orig=rand()%grafo.numNodos, dest;
    do{ dest=rand()%grafo.numNodos; }while(dest==orig);
    ResultadoDijkstra res=grafo.dijkstra(orig);
    vector<int> ruta=grafo.reconstruirRuta(res,dest);
    if((int)ruta.size()<2) return;
    TramaVis t;
    t.origen=orig; t.destino=dest; t.ruta=ruta;
    t.pos=0; t.entregada=false; t.ticksVida=0;
    t.vx=centroX(orig); t.vy=centroY(orig);
    tramas.push_back(t);
}

// ============================================================
//  ARRASTRE
// ============================================================
void manejarArrastre(){
    if(modoActual!=MODO_NORMAL) return;
    float mx=raton_x(),my=raton_y();
    if(mx>=RED_W) return;
    if(raton_boton_izq()&&!arrastrando){
        int idx=elementoBajoRaton(mx,my);
        if(idx>=0){
            arrastrando=true; indiceArrastrado=idx;
            offsetX=mx-elementos[idx].x;
            offsetY=my-elementos[idx].y;
            nodoSeleccionado=idx;
            // Mostrar ruta Dijkstra hacia este nodo si hay origen activo
            if(dijkstraOrigen>=0&&idx!=dijkstraOrigen&&
               !dijkstraRes.distancia.empty())
                dijkstraRuta=grafo.reconstruirRuta(dijkstraRes,idx);
        }
    }
    if(arrastrando&&raton_boton_izq()){
        int nx=(int)mx-offsetX, ny=(int)my-offsetY;
        int w=(elementos[indiceArrastrado].tipo==SWITCH)?TAM_SW_W:TAM_NODO;
        int h=(elementos[indiceArrastrado].tipo==SWITCH)?TAM_SW_H:TAM_NODO;
        if(nx<0) nx=0; if(ny<0) ny=0;
        if(nx+w>RED_W) nx=RED_W-w;
        if(ny+h>AH)    ny=AH-h;
        elementos[indiceArrastrado].x=nx;
        elementos[indiceArrastrado].y=ny;
    }
    if(!raton_boton_izq()){ arrastrando=false; indiceArrastrado=-1; }
}

// ============================================================
//  CLIC EN AREA DE RED
// ============================================================
static bool clicPrevio=false;

void manejarClicRed(){
    float mx=raton_x(),my=raton_y();
    bool clicAhora=raton_boton_izq()&&mx<RED_W;
    bool clicNuevo=clicAhora&&!clicPrevio;
    clicPrevio=clicAhora;
    if(!clicNuevo) return;

    if(modoActual==MODO_CREAR_PC){
        crearNodo((int)mx,(int)my,PC);
        modoActual=MODO_NORMAL; return;
    }
    if(modoActual==MODO_CREAR_ROUTER){
        crearNodo((int)mx,(int)my,ROUTER);
        modoActual=MODO_NORMAL; return;
    }
    if(modoActual==MODO_CREAR_SWITCH){
        crearNodo((int)mx,(int)my,SWITCH);
        modoActual=MODO_NORMAL; return;
    }
    if(modoActual==MODO_ENLAZAR){
        int idx=elementoBajoRaton(mx,my);
        if(idx>=0){
            if(nodoEnlaceOrigen==-1) nodoEnlaceOrigen=idx;
            else{ crearEnlace(nodoEnlaceOrigen,idx); nodoEnlaceOrigen=-1; modoActual=MODO_NORMAL; }
        }
        return;
    }
    if(modoActual==MODO_ELIMINAR_ENLACE){
        int idx=elementoBajoRaton(mx,my);
        if(idx>=0){
            if(elimEnlaceA==-1) elimEnlaceA=idx;
            else{ eliminarEnlace(elimEnlaceA,idx); elimEnlaceA=-1; modoActual=MODO_NORMAL; }
        }
        return;
    }
    if(modoActual==MODO_ELIMINAR_NODO){
        int idx=elementoBajoRaton(mx,my);
        if(idx>=0){ eliminarNodo(idx); modoActual=MODO_NORMAL; }
        return;
    }
    if(modoActual==MODO_VER_DIJKSTRA){
        int idx=elementoBajoRaton(mx,my);
        if(idx>=0){
            dijkstraOrigen=idx;
            dijkstraRes=grafo.dijkstra(idx);
            dijkstraRuta.clear();
            nodoSeleccionado=idx;
            modoActual=MODO_NORMAL;
        }
        return;
    }
    if(modoActual==MODO_NORMAL){
        int idx=elementoBajoRaton(mx,my);
        nodoSeleccionado=idx;
        // Actualizar ruta visualizada
        if(dijkstraOrigen>=0&&idx>=0&&idx!=dijkstraOrigen&&
           !dijkstraRes.distancia.empty())
            dijkstraRuta=grafo.reconstruirRuta(dijkstraRes,idx);
        else if(idx<0) dijkstraRuta.clear();
    }
}

// ============================================================
//  TICK SIMULACION
// ============================================================
void tickSimulacion(){
    if(simPausada) return;
    tickActual++;
    for(Nodo& n:grafo.nodos) n.actualizarSaturacion();
    actualizarTramas();
    if(tickActual%60==0&&grafo.numNodos>=2) enviarTrama();
}

// ============================================================
//  MAIN
// ============================================================
int main(){
    srand((unsigned)time(nullptr));
    calcularLayout();
    vredimensiona(AW,AH);
    inicializarBotones();

    while(true){
        // Fondo
        color_rgb(16,16,26);
        rectangulo_lleno(0,0,AW,AH);

        dibujarSeparador();
        dibujarEnlaces();
        dibujarElementos();
        dibujarTramas();

        // Botones
        btnDibujar(btnCrearPC,     modoActual==MODO_CREAR_PC);
        btnDibujar(btnCrearRouter, modoActual==MODO_CREAR_ROUTER);
        btnDibujar(btnCrearSwitch, modoActual==MODO_CREAR_SWITCH);
        btnDibujar(btnEnlazar,     modoActual==MODO_ENLAZAR);
        btnDibujar(btnElimEnlace,  modoActual==MODO_ELIMINAR_ENLACE);
        btnDibujar(btnElimNodo,    modoActual==MODO_ELIMINAR_NODO);
        btnDibujar(btnDijkstra,    modoActual==MODO_VER_DIJKSTRA||dijkstraOrigen>=0);
        btnDibujar(btnSimular,     !simPausada);
        btnDibujar(btnPausar,      simPausada&&tickActual>0);
        btnDibujar(btnReset);
        btnDibujar(btnEnviarTrama);

        dibujarModoActual();
        dibujarPanelDatos();

        // Acciones botones
        if(btnClick(btnCrearPC))    { modoActual=MODO_CREAR_PC;     nodoEnlaceOrigen=-1; elimEnlaceA=-1; }
        if(btnClick(btnCrearRouter)){ modoActual=MODO_CREAR_ROUTER; nodoEnlaceOrigen=-1; elimEnlaceA=-1; }
        if(btnClick(btnCrearSwitch)){ modoActual=MODO_CREAR_SWITCH; nodoEnlaceOrigen=-1; elimEnlaceA=-1; }
        if(btnClick(btnEnlazar))    { modoActual=MODO_ENLAZAR;      nodoEnlaceOrigen=-1; elimEnlaceA=-1; }
        if(btnClick(btnElimEnlace)) { modoActual=MODO_ELIMINAR_ENLACE; elimEnlaceA=-1;   nodoEnlaceOrigen=-1; }
        if(btnClick(btnElimNodo))   { modoActual=MODO_ELIMINAR_NODO;   elimEnlaceA=-1;   nodoEnlaceOrigen=-1; }
        if(btnClick(btnDijkstra))   { modoActual=MODO_VER_DIJKSTRA; dijkstraOrigen=-1; dijkstraRuta.clear(); dijkstraRes.distancia.clear(); }
        if(btnClick(btnSimular))    { simPausada=false; modoActual=MODO_NORMAL; }
        if(btnClick(btnPausar))     { simPausada=true; }
        if(btnClick(btnReset)){
            tramas.clear(); tickActual=0; totalEntregadas=0; simPausada=true;
            for(Nodo& n:grafo.nodos){ n.tramasRecibidas=0; n.tramasEnviadas=0; }
        }
        if(btnClick(btnEnviarTrama)) enviarTrama();

        manejarClicRed();
        manejarArrastre();

        for(int i=0;i<3;i++) tickSimulacion();

        if(tecla()==ESCAPE){ modoActual=MODO_NORMAL; nodoEnlaceOrigen=-1; elimEnlaceA=-1; }

        refresca();
        espera(16);
    }

    vcierra();
    return 0;
}