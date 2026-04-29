# RapidAid Emergency Bed and Ambulance Allocation

RapidAid is a C++17 Design and Analysis of Algorithms project with a Tkinter command-center frontend.
It models a 16-node city road network, uses Dijkstra shortest paths to route ambulances, scores hospitals
by distance/load/ICU availability, and demonstrates 10 emergency dispatch scenarios.

## Project Files

| File | Purpose |
| --- | --- |
| `types.h` | Graph, hospital, ambulance, request, log, and input helper structures |
| `dispatch.h` | Dispatch engine, hospital scoring, surge handling, preemption, network failure handling |
| `triage.h` | Six-question triage questionnaire that maps score to RED/ORANGE/YELLOW/GREEN |
| `display.h` | Console tables, status boards, routes, and dispatch logs |
| `main.cpp` | City setup, scenario demos, and interactive C++ menu |
| `gui.py` | Tkinter frontend with guided forms and automatic backend build/start |

## Build

Use this executable name on Windows:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -o rapid_aid.exe main.cpp
```

The older name `emergency_dispatch.exe` can trigger Windows UAC because it contains the word `patch`,
so this project now builds to `rapid_aid.exe`.

## Run Console

```powershell
.\rapid_aid.exe
```

## Run Frontend

```powershell
python gui.py
```

The frontend rebuilds `rapid_aid.exe` automatically when any C++ source/header file is newer than the
executable, starts the backend, and sends menu inputs through guided forms.

## Menu Summary

| Option | Action |
| --- | --- |
| 1 | Register emergency with MCQ triage and automatic dispatch |
| 2 | Run all 10 scenario demos |
| 3 | Manual dispatch |
| 4 | Release a hospital bed |
| 5 | Block a road |
| 6 | Operations dashboard: requests, staff queue, and recent logs |
| 7 | Hospital status board |
| 8 | Ambulance fleet |
| 11 | Shortest path finder |
| 12 | Nearest vs best hospital analysis |

## Verified Behaviors

- Dijkstra routing prints shortest paths between city nodes.
- Manual and triage RED cases use critical override logic.
- RED override can preempt a lower-priority ambulance when all units are busy.
- Load-balanced dispatch no longer indexes an invalid ambulance when no unit is available.
- Road blocking validates node IDs and avoids duplicate roads when restored.
