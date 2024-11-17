#include <iostream>
#include <vector>
#include <list>
#include <fstream>
#include <string>
#include <deque>
#include <algorithm>

using namespace std;

struct Pagina {
    int numero;
    int marco;
};

class SimuladorMemoriaVirtual {
public:
    SimuladorMemoriaVirtual(int marcos, const string& algoritmo) {
        numMarcos = marcos;
        tipoAlgoritmo = algoritmo;
        fallosPagina = 0;
        tablaPaginas.resize(marcos);
    }

    void leerReferencias(const string& archivo) {
        ifstream file(archivo);
        int pagina;
        while (file >> pagina) {
            referencias.push_back(pagina);
        }
    }

    void ejecutar() {
        if (tipoAlgoritmo == "FIFO") {
            reemplazoFIFO();
        } else if (tipoAlgoritmo == "LRU") {
            reemplazoLRU();
        } else if (tipoAlgoritmo == "OPT") {
            reemplazoOptimo();
        } else if (tipoAlgoritmo == "RELOJ") {
            reemplazoRelojSimple();
        } else {
            cout << "Algoritmo no válido.\n";
        }
        cout << "Total de fallos de página: " << fallosPagina << endl;
    }

private:
    int numMarcos;
    string tipoAlgoritmo;
    int fallosPagina;
    vector<list<Pagina>> tablaPaginas;
    vector<int> referencias;

    int hash(int clave) const {
        return (clave % numMarcos);
    }

    void insertarPagina(int numeroPagina, int marco) {
        int indice = hash(numeroPagina);
        for (auto& p : tablaPaginas[indice]) {
            if (p.numero == numeroPagina){
                return;
            }
        }
        tablaPaginas[indice].push_back({numeroPagina, marco});
    }

    void reemplazoFIFO() {
        deque<int> colaFIFO;
        for (int pagina : referencias) {
            if (!estaEnMemoria(pagina)) {
                if (colaFIFO.size() >= numMarcos) {
                    int paginaAEliminar = colaFIFO.front();
                    eliminarPagina(paginaAEliminar);
                    colaFIFO.pop_front();
                }
                colaFIFO.push_back(pagina);
                insertarPagina(pagina, colaFIFO.size() - 1);
                fallosPagina++;
            }
        }
    }

    void reemplazoLRU() {
        list<int> listaLRU;
        for (int pagina : referencias) {
            auto it = find(listaLRU.begin(), listaLRU.end(), pagina);
            if (it == listaLRU.end()) {
                if (listaLRU.size() >= numMarcos) {
                    int paginaAEliminar = listaLRU.front();
                    eliminarPagina(paginaAEliminar);
                    listaLRU.pop_front();
                }
                listaLRU.push_back(pagina);
                insertarPagina(pagina, listaLRU.size() - 1);
                fallosPagina++;
            } else {
                listaLRU.erase(it);
                listaLRU.push_back(pagina);
            }
        }
    }

    void reemplazoOptimo() {
        for (size_t i = 0; i < referencias.size(); i++) {
            int pagina = referencias[i];
            if (!estaEnMemoria(pagina)) {
                if (tablaPaginas.size() >= numMarcos) {
                    int victima = seleccionarOptima(i);
                    eliminarPagina(victima);
                }
                insertarPagina(pagina, i);
                fallosPagina++;
            }
        }
    }

    void reemplazoRelojSimple() {
        vector<bool> bitsReloj(numMarcos, false);
        vector<int> reloj;
        int puntero = 0;

        for (int pagina : referencias) {
            auto it = find(reloj.begin(), reloj.end(), pagina);
            if (it == reloj.end()) {
                while (bitsReloj[puntero]) {
                    bitsReloj[puntero] = false;
                    puntero = (puntero + 1) % numMarcos;
                }
                if (reloj.size() < numMarcos) {
                    reloj.push_back(pagina);
                } else {
                    eliminarPagina(reloj[puntero]);
                    reloj[puntero] = pagina;
                }
                bitsReloj[puntero] = true;
                puntero = (puntero + 1) % numMarcos;
                insertarPagina(pagina, puntero);
                fallosPagina++;
            } else {
                bitsReloj[it - reloj.begin()] = true;
            }
        }
    }

    void eliminarPagina(int numeroPagina) {
        int indice = hash(numeroPagina);
        auto& lista = tablaPaginas[indice];
        lista.remove_if([numeroPagina](const Pagina& p) { return p.numero == numeroPagina; });
    }

    bool estaEnMemoria(int numeroPagina) {
        int indice = hash(numeroPagina);
        for (const auto& p : tablaPaginas[indice]) {
            if (p.numero == numeroPagina) return true;
        }
        return false;
    }

    int seleccionarOptima(size_t indiceActual) {
        int maxDistancia = -1, paginaVictima = -1;
        for (const auto& bucket : tablaPaginas) {
            for (const auto& pagina : bucket) {
                size_t distancia = calcularDistancia(pagina.numero, indiceActual);
                if (distancia > maxDistancia) {
                    maxDistancia = distancia;
                    paginaVictima = pagina.numero;
                }
            }
        }
        return paginaVictima;
    }

    size_t calcularDistancia(int pagina, size_t indiceActual) {
        for (size_t i = indiceActual + 1; i < referencias.size(); ++i) {
            if (referencias[i] == pagina) return i - indiceActual;
        }
        return referencias.size();
    }
};

int main(int argc, char* argv[]) {
    if (argc != 7) {
        cout << "Uso: ./mvirtual -m <marcos> -a <algoritmo> -f <archivo>\n";
        return 1;
    }
    int marcos = stoi(argv[2]);
    string algoritmo = argv[4];
    string archivo = argv[6];

    SimuladorMemoriaVirtual simulador(marcos, algoritmo);
    simulador.leerReferencias(archivo);
    simulador.ejecutar();

    return 0;
}
