#pragma once
// Search & Sort tab: pick a search algorithm and a sorting algorithm from
// combo boxes and see the result, the comparison count and the reasoning.

#include "controller/ClinicManager.h"

#include <QWidget>

#include <vector>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace scapms::gui {

class SearchPanel : public QWidget {
    Q_OBJECT
public:
    explicit SearchPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

signals:
    void dataChanged(const QString& message);

public slots:
    void refresh();
    void focusSearch();

private slots:
    void onSearch();
    void onSort();
    void onShowAll();
    void onFieldChanged(int index);
    void onSortAlgorithmChanged(int index);

private:
    void showPatients(const std::vector<model::PatientRecord*>& patients);
    void updateSortState();

    controller::ClinicManager& manager_;

    QLineEdit* keywordEdit_;
    QComboBox* fieldCombo_;
    QComboBox* searchAlgorithmCombo_;
    QPushButton* searchButton_;

    QComboBox* sortAlgorithmCombo_;
    QComboBox* sortFieldCombo_;
    QPushButton* sortButton_;
    QLabel* sortStateLabel_;

    QLabel* infoLabel_;
    QTableWidget* table_;
};

} // namespace scapms::gui
