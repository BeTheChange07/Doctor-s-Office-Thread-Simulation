#include <iostream>
#include <pthread.h>
#include <string>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <semaphore.h>
#include <random>

using namespace std;



// structure of patient info
struct PatientInfo {
    int patientId;
    int doctorId;

    PatientInfo(int id , int doctorNum ){
        patientId = id;
        doctorId = doctorNum;
    }
};


// global queues
queue<PatientInfo> preReceptionistPatientQueue;
queue<PatientInfo> preNursePatientQueue[3];
queue<PatientInfo> doctorOfficeQueue[3];

// global variables
int patientCount = 0;
int nurseCount = 0;
int docCount = 0;
int patientLeft;



// Global semaphores
sem_t check_if_patient_left;
sem_t patient_after_receptionist_in_waiting_room;
sem_t print_to_consol;
sem_t patient_enter;
sem_t receptionist_available;
sem_t edit_patient_count;
sem_t edit_nurse_count;
sem_t edit_doctor_count;
sem_t edit_pre_receptionist_patient_queue;
sem_t patient_ready_receptionist;
sem_t check_in_patient;

sem_t nurse_get_doctor[3];
sem_t edit_nurse_queue[3];
sem_t edit_doc_office_queue[3];
sem_t tell_nurse_patient_ready[3];
sem_t patient_by_office[3];
sem_t patient_in_office[3];
sem_t office_empty[3];
sem_t doctor_visit_patient[3];
sem_t nurse_take_patient_to_office[3];


sem_t tell_patient_receptionist_done[15];
sem_t nurse_ready[15];
sem_t doctor_done_listen[15];
sem_t patient_check_out[15];


// this function is used to initialize all the semaphores
void initializeSemaphores() {

    sem_init(&check_if_patient_left,0, 1);
    sem_init(&patient_after_receptionist_in_waiting_room, 0, 0);
    sem_init(&print_to_consol,0,1);
    sem_init(&patient_enter,0,1);
    sem_init(&receptionist_available, 0, 1);
    sem_init(&edit_patient_count,0 ,1 );
    sem_init(&edit_nurse_count,0 , 1);
    sem_init(&edit_doctor_count,0 ,1 );
    sem_init(&edit_pre_receptionist_patient_queue,0 ,1 );
    sem_init(&patient_ready_receptionist,0 , 0);
    sem_init(&check_in_patient,0 , 1);


    for (int i = 0; i < 3; i++) {
        sem_init(&nurse_get_doctor[i],0,0);
        sem_init(&edit_nurse_queue[i],0 ,1);
        sem_init(&edit_doc_office_queue[i],0 , 1);
        sem_init(&tell_nurse_patient_ready[i],0 , 0 );
        sem_init(&patient_by_office[i],0 ,0);
        sem_init(&patient_in_office[i],0 ,0 );
        sem_init(&office_empty[i],0 , 1);
        sem_init(&doctor_visit_patient[i],0 ,1 );
        sem_init(&nurse_take_patient_to_office[i],0 ,1);


    }


    for (int i = 0; i < 15; i++) {

        sem_init(&doctor_done_listen[i],0 ,0 );
        sem_init(&nurse_ready[i],0 ,0);
        sem_init(&tell_patient_receptionist_done[i],0 ,0);
        sem_init(&patient_check_out[i],0 ,1 );


    }
}



// this function is the patient thread function
extern "C" void* PatientThreadStart(void* arg) {

    int doctorId;
    int patientId;

    // assign patient number
    sem_wait(&edit_patient_count);
    patientId = patientCount++;
    sem_post(&edit_patient_count);

    // get random doc num from 0-2
    random_device ranNum;
    mt19937 gen(ranNum());
    uniform_int_distribution<> distrib(0,2);
    doctorId = distrib(gen);

    // patient enter and waits to see receptionist
    sem_wait(&patient_enter);
    sem_wait(&edit_pre_receptionist_patient_queue);
    preReceptionistPatientQueue.emplace(patientId,doctorId);
    sem_post(&edit_pre_receptionist_patient_queue);
    sem_wait(&print_to_consol);
    cout << "Patient " << patientId << " enters waiting room, waits for receptionist\n" << flush;
    sem_post(&print_to_consol);
    sem_post(&patient_enter);

    // patient sees receptionist
    sem_post(&patient_ready_receptionist);
    sem_wait(&receptionist_available);
    sem_wait(&tell_patient_receptionist_done[patientId]);

    // patient leaves receptionist and goes back into the waiting room
    sem_wait(&print_to_consol);
    cout << "Patient " << patientId << " leaves receptionist and sits in waiting room\n" << flush;
    sem_post(&print_to_consol);


    sem_post(&patient_after_receptionist_in_waiting_room);

    // wait for nurse to call patient
    sem_wait(&nurse_ready[patientId]);

    // patient enters office
    sem_wait(&patient_by_office[patientId]);
    sem_wait(&print_to_consol);
    cout << "Patient " << patientId << " enters doctor " << doctorId << "'s office" << endl << flush;
    sem_post(&print_to_consol);

    // patient in doctors office
    sem_post(&patient_in_office[doctorId]);
    sem_wait(&doctor_done_listen[patientId]);
    sem_wait(&print_to_consol);
    cout << "Patient " << patientId << " receives advice from doctor " << doctorId << endl << flush;
    sem_post(&print_to_consol);

    // patient finished with doctor
    sem_wait(&patient_check_out[patientId]);
    sem_wait(&print_to_consol);
    cout << "Patient " << patientId << " leaves" << endl << flush;
    sem_post(&print_to_consol);
    sem_post(&office_empty[doctorId]);
    sem_post(&patient_check_out[patientId]);


    // mark off patients from patient list and exit if no more patients
    sem_wait(&check_if_patient_left);
    patientLeft--;
    if(patientLeft == 0){
        cout << "Simulation complete" << endl;
        exit(0);
    }
    sem_post(&check_if_patient_left);


    return nullptr;
}


// this function is the receptionist thread function
extern "C" void* ReceptionistThreadStart(void* arg) {

    int patientId;
    int doctorId;

    while(true){
        // receptionist check in patient
        sem_wait(&patient_ready_receptionist);
        sem_wait(&check_in_patient);
        sem_wait(&edit_pre_receptionist_patient_queue);
        PatientInfo patientInfo = preReceptionistPatientQueue.front();
        preReceptionistPatientQueue.pop();
        sem_post(&edit_pre_receptionist_patient_queue);
        patientId = patientInfo.patientId;
        doctorId = patientInfo.doctorId;
        sem_wait(&print_to_consol);
        cout << "Receptionist registers patient " << patientId << endl << flush;
        sem_post(&print_to_consol);
        sem_post(&check_in_patient);



        // put patient in line for doctor office
        sem_wait(&edit_nurse_queue[doctorId]);
        preNursePatientQueue[doctorId].emplace(patientInfo);
        sem_post(&edit_nurse_queue[doctorId]);


        //send patient back to waiting room
        sem_post(&tell_patient_receptionist_done[patientId]);

        // wait till patient in waiting room
        sem_wait(&patient_after_receptionist_in_waiting_room);

        // tell nurse patient ready
        sem_post(&tell_nurse_patient_ready[doctorId]);

        // signal receptionist can take next patient
        sem_post(&receptionist_available);

    }

    return nullptr;
}


// this function is the nurse thread function
extern "C" void* NurseThreadStart(void* arg) {
    int doctorId;
    int patientId;

    sem_wait(&edit_nurse_count);
    doctorId = nurseCount++;
    sem_post(&edit_nurse_count);

    while(true){

        // nurse waits for patient to be back in waiting room and office to be empty
        sem_wait(&tell_nurse_patient_ready[doctorId]);
        sem_wait(&office_empty[doctorId]);

        // nurse gets info of patient
        sem_wait(&edit_nurse_queue[doctorId]);
        PatientInfo patientInfo = preNursePatientQueue[doctorId].front();
        preNursePatientQueue[doctorId].pop();
        sem_post(&edit_nurse_queue[doctorId]);
        patientId = patientInfo.patientId;


        //nurse tells patient to come with them to office
        sem_wait(&nurse_take_patient_to_office[doctorId]);
        sem_post(&nurse_ready[patientId]);

        sem_wait(&print_to_consol);
        cout << "Nurse " << doctorId << " takes patient "<< patientId << " to doctor's office" << endl << flush;
        sem_post(&print_to_consol);

        // nurse adds patient to doctor office queue
        sem_wait(&edit_doc_office_queue[doctorId]);
        doctorOfficeQueue[doctorId].emplace(patientInfo);
        sem_post(&edit_doc_office_queue[doctorId]);

        // nurse lets patient enter office
        sem_post(&patient_by_office[patientId]);

        // nurse gets doctor
        sem_wait(&patient_in_office[doctorId]);
        sem_post(&nurse_get_doctor[doctorId]);

        sem_post(&nurse_take_patient_to_office[doctorId]);


    }

    return nullptr;
}

// this function is the doctor thread function
extern "C" void* DoctorThreadStart(void* arg) {

    int doctorId;
    int patientId;

    sem_wait(&edit_doctor_count);
    doctorId = docCount++;
    sem_post(&edit_doctor_count);

    while(true){

        // doctor waits till nurse tells them patient is in office
        sem_wait(&nurse_get_doctor[doctorId]);

        // doctor visits with patient
        sem_wait(&doctor_visit_patient[doctorId]);
        sem_wait(&edit_doc_office_queue[doctorId]);
        PatientInfo patientInfo = doctorOfficeQueue[doctorId].front();
        doctorOfficeQueue[doctorId].pop();
        sem_post(&edit_doc_office_queue[doctorId]);

        patientId = patientInfo.patientId;


        // doctor listens to patient
        sem_wait(&print_to_consol);
        cout << "Doctor " << doctorId << " listens to symptoms from patient " << patientId << endl << flush;
        sem_post(&print_to_consol);

        // goes back to patient to listen to advise
        sem_post(&doctor_done_listen[patientId]);

        sem_post(&doctor_visit_patient[doctorId]);

    }



    return nullptr;
}


int main(int argc, char *argv[]){
    // the first argument of the command prompt is number of doctors and nurses
    // the second argument of the command prompt is the number of patients
    int numberOfDoctorAndNurse = atoi(argv[1]);
    int numberOfPatient = atoi(argv[2]);

    // print to console number of patients, nurses, and doctors
    cout << "Run with " << numberOfPatient << " patients, " << numberOfDoctorAndNurse << " nurses, " << numberOfDoctorAndNurse << " doctors" << endl << endl;

    // assign number of patients left
    patientLeft = numberOfPatient;

    // initialize all the semaphores
    initializeSemaphores();


// Static arrays for threads
    pthread_t patientThreads[numberOfPatient];
    pthread_t receptionistThread;
    pthread_t doctorThreads[numberOfDoctorAndNurse];
    pthread_t nurseThreads[numberOfDoctorAndNurse];



    // Initialize and create the patient threads
    for (int i = 0; i < numberOfPatient; i++) {
        // create the patient threads
        pthread_create(&patientThreads[i], NULL, PatientThreadStart, NULL);

    }


    // create the receptionist thread
    pthread_create(&receptionistThread, NULL, ReceptionistThreadStart, NULL);



    // Initialize and create the doctor and nurse threads
    for (int i = 0; i < numberOfDoctorAndNurse; i++) {
        // creat the doctor and nurse threads
        pthread_create(&doctorThreads[i], NULL, DoctorThreadStart, NULL);
        pthread_create(&nurseThreads[i], NULL, NurseThreadStart, NULL);


    }


    // join all threads
    for (auto& thread : patientThreads) {
        pthread_join(thread, NULL);
    }

    pthread_join(receptionistThread, NULL);


    for (auto& thread : nurseThreads) {
        pthread_join(thread, NULL);
    }

    for (auto& thread : doctorThreads) {
        pthread_join(thread, NULL);
    }






    return 0;


}

