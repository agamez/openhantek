// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <vector>

#include "processor.h"

struct DsoSettingsPostProcessing;
struct DsoSettingsScope;

/// \brief Keeps the maximum magnitude of every spectrum bin over consecutive acquisitions.
///
/// The processor replaces the current spectrum with the held maximum when the max-hold
/// display mode is enabled. All other spectrum values (peak frequency, measurements) are
/// calculated before this processor runs and are therefore not affected.
class SpectrumMaxHoldProcessor : public Processor {
  public:
    SpectrumMaxHoldProcessor( const DsoSettingsPostProcessing *postprocessing, const DsoSettingsScope *scope );

  private:
    const DsoSettingsPostProcessing *postprocessing;
    const DsoSettingsScope *scope;

    std::vector< std::vector< double > > maxSamples; ///< Held maximum for each channel and frequency bin
    std::vector< double > lastSettings;              ///< Settings used for the held maxima
    bool initialized = false;                        ///< The held maxima have been initialized

    void reset();
    std::vector< double > settingsFingerprint() const;
    void process( PPresult *data ) override;
};
