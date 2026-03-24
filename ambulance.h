#ifndef AMBULANCE_H
#define AMBULANCE_H

enum Status {
    AVAILABLE,
    BUSY
};

struct Ambulance {
    int id;
    int location;
    Status status;
    int assignedPatient;
};

#endif