#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>

#define A 184
//#define B 0x936A655
#define D 11
#define E 147
#define G 115
#define I 114


const int file_numbers = A / E + (A % E > 0);
sem_t sem_1;
sem_t sem_2;

int loop = 1;

typedef struct {
    char *address;
    size_t size;
    FILE *file;
} thread_info;


typedef struct {
    char *address;
    int file_number;
} writer_thread_info;

typedef struct {
    int number_thread;
} reader_thread_info;


char *allocate() {
    return malloc(A * 1024 * 1024);
}

void *write_memory(void *data) {
    thread_info *cur_data = (thread_info *) data;
    char *block = malloc(cur_data->size * sizeof(char));
    fread(block, 1, cur_data->size, cur_data->file);
    for (size_t i = 0; i < cur_data->size; i++)
        cur_data->address[i] = block[i];
    free(block);
    pthread_exit(0);
}

void fill_space(char *address) {
    char *address_offset = address;

    thread_info information[D];
    pthread_t threads[D];

    size_t size_for_thread = A * 1024 * 1024 / D;
    FILE *file_random = fopen("/dev/urandom", "rb");

    for (int i = 0; i < D; i++) {
        information[i].address = address_offset;
        information[i].size = size_for_thread;
        information[i].file = file_random;
        address_offset += size_for_thread;
    }

    information[D - 1].size += A * 1024 * 1024 % D;


    for (int i = 0; i < D; i++)
        pthread_create(&threads[i], NULL, write_memory, &information[i]);

    for (int i = 0; i < D; i++)
        pthread_join(threads[i], NULL);

    fclose(file_random);

}

void *generate_info(void *data) {
    char *cur_data = (char *) data;
    while (loop == 1) {
        fill_space(cur_data);
    }

    pthread_exit(0);
}

sem_t *get_sem(int id) {
    if (id == 0) return &sem_1;
    if (id == 1) return &sem_2;
    return NULL;
}


void write_file(writer_thread_info *data, int file_id) {
    sem_t *cur_sem = get_sem(file_id);
    sem_wait(cur_sem);
    char *default_name = malloc(sizeof(char) * 6);
    snprintf(default_name, sizeof(default_name) + 1, "file_%d", file_id);
    FILE *file = fopen(default_name, "wb");
    size_t file_size = E * 1024 * 1024;
    size_t rest = 0;
    while (rest < file_size) {
        long size = file_size - rest >= G ? G : file_size - rest; // NOLINT(cppcoreguidelines-narrowing-conversions)
        rest += fwrite(data->address + rest, 1, size, file);
    }

    sem_post(cur_sem);
}

void *write_files(void *data) {
    writer_thread_info *cur_data = (writer_thread_info *) data;
    while (loop == 1)
        write_file(cur_data, cur_data->file_number);
    return NULL;
}

void read_file(int id_thread, int file_id) {
    sem_t *cur_sem = get_sem(file_id);
    sem_wait(cur_sem);

    char *default_name = malloc(sizeof(char) * 6);
    snprintf(default_name, sizeof(default_name) + 1, "file_%d", file_id);
    FILE *file = fopen(default_name, "rb");

    unsigned char block[G];

    unsigned int sum = 0;
    unsigned int count = 0;

    size_t parts = E * 1024 * 1024 / G;

    for (size_t part = 0; part < parts; part++) {
        size_t size = fread(&block, 1, G, file);
        for (size_t i = 0; i < size; i += 1)
            sum += block[i];
        count += size;
    }

    unsigned int avg = sum / count;
    fprintf(stdout, "\navg(file_%d) in thread-%d: %d / %d = %d", file_id, id_thread, sum, count, avg);
    fflush(stdout);
    fclose(file);
    sem_post(cur_sem);
}

void *read_files(void *data) {
    reader_thread_info *cur_data = (reader_thread_info *) data;
    while (loop) {
        for (int i = 0; i < file_numbers; i++)
            read_file(cur_data->number_thread, i);
    }
    return NULL;
}


int main(void) {
    printf("Before allocation");
    getchar();

    char *address = allocate();

    printf("After allocation");
    getchar();

    fill_space(address);

    printf("After filling");
    getchar();

    free(address);

    printf("After deallocation");
    getchar();

    pthread_t generate_thread;

    address = allocate();

    pthread_create(&generate_thread, NULL, generate_info, address);

    sem_init(&sem_1, 0, 1);
    sem_init(&sem_2, 0, 1);

    pthread_t writer_thread[file_numbers];
    writer_thread_info writer_information[file_numbers];

    for (int i = 0; i < file_numbers; i++) {
        writer_information[i].file_number = i;
        writer_information[i].address = address;
    }

    for (int i = 0; i < file_numbers; i++)
        pthread_create(&writer_thread[i], NULL, write_files, &writer_information[i]);

    pthread_t reader_thread[I];
    reader_thread_info reader_information[I];

    for (int i = 0; i < I; i++) {
        reader_information[i].number_thread = i + 1;
        pthread_create(&reader_thread[i], NULL, read_files, &reader_information[i]);
    }

    printf("Wait");
    getchar();

    printf("End");
    getchar();

    loop = 0;

    for (int i = 0; i < I; i++) {
        pthread_join(reader_thread[i], NULL);
    }

    for (int i = 0; i < file_numbers; i++)
        pthread_join(writer_thread[i], NULL);

    pthread_join(generate_thread, NULL);

    sem_destroy(&sem_1);
    sem_destroy(&sem_2);

    free(address);
    return 0;
}
