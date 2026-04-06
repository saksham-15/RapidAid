#pragma once
#include "types.h"

// ---------------------------------------------
//  DISPLAY HELPERS
// ---------------------------------------------

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void printLine(const string& ch = "-", int len = 68) {
    // Decorative lines removed to keep terminal clean!
}

void printHeader(const string& title) {
    cout << "\n" << CYAN << BOLD << title << RESET << "\n\n";
}

void printSubHeader(const string& title) {
    cout << "\n" << BOLD << title << RESET << "\n";
}

void printPriority(Priority p) {
    cout << priorityColor(p) << BOLD << priorityTag(p) << RESET;
}

void printLogEntry(const LogEntry& e) {
    string color = "\033[0m";
    if (e.type == "ALERT")   color = COL_RED;
    if (e.type == "WARN")    color = COL_YELLOW;
    if (e.type == "SUCCESS") color = COL_GREEN;
    if (e.type == "INFO")    color = WHITE;
    cout << DIM << "[" << e.timestamp << "] " << RESET
         << color << e.message << RESET << "\n";
}

// ---------------------------------------------
//  DISPLAY TABLES
// ---------------------------------------------

void displayHospitals(const vector<Hospital>& hospitals) {
    printSubHeader("HOSPITAL NETWORK STATUS");
    cout << left
         << setw(4) << "ID"
         << setw(20) << "Name"
         << setw(10) << "Node"
         << setw(10) << "Beds"
         << setw(10) << "ICU"
         << setw(10) << "Load"
         << setw(12) << "Status"
         << "\n";
    printLine();

    for (auto& h : hospitals) {
        string status = h.operational ? (h.hasBed() ? "OPEN" : "FULL") : "OFFLINE";
        string statusColor = (status == "OPEN") ? COL_GREEN :
                             (status == "FULL") ? COL_YELLOW : COL_RED;

        // Load bar
        int filled = (int)(h.loadFactor() * 10);
        string bar = "[";
        for (int i = 0; i < 10; i++)
            bar += (i < filled) ? "#" : ".";
        bar += "]";

        cout << left
             << setw(4) << h.id
             << setw(20) << h.name
             << setw(10) << h.locationNode
             << setw(10) << (to_string(h.availableBeds) + "/" + to_string(h.totalBeds))
             << setw(10) << (to_string(h.availableIcuBeds) + " ICU")
             << " " << bar << " "
             << to_string((int)(h.loadFactor() * 100)) << "% "
             << statusColor << status << RESET << "\n";
    }
}

void displayAmbulances(const vector<Ambulance>& ambulances) {
    printSubHeader("AMBULANCE FLEET");
    cout << left
         << setw(5) << "ID"
         << setw(12) << "Unit"
         << setw(10) << "Location"
         << setw(14) << "Status"
         << "Assigned Patient\n";
    printLine();

    for (auto& a : ambulances) {
        string status = a.available ? "AVAILABLE" : "EN ROUTE";
        string sc = a.available ? COL_GREEN : COL_YELLOW;
        cout << left
             << setw(5) << a.id
             << setw(12) << a.unitId
             << setw(10) << a.locationNode
             << sc << setw(14) << status << RESET
             << (a.available ? "-" : a.assignedPatient) << "\n";
    }
}

void displayRequests(const vector<EmergencyRequest>& requests) {
    printSubHeader("EMERGENCY REQUESTS");
    cout << left
         << setw(4) << "ID"
         << setw(18) << "Patient"
         << setw(12) << "Priority"
         << setw(8) << "Node"
         << setw(10) << "Amb"
         << setw(22) << "Hospital"
         << "Status\n";
    printLine();

    for (auto& r : requests) {
        string statusColor = (r.status == "DISPATCHED") ? COL_GREEN :
                             (r.status == "PENDING")    ? COL_YELLOW : COL_RED;
        cout << left
             << setw(4) << r.id
             << setw(18) << r.patientName
             << priorityColor(r.priority) << setw(12) << priorityTag(r.priority) << RESET
             << setw(8) << r.locationNode
             << setw(10) << (r.assignedAmbulanceId >= 0 ?
                             "AMB-" + to_string(r.assignedAmbulanceId) : "---")
             << setw(22) << (r.assignedHospitalId >= 0 ?
                             "H" + to_string(r.assignedHospitalId) : "---")
             << statusColor << r.status << RESET << "\n";
    }
}

void displayHospitalStaffView(const vector<Hospital>& hospitals) {
    printHeader("HOSPITAL STAFF DASHBOARD - INCOMING PATIENTS");
    for (auto& h : hospitals) {
        if (h.incomingQueue.empty()) continue;
        cout << "\n" << BOLD << COL_BLUE << "  > " << h.name << RESET
             << "  [" << h.availableBeds << " beds free, "
             << h.availableIcuBeds << " ICU]\n";
        printLine();
        cout << left
             << setw(4) << "#"
             << setw(20) << "Patient"
             << setw(14) << "Priority"
             << setw(12) << "Ambulance"
             << "ETA (min)\n";
        printLine(".");
        int rank = 1;
        for (auto& p : h.incomingQueue) {
            cout << setw(4) << rank++
                 << setw(20) << p.name
                 << priorityColor(p.priority) << setw(14) << priorityTag(p.priority) << RESET
                 << setw(12) << p.ambulanceId
                 << p.eta << " min\n";
        }
    }
}

void displayDispatchLog(const vector<LogEntry>& log, int last = 15) {
    printSubHeader("DISPATCH LOG (last " + to_string(last) + " entries)");
    int start = max(0, (int)log.size() - last);
    for (int i = start; i < (int)log.size(); i++)
        printLogEntry(log[i]);
}

void displayShortestPath(const Graph& g, int src, int dst,
                         const vector<int>& dist,
                         const vector<int>& prev) {
    if (dist[dst] == INF) {
        cout << COL_RED << "  No path reachable!\n" << RESET;
        return;
    }
    vector<int> path = g.getPath(prev, dst);
    cout << "  Route: ";
    for (int i = 0; i < (int)path.size(); i++) {
        if (i > 0) cout << COL_GREEN << " -> " << RESET;
        cout << g.name(path[i]);
    }
    cout << "\n";
    cout << "  Total travel time: " << BOLD << dist[dst] << " min" << RESET << "\n";
}
