cmake_minimum_required(VERSION 3.16)
project(SCAPMS VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# ---- Core library: model + controller + utils (no Qt dependency) ----
add_library(scapms_core STATIC
    src/model/PatientRecord.cpp
    src/model/GeneralPatient.cpp
    src/model/EmergencyPatient.cpp
    src/model/FollowUpPatient.cpp
    src/model/Appointment.cpp
    src/model/PatientFactory.cpp
    src/model/ClinicDatabase.cpp
    src/controller/Sorter.cpp
    src/controller/SearchEngine.cpp
    src/controller/ClinicManager.cpp
    src/controller/AppointmentController.cpp
    src/controller/ReportGenerator.cpp
    src/controller/SampleData.cpp
    src/utils/FileHandler.cpp
)
target_include_directories(scapms_core PUBLIC src)

# ---- GUI application ----
add_executable(SCAPMS WIN32
    src/main.cpp
    src/gui/MainWindow.cpp
    src/gui/DashboardPanel.cpp
    src/gui/PatientPanel.cpp
    src/gui/AppointmentPanel.cpp
    src/gui/QueuePanel.cpp
    src/gui/SearchPanel.cpp
    src/gui/ReportsPanel.cpp
    src/gui/AdminPanel.cpp
)
target_link_libraries(SCAPMS PRIVATE scapms_core Qt6::Widgets)

# Copy the data folder next to the executable so the app finds it on start.
add_custom_command(TARGET SCAPMS POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:SCAPMS>/data)

# ---- Console tests for the core logic ----
option(SCAPMS_BUILD_TESTS "Build the console test program" ON)
if(SCAPMS_BUILD_TESTS)
    enable_testing()
    add_executable(scapms_tests tests/core_tests.cpp)
    target_link_libraries(scapms_tests PRIVATE scapms_core)
    add_test(NAME core_tests COMMAND scapms_tests)
endif()
