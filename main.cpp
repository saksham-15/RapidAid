#include "types.h"
#include "display.h"
#include "triage.h"
#include "dispatch.h"

// ---------------------------------------------
//  CITY NETWORK SETUP
//  16 nodes: intersections, hospitals, depots
// ---------------------------------------------
/*
  NODE MAP (City Layout):
   0=CentralSquare  1=NorthJunction  2=SouthMarket   3=EastCross
   4=WestEnd        5=UnivDistrict   6=IndustrialPk  7=AirportRd
   8=H_CityGeneral  9=H_StMary      10=H_Northside  11=H_Trauma
  12=AmbDepotA     13=AmbDepotB     14=RiverBridge   15=Suburb
*/
void setupCity(Graph& g,
               vector<Hospital>& hospitals,
               vector<Ambulance>& ambulances) {

    // Node names
    g.setName(0,  "Central Square");
    g.setName(1,  "North Junction");
    g.setName(2,  "South Market");
    g.setName(3,  "East Cross");
    g.setName(4,  "West End");
    g.setName(5,  "Univ. District");
    g.setName(6,  "Industrial Park");
    g.setName(7,  "Airport Road");
    g.setName(8,  "City General Hosp.");
    g.setName(9,  "St. Mary's Hosp.");
    g.setName(10, "Northside Hosp.");
    g.setName(11, "Trauma Centre");
    g.setName(12, "Amb. Depot A");
    g.setName(13, "Amb. Depot B");
    g.setName(14, "River Bridge");
    g.setName(15, "Suburb Area");

    // Road network (u, v, weight = minutes)
    g.addEdge(0,  1,  5);
    g.addEdge(0,  2,  6);
    g.addEdge(0,  3,  7);
    g.addEdge(0,  4,  8);
    g.addEdge(1,  5,  4);
    g.addEdge(1, 10,  3);  // depot to Northside
    g.addEdge(2,  6,  5);
    g.addEdge(2,  9,  4);  // South->St.Mary's
    g.addEdge(3,  7,  6);
    g.addEdge(3,  8,  5);  // East->City General
    g.addEdge(4,  6,  4);
    g.addEdge(4, 12,  2);  // West->Depot A
    g.addEdge(5, 10,  5);
    g.addEdge(5, 13,  3);  // Univ->Depot B
    g.addEdge(6, 14,  7);
    g.addEdge(7, 11,  4);  // Airport->Trauma
    g.addEdge(7, 14,  5);
    g.addEdge(8,  9,  9);  // City Gen <-> St.Mary's
    g.addEdge(9, 11,  8);
    g.addEdge(10, 11, 10);
    g.addEdge(12, 13,  6);
    g.addEdge(13,  1,  3);
    g.addEdge(14, 15,  4);
    g.addEdge(15,  3,  6);
    g.addEdge(0,  14, 9);

    // -- Hospitals -------------------------------------
    hospitals = {
        {0, "City General",  8, 20, 12, 4, 2, true, {}},
        {1, "St. Mary's",    9, 15,  8, 2, 1, true, {}},
        {2, "Northside",    10, 25, 18, 6, 4, true, {}},
        {3, "Trauma Centre",11, 10,  5, 8, 5, true, {}},
    };

    // -- Ambulances ------------------------------------
    ambulances = {
        {0, "AMB-01", 12, true,  ""},
        {1, "AMB-02", 13, true,  ""},
        {2, "AMB-03",  0, true,  ""},
        {3, "AMB-04",  4, true,  ""},
        {4, "AMB-05",  7, true,  ""},
    };
}

// ---------------------------------------------
//  SCENARIO DEMO RUNNER
// ---------------------------------------------
void runScenarios(Graph& g,
                  vector<Hospital>& hospitals,
                  vector<Ambulance>& ambulances,
                  vector<EmergencyRequest>& requests,
                  vector<LogEntry>& log) {

    DispatchEngine engine(g, hospitals, ambulances, requests, log);
    vector<Ambulance> baseAmbulances = ambulances;
    auto resetAmbulances = [&]() {
        ambulances = baseAmbulances;
    };

    printHeader("SCENARIO DEMONSTRATIONS");

    // -- S1: Normal Case ------------------------------
    resetAmbulances();
    printSubHeader("SCENARIO 1 - Normal Allocation");
    EmergencyRequest r1 = {(int)requests.size(), "Rahul Sharma", 2,
                           Priority::P_ORANGE, "Chest pain", -1, -1, INF, false, "PENDING"};
    requests.push_back(r1);
    engine.allocateNormal(requests.back());
    displayRequests(requests);
    displayDispatchLog(log, 3);

    // -- S2: Hospital Full ----------------------------
    resetAmbulances();
    printSubHeader("SCENARIO 2 - Hospital Full");
    // Fill St. Mary's and City General
    hospitals[0].availableBeds = 0;
    hospitals[1].availableBeds = 0;
    EmergencyRequest r2 = {(int)requests.size(), "Priya Patel", 2,
                           Priority::P_YELLOW, "Fracture", -1, -1, INF, false, "PENDING"};
    requests.push_back(r2);
    engine.allocateNormal(requests.back());
    displayDispatchLog(log, 4);
    // Restore
    hospitals[0].availableBeds = 12;
    hospitals[1].availableBeds = 8;

    // -- S3: Multiple Equidistant Hospitals -----------
    resetAmbulances();
    printSubHeader("SCENARIO 3 - Multiple Equidistant Hospitals");
    cout << "  Patient at node 0 (Central Square)\n";
    cout << "  Running Dijkstra from Central Square...\n";
    auto res_dijsktra1 = g.dijkstra(0);
    auto dist3 = res_dijsktra1.first;
    auto prev3 = res_dijsktra1.second;
    cout << "  Distance to each hospital:\n";
    for (auto& h : hospitals) {
        cout << "    " << h.name << " -> ";
        if (dist3[h.locationNode] == INF) cout << COL_RED << "UNREACHABLE" << RESET;
        else cout << BOLD << dist3[h.locationNode] << " min" << RESET;
        cout << " (" << h.availableBeds << " beds)\n";
    }
    vector<int> equidist;
    int minD = INF;
    for (auto& h : hospitals)
        if (dist3[h.locationNode] < minD) minD = dist3[h.locationNode];
    for (auto& h : hospitals)
        if (dist3[h.locationNode] == minD) equidist.push_back(h.id);
    if (equidist.size() > 1) {
        int tie = engine.breakTieByCapacity(equidist);
        cout << "\n  Tie-break by capacity -> "
             << COL_GREEN << hospitals[tie].name << RESET << "\n";
    } else cout << "\n  No tie to break.\n";

    // -- S4: Critical Override ------------------------
    resetAmbulances();
    printSubHeader("SCENARIO 4 - Critical Override (RED)");
    EmergencyRequest r4 = {(int)requests.size(), "Amit Verma", 6,
                           Priority::P_RED, "Heart attack", -1, -1, INF, false, "PENDING"};
    requests.push_back(r4);
    engine.handleCriticalOverride(requests.back());
    displayDispatchLog(log, 5);

    // -- S5: Bed Release ------------------------------
    resetAmbulances();
    printSubHeader("SCENARIO 5 - Bed Release (Dynamic Update)");
    hospitals[1].availableBeds = 0;
    cout << "  " << hospitals[1].name << " is now FULL.\n";
    engine.addLog(hospitals[1].name + " marked FULL.", "WARN");
    cout << "  Simulating patient discharge...\n";
    engine.releaseBed(1, "Patient discharged from St. Mary's");
    cout << "  " << hospitals[1].name
         << " now has " << hospitals[1].availableBeds << " bed(s).\n";
    displayDispatchLog(log, 4);

    // -- S6: Surge Mode -------------------------------
    resetAmbulances();
    printSubHeader("SCENARIO 6 - Overload / Surge (Disaster Mode)");
    vector<EmergencyRequest> surge = {
        {100, "Patient A", 0, Priority::P_GREEN,  "Minor cut",     -1,-1,INF,false,"PENDING"},
        {101, "Patient B", 1, Priority::P_RED,    "Cardiac arrest",-1,-1,INF,false,"PENDING"},
        {102, "Patient C", 2, Priority::P_ORANGE, "Head trauma",   -1,-1,INF,false,"PENDING"},
        {103, "Patient D", 3, Priority::P_YELLOW, "Burns",         -1,-1,INF,false,"PENDING"},
        {104, "Patient E", 5, Priority::P_RED,    "Stroke",        -1,-1,INF,false,"PENDING"},
    };
    cout << "  " << surge.size() << " simultaneous emergencies incoming!\n";
    engine.handleSurge(surge);
    for (auto& s : surge) requests.push_back(s);
    displayDispatchLog(log, 6);

    // -- S7: Network Failure --------------------------
    resetAmbulances();
    printSubHeader("SCENARIO 7 - Road Block / Network Failure");
    cout << "  Blocking road: East Cross <-> City General (3->8)\n";
    EmergencyRequest r7 = {(int)requests.size(), "Sneha Rawat", 3,
                           Priority::P_ORANGE, "Accident", -1, -1, INF, false, "PENDING"};
    requests.push_back(r7);
    engine.handleNetworkFailure(requests.back(), 3, 8);
    displayDispatchLog(log, 5);
    // Restore edge
    g.addEdge(3, 8, 5);

    // -- S8: Load Balancing ---------------------------
    resetAmbulances();
    printSubHeader("SCENARIO 8 - Load Balancing");
    hospitals[3].availableBeds = 1;  // Trauma Centre nearly full
    hospitals[2].availableBeds = 15; // Northside under-utilised
    EmergencyRequest r8 = {(int)requests.size(), "Deepak Nair", 7,
                           Priority::P_YELLOW, "Sprain", -1, -1, INF, false, "PENDING"};
    requests.push_back(r8);
    engine.allocateWithLoadBalance(requests.back());
    hospitals[3].availableBeds = 5;
    displayDispatchLog(log, 4);

    // -- S9: Nearest vs Best --------------------------
    resetAmbulances();
    printSubHeader("SCENARIO 9 - Nearest vs Best Hospital");
    cout << "  Patient at node 7 (Airport Road), Priority RED\n";
    auto choice = engine.nearestVsBest(7, Priority::P_RED);
    cout << "  Nearest: " << COL_YELLOW << hospitals[choice.nearestId].name << RESET
         << "  (" << hospitals[choice.nearestId].availableBeds << " beds, "
         << (hospitals[choice.nearestId].hasIcu() ? "HAS ICU" : "NO ICU") << ")\n";
    cout << "  Best:    " << COL_GREEN << hospitals[choice.bestId].name << RESET
         << "  (" << hospitals[choice.bestId].availableBeds << " beds, "
         << (hospitals[choice.bestId].hasIcu() ? "HAS ICU" : "NO ICU") << ")\n";
    cout << "\n  " << BOLD << choice.recommendation << RESET << "\n";

    // -- No Hospital Available (Worst Case) -----------
    resetAmbulances();
    printSubHeader("SCENARIO 10 - No Hospital Available (Worst Case)");
    for (auto& h : hospitals) h.availableBeds = 0;
    EmergencyRequest r10 = {(int)requests.size(), "Worst Case Patient", 0,
                            Priority::P_RED, "Multi-trauma", -1, -1, INF, false, "PENDING"};
    requests.push_back(r10);
    engine.allocateNormal(requests.back());
    cout << "\n  Status: " << COL_RED << requests.back().status << RESET << "\n";
    cout << "  Action: Patient placed on priority waitlist.\n";
    cout << "         Notify nearest hospital to prepare emergency overflow.\n";
    displayDispatchLog(log, 3);
    // Restore
    for (auto& h : hospitals) {
        h.availableBeds = h.totalBeds / 2;
    }
}

// ---------------------------------------------
//  MAIN INTERACTIVE MENU
// ---------------------------------------------
void mainMenu(Graph& g,
              vector<Hospital>& hospitals,
              vector<Ambulance>& ambulances,
              vector<EmergencyRequest>& requests,
              vector<LogEntry>& log) {

    DispatchEngine engine(g, hospitals, ambulances, requests, log);
    int nextId = (int)requests.size();

    while (true) {
        printHeader("EMERGENCY HOSPITAL & AMBULANCE DISPATCH SYSTEM");
        cout << "\n"
             << BOLD << "  DISPATCH OPERATIONS\n" << RESET
             << "  " << CYAN << "1" << RESET << "  Register new emergency (patient intake + triage)\n"
             << "  " << CYAN << "2" << RESET << "  Run all scenario demonstrations\n"
             << "  " << CYAN << "3" << RESET << "  Manual dispatch (select patient + force allocate)\n"
             << "  " << CYAN << "4" << RESET << "  Release a hospital bed (patient discharged)\n"
             << "  " << CYAN << "5" << RESET << "  Block/restore a road (network failure test)\n"
             << "\n"
             << BOLD << "  VIEW\n" << RESET
             << "  " << CYAN << "6" << RESET << "  Operations dashboard (requests + staff + logs)\n"
             << "  " << CYAN << "7" << RESET << "  Hospital status board\n"
             << "  " << CYAN << "8" << RESET << "  Ambulance fleet\n"
             << "  " << CYAN << "11" << RESET << " Shortest path between two nodes\n"
             << "\n"
             << BOLD << "  ANALYSIS\n" << RESET
             << "  " << CYAN << "12" << RESET << " Nearest vs Best hospital for a location\n"
             << "\n"
             << "  " << CYAN << "0" << RESET << "  Exit\n\n";

        int choice;
        if (!readIntInRange("  Enter choice: ", choice, 0, 12)) return;
        cout << "\n";

        switch (choice) {
        case 1: {
            // New emergency with full MCQ triage
            EmergencyRequest req = collectPatientInfo(nextId++, g);
            requests.push_back(req);
            cout << "\n  Dispatching resources...\n";
            if (req.priority == Priority::P_RED)
                engine.handleCriticalOverride(requests.back());
            else
                engine.allocateNormal(requests.back());
            displayDispatchLog(log, 6);
            break;
        }
        case 2: {
            Graph demoGraph = g;
            vector<Hospital> demoHospitals = hospitals;
            vector<Ambulance> demoAmbulances = ambulances;
            vector<EmergencyRequest> demoRequests = requests;
            vector<LogEntry> demoLog = log;
            runScenarios(demoGraph, demoHospitals, demoAmbulances, demoRequests, demoLog);
            cout << "\n  Press ENTER to continue...";
            cin.get();
            break;
        }
        case 3: {
            // Manual quick dispatch
            printSubHeader("MANUAL DISPATCH");
            string name;
            int node, pri;
            cout << "  Patient name: "; getline(cin, name);
            readIntInRange("  Location node: ", node, 0, g.V - 1);
            readIntInRange("  Priority (1=RED 2=ORANGE 3=YELLOW 4=GREEN): ", pri, 1, 4);
            EmergencyRequest req = {nextId++, name, node,
                                    (Priority)pri, "Manual entry",
                                    -1, -1, INF, false, "PENDING"};
            requests.push_back(req);
            if (requests.back().priority == Priority::P_RED)
                engine.handleCriticalOverride(requests.back());
            else
                engine.allocateNormal(requests.back());
            displayDispatchLog(log, 5);
            break;
        }
        case 4: {
            printSubHeader("RELEASE BED");
            for (int i = 0; i < (int)hospitals.size(); i++)
                cout << "  " << i << ". " << hospitals[i].name
                     << " (" << hospitals[i].availableBeds << " free)\n";
            int hid;
            readIntInRange("  Hospital ID: ", hid, 0, (int)hospitals.size() - 1);
            engine.releaseBed(hid);
            break;
        }
        case 5: {
            printSubHeader("ROAD CONTROL");
            int action;
            readIntInRange("  Action (1=Block 2=Unblock): ", action, 1, 2);
            int u, v;
            readIntInRange("  Node A: ", u, 0, g.V - 1);
            readIntInRange("  Node B: ", v, 0, g.V - 1);

            if (action == 1) {
                if (g.blockRoad(u, v)) {
                    engine.addLog("Road blocked: " + g.name(u) + " <-> " + g.name(v), "WARN");
                    cout << COL_GREEN << "  Road blocked: "
                         << g.name(u) << " <-> " << g.name(v) << ".\n" << RESET;
                } else {
                    engine.addLog("Road block failed: " + g.name(u) + " <-> " + g.name(v), "WARN");
                    cout << COL_YELLOW << "  No direct road exists between those nodes.\n" << RESET;
                }
            } else {
                int minutes;
                readIntInRange("  Travel time in minutes: ", minutes, 1, 999);
                if (g.addEdge(u, v, minutes)) {
                    engine.addLog("Road unblocked: " + g.name(u) + " <-> " + g.name(v), "SUCCESS");
                    cout << COL_GREEN << "  Road unblocked: "
                         << g.name(u) << " <-> " << g.name(v)
                         << " (" << minutes << " min).\n" << RESET;
                } else {
                    engine.addLog("Road unblock failed: " + g.name(u) + " <-> " + g.name(v), "WARN");
                    cout << COL_RED << "  Road could not be restored.\n" << RESET;
                }
            }
            break;
        }
        case 6:
            displayOperationsDashboard(requests, hospitals, ambulances, log);
            break;
        case 7:
            displayHospitals(hospitals);
            break;
        case 8:
            displayAmbulances(ambulances);
            break;
        case 9:
            displayOperationsDashboard(requests, hospitals, ambulances, log);
            break;
        case 10:
            displayOperationsDashboard(requests, hospitals, ambulances, log);
            break;
        case 11: {
            int src, dst;
            readIntInRange("  From node: ", src, 0, g.V - 1);
            readIntInRange("  To node:   ", dst, 0, g.V - 1);
            auto res_dijsktra2 = g.dijkstra(src);
            auto dist = res_dijsktra2.first;
            auto prev = res_dijsktra2.second;
            cout << "\n";
            displayShortestPath(g, src, dst, dist, prev);
            break;
        }
        case 12: {
            int node, pri;
            readIntInRange("  From node: ", node, 0, g.V - 1);
            readIntInRange("  Priority (1=RED 2=ORANGE 3=YELLOW 4=GREEN): ", pri, 1, 4);
            auto choice = engine.nearestVsBest(node, (Priority)pri);
            cout << "\n";
            if (choice.nearestId >= 0)
                cout << "  Nearest: " << hospitals[choice.nearestId].name << "\n";
            if (choice.bestId >= 0)
                cout << "  Best:    " << hospitals[choice.bestId].name << "\n";
            cout << "  " << BOLD << choice.recommendation << RESET << "\n";
            break;
        }
        case 0:
            cout << CYAN << "\n  Emergency Dispatch System terminated.\n" << RESET;
            return;
        default:
            cout << COL_RED << "  Invalid option.\n" << RESET;
        }

        cout << "\n  Press ENTER to continue...";
        cin.get();
    }
}

// ---------------------------------------------
//  ENTRY POINT
// ---------------------------------------------
int main() {
    ios::sync_with_stdio(false);

    // 16-node city graph
    Graph g(16);
    vector<Hospital>         hospitals;
    vector<Ambulance>        ambulances;
    vector<EmergencyRequest> requests;
    vector<LogEntry>         log;

    setupCity(g, hospitals, ambulances);

    printHeader("HOSPITAL EMERGENCY ALLOCATION SYSTEM");
    cout << "\n  " << BOLD << "Design & Analysis of Algorithms - C++ Project\n" << RESET;
    cout << "  " << DIM << "Dijkstra . Priority Queues . Greedy Dispatch\n" << RESET;
    cout << "  " << DIM << "Graph: " << g.V << " nodes | "
         << hospitals.size() << " hospitals | "
         << ambulances.size() << " ambulances\n" << RESET;
    cout << "\n  Press ENTER to launch...";
    cin.get();

    mainMenu(g, hospitals, ambulances, requests, log);
    return 0;
}
