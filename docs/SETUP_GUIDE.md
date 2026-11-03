# SCAPMS Setup Guide: Get the App Running on Your Laptop

This guide takes you from a fresh Windows laptop to the clinic app running on screen. Follow the steps in order. The total time is about 1 to 2 hours, and most of that is waiting for downloads.

## What you need before you start

- A Windows 10 or Windows 11 laptop (64-bit).
- At least 15 GB of free disk space. Check in File Explorer > This PC.
- A good internet connection. The Qt download is about 3 to 5 GB, so use Wi-Fi rather than mobile data if you can.
- An email address, to create a free Qt account.
- The file `SCAPMS_COS202.zip`.

## Part 1: Install Qt and Qt Creator (one time only)

Qt is the toolkit the app is built with. Qt Creator is the program you use to open, build and run it.

**Step 1. Create a free Qt account.**
Go to https://login.qt.io/register and sign up with your email. Open the confirmation email and click the link. (If the link does not open, skip this step and use the **Sign up** button inside the installer in Step 4.)

**Step 2. Download the installer.**
Go to https://www.qt.io/development/download-qt-installer-oss and click the download button for **Windows x64**. You get a file named like `qt-online-installer-windows-x64-online.exe`.

If that page does not work, open https://download.qt.io/official_releases/online_installers/ and click `qt-online-installer-windows-x64-online.exe` to download it directly.

**Step 3. Run the installer.**
Double-click the downloaded file. If Windows shows "Windows protected your PC", click **More info**, then **Run anyway**.

**Step 4. Log in.**
Enter the email and password from Step 1 and click **Next**.

**Step 5. Accept the open-source terms.**
Tick the box to accept the open-source obligations. Tick "I am an individual person not using Qt for any company". Click **Next**.

**Step 6. Choose the folder.**
Leave the folder as `C:\Qt`. Choose **Custom installation** and click **Next**.

**Step 7. Pick the parts to install.** This is the most important step.

- Open **Qt**, then open the newest **Qt 6.x.x** in the list (for example Qt 6.8.3). Inside it, tick only **MinGW 64-bit** (the name includes a number like "MinGW 13.1.0 64-bit"). Leave the other compilers (MSVC, Android, WebAssembly) unticked. They are large and you do not need them.
- Open **Build Tools** (also called "Developer and Designer Tools"). Tick:
  - **MinGW 13.1.0 64-bit** (or whichever MinGW version matches the one you ticked above)
  - **CMake**
  - **Ninja**
- **Qt Creator** is ticked by default. Leave it ticked.

Click **Next**, accept the licence, then click **Install**.

**Step 8. Wait.**
The download and install take 30 to 90 minutes. Keep the laptop plugged in and connected. Click **Finish** when it is done.

## Part 2: Put the project on your laptop

**Step 9. Make a projects folder.**
In File Explorer, open `C:\` and create a new folder named `Projects`. Avoid the Desktop, Documents or OneDrive folders, because OneDrive syncing can lock files while the app builds.

**Step 10. Extract the zip.**
Right-click `SCAPMS_COS202.zip` > **Extract All...** and choose `C:\Projects` as the destination. You should now have the folder `C:\Projects\SCAPMS`, and inside it the folders `src`, `data`, `docs`, `tests` and the file `CMakeLists.txt`.

## Part 3: Open, build and run the app

**Step 11. Start Qt Creator.**
Open the Start menu, type **Qt Creator**, and open it.

**Step 12. Open the project.**
Click **File > Open File or Project...**, go to `C:\Projects\SCAPMS`, select **CMakeLists.txt** and click **Open**.

**Step 13. Choose the kit.**
Qt Creator shows a "Configure Project" page. Tick the kit named like **Desktop Qt 6.x.x MinGW 64-bit** and click **Configure Project**. Wait until the bottom of the window stops showing progress (about 30 seconds).

**Step 14. Select the app as the thing to run.**
In the bottom-left corner there is a small computer icon. Click it and make sure **SCAPMS** is selected under "Run" (not `scapms_tests`).

**Step 15. Build.**
Click the hammer icon at the bottom left, or press **Ctrl+B**. The first build takes 1 to 3 minutes. A green bar at the bottom right means it worked.

**Step 16. Run.**
Click the green triangle at the bottom left, or press **Ctrl+R**. The SCAPMS window opens with 15 sample patients already loaded.

## Part 4: Check that everything works

Click through each tab and try these:

1. **Dashboard:** numbers appear in the coloured cards.
2. **Patient Records:** click a patient row; the form on the right fills in. Change "Patient Type" and watch the form fields change. Type an age of 150 and click Add Patient; an error message appears.
3. **Appointments:** type a doctor's name, pick a future date and click Schedule Appointment.
4. **Waiting Queue:** add two patients, then click Call Next Patient. The first one added is served first.
5. **Search & Sort:** choose Merge Sort and Patient Name, click SORT. Then type part of a name and click Search.
6. **Reports:** switch between the four reports.
7. **Admin / Undo:** press Ctrl+Z to undo your last change.
8. **File > Save** (Ctrl+S) saves your changes. Close the app and run it again: the changes are still there.

**Fresh dates.** The sample appointments are dated early November 2026. To get sample data dated around today, use **Help > Load Demo Data**.

**Run the automated tests (optional).** Click the computer icon at the bottom left, choose **scapms_tests**, and press Ctrl+R. The "Application Output" panel should end with `76/76 checks passed`. Switch back to **SCAPMS** afterwards.

**Take screenshots.** Press **Windows + Shift + S**, drag over the app window, then paste into Paint or Word and save.

## Part 5: If something goes wrong

| Problem | Fix |
|---|---|
| No kit to choose in Step 13, or the kit has a red or yellow warning | Open `C:\Qt\MaintenanceTool.exe`, choose **Add or remove components**, and make sure Qt 6.x > MinGW 64-bit and Build Tools > MinGW, CMake and Ninja are ticked. |
| "Could not find a package configuration file provided by Qt6" | The wrong kit is selected. Click **Projects** on the left side bar, then **Manage Kits**, and choose the Desktop Qt 6 MinGW kit. |
| Build fails with red errors | Click **Issues** at the bottom of Qt Creator, copy the first red error and send it for help. |
| The app opens but shows "No data file found, loaded demo data" | Nothing is wrong. It means the program could not see the `data` folder, so it created sample data instead. |
| The app does not open when you press Run | Check the computer icon at the bottom left shows **SCAPMS**, not `scapms_tests`. |
| Windows Defender or antivirus blocks the program | Choose **Allow** or **Run anyway**. The app is built on your own laptop from the source code. |
| The download in Step 8 stops | Run `C:\Qt\MaintenanceTool.exe` again and choose **Update components**. It continues from where it stopped. |

## On a Mac instead of Windows

Use the same installer from Step 2 (choose the macOS version). In Step 7 tick Qt 6.x > **macOS**, plus **CMake** and **Ninja**. You also need Apple's compiler: open the Terminal app, type `xcode-select --install`, and press Enter. Then follow Parts 2 to 4 the same way. The kit in Step 13 is called **Desktop Qt 6.x.x clang 64-bit** or **macOS**.

## What the main folders contain

- `src/model`: patient and appointment classes (PatientRecord is the abstract base class).
- `src/controller`: the logic, including the search algorithms (`SearchEngine.cpp`) and the sort algorithms (`Sorter.cpp`).
- `src/gui`: the windows and tabs.
- `src/utils`: saving and loading files, ID numbers and error types.
- `data/clinic_data.csv`: the saved patients and appointments.
- `docs/COS202_SCAPMS_Submission.pdf`: the document to submit.
