# RapidAid Emergency Bed and Ambulance Allocation System

## 1. Introduction

RapidAid is an emergency healthcare dispatch system designed to allocate ambulances and hospital beds efficiently during medical emergencies. The project models a city as a weighted graph, where locations are represented as nodes and roads are represented as edges with travel time as weight.

The system uses graph algorithms, priority-based triage, hospital capacity tracking, and ambulance availability to decide where a patient should be sent and which ambulance should be assigned.

This project is implemented using C++ for the backend logic and Python Tkinter for the desktop frontend.

## 2. Project Objective

The main objective of this project is to simulate a real-time emergency dispatch system that can:

- Register emergency patients.
- Assign medical priority using triage.
- Find the nearest available ambulance.
- Select the most suitable hospital.
- Track available hospital beds and ICU beds.
- Handle blocked roads and rerouting.
- Display hospital, ambulance, and operations data clearly.
- Demonstrate important Design and Analysis of Algorithms concepts.

## 3. Technologies Used

| Component | Technology |
| --- | --- |
| Backend | C++17 |
| Frontend | Python Tkinter |
| Compiler | MinGW g++ |
| Main Algorithm | Dijkstra Shortest Path |
| Data Structures | Graph, Vector, Map, Priority Queue |
| Interface Type | Desktop GUI and Console Backend |

## 4. System Modules

### 4.1 `types.h`

This file contains the core data structures used by the system:

- `Graph`
- `Hospital`
- `Ambulance`
- `EmergencyRequest`
- `LogEntry`
- `Priority`

It also contains the Dijkstra shortest path implementation used to calculate travel time between city locations.

### 4.2 `dispatch.h`

This file contains the main dispatch logic through the `DispatchEngine` class.

Major responsibilities:

- Find nearest available ambulance.
- Score hospitals.
- Allocate ambulance and hospital.
- Handle critical RED priority override.
- Handle surge mode.
- Handle road/network failure.
- Perform load-balanced dispatch.
- Maintain dispatch logs.

### 4.3 `triage.h`

This file handles patient triage using a six-question multiple-choice form.

Based on the total triage score, patients are assigned one of four priority levels:

| Priority | Meaning |
| --- | --- |
| RED | Critical emergency |
| ORANGE | Serious condition |
| YELLOW | Moderate condition |
| GREEN | Minor condition |

### 4.4 `display.h`

This file handles output formatting and display tables.

It includes:

- Hospital status table
- Ambulance fleet table
- Operations dashboard
- Staff queue display
- Dispatch logs
- Shortest path display

The tables are formatted using bordered ASCII tables for better readability.

### 4.5 `main.cpp`

This is the main entry point of the backend program.

It performs:

- City graph setup
- Hospital and ambulance initialization
- Interactive backend menu
- Scenario demonstration runner
- User command handling

### 4.6 `gui.py`

This is the Python Tkinter frontend.

It provides:

- Quick Dispatch form
- Full Triage Form
- Hospital status view
- Ambulance fleet view
- Operations Dashboard
- Road Control
- Release Bed
- Shortest Path
- Nearest vs Best analysis
- Backend restart and exit controls

## 5. City Network

The city is represented using 16 nodes.

| Node | Location |
| --- | --- |
| 0 | Central Square |
| 1 | North Junction |
| 2 | South Market |
| 3 | East Cross |
| 4 | West End |
| 5 | University District |
| 6 | Industrial Park |
| 7 | Airport Road |
| 8 | City General Hospital |
| 9 | St. Mary's Hospital |
| 10 | Northside Hospital |
| 11 | Trauma Centre |
| 12 | Ambulance Depot A |
| 13 | Ambulance Depot B |
| 14 | River Bridge |
| 15 | Suburb Area |

Roads between these locations are weighted by travel time in minutes.

## 6. Hospitals

The system has four hospitals:

| Hospital | Node | Total Beds | ICU Beds |
| --- | --- | --- | --- |
| City General | 8 | 20 | 4 |
| St. Mary's | 9 | 15 | 2 |
| Northside | 10 | 25 | 6 |
| Trauma Centre | 11 | 10 | 8 |

The Hospital feature displays:

- Available beds
- Total beds
- Available ICU beds
- Total ICU beds
- Hospital readiness score

## 7. Ambulances

The system has five ambulances:

| Ambulance | Starting Node |
| --- | --- |
| AMB-01 | 12 |
| AMB-02 | 13 |
| AMB-03 | 0 |
| AMB-04 | 4 |
| AMB-05 | 7 |

The Ambulance feature displays:

- Number of available ambulances
- Number of busy ambulances
- Ambulance ID
- Location
- Status
- Assigned patient

## 8. Main Features

### 8.1 Quick Dispatch

Quick Dispatch allows the user to quickly enter:

- Patient name
- Location
- Priority

The system then automatically assigns an ambulance and hospital.

### 8.2 Full Triage Form

The Full Triage Form uses multiple-choice questions to calculate the patient priority automatically.

This is useful when the priority is not known manually.

### 8.3 Hospital Feature

The Hospital feature displays hospital capacity and readiness.

It shows:

- Available beds
- Total beds
- Available ICU beds
- Total ICU beds
- Score

The score represents hospital readiness based on bed and ICU availability.

### 8.4 Ambulance Feature

The Ambulance feature displays ambulance availability and assignments.

It shows:

- Available ambulance count
- Busy ambulance count
- Individual ambulance status

### 8.5 Operations Dashboard

The Operations Dashboard merges the earlier Requests, Staff, and Logs features into one screen.

It displays:

- Patient details
- Priority
- Assigned ambulance
- Assigned hospital
- ETA
- Dispatch status
- Staff queue position
- Recent system logs

This makes the system easier to use because all important operational information appears in one place.

### 8.6 Release Bed

The Release Bed feature simulates a patient discharge.

After releasing a bed, the system shows the updated available bed count for the selected hospital.

### 8.7 Road Control

Road Control supports:

- Blocking a road
- Unblocking a road

When unblocking, the system only shows roads that are already blocked. If no road is blocked, it displays:

```text
No Road is Blocked
```

### 8.8 Shortest Path

The Shortest Path feature finds the fastest route between two city nodes using Dijkstra's algorithm.

### 8.9 Nearest vs Best

This feature compares:

- The nearest hospital by distance
- The best hospital by overall score

The best hospital considers:

- Distance
- Bed availability
- ICU availability
- Hospital load

This is useful because the closest hospital may not always be the safest or most suitable hospital.

### 8.10 Scenario Demonstrations

The project includes 10 emergency scenarios:

1. Normal allocation
2. Hospital full
3. Multiple equidistant hospitals
4. Critical RED override
5. Bed release
6. Surge mode
7. Road block/network failure
8. Load balancing
9. Nearest vs best hospital
10. No hospital available

Scenario mode runs on copied demo data, so it does not affect live system data.

## 9. Algorithms Used

### 9.1 Dijkstra Shortest Path Algorithm

Dijkstra's algorithm is used to calculate shortest travel times from a patient location to all other city nodes.

It is used for:

- Finding nearest ambulance
- Finding nearest hospital
- Checking route availability
- Shortest path feature
- Network failure handling

Time complexity:

```text
O((V + E) log V)
```

Where:

- `V` is number of nodes
- `E` is number of roads

### 9.2 Greedy Hospital Scoring

Hospitals are scored based on:

- Distance
- Available beds
- ICU availability
- Incoming patient queue

The hospital with the best score is selected.

### 9.3 Priority-Based Dispatch

Patients are handled according to medical priority:

```text
RED > ORANGE > YELLOW > GREEN
```

RED patients can trigger critical override logic.

### 9.4 Sorting

Sorting is used in:

- Surge mode
- Hospital incoming patient queue

Patients are sorted by priority and ETA.

## 10. Critical Override

For RED priority patients, the system gives special treatment.

If all ambulances are busy, the system can preempt a lower-priority patient and reassign that ambulance to the RED patient.

This models real emergency behavior where critical patients are handled first.

## 11. Road Blocking and Unblocking

Road blocking removes an edge from the graph.

Road unblocking restores the original edge with its original travel time.

This allows the system to simulate:

- Accidents
- Floods
- Closed roads
- Rerouting conditions

## 12. Testing Performed

The following features were tested:

- Backend startup and exit
- Quick dispatch
- Full triage dispatch
- Hospital table
- Ambulance table
- Operations dashboard
- Release bed
- Road block
- Road unblock
- No-road-blocked condition
- Shortest path
- Nearest vs best
- Scenario demonstrations
- RED priority override
- Ambulance preemption
- GUI syntax
- C++ build

## 13. Advantages

- Easy-to-use GUI
- Clear hospital and ambulance views
- Uses real DAA algorithms
- Supports emergency priority handling
- Handles blocked roads
- Supports road unblocking
- Combines requests, staff, and logs into one dashboard
- Scenario mode is isolated from live data

## 14. Limitations

- Data is stored in memory only.
- Data resets when the backend restarts.
- It is a simulation, not connected to real maps or hospitals.
- No database is used.
- Travel times are fixed manually.

## 15. Future Enhancements

Possible future improvements:

- Add database storage.
- Add login roles for dispatcher and hospital staff.
- Add map visualization.
- Add real GPS/map API integration.
- Add report export to PDF.
- Add patient discharge workflow linked to actual patient records.
- Add ambulance return-to-base feature.

## 16. Conclusion

RapidAid successfully demonstrates how graph algorithms and priority-based decision making can be applied to emergency healthcare dispatch. The system finds ambulances, selects hospitals, handles critical cases, manages beds, supports road control, and displays operational data through a desktop GUI.

The project is a strong demonstration of Dijkstra's algorithm, greedy decision making, sorting, graph modification, and real-world emergency resource allocation.
