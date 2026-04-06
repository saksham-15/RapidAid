#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <limits>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <functional>
#include <ctime>

using namespace std;

// ---------------------------------------------
//  TRIAGE PRIORITY (4 levels with color codes)
// ---------------------------------------------
enum class Priority {
    P_RED = 1,  // Heart attack / Critical
    P_ORANGE = 2,  // Serious - urgent but stable
    P_YELLOW = 3,  // Moderate
    P_GREEN = 4    // Minor
};

string priorityName(Priority p) {
    switch (p) {
        case Priority::P_RED:    return "RED (Heart/Critical)";
        case Priority::P_ORANGE: return "ORANGE (Serious)";
        case Priority::P_YELLOW: return "YELLOW (Moderate)";
        case Priority::P_GREEN:  return "GREEN (Minor)";
    }
    return "UNKNOWN";
}

string priorityColor(Priority p) {
    switch (p) {
        case Priority::P_RED:    return "\033[1;31m"; // bold red
        case Priority::P_ORANGE: return "\033[1;33m"; // bold yellow (orange proxy)
        case Priority::P_YELLOW: return "\033[0;33m"; // yellow
        case Priority::P_GREEN:  return "\033[0;32m"; // green
    }
    return "\033[0m";
}

string priorityTag(Priority p) {
    switch (p) {
        case Priority::P_RED:    return "[* RED   ]";
        case Priority::P_ORANGE: return "[* ORANGE]";
        case Priority::P_YELLOW: return "[* YELLOW]";
        case Priority::P_GREEN:  return "[* GREEN ]";
    }
    return "[?]";
}

#define RESET  "\033[0m"
#define BOLD   "\033[1m"
#define CYAN   "\033[1;36m"
#define WHITE  "\033[1;37m"
#define COL_RED    "\033[1;31m"
#define COL_GREEN  "\033[0;32m"
#define COL_YELLOW "\033[0;33m"
#define COL_BLUE   "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define DIM    "\033[2m"

// ---------------------------------------------
//  GRAPH (Road Network)
// ---------------------------------------------
const int INF = numeric_limits<int>::max();

struct Edge {
    int to, weight;
};

struct Graph {
    int V;
    vector<vector<Edge>> adj;
    map<int, string> nodeNames;

    Graph(int v) : V(v), adj(v) {}

    void addEdge(int u, int v, int w) {
        adj[u].push_back({v, w});
        adj[v].push_back({u, w}); // undirected
    }

    void setName(int node, const string& name) {
        nodeNames[node] = name;
    }

    string name(int node) const {
        auto it = nodeNames.find(node);
        return it != nodeNames.end() ? it->second : "Node-" + to_string(node);
    }

    // Dijkstra - returns {dist[], prev[]}
    pair<vector<int>, vector<int>> dijkstra(int src) const {
        vector<int> dist(V, INF), prev(V, -1);
        priority_queue<pair<int,int>, vector<pair<int,int>>, greater<pair<int,int>>> pq;
        dist[src] = 0;
        pq.push({0, src});

        while (!pq.empty()) {
            auto top_el = pq.top(); pq.pop();
            int d = top_el.first;
            int u = top_el.second;
            if (d > dist[u]) continue;
            for (auto& e : adj[u]) {
                if (dist[u] + e.weight < dist[e.to]) {
                    dist[e.to] = dist[u] + e.weight;
                    prev[e.to] = u;
                    pq.push(make_pair(dist[e.to], e.to));
                }
            }
        }
        return {dist, prev};
    }

    // Reconstruct path from src to dst
    vector<int> getPath(const vector<int>& prev, int dst) const {
        vector<int> path;
        for (int at = dst; at != -1; at = prev[at])
            path.push_back(at);
        reverse(path.begin(), path.end());
        return path;
    }

    // Check reachability
    bool isReachable(int src, int dst) const {
        auto res = dijkstra(src);
        auto dist = res.first;
        return dist[dst] != INF;
    }

    // Block a road (remove edge) for network failure scenarios
    void blockRoad(int u, int v) {
        adj[u].erase(remove_if(adj[u].begin(), adj[u].end(),
            [v](const Edge& e){ return e.to == v; }), adj[u].end());
        adj[v].erase(remove_if(adj[v].begin(), adj[v].end(),
            [u](const Edge& e){ return e.to == u; }), adj[v].end());
    }
};

// ---------------------------------------------
//  HOSPITAL
// ---------------------------------------------
struct Hospital {
    int id;
    string name;
    int locationNode;
    int totalBeds;
    int availableBeds;
    int icuBeds;
    int availableIcuBeds;
    bool operational; // false = network failure

    // Incoming patient queue (hospital staff view)
    struct IncomingPatient {
        string name;
        Priority priority;
        string ambulanceId;
        int eta; // minutes
    };
    vector<IncomingPatient> incomingQueue;

    bool hasBed() const  { return operational && availableBeds > 0; }
    bool hasIcu() const  { return operational && availableIcuBeds > 0; }
    float loadFactor() const {
        if (totalBeds == 0) return 1.0f;
        return 1.0f - (float)availableBeds / totalBeds;
    }

    void addIncoming(const string& patientName, Priority p,
                     const string& ambId, int eta) {
        incomingQueue.push_back({patientName, p, ambId, eta});
        // Sort by priority then ETA
        sort(incomingQueue.begin(), incomingQueue.end(),
            [](const IncomingPatient& a, const IncomingPatient& b) {
                if ((int)a.priority != (int)b.priority)
                    return (int)a.priority < (int)b.priority;
                return a.eta < b.eta;
            });
    }

    void releaseBed() {
        if (availableBeds < totalBeds) availableBeds++;
    }
};

// ---------------------------------------------
//  AMBULANCE
// ---------------------------------------------
struct Ambulance {
    int id;
    string unitId;
    int locationNode;
    bool available;
    string assignedPatient;

    bool isAvailable() const { return available; }
};

// ---------------------------------------------
//  EMERGENCY REQUEST
// ---------------------------------------------
struct EmergencyRequest {
    int id;
    string patientName;
    int locationNode;
    Priority priority;
    string symptoms;

    // Assigned resources
    int assignedAmbulanceId;
    int assignedHospitalId;
    int travelTime;
    bool resolved;
    string status;

    EmergencyRequest() 
        : assignedAmbulanceId(-1), assignedHospitalId(-1), 
          travelTime(INF), resolved(false), status("PENDING") {}

    EmergencyRequest(int _id, string _pn, int _ln, Priority _pr, string _sym,
                     int _aId = -1, int _hId = -1, int _tT = INF, 
                     bool _res = false, string _stat = "PENDING")
        : id(_id), patientName(_pn), locationNode(_ln), priority(_pr), 
          symptoms(_sym), assignedAmbulanceId(_aId), assignedHospitalId(_hId), 
          travelTime(_tT), resolved(_res), status(_stat) {}
};

// ---------------------------------------------
//  DISPATCH LOG ENTRY
// ---------------------------------------------
struct LogEntry {
    string timestamp;
    string message;
    string type; // INFO, WARN, ALERT, SUCCESS
};

string currentTime() {
    time_t now = time(nullptr);
    char buf[20];
    strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&now));
    return string(buf);
}
