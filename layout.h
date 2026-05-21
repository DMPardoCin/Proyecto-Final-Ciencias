#ifndef LAYOUT_H
#define LAYOUT_H

#include "estado.h"
#include <windows.h>

inline void calcularLayout()
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

// Helpers geometricos (dependen del layout)
inline int ancho_elem(int idx)
{
    return elementos[idx].tipo == SWITCH ? TAM_SW_W : TAM_NODO;
}
inline int alto_elem(int idx)
{
    if (elementos[idx].tipo == SWITCH)
        return TAM_SW_H;
    if (elementos[idx].tipo == ROUTER)
        return ESCALA_NODO * 23;
    return TAM_NODO;
}
inline int centroX(int idx) { return elementos[idx].x + ancho_elem(idx) / 2; }
inline int centroY(int idx) { return elementos[idx].y + alto_elem(idx) / 2; }

inline int elementoBajoRaton(float mx, float my)
{
    for (int i = (int)elementos.size() - 1; i >= 0; i--)
    {
        if (mx >= elementos[i].x && mx <= elementos[i].x + ancho_elem(i) &&
            my >= elementos[i].y && my <= elementos[i].y + alto_elem(i))
            return i;
    }
    return -1;
}

#endif