import queue
import re
import subprocess
import threading
import time
import tkinter as tk
from pathlib import Path
from tkinter import messagebox, ttk


PROJECT_DIR = Path(__file__).resolve().parent
EXE_NAME = "rapid_aid.exe"
SOURCE_FILES = ["main.cpp", "types.h", "display.h", "triage.h", "dispatch.h"]

NODES = [
    (0, "Central Square"),
    (1, "North Junction"),
    (2, "South Market"),
    (3, "East Cross"),
    (4, "West End"),
    (5, "Univ. District"),
    (6, "Industrial Park"),
    (7, "Airport Road"),
    (8, "City General Hosp."),
    (9, "St. Mary's Hosp."),
    (10, "Northside Hosp."),
    (11, "Trauma Centre"),
    (12, "Amb. Depot A"),
    (13, "Amb. Depot B"),
    (14, "River Bridge"),
    (15, "Suburb Area"),
]

HOSPITALS = [
    (0, "City General"),
    (1, "St. Mary's"),
    (2, "Northside"),
    (3, "Trauma Centre"),
]

DEFAULT_AVAILABLE_BEDS = {
    0: 12,
    1: 8,
    2: 18,
    3: 5,
}

ROADS = [
    (0, 1, "Central Square <-> North Junction"),
    (0, 2, "Central Square <-> South Market"),
    (0, 3, "Central Square <-> East Cross"),
    (0, 4, "Central Square <-> West End"),
    (1, 5, "North Junction <-> Univ. District"),
    (1, 10, "North Junction <-> Northside Hosp."),
    (2, 6, "South Market <-> Industrial Park"),
    (2, 9, "South Market <-> St. Mary's Hosp."),
    (3, 7, "East Cross <-> Airport Road"),
    (3, 8, "East Cross <-> City General Hosp."),
    (4, 6, "West End <-> Industrial Park"),
    (4, 12, "West End <-> Amb. Depot A"),
    (5, 10, "Univ. District <-> Northside Hosp."),
    (5, 13, "Univ. District <-> Amb. Depot B"),
    (6, 14, "Industrial Park <-> River Bridge"),
    (7, 11, "Airport Road <-> Trauma Centre"),
    (7, 14, "Airport Road <-> River Bridge"),
    (8, 9, "City General Hosp. <-> St. Mary's Hosp."),
    (9, 11, "St. Mary's Hosp. <-> Trauma Centre"),
    (10, 11, "Northside Hosp. <-> Trauma Centre"),
    (12, 13, "Amb. Depot A <-> Amb. Depot B"),
    (13, 1, "Amb. Depot B <-> North Junction"),
    (14, 15, "River Bridge <-> Suburb Area"),
    (15, 3, "Suburb Area <-> East Cross"),
    (0, 14, "Central Square <-> River Bridge"),
]

PRIORITIES = [
    (1, "RED - Critical"),
    (2, "ORANGE - Serious"),
    (3, "YELLOW - Moderate"),
    (4, "GREEN - Minor"),
]

TRIAGE_QUESTIONS = [
    (
        "Primary complaint",
        [
            "1 - Chest pain / difficulty breathing",
            "2 - Severe bleeding / trauma",
            "3 - Loss of consciousness / seizure",
            "4 - Moderate pain or injury",
            "5 - Mild pain, fever, or nausea",
        ],
    ),
    (
        "Conscious and responding",
        [
            "1 - Unconscious / unresponsive",
            "2 - Conscious but confused",
            "3 - Conscious and alert",
        ],
    ),
    (
        "Breathing",
        [
            "1 - Not breathing / very laboured",
            "2 - Rapid / shallow",
            "3 - Normal",
        ],
    ),
    (
        "Visible bleeding",
        [
            "1 - Yes, uncontrolled",
            "2 - Yes, controlled/minor",
            "3 - No",
        ],
    ),
    (
        "Heart or stroke history",
        [
            "1 - Yes",
            "2 - Unknown",
            "3 - No",
        ],
    ),
    (
        "Symptom duration",
        [
            "1 - Just started (< 5 minutes)",
            "2 - Less than 1 hour",
            "3 - Several hours",
            "4 - More than a day",
        ],
    ),
]


def expected_release_line(text):
    return bool(re.match(r"^[0-3]\.\s+.+\(\d+\s+free\)$", text))


class EmergencyGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("RapidAid Emergency Dispatch")
        self.root.geometry("1180x760")
        self.root.minsize(1040, 660)
        self.root.configure(bg="#f4f7f6")

        self.process = None
        self.output_queue = queue.Queue()
        self.status_var = tk.StringVar(value="Starting backend")
        self.action_var = tk.StringVar(value="Ready")
        self.current_action_title = "Ready"
        self.available_beds = DEFAULT_AVAILABLE_BEDS.copy()
        self.blocked_roads = set()
        self._last_line_blank = False

        self.configure_styles()
        self.setup_ui()
        self.root.after(80, self.flush_output)
        threading.Thread(target=self.build_and_start_backend, daemon=True).start()

    def configure_styles(self):
        self.style = ttk.Style()
        self.style.theme_use("clam")
        self.style.configure("TFrame", background="#f4f7f6")
        self.style.configure("Panel.TFrame", background="#ffffff")
        self.style.configure("TLabel", background="#f4f7f6", foreground="#1b2630")
        self.style.configure("Header.TLabel", font=("Segoe UI", 19, "bold"), background="#f4f7f6")
        self.style.configure("Subtle.TLabel", font=("Segoe UI", 9), background="#f4f7f6", foreground="#61717f")
        self.style.configure("PanelTitle.TLabel", font=("Segoe UI", 11, "bold"), background="#ffffff")
        self.style.configure("Field.TLabel", font=("Segoe UI", 9), background="#ffffff", foreground="#334155")
        self.style.configure("CardTitle.TLabel", font=("Segoe UI", 9, "bold"), background="#ffffff", foreground="#61717f")
        self.style.configure("CardValue.TLabel", font=("Segoe UI", 16, "bold"), background="#ffffff", foreground="#1b2630")
        self.style.configure("TButton", font=("Segoe UI", 10), padding=(10, 7))
        self.style.configure("Small.TButton", font=("Segoe UI", 9), padding=(8, 5))
        self.style.configure("Accent.TButton", background="#0f766e", foreground="#ffffff")
        self.style.map("Accent.TButton", background=[("active", "#115e59")])
        self.style.configure("Danger.TButton", background="#b42318", foreground="#ffffff")
        self.style.map("Danger.TButton", background=[("active", "#8a1c13")])

    def setup_ui(self):
        top = ttk.Frame(self.root)
        top.pack(fill="x", padx=22, pady=(18, 10))

        title_block = ttk.Frame(top)
        title_block.pack(side="left", fill="x", expand=True)
        ttk.Label(title_block, text="RapidAid Dispatch Command", style="Header.TLabel").pack(anchor="w")
        ttk.Label(
            title_block,
            text="Dijkstra routing, triage priority, hospital capacity, ambulance assignment",
            style="Subtle.TLabel",
        ).pack(anchor="w", pady=(3, 0))

        status = ttk.Frame(top)
        status.pack(side="right")
        ttk.Label(status, textvariable=self.status_var, style="Subtle.TLabel").pack(anchor="e")
        ttk.Button(status, text="Restart Backend", command=self.restart_backend).pack(anchor="e", pady=(6, 0))

        body = ttk.Frame(self.root)
        body.pack(fill="both", expand=True, padx=22, pady=(0, 18))

        left = ttk.Frame(body, style="Panel.TFrame")
        left.pack(side="left", fill="y", padx=(0, 14))
        left.configure(width=330)
        left.pack_propagate(False)

        right = ttk.Frame(body)
        right.pack(side="right", fill="both", expand=True)

        self.create_control_panel(left)
        self.create_dashboard(right)
        self.create_console(right)

    def create_control_panel(self, parent):
        pad = {"padx": 16, "pady": 5}
        ttk.Label(parent, text="Quick Dispatch", style="PanelTitle.TLabel").pack(anchor="w", padx=16, pady=(16, 8))

        self.quick_name = self.panel_entry(parent, "Patient name")
        self.quick_location = self.panel_combo(parent, "Location", self.node_values())
        self.quick_priority = self.panel_combo(parent, "Priority", self.priority_values(), 1)
        ttk.Button(parent, text="Dispatch Patient", command=self.quick_dispatch, style="Accent.TButton").pack(fill="x", **pad)
        ttk.Button(parent, text="Full Triage Form", command=self.open_triage_dialog).pack(fill="x", **pad)

        ttk.Separator(parent).pack(fill="x", padx=16, pady=12)
        ttk.Label(parent, text="Tools", style="PanelTitle.TLabel").pack(anchor="w", padx=16, pady=(0, 8))

        tool_grid = ttk.Frame(parent, style="Panel.TFrame")
        tool_grid.pack(fill="x", padx=16)
        tools = [
            ("Release Bed", self.open_release_bed),
            ("Road Control", self.open_road_control),
            ("Shortest Path", self.open_shortest_path),
            ("Nearest vs Best", self.open_nearest_best),
        ]
        for index, (label, command) in enumerate(tools):
            button = ttk.Button(tool_grid, text=label, command=command, style="Small.TButton")
            button.grid(row=index // 2, column=index % 2, sticky="ew", padx=(0, 6), pady=4)
        tool_grid.columnconfigure(0, weight=1)
        tool_grid.columnconfigure(1, weight=1)

        ttk.Separator(parent).pack(fill="x", padx=16, pady=12)
        ttk.Label(parent, text="Views", style="PanelTitle.TLabel").pack(anchor="w", padx=16, pady=(0, 8))

        views = [
            ("Scenarios", ["2", ""]),
            ("Dashboard", ["6", ""]),
            ("Hospitals", ["7", ""]),
            ("Ambulances", ["8", ""]),
        ]
        view_grid = ttk.Frame(parent, style="Panel.TFrame")
        view_grid.pack(fill="x", padx=16)
        for index, (label, sequence) in enumerate(views):
            ttk.Button(
                view_grid,
                text=label,
                command=lambda name=label, seq=sequence: self.run_action(name, seq),
                style="Small.TButton",
            ).grid(row=index // 2, column=index % 2, sticky="ew", padx=(0, 6), pady=4)
        view_grid.columnconfigure(0, weight=1)
        view_grid.columnconfigure(1, weight=1)

        ttk.Separator(parent).pack(fill="x", padx=16, pady=12)
        ttk.Button(parent, text="Clear Console", command=self.clear_console_to_ready).pack(fill="x", **pad)
        tk.Button(
            parent,
            text="Exit",
            command=self.shutdown,
            bg="#b42318",
            fg="white",
            activebackground="#8a1c13",
            activeforeground="white",
            font=("Segoe UI", 10, "bold"),
            relief="raised",
            bd=1,
        ).pack(fill="x", padx=16, pady=(6, 16))

    def panel_entry(self, parent, label):
        ttk.Label(parent, text=label, style="Field.TLabel").pack(anchor="w", padx=16, pady=(6, 2))
        entry = ttk.Entry(parent)
        entry.pack(fill="x", padx=16, pady=(0, 4))
        return entry

    def panel_combo(self, parent, label, values, index=0):
        ttk.Label(parent, text=label, style="Field.TLabel").pack(anchor="w", padx=16, pady=(6, 2))
        combo = ttk.Combobox(parent, values=values, state="readonly")
        combo.current(index)
        combo.pack(fill="x", padx=16, pady=(0, 4))
        return combo

    def create_dashboard(self, parent):
        cards = ttk.Frame(parent)
        cards.pack(fill="x")
        card_data = [
            ("Hospitals", "4 live sites", "#0f766e"),
            ("Ambulances", "5 units", "#1d4ed8"),
            ("Network", "16 nodes", "#7c2d12"),
            ("Triage", "4 levels", "#9a3412"),
        ]
        for title, value, color in card_data:
            card = ttk.Frame(cards, style="Panel.TFrame")
            card.pack(side="left", fill="x", expand=True, padx=(0, 10))
            stripe = tk.Frame(card, height=3, bg=color)
            stripe.pack(fill="x")
            ttk.Label(card, text=title, style="CardTitle.TLabel").pack(anchor="w", padx=14, pady=(12, 2))
            ttk.Label(card, text=value, style="CardValue.TLabel").pack(anchor="w", padx=14, pady=(0, 14))

    def create_console(self, parent):
        shell = ttk.Frame(parent, style="Panel.TFrame")
        shell.pack(fill="both", expand=True, pady=(14, 0))
        header = ttk.Frame(shell, style="Panel.TFrame")
        header.pack(fill="x", padx=14, pady=(12, 6))
        ttk.Label(header, text="Backend Console", style="PanelTitle.TLabel").pack(side="left")
        ttk.Label(header, textvariable=self.action_var, style="CardTitle.TLabel").pack(side="right")

        console_frame = ttk.Frame(shell, style="Panel.TFrame")
        console_frame.pack(fill="both", expand=True, padx=14, pady=(0, 8))

        self.console = tk.Text(
            console_frame,
            bg="#101820",
            fg="#dbe7ef",
            insertbackground="#dbe7ef",
            relief="flat",
            wrap="none",
            font=("Consolas", 10),
            padx=12,
            pady=12,
        )
        y_scroll = ttk.Scrollbar(console_frame, orient="vertical", command=self.console.yview)
        x_scroll = ttk.Scrollbar(console_frame, orient="horizontal", command=self.console.xview)
        self.console.configure(yscrollcommand=y_scroll.set, xscrollcommand=x_scroll.set)
        self.console.grid(row=0, column=0, sticky="nsew")
        y_scroll.grid(row=0, column=1, sticky="ns")
        x_scroll.grid(row=1, column=0, sticky="ew")
        console_frame.rowconfigure(0, weight=1)
        console_frame.columnconfigure(0, weight=1)
        self.console.tag_config("input", foreground="#fbbf24")
        self.console.tag_config("error", foreground="#fca5a5")
        self.console.tag_config("system", foreground="#67e8f9")

        footer = ttk.Frame(shell, style="Panel.TFrame")
        footer.pack(fill="x", padx=14, pady=(0, 14))
        ttk.Label(
            footer,
            text="Results appear here. Use the scrollbars for long scenario output.",
            style="CardTitle.TLabel",
        ).pack(anchor="w")

    def executable_path(self):
        return PROJECT_DIR / EXE_NAME

    def needs_rebuild(self):
        exe = self.executable_path()
        if not exe.exists():
            return True
        exe_time = exe.stat().st_mtime
        return any((PROJECT_DIR / name).stat().st_mtime > exe_time for name in SOURCE_FILES)

    def build_and_start_backend(self):
        try:
            if self.needs_rebuild():
                self.queue_system("Building C++ backend...\n")
                cmd = ["g++", "-std=c++17", "-O2", "-Wall", "-Wextra", "-o", EXE_NAME, "main.cpp"]
                result = subprocess.run(
                    cmd,
                    cwd=PROJECT_DIR,
                    capture_output=True,
                    text=True,
                    encoding="utf-8",
                    errors="replace",
                )
                if result.stdout:
                    self.queue_system(result.stdout)
                if result.stderr:
                    self.queue_error(result.stderr)
                if result.returncode != 0:
                    self.set_status("Build failed")
                    return
                self.queue_system("Build complete: rapid_aid.exe\n")
            self.start_backend()
        except FileNotFoundError:
            self.queue_error("g++ was not found. Install MinGW or add g++ to PATH.\n")
            self.set_status("Compiler missing")
        except Exception as exc:
            self.queue_error(f"Backend startup failed: {exc}\n")
            self.set_status("Startup failed")

    def start_backend(self):
        self.process = subprocess.Popen(
            [str(self.executable_path())],
            cwd=PROJECT_DIR,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            bufsize=1,
        )
        self.set_status("Backend running")
        threading.Thread(target=self.read_output, daemon=True).start()
        time.sleep(0.15)
        self.write_to_backend("")
        self.root.after(700, self.show_ready_screen)

    def read_output(self):
        in_ansi = False
        line = ""
        while self.process and self.process.poll() is None:
            char = self.process.stdout.read(1)
            if not char:
                break
            if char == "\x1b":
                in_ansi = True
                continue
            if in_ansi:
                if char.isalpha():
                    in_ansi = False
                continue
            if char == "\r":
                continue
            line += char
            if char == "\n":
                self.emit_output_line(line)
                line = ""
        if line:
            self.emit_output_line(line + "\n")
        self.set_status("Backend stopped")

    def emit_output_line(self, line):
        if self.hide_backend_line(line):
            return
        line = self.clean_backend_line(line)
        if not line:
            return
        self.output_queue.put(("clear_loading", ""))
        if not line.strip():
            if self._last_line_blank:
                return
            self._last_line_blank = True
        else:
            self._last_line_blank = False
        self.output_queue.put(("output", line))

    def clean_backend_line(self, line):
        replacements = [
            "Patient name:",
            "Location node:",
            "Priority (1=RED 2=ORANGE 3=YELLOW 4=GREEN):",
            "Hospital ID:",
            "Action (1=Block 2=Unblock):",
            "Travel time in minutes:",
            "Node A:",
            "Node B:",
            "From node:",
            "To node:",
            "Your choice [1-5]:",
            "Your choice [1-4]:",
            "Your choice [1-3]:",
        ]
        for item in replacements:
            line = line.replace(item, "")
        return line

    def hide_backend_line(self, line):
        text = " ".join(line.strip().split())
        if not text:
            return False
        hidden_exact = {
            "HOSPITAL EMERGENCY ALLOCATION SYSTEM",
            "EMERGENCY HOSPITAL & AMBULANCE DISPATCH SYSTEM",
            "DISPATCH OPERATIONS",
            "VIEW",
            "ANALYSIS",
            "0 Exit",
        }
        if text in hidden_exact:
            return True
        hidden_prefixes = (
            "Design & Analysis",
            "Dijkstra .",
            "Graph:",
            "Press ENTER",
            "Enter choice:",
            "RELEASE BED",
            "ROAD CONTROL",
            "1 Register new emergency",
            "2 Run all scenario",
            "3 Manual dispatch",
            "4 Release a hospital",
            "5 Block/restore",
            "6 Operations dashboard",
            "7 Hospital status",
            "8 Ambulance fleet",
            "9 Hospital staff",
            "10 Dispatch log",
            "11 Shortest path",
            "12 Nearest vs Best",
        )
        if expected_release_line(text):
            return True
        return any(text.startswith(prefix) for prefix in hidden_prefixes)

    def flush_output(self):
        try:
            while True:
                tag, text = self.output_queue.get_nowait()
                if tag == "clear_loading":
                    start = self.console.search("Loading result...\n\n", "1.0", "end")
                    if start:
                        self.console.delete(start, f"{start}+19c")
                    continue
                self.console.insert("end", text, None if tag == "output" else tag)
                self.console.see("end")
        except queue.Empty:
            pass
        self.root.after(80, self.flush_output)

    def queue_system(self, text):
        self.output_queue.put(("system", text))

    def queue_error(self, text):
        self.output_queue.put(("error", text))

    def set_status(self, text):
        self.root.after(0, self.status_var.set, text)

    def backend_alive(self):
        return self.process is not None and self.process.poll() is None

    def write_to_backend(self, text):
        if not self.backend_alive():
            messagebox.showwarning("Backend unavailable", "The C++ backend is not running.")
            return False
        try:
            self.process.stdin.write(text + "\n")
            self.process.stdin.flush()
            return True
        except OSError as exc:
            self.queue_error(f"Could not write to backend: {exc}\n")
            return False

    def send_sequence(self, values, clear_first=False, echo=False):
        if clear_first:
            self.clear_console()
        for value in values:
            if not self.write_to_backend(str(value)):
                return
            if echo and value != "":
                self.console.insert("end", f"{value}\n", "input")
        self.console.see("end")

    def clear_console(self):
        self.console.delete("1.0", "end")
        self._last_line_blank = False

    def clear_console_to_ready(self):
        self.clear_console()
        self.current_action_title = "Ready"
        self.action_var.set("Ready")
        self.console.insert(
            "end",
            "RapidAid is ready. Use Quick Dispatch, Full Triage Form, or any view button on the left.\n",
            "system",
        )

    def show_ready_screen(self):
        if self.backend_alive():
            self.clear_console_to_ready()

    def restart_backend(self):
        self.stop_backend()
        self.clear_console()
        self.set_status("Restarting backend")
        threading.Thread(target=self.build_and_start_backend, daemon=True).start()

    def stop_backend(self):
        if self.backend_alive():
            try:
                self.process.stdin.write("0\n")
                self.process.stdin.flush()
            except OSError:
                pass
            self.process.terminate()
            try:
                self.process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.process.kill()

    def shutdown(self):
        self.stop_backend()
        self.root.destroy()

    def selected_id(self, value):
        return int(value.split(" - ", 1)[0])

    def run_action(self, title, values):
        self.current_action_title = title
        self.action_var.set(title)
        self.clear_console()
        self.console.insert("end", f"{title}\n", "system")
        self.console.insert("end", "-" * 72 + "\n", "system")
        summary = self.action_summary(title, values)
        if summary:
            self.console.insert("end", summary + "\n\n", "system")
        self.console.insert("end", "Loading result...\n\n", "system")
        threading.Thread(target=self.send_sequence_slow, args=(values,), daemon=True).start()
        self.root.after(1800, lambda expected=title: self.ensure_action_response(expected))

    def send_sequence_slow(self, values):
        # First ENTER clears a previous "Press ENTER to continue" prompt if
        # the backend is sitting there; at the main menu it is harmless.
        for value in [""] + list(values):
            if not self.write_to_backend(str(value)):
                return
            time.sleep(0.12)

    def ensure_action_response(self, expected_title):
        if self.current_action_title != expected_title:
            return
        content = self.console.get("1.0", "end").strip()
        visible_lines = [
            line.strip() for line in content.splitlines()
            if line.strip() and line.strip() != "-" * 72 and line.strip() != "Loading result..."
        ]
        if visible_lines != [expected_title]:
            return

        start = self.console.search("Loading result...\n\n", "1.0", "end")
        if start:
            self.console.delete(start, f"{start}+19c")

        if expected_title == "Dashboard":
            message = (
                "No live requests or logs yet.\n"
                "Dispatch a patient first, then open Dashboard again.\n"
            )
        else:
            message = "No result returned yet. Click Restart Backend if this view stays empty.\n"
            self.console.insert("end", message, "system")

    def append_system_line(self, text):
        self.console.insert("end", text + "\n", "system")
        self.console.see("end")

    def action_summary(self, title, values):
        try:
            command = str(values[0])
            if command == "3":
                priority = next(name for pid, name in PRIORITIES if pid == int(values[3]))
                location = next(name for node, name in NODES if node == int(values[2]))
                return f"Dispatching {values[1]} from {location} as {priority}."
            if command == "4":
                hospital = next(name for hid, name in HOSPITALS if hid == int(values[1]))
                return f"Releasing one bed at {hospital}."
            if command == '5':
                action = str(values[1])
                node_a = next(name for node, name in NODES if node == int(values[2]))
                node_b = next(name for node, name in NODES if node == int(values[3]))
                if action == '1':
                    return f'Blocking road between {node_a} and {node_b}.'
                return f'Unblocking road between {node_a} and {node_b}.'
            if command == "11":
                src = next(name for node, name in NODES if node == int(values[1]))
                dst = next(name for node, name in NODES if node == int(values[2]))
                return f"Finding fastest route from {src} to {dst}."
            if command == "12":
                location = next(name for node, name in NODES if node == int(values[1]))
                priority = next(name for pid, name in PRIORITIES if pid == int(values[2]))
                return f"Comparing nearest and best hospital from {location} for {priority}."
        except Exception:
            return ""
        return ""

    def quick_dispatch(self):
        patient = self.quick_name.get().strip() or "Unknown"
        values = [
            "3",
            patient,
            self.selected_id(self.quick_location.get()),
            self.selected_id(self.quick_priority.get()),
            "",
        ]
        self.run_action("Manual dispatch", values)
        self.quick_name.delete(0, "end")

    def node_values(self):
        return [f"{node} - {name}" for node, name in NODES]

    def hospital_values(self):
        return [f"{hid} - {name}" for hid, name in HOSPITALS]

    def priority_values(self):
        return [f"{pid} - {name}" for pid, name in PRIORITIES]

    def road_values(self, blocked_only=False):
        values = []
        for index, (_u, _v, label) in enumerate(ROADS):
            if blocked_only and index not in self.blocked_roads:
                continue
            values.append(f'{index} - {label}')
        return values

    def road_minutes(self, road_index):
        minutes = [5, 6, 7, 8, 4, 3, 5, 4, 6, 5, 4, 2, 5, 3, 7, 4, 5, 9, 8, 10, 6, 3, 4, 6, 9]
        return minutes[road_index]

    def make_dialog(self, title):
        dialog = tk.Toplevel(self.root)
        dialog.title(title)
        dialog.configure(bg="#f4f7f6")
        dialog.transient(self.root)
        dialog.grab_set()
        return dialog

    def labeled_entry(self, parent, label):
        ttk.Label(parent, text=label).pack(anchor="w", pady=(8, 3))
        entry = ttk.Entry(parent)
        entry.pack(fill="x")
        return entry

    def labeled_combo(self, parent, label, values, index=0):
        ttk.Label(parent, text=label).pack(anchor="w", pady=(8, 3))
        combo = ttk.Combobox(parent, values=values, state="readonly")
        combo.current(index)
        combo.pack(fill="x")
        return combo

    def open_manual_dispatch(self):
        dialog = self.make_dialog("Manual Dispatch")
        body = ttk.Frame(dialog)
        body.pack(fill="both", expand=True, padx=18, pady=18)
        name = self.labeled_entry(body, "Patient name")
        node = self.labeled_combo(body, "Location", self.node_values())
        priority = self.labeled_combo(body, "Priority", self.priority_values(), 1)

        def submit():
            patient = name.get().strip() or "Unknown"
            self.run_action(
                "Manual dispatch",
                ["3", patient, self.selected_id(node.get()), self.selected_id(priority.get()), ""],
            )
            dialog.destroy()

        ttk.Button(body, text="Dispatch", command=submit, style="Accent.TButton").pack(fill="x", pady=(14, 0))

    def open_release_bed(self):
        dialog = self.make_dialog("Release Bed")
        body = ttk.Frame(dialog)
        body.pack(fill="both", expand=True, padx=18, pady=18)
        hospital = self.labeled_combo(body, "Hospital", self.hospital_values())

        def submit():
            hospital_id = self.selected_id(hospital.get())
            self.run_action("Release bed", ["4", hospital_id, ""])
            self.available_beds[hospital_id] = self.available_beds.get(hospital_id, 0) + 1
            hospital_name = next(name for hid, name in HOSPITALS if hid == hospital_id)
            self.root.after(
                900,
                lambda: self.append_system_line(
                    f"{hospital_name} now has {self.available_beds[hospital_id]} available beds."
                ),
            )
            dialog.destroy()

        ttk.Button(body, text="Release", command=submit, style="Accent.TButton").pack(fill="x", pady=(14, 0))

    def open_road_control(self):
        dialog = self.make_dialog("Road Control")
        dialog.geometry("560x300")
        dialog.minsize(560, 300)
        body = ttk.Frame(dialog)
        body.pack(fill="both", expand=True, padx=18, pady=18)

        mode = tk.StringVar(value="block")
        mode_frame = ttk.Frame(body)
        mode_frame.pack(fill="x", pady=(0, 10))
        ttk.Radiobutton(mode_frame, text="Block road", variable=mode, value="block").pack(side="left")
        ttk.Radiobutton(mode_frame, text="Unblock road", variable=mode, value="unblock").pack(side="left", padx=(14, 0))

        ttk.Label(body, text="Road").pack(anchor="w", pady=(8, 3))
        road = ttk.Combobox(body, values=self.road_values(), state="readonly", width=62)
        road.current(0)
        road.pack(fill="x")

        selected_text = tk.StringVar(value=road.get())
        status_text = tk.StringVar(value="")
        ttk.Label(body, textvariable=selected_text, style="Subtle.TLabel", wraplength=510).pack(
            anchor="w", fill="x", pady=(8, 0)
        )
        ttk.Label(body, textvariable=status_text, style="Subtle.TLabel", wraplength=510).pack(
            anchor="w", fill="x", pady=(8, 0)
        )

        def refresh_roads():
            if mode.get() == "unblock":
                values = self.road_values(blocked_only=True)
                if not values:
                    road.configure(values=[])
                    road.set("")
                    selected_text.set("No Road is Blocked")
                    status_text.set("No Road is Blocked")
                    return
            else:
                values = self.road_values(blocked_only=False)
                status_text.set("")

            road.configure(values=values)
            if values:
                road.current(0)
                selected_text.set(road.get())

        def on_mode_change(*_args):
            refresh_roads()

        mode.trace_add("write", on_mode_change)
        road.bind("<<ComboboxSelected>>", lambda _event: selected_text.set(road.get()))

        def submit():
            if mode.get() == "unblock" and not self.road_values(blocked_only=True):
                self.run_action("Road control", [])
                self.append_system_line("No Road is Blocked")
                dialog.destroy()
                return

            if not road.get():
                status_text.set("No Road is Blocked")
                return

            road_index = self.selected_id(road.get())
            node_a, node_b, label = ROADS[road_index]
            if mode.get() == "block":
                self.blocked_roads.add(road_index)
                self.run_action("Block road", ["5", "1", node_a, node_b, ""])
            else:
                self.blocked_roads.discard(road_index)
                self.run_action("Unblock road", ["5", "2", node_a, node_b, self.road_minutes(road_index), ""])
            dialog.destroy()

        ttk.Button(body, text="Apply", command=submit, style="Accent.TButton").pack(fill="x", pady=(18, 0))
    def open_shortest_path(self):
        dialog = self.make_dialog("Shortest Path")
        body = ttk.Frame(dialog)
        body.pack(fill="both", expand=True, padx=18, pady=18)
        src = self.labeled_combo(body, "From", self.node_values())
        dst = self.labeled_combo(body, "To", self.node_values(), 8)

        def submit():
            self.run_action(
                "Shortest path",
                ["11", self.selected_id(src.get()), self.selected_id(dst.get()), ""],
            )
            dialog.destroy()

        ttk.Button(body, text="Find Route", command=submit, style="Accent.TButton").pack(fill="x", pady=(14, 0))

    def open_nearest_best(self):
        dialog = self.make_dialog("Nearest vs Best")
        body = ttk.Frame(dialog)
        body.pack(fill="both", expand=True, padx=18, pady=18)
        node = self.labeled_combo(body, "Location", self.node_values())
        priority = self.labeled_combo(body, "Priority", self.priority_values())

        def submit():
            self.run_action(
                "Nearest vs best",
                ["12", self.selected_id(node.get()), self.selected_id(priority.get()), ""],
            )
            dialog.destroy()

        ttk.Button(body, text="Analyze", command=submit, style="Accent.TButton").pack(fill="x", pady=(14, 0))

    def open_triage_dialog(self):
        dialog = self.make_dialog("Register Emergency")
        dialog.geometry("560x640")
        canvas = tk.Canvas(dialog, bg="#f4f7f6", highlightthickness=0)
        scroll = ttk.Scrollbar(dialog, orient="vertical", command=canvas.yview)
        body = ttk.Frame(canvas)
        body.bind("<Configure>", lambda _event: canvas.configure(scrollregion=canvas.bbox("all")))
        canvas.create_window((0, 0), window=body, anchor="nw", width=536)
        canvas.configure(yscrollcommand=scroll.set)
        canvas.pack(side="left", fill="both", expand=True)
        scroll.pack(side="right", fill="y")

        form = ttk.Frame(body)
        form.pack(fill="both", expand=True, padx=18, pady=18)
        name = self.labeled_entry(form, "Patient name")
        node = self.labeled_combo(form, "Location", self.node_values())
        answers = []
        for question, options in TRIAGE_QUESTIONS:
            answers.append(self.labeled_combo(form, question, options))

        def submit():
            patient = name.get().strip() or "Unknown"
            sequence = ["1", patient, self.selected_id(node.get())]
            sequence.extend(self.selected_id(answer.get()) for answer in answers)
            sequence.append("")
            self.run_action("Triage dispatch", sequence)
            dialog.destroy()

        ttk.Button(form, text="Register and Dispatch", command=submit, style="Accent.TButton").pack(fill="x", pady=(16, 4))


if __name__ == "__main__":
    app_root = tk.Tk()
    app = EmergencyGUI(app_root)
    app_root.protocol("WM_DELETE_WINDOW", app.shutdown)
    app_root.mainloop()




