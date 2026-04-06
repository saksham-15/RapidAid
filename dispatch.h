#pragma once
#include "types.h"

class DispatchEngine {
public:
    Graph& graph;
    vector<Hospital>& hospitals;
    vector<Ambulance>& ambulances;
    vector<EmergencyRequest>& requests;
    vector<LogEntry>& log;

    DispatchEngine(Graph& g, vector<Hospital>& h,
                   vector<Ambulance>& a, vector<EmergencyRequest>& r,
                   vector<LogEntry>& l)
        : graph(g), hospitals(h), ambulances(a), requests(r), log(l) {}

    void addLog(const string& msg, const string& type = "INFO") {
        log.push_back({currentTime(), msg, type});
    }

    // -- Find nearest available ambulance ------------------------------
    int findNearestAmbulance(int fromNode, const vector<int>& dist) {
        int best = -1, bestDist = INF;
        for (auto& amb : ambulances) {
            if (!amb.available) continue;
            if (dist[amb.locationNode] < bestDist) {
                bestDist = dist[amb.locationNode];
                best = amb.id;
            }
        }
        return best;
    }

    // -- Score a hospital (lower = better) -----------------------------
    // Factors: distance, load factor, bed availability, ICU for RED
    int scoreHospital(const Hospital& h, int distToHosp,
                      Priority priority, bool needIcu) {
        if (!h.hasBed()) return INF;
        if (!h.operational) return INF;
        if (distToHosp == INF) return INF;

        int score = distToHosp;

        // Penalise high load
        score += (int)(h.loadFactor() * 50);

        // Bonus if ICU available for COL_RED
        if (needIcu && h.hasIcu()) score -= 30;

        // Load balancing: penalise hospitals with incoming queue
        score += (int)h.incomingQueue.size() * 5;

        return score;
    }

    // -- Find best hospital for a request ------------------------------
    // Returns {hospitalId, distToHospital}
    pair<int,int> findBestHospital(int fromNode,
                                   Priority priority,
                                   bool preferIcu = false) {
        auto res = graph.dijkstra(fromNode);
        auto dist = res.first;
        auto prev = res.second;
        int bestId = -1, bestScore = INF;

        for (auto& h : hospitals) {
            if (!h.operational || !h.hasBed()) continue;
            int d = dist[h.locationNode];
            if (d == INF) continue;

            int score = scoreHospital(h, d, priority, preferIcu);
            if (score < bestScore) {
                bestScore = score;
                bestId = h.id;
            }
        }
        return {bestId, bestId >= 0 ? dist[hospitals[bestId].locationNode] : INF};
    }

    // -- SCENARIO 1: Normal Allocation ---------------------------------
    bool allocateNormal(EmergencyRequest& req) {
        addLog("Processing request #" + to_string(req.id) +
               " for " + req.patientName, "INFO");

        // Dijkstra from patient location
        auto res = graph.dijkstra(req.locationNode);
        auto dist = res.first;
        auto prev = res.second;

        // Find nearest ambulance
        int ambId = findNearestAmbulance(req.locationNode, dist);
        if (ambId < 0) {
            addLog("No ambulance available for #" + to_string(req.id), "WARN");
            req.status = "NO_AMBULANCE";
            return false;
        }

        bool needIcu = (req.priority == Priority::P_RED);
        auto hRes = findBestHospital(req.locationNode,
                                                    req.priority, needIcu);
        int hospId = hRes.first;
        int hospDist = hRes.second;
        if (hospId < 0) {
            addLog("No hospital available for #" + to_string(req.id), "WARN");
            req.status = "NO_HOSPITAL";
            return false;
        }

        // Assign resources
        ambulances[ambId].available = false;
        ambulances[ambId].assignedPatient = req.patientName;
        hospitals[hospId].availableBeds--;
        if (needIcu && hospitals[hospId].hasIcu())
            hospitals[hospId].availableIcuBeds--;

        req.assignedAmbulanceId = ambId;
        req.assignedHospitalId  = hospId;
        req.travelTime          = hospDist;
        req.resolved            = true;
        req.status              = "DISPATCHED";

        // Notify hospital
        hospitals[hospId].addIncoming(req.patientName, req.priority,
            ambulances[ambId].unitId, hospDist);

        addLog("Dispatched " + ambulances[ambId].unitId +
               " -> " + hospitals[hospId].name +
               " (ETA ~" + to_string(hospDist) + " min)", "SUCCESS");
        return true;
    }

    // -- SCENARIO 2: Hospital Full - redirect --------------------------
    bool handleHospitalFull(EmergencyRequest& req) {
        addLog("All nearby hospitals full - searching extended network...", "WARN");
        // Disable full hospitals temporarily, re-run allocation
        for (auto& h : hospitals) {
            if (h.availableBeds == 0) h.operational = false;
        }
        bool ok = allocateNormal(req);
        for (auto& h : hospitals)
            if (h.availableBeds == 0) h.operational = true;

        if (!ok) {
            req.status = "ALL_FULL";
            addLog("CRITICAL: All hospitals full - patient queued!", "ALERT");
        }
        return ok;
    }

    // -- SCENARIO 3: Multiple Equidistant Hospitals --------------------
    // Returns the one with most capacity
    int breakTieByCapacity(const vector<int>& candidates) {
        int best = -1, bestBeds = -1;
        for (int id : candidates) {
            if (hospitals[id].availableBeds > bestBeds) {
                bestBeds = hospitals[id].availableBeds;
                best = id;
            }
        }
        return best;
    }

    // -- SCENARIO 4: Critical Override ---------------------------------
    // RED priority always gets the nearest available ICU, overriding queue
    bool handleCriticalOverride(EmergencyRequest& req) {
        addLog("CRITICAL OVERRIDE: Priority RED patient - " + req.patientName, "ALERT");
        req.priority = Priority::P_RED;
        auto res = graph.dijkstra(req.locationNode);
        auto dist = res.first;
        auto prev = res.second;

        // Nearest ambulance (including ones marked busy for lower-priority)
        int ambId = -1, bestAmbDist = INF;
        for (auto& amb : ambulances) {
            // For RED, we reassign from GREEN/YELLOW
            if (!amb.available) continue;
            if (dist[amb.locationNode] < bestAmbDist) {
                bestAmbDist = dist[amb.locationNode];
                ambId = amb.id;
            }
        }
        if (ambId < 0) {
            addLog("Attempting to reassign ambulance from lower-priority...", "WARN");
            // Find ambulance assigned to GREEN patient
            for (auto& amb : ambulances) {
                // Check if assigned patient is COL_GREEN
                for (auto& r : requests) {
                    if (!r.resolved) continue;
                    if (r.assignedAmbulanceId == amb.id &&
                        r.priority == Priority::P_GREEN) {
                        addLog("Reassigning " + amb.unitId +
                               " from GREEN patient to RED override", "ALERT");
                        ambId = amb.id;
                        break;
                    }
                }
                if (ambId >= 0) break;
            }
        }
        return allocateNormal(req);
    }

    // -- SCENARIO 5: Bed Release (Dynamic Update) ----------------------
    void releaseBed(int hospitalId, const string& reason = "Patient discharged") {
        if (hospitalId < 0 || hospitalId >= (int)hospitals.size()) return;
        hospitals[hospitalId].releaseBed();
        addLog(hospitals[hospitalId].name + " - " + reason +
               ". Beds now: " + to_string(hospitals[hospitalId].availableBeds), "SUCCESS");

        // Re-check any pending requests
        for (auto& req : requests) {
            if (!req.resolved && req.status == "ALL_FULL") {
                addLog("Retrying pending request #" + to_string(req.id) +
                       " after bed release...", "INFO");
                allocateNormal(req);
            }
        }
    }

    // -- SCENARIO 6: Surge / Disaster Mode ----------------------------
    // Processes all pending requests sorted by priority
    void handleSurge(vector<EmergencyRequest>& incoming) {
        addLog("=== SURGE MODE ACTIVATED ===", "ALERT");
        sort(incoming.begin(), incoming.end(),
            [](const EmergencyRequest& a, const EmergencyRequest& b) {
                return (int)a.priority < (int)b.priority;
            });
        int allocated = 0, failed = 0;
        for (auto& req : incoming) {
            if (allocateNormal(req)) allocated++;
            else failed++;
        }
        addLog("Surge handled: " + to_string(allocated) + " allocated, " +
               to_string(failed) + " pending.", failed > 0 ? "WARN" : "SUCCESS");
    }

    // -- SCENARIO 7: Network Failure (Road Blocked) --------------------
    bool handleNetworkFailure(EmergencyRequest& req, int blockedU, int blockedV) {
        addLog("Road blocked: " + graph.name(blockedU) +
               " <-> " + graph.name(blockedV) + " - rerouting...", "WARN");
        graph.blockRoad(blockedU, blockedV);

        // Check reachability
        bool reached = false;
        for (auto& h : hospitals) {
            if (graph.isReachable(req.locationNode, h.locationNode) && h.hasBed()) {
                reached = true; break;
            }
        }
        bool ok = reached ? allocateNormal(req) : false;
        if (!ok) {
            req.status = "UNREACHABLE";
            addLog("Hospital UNREACHABLE from " +
                   graph.name(req.locationNode) + "!", "ALERT");
        }
        return ok;
    }

    // -- SCENARIO 8: Load Balancing ------------------------------------
    // Redirect to under-utilised hospital even if slightly farther
    bool allocateWithLoadBalance(EmergencyRequest& req, float overloadThreshold = 0.75f) {
        auto res = graph.dijkstra(req.locationNode);
        auto dist = res.first;
        auto prev = res.second;
        int bestId = -1, bestScore = INF;

        for (auto& h : hospitals) {
            if (!h.operational || !h.hasBed()) continue;
            int d = dist[h.locationNode];
            if (d == INF) continue;

            int score = d;
            // Heavy penalty for over-loaded hospital
            if (h.loadFactor() > overloadThreshold)
                score += 100;

            score += (int)(h.loadFactor() * 60);
            if (score < bestScore) { bestScore = score; bestId = h.id; }
        }

        if (bestId < 0) { allocateNormal(req); return false; }

        auto res2 = graph.dijkstra(req.locationNode);
        auto dist2 = res2.first;
        auto prev2 = res2.second;
        ambulances[findNearestAmbulance(req.locationNode, dist2)].available = false;
        hospitals[bestId].availableBeds--;
        req.assignedHospitalId = bestId;
        req.status = "DISPATCHED";
        req.resolved = true;
        hospitals[bestId].addIncoming(req.patientName, req.priority,
            "AMB-LB", dist[hospitals[bestId].locationNode]);

        addLog("Load-balanced to " + hospitals[bestId].name +
               " (load: " + to_string((int)(hospitals[bestId].loadFactor()*100)) +
               "%, dist: " + to_string(dist[hospitals[bestId].locationNode]) + ")", "SUCCESS");
        return true;
    }

    // -- SCENARIO 9: Nearest vs Best (ICU-aware) ----------------------
    // Returns {nearestId, bestId, recommendation}
    struct HospitalChoice {
        int nearestId, bestId;
        string recommendation;
    };

    HospitalChoice nearestVsBest(int fromNode, Priority priority) {
        auto res = graph.dijkstra(fromNode);
        auto dist = res.first;
        auto prev = res.second;
        int nearestId = -1, nearestDist = INF;
        int bestId = -1, bestScore = INF;

        for (auto& h : hospitals) {
            if (!h.operational || !h.hasBed()) continue;
            int d = dist[h.locationNode];
            if (d == INF) continue;

            // Nearest
            if (d < nearestDist) { nearestDist = d; nearestId = h.id; }

            // Best (score considers ICU, capacity)
            int score = d;
            if (priority == Priority::P_RED && !h.hasIcu()) score += 80;
            score += (int)(h.loadFactor() * 40);
            if (score < bestScore) { bestScore = score; bestId = h.id; }
        }

        string rec;
        if (nearestId == bestId) {
            rec = "Nearest and best are the same hospital.";
        } else if (priority == Priority::P_RED &&
                   hospitals[bestId].hasIcu() &&
                   !hospitals[nearestId].hasIcu()) {
            rec = "RECOMMEND farther hospital (has ICU) for RED priority.";
        } else if (hospitals[nearestId].availableBeds == 1 &&
                   hospitals[bestId].availableBeds > 5) {
            rec = "RECOMMEND farther hospital (more capacity, avoids blocking).";
        } else {
            rec = "Go to nearest hospital (sufficient for this case).";
        }
        return {nearestId, bestId, rec};
    }
};
