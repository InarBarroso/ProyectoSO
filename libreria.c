#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <mqueue.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/mman.h>

#define COLA "/cola_mostrador_deposito"

pid_t pid_mostrador, pid_deposito;

int tiempo_aleatorio(int min, int max) {
    return rand() % (max - min + 1) + min;
}

void* busqueda(void* arg) {
    printf("[Hilo Búsqueda] Comienzo mi ejecución...\n");

    printf("[Hilo Búsqueda] Buscando ejemplar %s...\n", (char *)arg);
    sleep(tiempo_aleatorio(1, 3));
    printf("[Hilo Búsqueda] Ejemplar %s localizado. Enviando a extracción...\n", (char *)arg);


}

void* extraccion(void* arg) {
    printf("[Hilo Extracción] Comienzo mi ejecución...\n");

    printf("[Hilo Extracción] Extrayendo ejemplar %s de la estantería...\n", (char *)arg);
    sleep(tiempo_aleatorio(2, 4));
    printf("[Hilo Extracción] Ejemplar %s extraído. Enviando a registro...\n", (char *)arg);


}

void* registro(void* arg) {
    printf("[Hilo Registro] Comienzo mi ejecución...\n");

    printf("[Hilo Registro] Registrando préstamo del ejemplar %s...\n", (char *)arg);
    sleep(tiempo_aleatorio(1, 2));
    printf("[Hilo Registro] Préstamo del ejemplar %s registrado. Notificando al mostrador...\n", (char *)arg);

}

int main(int argc, char* argv[]) {
    // Creacion de la cola para que se comunique mostrador con deposito
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(int);
    attr.mq_curmsgs = 0;
    mqd_t cola_mostrador_deposito = mq_open(COLA, O_RDWR | O_CREAT, 0666, &attr);

    pid_mostrador = fork();

    if (pid_mostrador != 0) {
        pid_deposito = fork();
        if (pid_deposito != 0) {
            /* Proceso padre */

            printf("[Proceso Padre] Esperando a que terminen mis hijos...\n");



            //Eliminamos la cola cuando el proceso padre ha terminado
            mq_close(cola_mostrador_deposito);
            mq_unlink(COLA);
            printf("[Proceso Padre] Ejecución finalizada.\n");
        } else {
            /* Proceso Depósito */
            printf("[Proceso Depósito] Comienzo mi ejecución...\n");
            
            
            while(1) {
            //Recibir mensaje desde la cola
            int buff;
            ssize_t mensajeRecibido = mq_receive(cola_mostrador_deposito,&buff, sizeof(buff), NULL);
            
            //Creacion de los 3 hilos
            //Variables que almacenaran los ID de los hilos
            pthread_t tid_busqueda;
            pthread_t tid_extraccion;
            pthread_t tid_registro;

            //Variables que almacenaran los atributos de los hilos
            pthread_attr_t attr_busqueda;
            pthread_attr_t attr_extraccion;
            pthread_attr_t attr_registro;

            //Inicializamos los atributos de default de los hilos
            pthread_attr_init(&attr_busqueda);
            pthread_attr_init(&attr_extraccion);
            pthread_attr_init(&attr_registro);

            //Inicializamos los hilos
            pthread_create(&tid_busqueda, &attr_busqueda, busqueda, argv[1]);
            pthread_create(&tid_extraccion, &attr_extraccion, extraccion, argv[1]);
            pthread_create(&tid_registro, &attr_registro, registro, argv[1]);

            //Esperar a que terminen los hilos, DUDA ¿Hay que esperar en paralelo?

            pthread_join(tid_busqueda, NULL);
            pthread_join(tid_extraccion, NULL);
            pthread_join(tid_registro, NULL);
        
        
            }

            //Cerramos la cola cuando termine el proceso
            mq_close(cola_mostrador_deposito);
        }
    } else {
        /* Proceso Mostrador */
        printf("[Proceso Mostrador] Comienzo mi ejecución...\n");
        int num_solicitud = 0;

        while (1) {

            sleep(tiempo_aleatorio(3, 8));
            printf("[Proceso Mostrador] Recibida solicitud de préstamo nº %d. Enviando al depósito...\n", num_solicitud);
            // Enviar solicitud al depósito mediante cola
            if (mq_send(cola_mostrador_deposito, (const char *)&num_solicitud, sizeof(num_solicitud),0) == -1) {
                perror("mq_send"); //si da error
            }
            else{

            }

            num_solicitud++;
            
            //Cerramos la cola cuando termine el proceso
            mq_close(cola_mostrador_deposito);

        }
    }

    exit(0);
}