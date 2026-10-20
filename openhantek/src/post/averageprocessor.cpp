// SPDX-License-Identifier: GPL-2.0+

#include <algorithm>
#include <vector>

#include "averageprocessor.h"
#include "postprocessingsettings.h"
#include "ppresult.h"
#include "scopesettings.h"

AverageProcessor::AverageProcessor( const DsoSettingsPostProcessing *postprocessing, const DsoSettingsScope *scope )
    : postprocessing( postprocessing ), scope( scope ) {}


void AverageProcessor::reset() {
    averageSamples.clear();
    lastSettings.clear();
    referenceTriggerPosition = 0;
    lastAverageCount = 0;
    initialized = false;
}


std::vector< double > AverageProcessor::settingsFingerprint() const {
    std::vector< double > values;
    values.reserve( 8 + scope->voltage.size() * 6 );

    values.push_back( scope->horizontal.timebase );
    values.push_back( scope->horizontal.samplerate );
    values.push_back( double( scope->horizontal.format ) );
    values.push_back( scope->trigger.offset );
    values.push_back( double( scope->trigger.source ) );
    values.push_back( double( scope->trigger.slope ) );
    values.push_back( double( scope->trigger.mode ) );
    values.push_back( scope->trigger.smooth ? 1.0 : 0.0 );

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


void AverageProcessor::process( PPresult *data ) {
    const unsigned requestedAverages = postprocessing->averageCount;

    // Averaging needs triggered acquisitions. Roll/free-running data is shown unchanged.
    if ( requestedAverages <= 1 || data->triggeredPosition == 0 ) {
        reset();
        return;
    }

    const unsigned channelCount = data->channelCount();
    const std::vector< double > settings = settingsFingerprint();

    bool resetNeeded = requestedAverages != lastAverageCount || settings != lastSettings ||
                       averageSamples.size() != channelCount;
    if ( !resetNeeded ) {
        for ( ChannelID channel = 0; channel < channelCount; ++channel ) {
            if ( averageSamples[ channel ].size() != data->data( channel )->voltage.sample.size() ) {
                resetNeeded = true;
                break;
            }
        }
    }

    if ( resetNeeded ) {
        averageSamples.assign( channelCount, std::vector< double >() );
        for ( ChannelID channel = 0; channel < channelCount; ++channel )
            averageSamples[ channel ].assign( data->data( channel )->voltage.sample.size(), 0.0 );
        initialized = false;
        referenceTriggerPosition = data->triggeredPosition;
        lastAverageCount = requestedAverages;
        lastSettings = settings;
    }

    // Align the trigger of this acquisition with the trigger position of the running average.
    // The resulting trace keeps the trigger position of the first acquisition after a reset.
    const int shift = int( data->triggeredPosition ) - int( referenceTriggerPosition );
    // alpha = 2 / (N + 1) gives the same noise power reduction as an N-acquisition block average.
    const double alpha = 2.0 / ( double( requestedAverages ) + 1.0 );

    for ( ChannelID channel = 0; channel < channelCount; ++channel ) {
        const std::vector< double > &samples = data->data( channel )->voltage.sample;
        std::vector< double > &average = averageSamples[ channel ];
        if ( samples.empty() )
            continue;

        if ( !initialized ) {
            for ( size_t i = 0; i < samples.size(); ++i ) {
                int source = int( i ) + shift;
                if ( source < 0 )
                    source = 0;
                else if ( source >= int( samples.size() ) )
                    source = int( samples.size() ) - 1;
                average[ i ] = samples[ size_t( source ) ];
            }
        } else {
            for ( size_t i = 0; i < samples.size(); ++i ) {
                int source = int( i ) + shift;
                if ( source < 0 )
                    source = 0;
                else if ( source >= int( samples.size() ) )
                    source = int( samples.size() ) - 1;
                average[ i ] += alpha * ( samples[ size_t( source ) ] - average[ i ] );
            }
        }

        std::vector< double > &output = data->modifiableData( channel )->voltage.sample;
        for ( size_t i = 0; i < output.size(); ++i )
            output[ i ] = average[ i ];
    }
    initialized = true;
    data->triggeredPosition = referenceTriggerPosition;
}
