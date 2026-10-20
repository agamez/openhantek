// SPDX-License-Identifier: GPL-2.0+

#include <limits>
#include <vector>

#include "spectrummaxholdprocessor.h"
#include "postprocessingsettings.h"
#include "ppresult.h"
#include "scopesettings.h"

SpectrumMaxHoldProcessor::SpectrumMaxHoldProcessor( const DsoSettingsPostProcessing *postprocessing,
                                                    const DsoSettingsScope *scope )
    : postprocessing( postprocessing ), scope( scope ) {}


void SpectrumMaxHoldProcessor::reset() {
    maxSamples.clear();
    lastSettings.clear();
    initialized = false;
}


std::vector< double > SpectrumMaxHoldProcessor::settingsFingerprint() const {
    std::vector< double > values;
    values.reserve( 12 + scope->voltage.size() * 6 );

    values.push_back( scope->horizontal.timebase );
    values.push_back( scope->horizontal.samplerate );
    values.push_back( double( scope->horizontal.format ) );
    values.push_back( scope->trigger.offset );
    values.push_back( double( scope->trigger.source ) );
    values.push_back( double( scope->trigger.slope ) );
    values.push_back( double( scope->trigger.mode ) );
    values.push_back( scope->trigger.smooth ? 1.0 : 0.0 );
    values.push_back( double( postprocessing->spectrumWindow ) );
    values.push_back( postprocessing->spectrumReference );
    values.push_back( postprocessing->spectrumLimit );
    values.push_back( double( postprocessing->averageCount ) );

    for ( const DsoSettingsScopeVoltage &channel : scope->voltage ) {
        values.push_back( channel.used ? 1.0 : 0.0 );
        values.push_back( double( channel.gainStepIndex ) );
        values.push_back( double( channel.couplingOrMathIndex ) );
        values.push_back( channel.inverted ? 1.0 : 0.0 );
        values.push_back( channel.probeAttn );
        values.push_back( channel.trigger );
    }
    return values;
}


void SpectrumMaxHoldProcessor::process( PPresult *data ) {
    if ( !postprocessing->spectrumMaxHold ) {
        reset();
        return;
    }

    const unsigned channelCount = data->channelCount();
    const std::vector< double > settings = settingsFingerprint();

    bool resetNeeded = !initialized || settings != lastSettings || maxSamples.size() != channelCount;
    if ( !resetNeeded ) {
        for ( ChannelID channel = 0; channel < channelCount; ++channel ) {
            const std::vector< double > &samples = data->data( channel )->spectrum.sample;
            if ( !samples.empty() && maxSamples[ channel ].size() != samples.size() ) {
                resetNeeded = true;
                break;
            }
        }
    }

    if ( resetNeeded ) {
        maxSamples.assign( channelCount, std::vector< double >() );
        initialized = true;
        lastSettings = settings;
    }

    for ( ChannelID channel = 0; channel < channelCount; ++channel ) {
        std::vector< double > &samples = data->modifiableData( channel )->spectrum.sample;
        std::vector< double > &maxima = maxSamples[ channel ];

        if ( !scope->spectrum[ channel ].used || samples.empty() ) {
            maxima.clear();
            continue;
        }

        if ( maxima.size() != samples.size() )
            maxima.assign( samples.size(), -std::numeric_limits< double >::infinity() );

        for ( size_t position = 0; position < samples.size(); ++position ) {
            if ( samples[ position ] > maxima[ position ] )
                maxima[ position ] = samples[ position ];
            samples[ position ] = maxima[ position ];
        }
    }
}
