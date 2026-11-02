#pragma once
// Fills the clinic with demonstration data, with dates relative to today so
// reminders, reports and the daily summary always have something to show.

#include "controller/ClinicManager.h"

namespace scapms::controller {

void loadSampleData(ClinicManager& manager);

} // namespace scapms::controller
