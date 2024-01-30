#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

sem_t mutex;
sem_t sem_a;
sem_t sem_b;

pthread_barrier_t car_drive;

int car_search_a = 0;
int car_search_b = 0;
int car_ID = -1;

void* fan_thread(void* arg){

    //FIRST PHASE - SEARCHING A RIDE

    int driver = 0;

    sem_wait(&mutex);

	printf("Thread ID: %ld, Team: %s, I am looking for a car\n", pthread_self(), (char*) arg);

    if ((char*) arg == "A") {
        car_search_a += 1;
    }
    else {
        car_search_b += 1;
    }

    //CHECK IF VALID COMBINATOIN EXIST -IF SO WAKE THE OTHER FANS
    if ( car_search_a == 4) {

        driver = 1;
        car_ID += 1;
        car_search_a = 0;
        sem_post(&sem_a);
        sem_post(&sem_a);
        sem_post(&sem_a);

    }
    else if( car_search_b == 4) {
        
        driver = 1;
        car_ID += 1;
        car_search_b = 0;
        sem_post(&sem_b);
        sem_post(&sem_b);
        sem_post(&sem_b);

    }
    else if( car_search_b >= 2 && car_search_a >= 2) {

        driver = 1;
        car_ID += 1;
        car_search_a -= 2;
        car_search_b -= 2;
        sem_post(&sem_a);
        sem_post(&sem_b);

        if ((char*) arg == "A") {
            sem_post(&sem_b);
        }
        else {
            sem_post(&sem_a);
        }        
    }
    else {
        
        sem_post(&mutex);
        if ((char*) arg == "A") {
            sem_wait(&sem_a);
        }
        else {
            sem_wait(&sem_b);
        }
    }

    //PHASE 2 - GET INTO CAR AND WAIT UNTIL OTHER FANS JOIN
    printf("Thread ID: %ld, Team: %s, I have found a spot in a car\n", pthread_self(), (char*) arg);
    pthread_barrier_wait(&car_drive);

    //PHASE 3 - DRIVER DRIVES THE CAR
    if ( driver ) {
        printf("Thread ID: %ld, Team: %s, I am the captain and driving the car with ID %d\n", pthread_self(), (char*) arg, car_ID);
        sem_post(&mutex);
    }
    
	return NULL;
}

int main(int argc, char* argv[]){
	
    //GET ARGUMENTS
    int team_a = atoi(argv[1]);
    int team_b = atoi(argv[2]);
    int total = team_a + team_b;

    //CHECK ARGUMENTS
    if ( ( total % 4 != 0 ) || ( team_a % 2 != 0 ) || ( team_b % 2 != 0 ) ) {

        printf("The main terminates\n");
        return 0;
    }

    //INITIALIZE SEMAPHORES AND BARRIER
    sem_init(&mutex, 0, 1);
    sem_init(&sem_a, 0, 0);
    sem_init(&sem_b, 0, 0);
    pthread_barrier_init(&car_drive, NULL, 4);

    //CREATE FAN THREADS AND KEEP THEIR ID IN AN ARRAY
    pthread_t* threads = (pthread_t*)malloc(total * sizeof(pthread_t));

	for (int i = 0; i < team_a; i++ ) { //TEAM A

        pthread_t thread;
	    pthread_create(&thread, NULL, fan_thread, "A");
        threads[i] = thread;
    }

    for (int i = 0; i < team_b; i++ ) { //TEAM B

        pthread_t thread;
	    pthread_create(&thread, NULL, fan_thread, "B");
        threads[i + team_a] = thread;
    }

    //JOIN THE FAN THREADS AND TERMINATE THE PROGRAM
    for (int i = 0; i < team_a + team_b; i++ ) {   

        pthread_join(threads[i], NULL);
    }

	printf("The main terminates\n");

	return 0;
}