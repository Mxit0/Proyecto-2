#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <chrono>
#include <unistd.h>

using namespace std;

class ColaCircular {
private:
    vector<int> cola;
    size_t frente, atras, capacidad, tamaño;
    mutex mtx;
    condition_variable no_llena, no_vacia;

public:
    ColaCircular(size_t capacidad_inicial)
        : capacidad(capacidad_inicial), frente(0), atras(0), tamaño(0) {
        cola.resize(capacidad);
    }

    void producir(int item) {
        unique_lock<mutex> lock(mtx); 
        while (tamaño >= capacidad) {
            no_llena.wait(lock);
        }

        cola[atras] = item;
        atras = (atras + 1) % capacidad;
        ++tamaño;

        redimensionar_si_necesario();
        no_vacia.notify_one();
    }

    int consumir() {
        unique_lock<mutex> lock(mtx);
        while (tamaño <= 0) {
            no_vacia.wait(lock);
        }

        int item = cola[frente];
        frente = (frente + 1) % capacidad;
        --tamaño;

        redimensionar_si_necesario();
        no_llena.notify_one();
        return item;
    }

    void redimensionar_si_necesario() {
        if (tamaño == capacidad) {
            redimensionar(capacidad * 2);
            registrar_cambio_tamaño("Doblada");
        } else if (tamaño <= capacidad / 4 && capacidad > 1) {
            redimensionar(capacidad / 2);
            registrar_cambio_tamaño("Reducida a la mitad");
        }
    }

    void redimensionar(size_t nueva_capacidad) {
        vector<int> nueva_cola(nueva_capacidad);
        for (size_t i = 0; i < tamaño; ++i) {
            nueva_cola[i] = cola[(frente + i) % capacidad];
        }
        cola = move(nueva_cola);
        capacidad = nueva_capacidad;
        frente = 0;
        atras = tamaño;
    }

    void registrar_cambio_tamaño(const string& accion) {
        ofstream archivo_log("cola_log.txt", ios_base::app);
        if (archivo_log.is_open()) {
            archivo_log << "Capacidad " << accion << " a " << capacidad << " elementos.\n";
            archivo_log.close();
        }
    }
};

void productor(ColaCircular& cola, int id) {
    for (int i = 0; i < 10; ++i) {
        cola.producir(i);
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

void consumidor(ColaCircular& cola, int id, int tiempo_max_espera) {
    auto inicio = chrono::steady_clock::now();
    while (true) {
        auto transcurrido = chrono::steady_clock::now() - inicio;
        if (chrono::duration_cast<chrono::seconds>(transcurrido).count() > tiempo_max_espera) {
            break;
        }
        int item = cola.consumir();
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

int main(int argc, char *argv[]) {
    int num_productores = 0, num_consumidores = 0, tamaño_inicial = 0, tiempo_max_espera = 0;

    int opt;
    while ((opt = getopt(argc, argv, "p:c:s:t:")) != -1) {
        switch (opt) {
            case 'p': num_productores = stoi(optarg); break;
            case 'c': num_consumidores = stoi(optarg); break;
            case 's': tamaño_inicial = stoi(optarg); break;
            case 't': tiempo_max_espera = stoi(optarg); break;
            default:
                return EXIT_FAILURE;
        }
    }

    ColaCircular cola(tamaño_inicial);

    vector<thread> hilos_productores;
    for (int i = 0; i < num_productores; ++i) {
        hilos_productores.emplace_back(productor, ref(cola), i);
    }

    vector<thread> hilos_consumidores;
    for (int i = 0; i < num_consumidores; ++i) {
        hilos_consumidores.emplace_back(consumidor, ref(cola), i, tiempo_max_espera);
    }

    for (size_t i = 0; i < hilos_productores.size(); ++i) {
        hilos_productores[i].join();
    }

    for (size_t i = 0; i < hilos_consumidores.size(); ++i) {
        hilos_consumidores[i].join();
    }

    cout << "Simulación completada. Archivo 'cola_log.txt' creado .\n";
    return EXIT_SUCCESS;
}
