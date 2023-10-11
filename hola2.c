#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>

#define SHM_SIZE 10

int main() {
    // Generar una semilla aleatoria para rand()
    srand(time(NULL));

    // Crear una clave para la memoria compartida
    key_t key = ftok("shmfile", 65);
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    // Crear la memoria compartida
    int shmid = shmget(key, SHM_SIZE * sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    // Adjuntar la memoria compartida
    int *shm = (int *)shmat(shmid, NULL, 0);
    if ((int)shm == -1) {
        perror("shmat");
        exit(1);
    }

    // Crear proceso hijo
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        // Proceso hijo
        sleep(1); // Espera 1 segundo para sincronizarse con el padre

        int values[SHM_SIZE];
        int sum = 0;

        for (int i = 0; i < SHM_SIZE; i++) {
            printf("hijo: leyendo %d\n", shm[i]);
            values[i] = shm[i];
            sum += shm[i];
        }

        float average = (float)sum / SHM_SIZE;

        FILE *file = fopen("resultados.txt", "w");
        if (file == NULL) {
            perror("fopen");
            exit(1);
        }

        fprintf(file, "Valores: ");
        for (int i = 0; i < SHM_SIZE; i++) {
            fprintf(file, "%d", values[i]);
            if (i < SHM_SIZE - 1) {
                fprintf(file, ", ");
            }
        }
        fprintf(file, "\nPromedio: %.2f\n", average);

        fclose(file);
    } else {
        // Proceso padre
        for (int i = 0; i < SHM_SIZE; i++) {
            int random_value = (rand() % 10) + 1;
            printf("padre: escribiendo %d\n", random_value);
            shm[i] = random_value;
            sleep(2); // Esperar 2 segundos entre escrituras
        }

        // Esperar a que el hijo termine
        wait(NULL);

        // Liberar recursos
        shmdt(shm);
        shmctl(shmid, IPC_RMID, NULL);
    }

    return 0;
}

