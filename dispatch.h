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

    bool commitDispatch(EmergencyRequest& req, int ambId, int hospId,
                        int travelTime, bool needIcu,
                        const string& logPrefix = "Dispatched") {
        if (ambId < 0 || ambId >= (int)ambulances.size() ||
            hospId < 0 || hospId >= (int)hospitals.size()) {
            req.status = "DISPATCH_ERROR";
            addLog("Dispatch failed for #" + to_string(req.id) +
                   " because a selected resource was invalid.", "WARN");
            return false;
        }

        Ambulance& ambulance = ambulances[ambId];
        Hospital& hospital = hospitals[hospId];
        if (!hospital.hasBed()) {
            req.status = "NO_HOSPITAL";
            addLog(hospital.name + " has no bed available for #" +
                   to_string(req.id), "WARN");
            return false;
        }

        ambulance.available = false;
        ambulance.assignedPatient = req.patientName;
        hospital.availableBeds--;
        if (needIcu && hospital.hasIcu()) hospital.availableIcuBeds--;

        req.assignedAmbulanceId = ambId;
        req.assignedHospitalId  = hospId;
        req.travelTime          = travelTime;
        req.resolved            = true;
        req.status              = "DISPATCHED";

        hospital.addIncoming(req.patientName, req.priority,
            ambulance.unitId, travelTime);

        addLog(logPrefix + " " + ambulance.unitId +
               " -> " + hospital.name +
               " (ETA ~" + to_string(travelTime) + " min)", "SUCCESS");
        return true;
    }

    // -- Find nearest available ambulance ------------------------------
    int findNearestAmbulance(const vector<int>& dist) {
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

        // Critical patients strongly prefer ICU capacity; lower priorities get
        // a small preference for lower-load hospitals instead of ICU.
        if (needIcu && h.hasIcu()) score -= 30;
        if (needIcu && !h.hasIcu()) score += 45;
        if (priority == Priority::P_ORANGE) score += (int)(h.loadFactor() * 10);
        if (priority == Priority::P_GREEN) score += (int)h.incomingQueue.size() * 2;

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

        if (!graph.validNode(req.locationNode)) {
            req.status = "INVALID_LOCATION";
            addLog("Invalid patient location for #" + to_string(req.id), "WARN");
            return false;
        }

        // Dijkstra from patient location
        auto res = graph.dijkstra(req.locationNode);
        auto dist = res.first;

        // Find nearest ambulance
        int ambId = findNearestAmbulance(dist);
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

        return commitDispatch(req, ambId, hospId, hospDist, needIcu);
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
        if (!graph.validNode(req.locationNode)) {
            req.status = "INVALID_LOCATION";
            addLog("Invalid RED patient location for #" + to_string(req.id), "WARN");
            return false;
        }

        auto res = graph.dijkstra(req.locationNode);
        auto dist = res.first;

        int ambId = findNearestAmbulance(dist);
        if (ambId < 0) {
            addLog("Attempting to reassign ambulance from lower-priority...", "WARN");
            int victimReq = -1;
            int victimPriority = (int)Priority::P_RED;
            for (auto& amb : ambulances) {
                for (auto& r : requests) {
                    if (!r.resolved) continue;
                    if (r.assignedAmbulanceId == amb.id &&
                        (int)r.priority > victimPriority) {
                        ambId = amb.id;
                        victimReq = r.id;
                        victimPriority = (int)r.priority;
                    }
                }
            }
            if (victimReq >= 0) {
                for (auto& r : requests) {
                    if (r.id == victimReq) {
                        if (r.assignedHospitalId >= 0 &&
                            r.assignedHospitalId < (int)hospitals.size()) {
                            hospitals[r.assignedHospitalId].releaseBed();
                            hospitals[r.assignedHospitalId].removeIncoming(r.patientName);
                        }
                        r.resolved = false;
                        r.status = "PREEMPTED_BY_RED";
                        r.assignedAmbulanceId = -1;
                        r.assignedHospitalId = -1;
                        r.travelTime = INF;
                        break;
                    }
                }
                ambulances[ambId].available = true;
                addLog("Reassigning " + ambulances[ambId].unitId +
                       " from lower-priority patient to RED override", "ALERT");
            }
        }

        if (ambId < 0) {
            req.status = "NO_AMBULANCE";
            addLog("No ambulance could be reassigned for RED patient #" +
                   to_string(req.id), "ALERT");
            return false;
        }

        auto hRes = findBestHospital(req.locationNode, req.priority, true);
        if (hRes.first < 0) {
            req.status = "NO_HOSPITAL";
            addLog("No hospital available for RED patient #" +
                   to_string(req.id), "ALERT");
            return false;
        }

        return commitDispatch(req, ambId, hRes.first, hRes.second,
                              true, "RED override dispatched");
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
        if (!graph.blockRoad(blockedU, blockedV)) {
            addLog("Road block ignored because the edge was invalid or absent.", "WARN");
        }

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
        if (!graph.validNode(req.locationNode)) {
            req.status = "INVALID_LOCATION";
            addLog("Invalid patient location for load-balanced request #" +
                   to_string(req.id), "WARN");
            return false;
        }

        auto res = graph.dijkstra(req.locationNode);
        auto dist = res.first;
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

        if (bestId < 0) return allocateNormal(req);

        int ambId = findNearestAmbulance(dist);
        if (ambId < 0) {
            req.status = "NO_AMBULANCE";
            addLog("No ambulance available for load-balanced request #" +
                   to_string(req.id), "WARN");
            return false;
        }

        int travelTime = dist[hospitals[bestId].locationNode];
        bool ok = commitDispatch(req, ambId, bestId, travelTime,
                                 req.priority == Priority::P_RED,
                                 "Load-balanced dispatch");
        if (!ok) return false;

        addLog("Load-balanced to " + hospitals[bestId].name +
               " (load: " + to_string((int)(hospitals[bestId].loadFactor()*100)) +
               "%, dist: " + to_string(travelTime) + ")", "SUCCESS");
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
        if (nearestId < 0 || bestId < 0) {
            rec = "No reachable hospital with available beds.";
        } else if (nearestId == bestId) {
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
