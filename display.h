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
    string token = ch.empty() ? "-" : ch;
    for (int i = 0; i < len; i++) cout << token[0];
    cout << "\n";
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

string fitCell(const string& value, int width) {
    if ((int)value.size() <= width) return value + string(width - value.size(), ' ');
    if (width <= 3) return value.substr(0, width);
    return value.substr(0, width - 3) + "...";
}

void printTableBorder(const vector<int>& widths) {
    cout << "+";
    for (int width : widths) {
        cout << string(width + 2, '-') << "+";
    }
    cout << "\n";
}

void printTableRow(const vector<string>& cells, const vector<int>& widths) {
    cout << "|";
    for (int i = 0; i < (int)widths.size(); i++) {
        string value = (i < (int)cells.size()) ? cells[i] : "";
        cout << " " << fitCell(value, widths[i]) << " |";
    }
    cout << "\n";
}

// ---------------------------------------------
//  DISPLAY TABLES
// ---------------------------------------------

void displayHospitals(const vector<Hospital>& hospitals) {
    printSubHeader("HOSPITAL NETWORK STATUS");
    vector<int> widths = {3, 18, 6, 15, 10, 13, 9, 7};
    printTableBorder(widths);
    printTableRow({"ID", "Name", "Node", "Available Beds", "Total Beds",
                   "Available ICU", "Total ICU", "Score"}, widths);
    printTableBorder(widths);

    for (auto& h : hospitals) {
        int bedScore = h.totalBeds > 0
            ? (int)((100.0 * h.availableBeds) / h.totalBeds)
            : 0;
        int icuScore = h.icuBeds > 0
            ? (int)((100.0 * h.availableIcuBeds) / h.icuBeds)
            : 0;
        int readinessScore = (int)(0.7 * bedScore + 0.3 * icuScore);

        printTableRow({to_string(h.id), h.name, to_string(h.locationNode),
                       to_string(h.availableBeds), to_string(h.totalBeds),
                       to_string(h.availableIcuBeds), to_string(h.icuBeds),
                       to_string(readinessScore)},
                      widths);
        printTableBorder(widths);
    }
}

void displayAmbulances(const vector<Ambulance>& ambulances) {
    printSubHeader("AMBULANCE FLEET");
    int availableCount = 0;
    for (auto& a : ambulances) {
        if (a.available) availableCount++;
    }
    cout << "  Available ambulances: " << BOLD << availableCount << RESET
         << "/" << ambulances.size()
         << "  |  Busy: " << (ambulances.size() - availableCount) << "\n\n";

    vector<int> widths = {3, 8, 8, 11, 20};
    printTableBorder(widths);
    printTableRow({"ID", "Unit", "Location", "Status", "Assigned Patient"}, widths);
    printTableBorder(widths);

    for (auto& a : ambulances) {
        string status = a.available ? "AVAILABLE" : "EN ROUTE";
        printTableRow({to_string(a.id), a.unitId, to_string(a.locationNode),
                       status, (a.available ? "-" : a.assignedPatient)},
                      widths);
        printTableBorder(widths);
    }
}

void displayRequests(const vector<EmergencyRequest>& requests) {
    printSubHeader("EMERGENCY REQUESTS");
    vector<int> widths = {3, 18, 12, 6, 8, 10, 16};
    printTableBorder(widths);
    printTableRow({"ID", "Patient", "Priority", "Node", "Amb", "Hospital", "Status"}, widths);
    printTableBorder(widths);

    for (auto& r : requests) {
        printTableRow({to_string(r.id), r.patientName, priorityTag(r.priority),
                       to_string(r.locationNode),
                       (r.assignedAmbulanceId >= 0 ?
                        "AMB-" + to_string(r.assignedAmbulanceId) : "---"),
                       (r.assignedHospitalId >= 0 ?
                        "H" + to_string(r.assignedHospitalId) : "---"),
                       r.status},
                      widths);
        printTableBorder(widths);
    }
}

string hospitalLabel(const vector<Hospital>& hospitals, int hospitalId) {
    if (hospitalId < 0 || hospitalId >= (int)hospitals.size()) return "---";
    return hospitals[hospitalId].name;
}

string ambulanceLabel(const vector<Ambulance>& ambulances, int ambulanceId) {
    if (ambulanceId < 0 || ambulanceId >= (int)ambulances.size()) return "---";
    return ambulances[ambulanceId].unitId;
}

string staffQueueLabel(const vector<Hospital>& hospitals,
                       const EmergencyRequest& request) {
    if (request.assignedHospitalId < 0 ||
        request.assignedHospitalId >= (int)hospitals.size()) {
        return "---";
    }

    const Hospital& hospital = hospitals[request.assignedHospitalId];
    for (int i = 0; i < (int)hospital.incomingQueue.size(); i++) {
        if (hospital.incomingQueue[i].name == request.patientName) {
            return hospital.name + " #" + to_string(i + 1);
        }
    }
    return hospital.name;
}

void displayOperationsDashboard(const vector<EmergencyRequest>& requests,
                                const vector<Hospital>& hospitals,
                                const vector<Ambulance>& ambulances,
                                const vector<LogEntry>& log) {
    printHeader("OPERATIONS DASHBOARD - REQUESTS, STAFF QUEUE & LOGS");

    printSubHeader("PATIENT / STAFF TRACKING");
    if (requests.empty()) {
        cout << "  No live emergency requests yet.\n";
    } else {
        vector<int> widths = {3, 16, 12, 5, 8, 16, 5, 16, 18};
        printTableBorder(widths);
        printTableRow({"ID", "Patient", "Priority", "Node", "Amb",
                       "Hospital", "ETA", "Status", "Staff Queue"}, widths);
        printTableBorder(widths);

        for (auto& r : requests) {
            string eta = (r.travelTime == INF) ? "---" : to_string(r.travelTime);
            printTableRow({to_string(r.id), r.patientName, priorityTag(r.priority),
                           to_string(r.locationNode),
                           ambulanceLabel(ambulances, r.assignedAmbulanceId),
                           hospitalLabel(hospitals, r.assignedHospitalId),
                           eta,
                           r.status,
                           staffQueueLabel(hospitals, r)},
                          widths);
            printTableBorder(widths);
        }
    }

    printSubHeader("RECENT SYSTEM LOGS");
    if (log.empty()) {
        cout << "  No dispatch log entries yet.\n";
    } else {
        vector<int> widths = {10, 9, 74};
        printTableBorder(widths);
        printTableRow({"Time", "Type", "Message"}, widths);
        printTableBorder(widths);
        int start = max(0, (int)log.size() - 12);
        for (int i = start; i < (int)log.size(); i++) {
            printTableRow({log[i].timestamp, log[i].type, log[i].message}, widths);
            printTableBorder(widths);
        }
    }
}

void displayHospitalStaffView(const vector<Hospital>& hospitals) {
    printHeader("HOSPITAL STAFF DASHBOARD - INCOMING PATIENTS");
    bool hasIncoming = false;
    for (auto& h : hospitals) {
        if (h.incomingQueue.empty()) continue;
        hasIncoming = true;
        cout << "\n" << BOLD << COL_BLUE << "  > " << h.name << RESET
             << "  [" << h.availableBeds << " beds free, "
             << h.availableIcuBeds << " ICU]\n";
        vector<int> widths = {3, 18, 12, 10, 9};
        printTableBorder(widths);
        printTableRow({"#", "Patient", "Priority", "Ambulance", "ETA"}, widths);
        printTableBorder(widths);
        int rank = 1;
        for (auto& p : h.incomingQueue) {
            printTableRow({to_string(rank++), p.name, priorityTag(p.priority),
                           p.ambulanceId, to_string(p.eta) + " min"},
                          widths);
            printTableBorder(widths);
        }
    }
    if (!hasIncoming) {
        cout << "  No incoming patients yet.\n";
        cout << "  Register or manually dispatch a patient, then open this dashboard again.\n";
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
    cout << "  From: " << g.name(src) << "  To: " << g.name(dst) << "\n";
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
