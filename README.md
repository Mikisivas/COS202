# SCAPMS: Smart Clinic Appointment & Patient Management System

COS202 Programming II lab project. C++17 and Qt 6 Widgets.

## Build and run

Option 1, Qt Creator (Windows, macOS or Linux):

1. Install Qt 6 (6.4 or newer) with the Qt Creator IDE and a compiler (MinGW on Windows).
2. File > Open File or Project, choose `CMakeLists.txt`.
3. Pick a kit, then press Run (Ctrl+R).

Option 2, command line:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

```bash
cmake --build build
```

```bash
./build/SCAPMS
```

Run the core tests (76 checks):

```bash
./build/scapms_tests
```

On start the program loads `data/clinic_data.csv` (the build copies it next to the executable). Help > Load Demo Data creates fresh sample data dated around today.

## Folder layout

```
src/model       PatientRecord (abstract), GeneralPatient, EmergencyPatient,
                FollowUpPatient, Appointable (interface), Appointment,
                ClinicDatabase, PatientFactory
src/controller  ClinicManager, AppointmentController, SearchEngine, Sorter,
                ReportGenerator, SampleData
src/gui         MainWindow, DashboardPanel, PatientPanel, AppointmentPanel,
                QueuePanel, SearchPanel, ReportsPanel, AdminPanel, GuiUtils
src/utils       FileHandler, IDGenerator, Exceptions, DateUtils, StringUtils
data            clinic_data.csv
tests           core_tests.cpp
docs            report, UML diagram, screenshots, submission PDF
```

## Shortcuts

Ctrl+N new, Ctrl+O load, Ctrl+S save, Ctrl+Shift+S save as, Ctrl+F search, Ctrl+Z undo, Ctrl+Q exit, Alt+1 to Alt+7 switch tabs.
