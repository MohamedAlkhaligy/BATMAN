#include "BATMAN.h"
#include "pthread.h"
#include "unistd.h"
#include "string"
#include "iostream"
#include "vector"
#include "stdio.h"

#define DIRECTIONS 4
#define CROSS_TIME 1
#define DEADLOCK_CHECK_GAP_TIME 1

using namespace std;

enum direction {NORTH, EAST, SOUTH, WEST};

int BATs_counter, directions_counter[DIRECTIONS] = {0};
pthread_cond_t direction_queues[DIRECTIONS], direction_first_queues[DIRECTIONS];
pthread_mutex_t manager_mutex, directions_mutex[DIRECTIONS];
bool is_crossing, is_deadlock;

/**
* BAT struct which represents the Bidirectional Autonomous Trolley in which it
* contains number -ID- and initial or original direction of a certain BAT.
*/
struct BAT {
    direction original_direction;
    int number;

    BAT(char character) {
        switch (character) {
            case 'n': original_direction = NORTH; break;
            case 'e': original_direction = EAST;  break;
            case 's': original_direction = SOUTH; break;
            case 'w': original_direction = WEST;  break;
        }
    }
};

void initialize();
void initialize_BAT_number(int* number);
void* BAT_thread(void* arguments);
void* deadlock_checker(void* arguments);
void arrive(BAT* b);
void cross(BAT* b);
void leave(BAT* b);

/**
* Initialize the BATs' counter that keeps track of BATs' numbers.
* Initialize -allocate space- the required conditional variables and mutexes.
*/
BATMAN::BATMAN()
{
    BATs_counter = 0;
    is_crossing = false;
    is_deadlock = false;
    for (unsigned int i = 0; i < DIRECTIONS; i++) {
        pthread_cond_init(&direction_queues[i], NULL);
        pthread_cond_init(&direction_first_queues[i], NULL);
        pthread_mutex_init(&directions_mutex[i], NULL);
    }
    pthread_mutex_init(&manager_mutex, NULL);
}

/**
* Destroy -deallocate- the conditional variables and mutexes.
*/
BATMAN::~BATMAN()
{
    for (unsigned int i = 0; i < DIRECTIONS; i++) {
        pthread_cond_destroy(&direction_queues[i]);
        pthread_cond_destroy(&direction_first_queues[i]);
        pthread_mutex_destroy(&directions_mutex[i]);
    }
    pthread_mutex_destroy(&manager_mutex);
}

/**
* Initializes the BATs' threads in which the characters of the
* given string represents the BATs. A thread is created for each
* BAT.
*/
void BATMAN::initialize(){
    string BATs;
    cin >> BATs;

    unsigned int number_threads = BATs.size();
    vector<pthread_t> BATs_threads(number_threads);
    pthread_t batman_thread;

    pthread_create(&batman_thread, NULL, deadlock_checker, NULL);
    for (unsigned int i = 0; i < number_threads; i++) {
        pthread_create(&BATs_threads[i], NULL, BAT_thread, &BATs[i]);
    }

    for (unsigned int i = 0; i < number_threads; i++) {
        pthread_join(BATs_threads[i], NULL);
    }

    cout << "All BATs Leaved, no more BATs." << endl;

}

/**
* BAT_thread is the thread of the BAT which goes through BAT's life cycle;
* arriving, crossing and finally leaving the crossroad.
* @param argument; the direction of the BAT.
*/
void* BAT_thread(void* argument) {
    char character = *((char*) argument);
    BAT bat(character);
    initialize_BAT_number(&bat.number);
    arrive(&bat);
    cross(&bat);
    leave(&bat);
    pthread_exit(NULL);
}

/**
* Initializes the BAT's number.
* @param number is the uninitialized BAT's number.
*/
void initialize_BAT_number(int* number) {
    pthread_mutex_lock(&manager_mutex);
    BATs_counter++;
    *number = BATs_counter;
    pthread_mutex_unlock(&manager_mutex);
}

/**
* Checks for deadlock and solve it if exists.
*/
void check() {
    if (directions_counter[NORTH] && directions_counter[EAST]
        && directions_counter[SOUTH] && directions_counter[WEST] && !is_crossing) {
            printf("DEADLOCK: BAT jam detected, signalling North to go\n");
            is_deadlock = true;
            pthread_cond_signal(&direction_first_queues[NORTH]);
        }
}

/**
* The thread which is responsible for calling check function
* every DEADLOCK_CHECK_GAP_TIME seconds.
*/
void* deadlock_checker(void* arguments) {
    while (true) {
        check();
        sleep(DEADLOCK_CHECK_GAP_TIME);
    }
    pthread_exit(NULL);
}

/**
* Changes a direction enum to a string to be printed.
* @param BAT_direction; the direction to be mapped to a string.
*/
char const* direction_enum_to_string(direction BAT_direction) {
    char const* direction;
    switch (BAT_direction) {
        case NORTH: direction = "North"; break;
        case EAST:  direction = "East";  break;
        case SOUTH: direction = "South"; break;
        case WEST:  direction = "West";  break;
    }
    return direction;
}

/**
* Handles the BATs arriving at the crossroad.
* @param b the bat that arrived recently.
*/
void arrive(BAT* b) {
    pthread_mutex_lock(&directions_mutex[b -> original_direction]);
    directions_counter[b -> original_direction]++;

    printf("BAT %d from %s arrives at crossing\n", b -> number,
        direction_enum_to_string(b -> original_direction));

    if (directions_counter[b -> original_direction] > 1) {
        pthread_cond_wait(&direction_queues[b -> original_direction],
            &directions_mutex[b -> original_direction]);
    }


    pthread_mutex_lock(&manager_mutex);
    int right_direction = (b -> original_direction + DIRECTIONS - 1) % DIRECTIONS;
    if (directions_counter[right_direction] > 0) {
        pthread_cond_wait(&direction_first_queues[b -> original_direction], &manager_mutex);
    }
    pthread_mutex_unlock(&manager_mutex);
    pthread_mutex_unlock(&directions_mutex[b -> original_direction]);
}

/**
* Handles the BATs crossing the crossroad.
* @param b the bat currently crossing.
*/
void cross(BAT* b) {
    pthread_mutex_lock(&manager_mutex);
    is_crossing = true;
    printf("BAT %d from %s crossing\n", b -> number,
        direction_enum_to_string(b -> original_direction));
    sleep(CROSS_TIME);
    is_crossing = false;
    pthread_mutex_unlock(&manager_mutex);
}

/**
* Handles the BATs leaving the crossroad.
* @param b the bat currently leaving.
*/
void leave(BAT* b) {
    pthread_mutex_lock(&directions_mutex[b -> original_direction]);
    directions_counter[b -> original_direction]--;

    printf("BAT %d from %s leaving crossing\n", b -> number,
        direction_enum_to_string(b -> original_direction));

    int left_direction = (b -> original_direction + DIRECTIONS + 1) % DIRECTIONS;
    pthread_cond_signal(&direction_first_queues[left_direction]);
    pthread_cond_signal(&direction_queues[b -> original_direction]);
    pthread_mutex_unlock(&directions_mutex[b -> original_direction]);
}
