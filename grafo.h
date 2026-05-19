using namespace std;
#include "red.h"

//  Clase Grafo  -  topologia de red + algoritmo de Dijkstra

class Grafo {
public:
    vector<Nodo> nodos;
    int numNodos;

    Grafo() : numNodos(0) {}

    // Construccion 
    int agregarNodo(const string& nombre) {
        int id = numNodos++;
        nodos.emplace_back(id, nombre);
        // Inicializar cargas aleatorias entre 10 % y 60 %
        nodos.back().cargaCPU    = 0.1 + (rand() % 51) / 100.0;
        nodos.back().cargaMemoria= 0.1 + (rand() % 51) / 100.0;
        nodos.back().saturacion  =
            0.6 * nodos.back().cargaCPU + 0.4 * nodos.back().cargaMemoria;
        return id;
    }

    // Agrega enlace bidireccional
    void agregarEnlace(int u, int v, int bandwidthBps, double latenciaMs) {
        if (u < 0 || u >= numNodos || v < 0 || v >= numNodos) {
            cerr << "[ERROR] Nodo fuera de rango al agregar enlace.\n";
            return;
        }
        nodos[u].agregarEnlace(v, bandwidthBps, latenciaMs);
        nodos[v].agregarEnlace(u, bandwidthBps, latenciaMs);
    }

    vector<tuple<int, int, Enlace>> getEnlaces() {
        vector<tuple<int, int, Enlace>> lista;
        for (int u = 0; u < numNodos; ++u) {
            for (const Enlace& e : nodos[u].adyacentes) {
                if (u < e.destino) {  // evitar duplicados
                    lista.emplace_back(u, e.destino, e);
                }
            }
        }
        return lista;
    }

    // --- Dijkstra con metrica OSPF -------------------
    //
    //  Costo efectivo de un enlace:
    //    costoBase  = REF_BW / bandwidth          (metrica OSPF)
    //    penalidad  = costoBase * saturacion_nodo_destino
    //    costoTotal = costoBase + penalidad
    //
    //  Esto replica como OSPF ajusta costos segun carga.
    // ------------------------------------------------
    ResultadoDijkstra dijkstra(int origen) const {
        ResultadoDijkstra res;
        res.distancia.assign(numNodos, INF);
        res.predecesor.assign(numNodos, -1);
        res.distancia[origen] = 0;

        // min-heap: (costo_acumulado, nodo)
        using Par = pair<int,int>;
        priority_queue<Par, vector<Par>, greater<Par>> pq;
        pq.push({0, origen});

        while (!pq.empty()) {
            auto p = pq.top(); pq.pop();
            int dist = p.first;
            int u    = p.second;

            if (dist > res.distancia[u]) continue; // entrada obsoleta

            for (const Enlace& e : nodos[u].adyacentes) {
                int v = e.destino;
                // Penalidad por saturacion del nodo destino
                double penalidad = e.costoOSPF * nodos[v].saturacion;
                int costoEfectivo = e.costoOSPF + static_cast<int>(penalidad);
                if (costoEfectivo < 1) costoEfectivo = 1;

                int nuevaDist = res.distancia[u] + costoEfectivo;
                if (nuevaDist < res.distancia[v]) {
                    res.distancia[v]   = nuevaDist;
                    res.predecesor[v]  = u;
                    pq.push({nuevaDist, v});
                }
            }
        }
        return res;
    }

    // Reconstruye la ruta desde origen hasta destino
    vector<int> reconstruirRuta(const ResultadoDijkstra& res, int destino) const {
        vector<int> ruta;
        if (res.distancia[destino] == INF) return ruta; // sin ruta

        for (int v = destino; v != -1; v = res.predecesor[v])
            ruta.push_back(v);

        reverse(ruta.begin(), ruta.end());
        return ruta;
    }

    // --- Helpers -------------------------------------
    Enlace* buscarEnlace(int u, int v) {
        for (Enlace& e : nodos[u].adyacentes)
            if (e.destino == v) return &e;
        return nullptr;
    }

    void imprimirTopologia() const {
        cout << "\n+==========================================+\n";
        cout << "|         TOPOLOGIA DE LA RED               |\n";
        cout << "+==========================================+\n";
        for (const Nodo& n : nodos) {
            cout << "  [" << n.id << "] " << n.nombre
                      << "  (CPU=" << fixed << setprecision(1)
                      << n.cargaCPU * 100 << "%"
                      << "  RAM=" << n.cargaMemoria * 100 << "%"
                      << "  SAT=" << n.saturacion * 100 << "%)\n";
            for (const Enlace& e : n.adyacentes) {
                cout << "       +-> [" << e.destino << "] "
                          << nodos[e.destino].nombre
                          << "  BW=" << e.bandwidthBps / 1000000 << " Mbps"
                          << "  Cost=" << e.costoOSPF
                          << "  Lat=" << e.latenciaMs << "ms\n";
            }
        }
        cout << endl;
    }

    void imprimirTablaRutas(int origen) const {
        ResultadoDijkstra res = dijkstra(origen);
        cout << "\n+-------------------------------------------------+\n";
        cout << "|  TABLA DE RUTAS (Dijkstra OSPF) desde ["
                  << nodos[origen].nombre << "]       |\n";
        cout << "+--------+------------+----------+-----------------+\n";
        cout << "| Destino|    Nombre  |  Costo   |      Ruta       |\n";
        cout << "+--------+------------+----------+-----------------+\n";

        for (int i = 0; i < numNodos; ++i) {
            if (i == origen) continue;
            vector<int> ruta = reconstruirRuta(res, i);
            ostringstream oss;
            for (int j = 0; j < (int)ruta.size(); ++j) {
                if (j) oss << "-->";
                oss << nodos[ruta[j]].nombre;
            }
            string rutaStr = ruta.empty() ? "SIN RUTA" : oss.str();
            cout << "| [" << setw(2) << i << "]  | "
                      << left << setw(10) << nodos[i].nombre << " | "
                      << right << setw(8)
                      << (res.distancia[i] == INF ? -1 : res.distancia[i])
                      << " | " << left << setw(15) << rutaStr << " |\n";
        }
        cout << "+--------+------------+----------+-----------------+\n\n";
    }
};

