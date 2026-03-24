#include <iostream>
#include <fstream>
#include <queue>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include "patient.h"
#include "ambulance.h"
#include "hospital.h"
#include "graph.h"

using namespace std;

// COLORS
#define RED     "\033[31m"
#define ORANGE  "\033[33m"
#define YELLOW  "\033[93m"
#define GREEN   "\033[32m"
#define RESET   "\033[0m"

// Comparator
struct Compare {
    bool operator()(Patient a, Patient b) {
        return a.priority < b.priority;
    }
};

// Print Category
void printCategory(int category) {
    if (category == 1)
        cout << RED << "RED" << RESET;
    else if (category == 2)
        cout << ORANGE << "ORANGE" << RESET;
    else if (category == 3)
        cout << YELLOW << "YELLOW" << RESET;
    else
        cout << GREEN << "GREEN" << RESET;
}

// ---------------- ADD PATIENT ----------------
void addPatient() {
    ofstream fout("patients.txt", ios::app);

    Patient p;
    cout << "\nEnter Name: ";
    cin >> p.name;

    cout << "\n1.RED  2.ORANGE  3.YELLOW  4.GREEN\n";
    cout << "Select Category: ";
    cin >> p.category;

    cout << "Location(Node): ";
    cin >> p.distance;

    p.arrivalTime = time(0);

    fout << p.name << " "
         << p.category << " "
         << p.distance << " "
         << p.arrivalTime << endl;

    fout.close();
}

// ---------------- PROCESS ----------------
void processPatients() {

    system("python priority.py");

    ifstream fin("patients.txt");

    priority_queue<Patient, vector<Patient>, Compare> pq;
    queue<Patient> waitingQueue;

    Patient p;

    while (fin >> p.name >> p.category >> p.distance >> p.arrivalTime >> p.priority) {
        pq.push(p);
    }
    fin.close();

    // GRAPH
    Graph g(5);
    g.addEdge(0,1,2);
    g.addEdge(1,2,4);
    g.addEdge(0,3,6);
    g.addEdge(3,4,3);
    g.addEdge(2,4,1);

    // AMBULANCES
    vector<Ambulance> amb = {
        {1,0,AVAILABLE,-1},
        {2,2,AVAILABLE,-1}
    };

    // HOSPITALS (5 beds each)
    vector<Hospital> hosp = {
        {"Max",5},{"Synergy",5},{"Doon",5},{"GraphicEra",5},{"Indiresh",5}
    };

    vector<int> hospLocation = {0,1,2,3,4};

    while (!pq.empty()) {

        p = pq.top();
        pq.pop();

        cout << "\nProcessing: " << p.name << " | ";
        printCategory(p.category);
        cout << endl;

        // -------- FIND AMBULANCE --------
        int bestAmb = -1, minDist = INT_MAX;

        for (int i = 0; i < amb.size(); i++) {
            if (amb[i].status == AVAILABLE) {
                int d = g.dijkstra(amb[i].location, p.distance);
                if (d < minDist) {
                    minDist = d;
                    bestAmb = i;
                }
            }
        }

        if (bestAmb == -1) {
            cout << "No ambulance → Waiting\n";
            waitingQueue.push(p);
            continue;
        }

        // STATE: AVAILABLE → BUSY
        amb[bestAmb].status = BUSY;
        cout << "Ambulance " << amb[bestAmb].id << " DISPATCHED\n";

        // -------- SHOW NEAREST HOSPITALS --------
        vector<pair<int,int>> hospitalDist;

        for (int i = 0; i < hosp.size(); i++) {
            int d = g.dijkstra(p.distance, hospLocation[i]);
            hospitalDist.push_back({d, i});
        }

        sort(hospitalDist.begin(), hospitalDist.end());

        cout << "Nearest Hospitals:\n";
        for (auto x : hospitalDist) {
            int idx = x.second;
            cout << hosp[idx].name
                 << " | Distance: " << x.first
                 << " | Beds: " << hosp[idx].beds << endl;
        }

        // -------- SELECT HOSPITAL --------
        int bestHospital = -1;
        int minCost = INT_MAX;

        for (int i = 0; i < hosp.size(); i++) {

            if (hosp[i].beds <= 0) continue;

            int d = g.dijkstra(p.distance, hospLocation[i]);

            int load = 5 - hosp[i].beds;
            int cost = d + load;

            // Critical override
            if (p.category == 1) {
                if (bestHospital == -1 || hosp[i].beds > hosp[bestHospital].beds) {
                    bestHospital = i;
                }
            }
            else {
                if (bestHospital == -1 ||
                    cost < minCost ||
                    (cost == minCost && hosp[i].beds > hosp[bestHospital].beds))
                {
                    minCost = cost;
                    bestHospital = i;
                }
            }
        }

        if (bestHospital == -1) {
            cout << "No hospital → Waiting\n";
            waitingQueue.push(p);
            amb[bestAmb].status = AVAILABLE;
            continue;
        }

        // Allocate bed
        hosp[bestHospital].beds--;

        cout << "Hospital: " << hosp[bestHospital].name << endl;

        // STATE: BUSY → AVAILABLE
        cout << "Ambulance " << amb[bestAmb].id << " reached hospital\n";

        amb[bestAmb].status = AVAILABLE;
        amb[bestAmb].location = hospLocation[bestHospital];

        cout << "Ambulance " << amb[bestAmb].id << " AVAILABLE again\n";
    }

    // -------- WAITING QUEUE --------
    if (!waitingQueue.empty()) {
        cout << "\nWaiting Patients:\n";
        while (!waitingQueue.empty()) {
            cout << waitingQueue.front().name << endl;
            waitingQueue.pop();
        }
    }
}

// ---------------- MAIN ----------------
int main() {
    int ch;

    while (true) {
        cout << "\n1.Add Patient\n2.Process\n3.Exit\nChoice: ";
        cin >> ch;

        if (ch == 1) addPatient();
        else if (ch == 2) processPatients();
        else break;
    }
}