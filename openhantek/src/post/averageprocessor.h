// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <vector>

#include "processor.h"

struct DsoSettingsPostProcessing;
struct DsoSettingsScope;

/// \brief Continuously averages corresponding samples over triggered acquisitions.
///
/// This is a moving average across acquisitions, not a boxcar filter over adjacent samples
/// of a single acquisition. The weighting factor is chosen so that the noise reduction
/// matches a block average of averageCount acquisitions.
class AverageProcessor : public Processor {
  public:
    AverageProcessor( const DsoSettingsPostProcessing *postprocessing, const DsoSettingsScope *scope );

  private:
    const DsoSettingsPostProcessing *postprocessing;
    const DsoSettingsScope *scope;

    std::vector< std::vector< double > > averageSamples; ///< Running average of the aligned samples
    std::vector< double > lastSettings;                  ///< Settings used for the running average
    unsigned referenceTriggerPosition = 0;               ///< Trigger position the average is aligned to
    unsigned lastAverageCount = 0;                       ///< Average count used for the running average
    bool initialized = false;                            ///< The first acquisition initializes the average

    void reset();
    std::vector< double > settingsFingerprint() const;
    void process( PPresult *data ) override;
};
