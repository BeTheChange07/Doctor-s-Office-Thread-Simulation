#include <iostream>
#include <pthread.h>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <semaphore.h>
#include <random>

using namespace std;

// create a global patient queue of type int
queue<int> patientQueue;

// global patient id
int patientId = 1;

// Global semaphores
sem_t create_patient;
sem_t print_to_consol;
sem_t receptionist_available;
sem_t add_patient_queue_edit;
sem_t nurse_call_patient[3];
sem_t patient_by_office[3];
sem_t office_empty[3];
sem_t patient_ready_receptionist[15];
sem_t receptionist_done_patient[15];
sem_t patient_ready_nurse[15];
sem_t patient_in_waiting_room_ready_nurse[15];
sem_t patient_in_office[15];
sem_t patient_left[15];
sem_t doctor_heard_patient[3];
sem_t doctor_gives_advice[3];


// this function is used to initialize all the semaphores
void initializeSemaphores() {

    sem_init(&receptionist_available, 0, 1);
    sem_init(&add_patient_queue_edit, 0, 1);
    sem_init(&print_to_consol,0,1);
    sem_init(&create_patient,0,1);

    for (int i = 0; i < 3; i++) {
        sem_init(&nurse_call_patient[i], 0, 1);
        sem_init(&patient_by_office[i], 0, 0);
        sem_init(&office_empty[i], 0, 1);
        sem_init(&doctor_heard_patient[i], 0, 0);
        sem_init(&doctor_gives_advice[i], 0, 0);
    }
    for (int i = 0; i < 15; i++) {
        sem_init(&patient_ready_receptionist[i], 0, 0);
        sem_init(&receptionist_done_patient[i], 0, 0);
        sem_init(&patient_ready_nurse[i], 0, 0);
        sem_init(&patient_in_waiting_room_ready_nurse[i], 0, 0);
        sem_init(&patient_in_office[i], 0, 0);
        sem_init(&patient_left[i], 0, 0);
    }
}

extern "C" void* PatientThreadStart(void* arg) {

    sem_wait(&create_patient);

    int internalPatientId = patientId++;
    int patientIndex = internalPatientId - 1;


    // get random doctor and assign to doctor id
    random_device ranNum;
    mt19937 gen(ranNum());
    uniform_int_distribution<> distrib(1,3);
    int doctorId = distrib(gen);
    int doctorIndex = doctorId -1;

    sem_post(&create_patient);


    // the sequence of events for patient
    sem_wait(&print_to_consol);
    cout << "Patient " << internalPatientId << " enters waiting room, waits for receptionist\n" << flush;
    sem_post(&print_to_consol);
    sem_wait(&add_patient_queue_edit);
    patientQueue.push(internalPatientId);  // push the patient id to the queue
    sem_post(&add_patient_queue_edit);
    sem_post(&patient_ready_receptionist[patientIndex]);
    sem_wait(&receptionist_available);
    sem_wait(&receptionist_done_patient[patientIndex]);
    cout << "Patient " << internalPatientId << " leaves receptionist and sits in waiting room\n" << flush;
    sem_post(&patient_in_waiting_room_ready_nurse[patientIndex]);
    sem_wait(&nurse_call_patient[doctorIndex]);
    sem_wait(&patient_by_office[doctorIndex]);
    cout << "Patient " << internalPatientId << " enters doctor " << doctorId << "'s office\n" << flush;
    sem_post(&patient_in_office[doctorIndex]);
    sem_wait(&doctor_heard_patient[doctorIndex]);
    cout << "Patient " << internalPatientId << " receives advice from doctor " << doctorId  << endl << flush;
    sem_wait(&doctor_gives_advice[doctorIndex]);
    cout << "Patient " << internalPatientId << " leaves\n" << flush;
    sem_post(&patient_left[patientIndex]);



    return nullptr;
}


extern "C" void* ReceptionistThreadStart(void* arg) {

    int patientId;
    int patientIndex;

    while(true){
        if(!patientQueue.empty()){
            // get the at the patient front of the queue
            sem_wait(&add_patient_queue_edit);
            patientId = patientQueue.front();
            patientQueue.pop();
            sem_post(&add_patient_queue_edit);
            patientIndex = patientId -1;

            // process the patient
            sem_wait(&patient_ready_receptionist[patientIndex]);
            sem_wait(&print_to_consol);
            cout << "Receptionist registers patient " << patientId << endl <<flush;
            sem_post(&print_to_consol);
            sem_post(&receptionist_done_patient[patientIndex]);
            sem_post(&patient_ready_nurse[patientIndex]);
            sem_post(&receptionist_available);


        }
        else{
            continue;
        }

    return nullptr;
}
}


int main(int argc, char *argv[]){
    // the first argument of the command prompt is number of doctors and nurses
    // the second argument of the command prompt is the number of patients
    int numberOfDoctorAndNurse = atoi(argv[1]);
    int numberOfPatient = atoi(argv[2]);

    // initialize all the semaphores
    initializeSemaphores();

    // Define maximum numbers for patients, doctors, and nurses
    const int MAX_PATIENTS = 20;
    const int MAX_DOCTORS_AND_NURSES = 10;

// Static arrays for threads
    pthread_t patientThreads[MAX_PATIENTS];
    pthread_t receptionistThread;
    pthread_t doctorThreads[MAX_DOCTORS_AND_NURSES];
    pthread_t nurseThreads[MAX_DOCTORS_AND_NURSES];



    // Initialize and create the patient threads
    for (int i = 0; i < numberOfPatient; i++) {
        // create the patient threads
        pthread_create(&patientThreads[i], NULL, PatientThreadStart, NULL);

    }


    // create the receptionist thread
    pthread_create(&receptionistThread, NULL, ReceptionistThreadStart, NULL);



    /*/ Initialize and create the doctor and nurse threads
    for (int i = 0; i < numberOfDoctorAndNurse; i++) {
        // creat the doctor and nurse threads
        pthread_create(&doctorThreads[i], NULL, DoctorThreadStart, (void*)&doctors[i]);
        pthread_create(&nurseThreads[i], NULL, NurseThreadStart, (void*)&nurses[i]);


    }
     */

    // join all threads
    for (auto& thread : patientThreads) {
        pthread_join(thread, NULL);
    }

    pthread_join(receptionistThread, NULL);

    /*
    for (auto& thread : nurseThreads) {
        pthread_join(thread, NULL);
    }

    for (auto& thread : doctorThreads) {
        pthread_join(thread, NULL);
    }
     */




}

