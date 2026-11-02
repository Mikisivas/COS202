# Project Report: Smart Clinic Appointment & Patient Management System (SCAPMS)

## 1. Project Description

SCAPMS is a desktop application for a university clinic, written in C++17 with the Qt 6 Widgets framework. Clinic staff use it to register patients, book and cancel appointments, manage the consultation queue, search and sort records, undo mistakes and print simple reports. All data is saved to and loaded from a CSV file, so nothing is lost between sessions.

The code is split into four namespaces: `scapms::model` (data classes), `scapms::controller` (logic and algorithms), `scapms::gui` (Qt windows) and `scapms::utils` (files, IDs, exceptions). Model and controller have no Qt dependency and compile into a library (`scapms_core`) checked by 76 automated console tests.

## 2. System Features

- Register, edit and delete General, Emergency and Follow-Up patients.
- Schedule, cancel, complete and remove appointments, with clash and date checks.
- A first-in, first-out waiting queue with live waiting times and long-wait alerts.
- Search by ID, name or category, and sort by name, ID, age or next appointment date, with the algorithm chosen from a combo box.
- Undo the last administrative operation (register, modify, delete, cancel, remove).
- Four reports (most attended patients, pending appointments, category distribution, daily summary), exportable to text.
- A dashboard with live statistics, today's schedule, the recent-patient cache and a triage list.

## 3. Class Hierarchy

`PatientRecord` is the abstract base class. It stores the ID, name, age, contact and linked appointment IDs, and declares six pure virtual functions: `getCategory()`, `getPriorityLevel()`, `getPriorityLabel()`, `getDetails()`, `getExtraFields()` and `clone()`. Three subclasses override them:

| Class | Extra data | Priority | Behaviour |
|---|---|---|---|
| GeneralPatient | symptoms | Normal | Symptoms become the appointment reason if none is typed |
| EmergencyPatient | condition, severity 1-5 | High Priority | Cannot be booked; uses the waiting queue |
| FollowUpPatient | previous diagnosis, last visit date | Scheduled | Appointment must be after the last visit |

`Appointable` is an interface-like abstract class with only pure virtual functions: `scheduleAppointment()`, `cancelAppointment()` and `bookingRule()`. GeneralPatient and FollowUpPatient inherit from both `PatientRecord` and `Appointable`. EmergencyPatient inherits only from `PatientRecord`. `AppointmentController` uses `dynamic_cast<Appointable*>` to decide at run time whether a patient can be booked.

Runtime polymorphism appears in several places. `triageLine(const PatientRecord&)` and `ClinicManager::triageOverview()` process every patient through a base-class reference or pointer, and the output changes with the real type. The patient table, the file writer and the undo snapshots (`clone()`) also work only through `PatientRecord*`.

Composition: `ClinicDatabase` owns all patients (`vector<unique_ptr<PatientRecord>>`) and all appointments (`vector<Appointment>`). `ClinicManager` owns the database, and `MainWindow` owns the `ClinicManager` and `AppointmentController`. Each patient is associated with zero or more appointments through its list of appointment IDs.

## 4. Data Structures Used

- `std::vector<std::unique_ptr<PatientRecord>>` stores registered patients. One vector holds all three subclasses, and `unique_ptr` deletes them automatically.
- `std::queue<QueueEntry>` holds patients waiting for consultation. `push()` adds at the back and `front()`/`pop()` serve the patient who arrived first.
- `std::stack<AdminAction>` implements Undo. Each action stores a type, a description and a snapshot (a cloned patient or a copied appointment). Undo pops the top action and reverses it, so the most recent operation is undone first.
- `PatientRecord* recentPatients_[10]` is a fixed-size array cache of the 10 most recently accessed patients. `getPatient()` checks the cache before the full list and moves the record to slot 0. When the array is full, the least recently used record drops off the end. Hits and misses show on the dashboard.

## 5. Searching Algorithms

All searches are written by hand in `SearchEngine`. No `std::find` or `std::binary_search` is used.

- **Linear search** by patient ID checks each record in turn. It works on unsorted data and costs O(n).
- **Binary search** by ID or exact name halves the range at each step, O(log n). It only runs when `ClinicDatabase::sortedBy()` matches the searched field. Otherwise the program throws a `SearchException` that tells the user to sort first.
- **Recursive search** by partial name splits the range in half, searches the left half, then the right half, and checks a single record at the base case. Recursion depth stays at log n, so a large list will not overflow the stack.

The **Auto** option picks the technique from the state of the data: binary search when the list is sorted by the searched field, linear or recursive search when it is not. Category search is always linear because many patients share a category.

## 6. Sorting Algorithms

`Sorter` contains four hand-written algorithms. They move the `unique_ptr` values inside the vector, so the patient objects never move in memory and the cache pointers stay valid. Each sort reports comparisons, moves and time in milliseconds.

| Algorithm | Complexity | Why it suits the clinic |
|---|---|---|
| Insertion Sort | O(n) best, O(n²) worst, stable | Staff add one patient at a time, so the list is nearly sorted and one pass is often enough |
| Selection Sort | O(n²) always, at most n-1 swaps | Simple and predictable for very small lists |
| Merge Sort | O(n log n) always, stable | Best for large lists; two patients with the same name keep their order |
| Quick Sort | O(n log n) average, in place | Fast on large random lists; the middle pivot avoids the O(n²) case on sorted input |

The GUI shows the reason for the selected algorithm under the combo boxes. Comparators break ties by patient ID, so every algorithm gives the same final order and binary search stays correct.

## 7. Recursive Algorithm

Three components use recursion: merge sort (`mergeSortRange`), quick sort (`quickSortRange`) and the recursive name search (`recursiveSearchByName`). Each has a clear base case (a range of 0 or 1 records) and makes two recursive calls on smaller halves.

## 8. GUI Design

`MainWindow` is a `QMainWindow` with a File/Edit/View/Help menu bar, a `QTabWidget` with seven tabs (Dashboard, Patient Records, Appointments, Waiting Queue, Search & Sort, Reports, Admin/Undo) and a status bar showing patient count, queue length and the current file.

Layouts: `QFormLayout` for the patient and appointment forms, because it lines labels up with their fields and handles adding and removing rows. `QHBoxLayout` and `QVBoxLayout` divide each tab into a list side and a form side and stack controls vertically. `QGridLayout` arranges the dashboard statistic cards and the button groups in even rows and columns. These layouts resize with the window, so no widget uses fixed positions.

Advanced techniques implemented:

1. Custom table presentation: rows are coloured by category (red for Emergency, green for General, blue for Follow-Up) and appointments by status.
2. Dynamic components: changing the patient type removes the old type-specific rows and adds new ones (symptoms, or condition and severity, or diagnosis and last visit date).
3. File dialogs for loading and saving clinic data and for exporting reports.
4. Three timers: dashboard refresh every 5 s, waiting times every 1 s with long-wait alerts, and appointment reminders every 30 s.
5. Input validation: name and age fields turn red while invalid, and dialogs report errors such as "Patient age must be between 0 and 120."
6. Keyboard shortcuts: Ctrl+S, Ctrl+O, Ctrl+N, Ctrl+F, Ctrl+Z, Ctrl+Q and Alt+1 to Alt+7 for tabs.
7. Tooltips on buttons, combo boxes and tables.

Events use Qt signals and slots (`clicked`, `currentIndexChanged`, `textChanged`, `returnPressed`, `itemSelectionChanged`, `QTimer::timeout`). Every panel emits `dataChanged`, and `MainWindow` refreshes all panels in one slot.

## 9. Exception Handling Approach

All errors derive from `ClinicException` (itself a `std::runtime_error`). Six custom subclasses cover the required cases: `ValidationException` (invalid patient information, age or appointment data), `DuplicateIdException`, `NotFoundException`, `AppointmentException`, `FileException` and `SearchException` (empty keyword or binary search on unsorted data). The model throws them, and each GUI slot wraps its work in `try { ... } catch (const std::exception&)`. A dialog then shows the message with a title taken from the exception type, and the program keeps running. File loading reads into a temporary database first, so a damaged file reports the bad line number and leaves the current data untouched.

## 10. Challenges and Solutions

- **Storing different patient types in one container.** A vector of objects would slice subclasses. Solution: `vector<unique_ptr<PatientRecord>>` with virtual functions and a virtual `clone()`.
- **Undo for a deleted patient.** The object is destroyed on delete. Solution: store a `clone()` snapshot and copies of its appointments in the stack entry and re-insert them on undo.
- **Dangling pointers in the cache.** Deleting or replacing a patient would leave a stale pointer in the array. Solution: `evictRecent()` runs before every delete or replace, and sorting moves only the smart pointers, never the objects.
- **Binary search on unsorted data gave wrong answers.** Solution: the database records which field it is sorted by, any change resets it, and search refuses binary mode or Auto switches to linear.
- **Emergency patients should not be booked.** Solution: only bookable types implement `Appointable`, checked with `dynamic_cast`.
- **Keeping the GUI and logic apart.** Solution: the core library uses only standard C++. The GUI converts between `QString` and `std::string`, and the core is tested without a window.
