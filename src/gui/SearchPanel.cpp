#include "gui/SearchPanel.h"

#include "controller/SearchEngine.h"
#include "controller/Sorter.h"
#include "gui/GuiUtils.h"

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableWidget>
#include <QVBoxLayout>

namespace scapms::gui {

using controller::SearchAlgorithm;
using controller::SearchField;
using controller::SortAlgorithm;
using model::SortField;

namespace {
// Combo box index -> SortField
const SortField kSortFields[] = {SortField::Name, SortField::Id, SortField::Age, SortField::AppointmentDate};
} // namespace

SearchPanel::SearchPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    auto* layout = new QVBoxLayout(this);
    auto* top = new QHBoxLayout();

    // ---------- search controls (QGridLayout) ----------
    auto* searchBox = new QGroupBox("Search Patients");
    auto* searchGrid = new QGridLayout(searchBox);
    keywordEdit_ = new QLineEdit();
    keywordEdit_->setObjectName("keywordEdit");
    keywordEdit_->setPlaceholderText("ID (P0003), name (ade) or category (Emergency)");
    keywordEdit_->setToolTip("Press Enter to search (Ctrl+F jumps here)");
    keywordEdit_->setClearButtonEnabled(true);
    fieldCombo_ = new QComboBox();
    fieldCombo_->setObjectName("fieldCombo");
    fieldCombo_->addItems({"Patient ID", "Name", "Category"});
    fieldCombo_->setToolTip("Which field to search");
    searchAlgorithmCombo_ = new QComboBox();
    searchAlgorithmCombo_->setObjectName("searchAlgorithmCombo");
    searchAlgorithmCombo_->addItems({"Auto (choose for me)", "Linear Search", "Binary Search", "Recursive Search"});
    searchAlgorithmCombo_->setToolTip("Auto uses binary search when the data is sorted by the searched field");
    searchButton_ = new QPushButton("Search");
    searchButton_->setObjectName("searchButton");
    searchButton_->setToolTip("Run the selected search algorithm (Enter)");
    searchButton_->setEnabled(false);
    auto* showAllButton = new QPushButton("Show All");
    showAllButton->setToolTip("List every patient in the current stored order");

    searchGrid->addWidget(new QLabel("Keyword:"), 0, 0);
    searchGrid->addWidget(keywordEdit_, 0, 1, 1, 3);
    searchGrid->addWidget(new QLabel("Search by:"), 1, 0);
    searchGrid->addWidget(fieldCombo_, 1, 1);
    searchGrid->addWidget(new QLabel("Algorithm:"), 1, 2);
    searchGrid->addWidget(searchAlgorithmCombo_, 1, 3);
    searchGrid->addWidget(searchButton_, 2, 2);
    searchGrid->addWidget(showAllButton, 2, 3);
    top->addWidget(searchBox, 3);

    // ---------- sort controls ----------
    auto* sortBox = new QGroupBox("Sort Records");
    auto* sortGrid = new QGridLayout(sortBox);
    sortAlgorithmCombo_ = new QComboBox();
    sortAlgorithmCombo_->setObjectName("sortAlgorithmCombo");
    sortAlgorithmCombo_->addItems({"Insertion Sort", "Selection Sort", "Merge Sort", "Quick Sort"});
    sortAlgorithmCombo_->setToolTip("All four algorithms are written by hand (no std::sort)");
    sortFieldCombo_ = new QComboBox();
    sortFieldCombo_->setObjectName("sortFieldCombo");
    sortFieldCombo_->addItems({"Patient Name", "Patient ID", "Age", "Next Appointment Date"});
    sortButton_ = new QPushButton("SORT");
    sortButton_->setObjectName("sortButton");
    sortButton_->setToolTip("Sort the stored patient list. Binary search then works on that field.");
    sortStateLabel_ = new QLabel();
    sortGrid->addWidget(new QLabel("Sort Algorithm:"), 0, 0);
    sortGrid->addWidget(sortAlgorithmCombo_, 0, 1);
    sortGrid->addWidget(new QLabel("Sort By:"), 1, 0);
    sortGrid->addWidget(sortFieldCombo_, 1, 1);
    sortGrid->addWidget(sortButton_, 2, 1);
    sortGrid->addWidget(sortStateLabel_, 2, 0);
    top->addWidget(sortBox, 2);
    layout->addLayout(top);

    infoLabel_ = new QLabel();
    infoLabel_->setObjectName("algorithmInfo");
    infoLabel_->setWordWrap(true);
    infoLabel_->setTextFormat(Qt::RichText);
    infoLabel_->setStyleSheet("background:#fffde7; color:#333; padding:8px; border:1px solid #f0e68c; border-radius:4px;");
    layout->addWidget(infoLabel_);

    table_ = new QTableWidget(0, 6);
    table_->setObjectName("searchTable");
    table_->setHorizontalHeaderLabels({"ID", "Name", "Age", "Category", "Priority", "Details"});
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(table_, 1);

    connect(searchButton_, &QPushButton::clicked, this, &SearchPanel::onSearch);
    connect(keywordEdit_, &QLineEdit::returnPressed, this, &SearchPanel::onSearch);
    connect(keywordEdit_, &QLineEdit::textChanged, this,
            [this](const QString& text) { searchButton_->setEnabled(!text.trimmed().isEmpty()); });
    connect(showAllButton, &QPushButton::clicked, this, &SearchPanel::onShowAll);
    connect(sortButton_, &QPushButton::clicked, this, &SearchPanel::onSort);
    connect(fieldCombo_, &QComboBox::currentIndexChanged, this, &SearchPanel::onFieldChanged);
    connect(sortAlgorithmCombo_, &QComboBox::currentIndexChanged, this, &SearchPanel::onSortAlgorithmChanged);

    onSortAlgorithmChanged(0);
    refresh();
}

void SearchPanel::refresh() {
    updateSortState();
    std::vector<model::PatientRecord*> all;
    for (const auto& patient : manager_.database().patients()) {
        all.push_back(patient.get());
    }
    showPatients(all);
}

void SearchPanel::focusSearch() {
    keywordEdit_->setFocus();
    keywordEdit_->selectAll();
}

void SearchPanel::updateSortState() {
    sortStateLabel_->setText("<i>Currently: " + toQ(model::sortFieldToString(manager_.database().sortedBy())) + "</i>");
}

void SearchPanel::showPatients(const std::vector<model::PatientRecord*>& patients) {
    table_->setRowCount(0);
    for (const model::PatientRecord* patient : patients) {
        const int row = table_->rowCount();
        table_->insertRow(row);
        const QColor background = categoryBackground(patient->getCategory());
        table_->setItem(row, 0, makeItem(toQ(patient->getId()), background));
        table_->setItem(row, 1, makeItem(toQ(patient->getName()), background));
        table_->setItem(row, 2, makeItem(QString::number(patient->getAge()), background));
        table_->setItem(row, 3, makeItem(toQ(patient->getCategoryName()), background));
        table_->setItem(row, 4, makeItem(toQ(patient->getPriorityLabel()), background));
        table_->setItem(row, 5, makeItem(toQ(patient->getDetails()), background));
    }
}

// Text and combo events: Recursive only makes sense for names, and
// category search is always linear, so unavailable choices are disabled.
void SearchPanel::onFieldChanged(int index) {
    const auto field = static_cast<SearchField>(index);
    auto* model = qobject_cast<QStandardItemModel*>(searchAlgorithmCombo_->model());
    if (model != nullptr) {
        model->item(2)->setEnabled(field != SearchField::Category);
        model->item(3)->setEnabled(field == SearchField::Name);
    }
    const int current = searchAlgorithmCombo_->currentIndex();
    if ((current == 3 && field != SearchField::Name) || (current == 2 && field == SearchField::Category)) {
        searchAlgorithmCombo_->setCurrentIndex(0);
    }
}

void SearchPanel::onSortAlgorithmChanged(int index) {
    const auto algorithm = static_cast<SortAlgorithm>(index);
    infoLabel_->setText(QString("<b>%1</b> (%2)<br>Why use it: %3")
                            .arg(toQ(controller::Sorter::name(algorithm)),
                                 toQ(controller::Sorter::complexity(algorithm)),
                                 toQ(controller::Sorter::explain(algorithm))));
}

void SearchPanel::onSearch() {
    try {
        const auto field = static_cast<SearchField>(fieldCombo_->currentIndex());
        const auto algorithm = static_cast<SearchAlgorithm>(searchAlgorithmCombo_->currentIndex());
        const controller::SearchResult result = controller::SearchEngine::search(
            manager_.database(), field, fromQ(keywordEdit_->text()), algorithm);
        showPatients(result.matches);
        // Opening a single exact match counts as accessing that record (cache).
        if (result.matches.size() == 1) {
            manager_.getPatient(result.matches.front()->getId());
        }
        infoLabel_->setText(QString("<b>%1</b> found <b>%2</b> match(es) using %3 comparison(s).<br>%4")
                                .arg(toQ(result.algorithmUsed))
                                .arg(result.matches.size())
                                .arg(result.comparisons)
                                .arg(toQ(result.note)));
        if (result.matches.empty()) {
            infoLabel_->setText(infoLabel_->text() + "<br><span style='color:#c62828'>No patients matched \"" +
                                keywordEdit_->text().toHtmlEscaped() + "\".</span>");
        }
    } catch (const std::exception& error) {
        showError(this, error);
    }
}

void SearchPanel::onSort() {
    try {
        const auto algorithm = static_cast<SortAlgorithm>(sortAlgorithmCombo_->currentIndex());
        const SortField field = kSortFields[sortFieldCombo_->currentIndex()];
        const controller::SortStats stats = manager_.sortPatients(algorithm, field);
        refresh();
        infoLabel_->setText(QString("<b>%1</b> sorted %2 records by <b>%3</b>: %4 comparisons, %5 moves, %6 ms.<br>"
                                    "Complexity: %7<br>Why use it: %8")
                                .arg(toQ(controller::Sorter::name(algorithm)))
                                .arg(manager_.database().patientCount())
                                .arg(toQ(model::sortFieldToString(field)))
                                .arg(stats.comparisons)
                                .arg(stats.moves)
                                .arg(stats.milliseconds, 0, 'f', 3)
                                .arg(toQ(controller::Sorter::complexity(algorithm)))
                                .arg(toQ(controller::Sorter::explain(algorithm))));
        emit dataChanged(QString("Sorted patients by %1 using %2")
                             .arg(toQ(model::sortFieldToString(field)),
                                  toQ(controller::Sorter::name(algorithm))));
    } catch (const std::exception& error) {
        showError(this, error);
    }
}

void SearchPanel::onShowAll() {
    keywordEdit_->clear();
    refresh();
    infoLabel_->setText(QString("Showing all %1 patients in stored order (%2).")
                            .arg(manager_.database().patientCount())
                            .arg(toQ(model::sortFieldToString(manager_.database().sortedBy()))));
}

} // namespace scapms::gui
