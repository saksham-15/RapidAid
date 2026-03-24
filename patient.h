#ifndef PATIENT_H
#define PATIENT_H

#include <string>
using namespace std;

struct Patient {
    string name;
    int category;
    int distance;
    long arrivalTime;
    int priority;
};

#endif