# Hospital Emergency Bed & Ambulance Allocation System
### Design & Analysis of Algorithms — C++ Project

---

## Project Files

| File | Purpose |
|------|---------|
| `types.h` | All data structures: Graph, Hospital, Ambulance, EmergencyRequest, Priority enum |
| `dispatch.h` | Core dispatch engine — all 10 scenarios using Dijkstra + Priority Queue |
| `triage.h` | MCQ triage questionnaire (6 questions → RED/ORANGE/YELLOW/GREEN) |
| `display.h` | Terminal UI — colored tables, dashboards, log output |
| `main.cpp` | City network setup, scenario runner, interactive menu |
| `.vscode/tasks.json` | VS Code build task (Ctrl+Shift+B) |
| `.vscode/launch.json` | VS Code debugger config (F5) |
| `.vscode/c_cpp_properties.json` | IntelliSense config |

---

## HOW TO SET UP IN VS CODE

### STEP 1 — Install VS Code
Download from: https://code.visualstudio.com/download
Install and open it.

---

### STEP 2 — Install C++ Extension
1. Open VS Code
2. Press `Ctrl+Shift+X` (Extensions panel)
3. Search: `C/C++`
4. Install the one by **Microsoft** (ms-vscode.cpptools)

---

### STEP 3 — Install a C++ Compiler

#### Windows (MinGW):
1. Download MinGW-w64 from: https://www.mingw-w64.org/downloads/
   - OR install via winget: open PowerShell and run:
     ```
     winget install -e --id=MSYS2.MSYS2
     ```
2. After MSYS2 installs, open **MSYS2 MinGW 64-bit** terminal and run:
   ```
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-gdb
   ```
3. Add to PATH: `C:\msys64\mingw64\bin`
   - Search "Environment Variables" in Windows Start
   - Edit PATH → Add `C:\msys64\mingw64\bin`
4. Verify in PowerShell: `g++ --version`

#### macOS:
```bash
xcode-select --install
```
Then verify: `g++ --version`

#### Linux (Ubuntu/Debian):
```bash
sudo apt update && sudo apt install g++ gdb
```

---

### STEP 4 — Open the Project Folder
1. Open VS Code
2. Go to `File → Open Folder`
3. Select the `emergency_system` folder (the one containing all .h and .cpp files)

You should see all 5 source files in the Explorer panel on the left.

---

### STEP 5 — Build the Project

**Method A — Keyboard shortcut:**
Press `Ctrl+Shift+B`
This runs the build task from `.vscode/tasks.json`

**Method B — Terminal:**
1. Open integrated terminal: `Ctrl+` ` (backtick)
2. Run:
```bash
g++ -std=c++17 -O2 -o emergency_dispatch main.cpp
```

A file called `emergency_dispatch` (Linux/Mac) or `emergency_dispatch.exe` (Windows) will appear.

---

### STEP 6 — Run the Program

In the VS Code terminal:

**Linux / Mac:**
```bash
./emergency_dispatch
```

**Windows:**
```bash
emergency_dispatch.exe
```

**IMPORTANT for Windows users:**
The program uses ANSI color codes. Enable them by running in Windows Terminal
(not the old cmd.exe). Or run this once in PowerShell:
```
Set-ItemProperty HKCU:\Console VirtualTerminalLevel -Type DWORD 1
```

---

### STEP 7 — Debug with F5 (Optional)
1. Set a breakpoint by clicking left of a line number
2. Press `F5`
3. VS Code will build and launch with the debugger attached

---

## PROGRAM MENU GUIDE

```
1  → Register new emergency      (enter patient name → MCQ triage → auto-dispatch)
2  → Run all 10 scenario demos   (shows every algorithm scenario)
3  → Manual quick dispatch
4  → Release a hospital bed      (simulate patient discharge)
5  → Block a road                (simulate network failure)
6  → View all requests
7  → Hospital status board
8  → Ambulance fleet
9  → Hospital staff dashboard    (incoming patient queue per hospital)
10 → Dispatch log
11 → Shortest path finder
12 → Nearest vs Best hospital analysis
13 → Network load overview
0  → Exit
```

---

## TRIAGE PRIORITY LEVELS

| Level | Color | Condition | Action |
|-------|-------|-----------|--------|
| 1 | RED | Heart attack, critical, unresponsive | Immediate dispatch, ICU preferred |
| 2 | ORANGE | Serious trauma, altered consciousness | Urgent dispatch |
| 3 | YELLOW | Moderate injury, stable | Standard dispatch |
| 4 | GREEN | Minor injury, walking wounded | Queued dispatch |

---

## ALGORITHM SUMMARY

| Algorithm | Where Used |
|-----------|-----------|
| Dijkstra's Shortest Path | Finding nearest ambulance + best hospital |
| Priority Queue (min-heap) | Dijkstra implementation, triage ordering |
| Greedy Scoring | Hospital selection (distance + load + ICU) |
| Graph BFS/reachability | Network failure detection |
| Sorting by priority | Surge mode batch allocation |

---

## CITY NETWORK (16 nodes)

```
Node  0  — Central Square        Node  8  — City General Hospital
Node  1  — North Junction        Node  9  — St. Mary's Hospital
Node  2  — South Market          Node 10  — Northside Hospital
Node  3  — East Cross            Node 11  — Trauma Centre
Node  4  — West End              Node 12  — Ambulance Depot A
Node  5  — University District   Node 13  — Ambulance Depot B
Node  6  — Industrial Park       Node 14  — River Bridge
Node  7  — Airport Road          Node 15  — Suburb Area
```
