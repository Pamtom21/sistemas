#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <pthread.h>

#define SHM_SIZE 1024

int main() {
    // Crear una clave para la memoria compartida
    key_t key = ftok("shmfile", 65);
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // Crear la memoria compartida
    int shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    // Adjuntar la memoria compartida
    int *shm = shmat(shmid, NULL, 0);
        if (shm == (int *)(-1)) {
        perror("shmat");
        exit(1);
    }

    // Inicializar mutex
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    // Crear proceso hijo
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        // Proceso hijo
        int values[10];
        int sum = 0;

        for (int i = 0; i < 10; i++) {
            // Esperar a que el padre escriba en memoria compartida
            pthread_mutex_lock(&mutex);
            int value = shm[i];
            pthread_mutex_unlock(&mutex);

            printf("hijo: leyendo %d\n", value);
            values[i] = value;
            sum += value;
        }

        // Calcular el promedio
        float average = (float)sum / 10;

        // Abrir y escribir en el archivo "resultados.txt"
        FILE *file = fopen("resultados.txt", "w");
        if (file == NULL) {
            perror("fopen");
            exit(1);
        }

        fprintf(file, "Valores: ");
        for (int i = 0; i < 10; i++) {
            fprintf(file, "%d", values[i]);
            if (i < 9) {
                fprintf(file, ", ");
            }
        }
        fprintf(file, "\nPromedio: %.2f\n", average);

        fclose(file);
    } else {
        // Proceso padre
        srand(time(NULL));

        for (int i = 0; i < 10; i++) {
            int random_value = rand() % 10 + 1;
            printf("padre: escribiendo %d\n", random_value);

            // Escribir en memoria compartida
            pthread_mutex_lock(&mutex);
            shm[i] = random_value;
            pthread_mutex_unlock(&mutex);

            sleep(2); // Esperar 2 segundos entre escrituras
        }

        // Esperar a que el hijo termine
        wait(NULL);

        // Abrir y mostrar el contenido del archivo "resultados.txt"
        FILE *file = fopen("resultados.txt", "r");
        if (file == NULL) {
            perror("fopen");
            exit(1);
        }

        char ch;
        while ((ch = fgetc(file)) != EOF) {
            putchar(ch);
        }
        fclose(file);

        // Liberar recursos
        shmdt(shm);
        shmctl(shmid, IPC_RMID, NULL);
        pthread_mutex_destroy(&mutex);
    }

    return 0;
}
