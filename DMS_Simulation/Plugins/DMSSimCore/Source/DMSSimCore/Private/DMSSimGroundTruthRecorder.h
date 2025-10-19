#pragma once

#include "DMSSimConfig.h"
#include <ostream>

namespace DMSSimGroundTruthRecorder{
	/**
	 * Adds header to the ground truth CSV file.
	 */
	void AddHeader(std::ostream& Stream);

	/**
	 * Adds a data row to the ground truth CSV file.
	 */
	void AddFrame(std::ostream& Stream, double Time, const DMSSimGroundTruthFrame& Frame);
} // namespace DMSSimGroundTruthRecorder