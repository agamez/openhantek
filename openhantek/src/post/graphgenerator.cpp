// SPDX-License-Identifier: GPL-2.0+

#include <QDebug>
#include <QMutex>
#include <cmath>
#include <exception>
#include <math.h>

#include "graphgenerator.h"
#include "hantekdso/controlspecification.h"
#include "postprocessingsettings.h"
#include "ppresult.h"
#include "scopesettings.h"
#include "utils/printutils.h"
#include "viewconstants.h"
#include "viewsettings.h"


static const SampleValues &useSpecSamplesOf( ChannelID channel, const PPresult *result, const DsoSettingsScope *scope ) {
    static SampleValues emptyDefault;
    if ( !scope->spectrum[ channel ].used || !result->data( channel ) )
        return emptyDefault;
    return result->data( channel )->spectrum;
}


static const SampleValues &useVoltSamplesOf( ChannelID channel, const PPresult *result, const DsoSettingsScope *scope ) {
    static SampleValues emptyDefault;
    if ( !scope->voltage[ channel ].used || !result->data( channel ) )
        return emptyDefault;
    return result->data( channel )->voltage;
}


GraphGenerator::GraphGenerator( const DsoSettingsScope *scope, const DsoSettingsPostProcessing *postprocessing )
    : scope( scope ), postprocessing( postprocessing ) {
    // printf( "GraphGenerator::GraphGenerator()\n" );
}


void GraphGenerator::process( PPresult *data ) {
    // printf( "GraphGenerator::process()\n" );
    if ( scope->horizontal.format == Dso::GraphFormat::TY ) {
        ready = true;
        generateGraphsTYvoltage( data );
        generateGraphsTYspectrum( data );
    } else
        generateGraphsXY( data );
}


void GraphGenerator::generateGraphsTYvoltage( PPresult *result ) {
    // printf( "GraphGenerator::generateGraphsTYvoltage()\n" );
    result->vaChannelVoltage.resize( scope->voltage.size() );
    result->vaChannelHistogram.resize( scope->voltage.size() );
    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        ChannelGraph &graphVoltage = result->vaChannelVoltage[ channel ];
        ChannelGraph &graphHistogram = result->vaChannelHistogram[ channel ];
        const SampleValues &samples = useVoltSamplesOf( channel, result, scope );

        // Check if this channel is used and available at the data analyzer
        if ( samples.sample.empty() ) {
            // Delete all vector arrays
            graphVoltage.clear();
            graphHistogram.clear();
            continue;
        }

        // time distance between sampling points
        double horizontalFactor = ( samples.interval / scope->horizontal.timebase );
        unsigned dotsOnScreen = unsigned( ceil( DIVS_TIME / horizontalFactor ) );
        unsigned preTrigSamples = unsigned( scope->trigger.offset * dotsOnScreen );
        int leftmostSample = int( result->triggeredPosition );
        unsigned fullRecordStart = 0;
        if ( scope->horizontal.fullRecord ) {
            // Stretch the complete capture over the full screen width and rotate it so the
            // trigger stays at the configured screen position. Trigger search and Single-shot
            // behavior are unchanged; only the displayed record window is different.
            dotsOnScreen = unsigned( samples.sample.size() );
            horizontalFactor = DIVS_TIME / double( dotsOnScreen );
            preTrigSamples = 0;
            leftmostSample = 0;
            if ( result->triggeredPosition && dotsOnScreen ) {
                // Record navigation is centered on the trigger. The selected display
                // window may later move through the record, but its initial position
                // shows both the capture and the trigger symmetrically.
                const long long offsetSamples = ( long long )( double( dotsOnScreen ) / 2.0 + 0.5 );
                long long start = ( long long )result->triggeredPosition - offsetSamples;
                start %= ( long long )dotsOnScreen;
                if ( start < 0 )
                    start += ( long long )dotsOnScreen;
                fullRecordStart = unsigned( start );
            }
        } else if ( leftmostSample ) {          // adjust position if triggered, else start from sample[0]
            leftmostSample -= preTrigSamples;   // shift samples to show a stable trace
        }
        int leftmostPosition = 0;               // start position on display
        if ( leftmostSample < 0 ) {             // trig pos or time/div was increased
            leftmostPosition = -leftmostSample; // trace can't start on left margin
            leftmostSample = 0;                 // show as much as we have on left side
        }

        const unsigned binsPerDiv = 50;

        // Set size directly to avoid reallocations
        graphVoltage.reserve( scope->horizontal.fullRecord ? 2 * dotsOnScreen : dotsOnScreen + 1 );
        graphHistogram.reserve( int( 2 * ( binsPerDiv * DIVS_VOLTAGE ) ) );

        const double gain = scope->gain( channel );
        const double offset = scope->voltage[ channel ].offset;

        graphVoltage.clear();   // remove all previous dots and fill in new trace
        graphHistogram.clear(); // remove all previous dots and fill in new trace
        unsigned bins[ int( binsPerDiv * DIVS_VOLTAGE ) ] = {0};
        if ( scope->horizontal.fullRecord ) {
            auto countBin = [&]( double y ) {
                if ( scope->histogram ) {
                    int bin = int( round( binsPerDiv * ( y + DIVS_VOLTAGE / 2 ) ) );
                    if ( bin > 0 && bin < binsPerDiv * DIVS_VOLTAGE ) // if trace is on screen
                        ++bins[ bin ];                                // count value
                }
            };
            auto sampleAt = [&]( unsigned position ) -> double {
                unsigned index = fullRecordStart + position;
                if ( index >= dotsOnScreen )
                    index -= dotsOnScreen;
                return samples.sample[ index ] / gain + offset;
            };
            double prevX = MARGIN_LEFT;
            double prevY = sampleAt( 0 );
            unsigned prevIndex = fullRecordStart;
            countBin( prevY );
            for ( unsigned int position = 1; position < dotsOnScreen; ++position ) {
                const unsigned int index = prevIndex + 1 < dotsOnScreen ? prevIndex + 1 : 0;
                const double x = double( MARGIN_LEFT + position * horizontalFactor );
                const double y = sampleAt( position );
                countBin( y );
                // Do not connect the last sample of the capture with the first one; the
                // circular record wraps there and a connecting line would be misleading.
                if ( !( prevIndex == dotsOnScreen - 1 && index == 0 ) ) {
                    if ( !scope->histogram || x < MARGIN_RIGHT - 1.1 ) {
                        graphVoltage.push_back( QVector3D( float( prevX ), float( prevY ), 0.0f ) );
                        graphVoltage.push_back( QVector3D( float( x ), float( y ), 0.0f ) );
                    }
                }
                prevX = x;
                prevY = y;
                prevIndex = index;
            }
        } else {
            auto sampleIterator = samples.sample.cbegin() + leftmostSample; // -> visible samples
            auto sampleEnd = samples.sample.cend();
            // printf("samples: %lu, dotsOnScreen: %d\n", samples.sample.size(), dotsOnScreen);
            for ( unsigned int position = unsigned( leftmostPosition ); position < dotsOnScreen && sampleIterator < sampleEnd;
                  ++position, ++sampleIterator ) {
                double x = double( MARGIN_LEFT + position * horizontalFactor );
                double y = *sampleIterator / gain + offset;
                if ( !scope->histogram ) { // show complete trace
                    graphVoltage.push_back( QVector3D( float( x ), float( y ), 0.0f ) );
                } else { // histogram replaces trace in rightmost div
                    int bin = int( round( binsPerDiv * ( y + DIVS_VOLTAGE / 2 ) ) );
                    if ( bin > 0 && bin < binsPerDiv * DIVS_VOLTAGE ) // if trace is on screen
                        ++bins[ bin ];                                // count value
                    if ( x < MARGIN_RIGHT - 1.1 ) {                   // show trace unless in last div + 10% margin
                        graphVoltage.push_back( QVector3D( float( x ), float( y ), 0.0f ) );
                    }
                }
            }
        }
        if ( ( scope->horizontal.format == Dso::GraphFormat::TY ) && scope->histogram ) { // scale and display the histogram
            double max = 0;                                                               // find max histo count
            for ( int bin = 0; bin < binsPerDiv * DIVS_VOLTAGE; ++bin ) {
                if ( bins[ bin ] > max ) {
                    max = bins[ bin ];
                }
            }
            for ( int bin = 0; bin < binsPerDiv * DIVS_VOLTAGE; ++bin ) {
                if ( bins[ bin ] ) { // show bar (= start and end point) if value exists
                    double y = double( bin ) / binsPerDiv - DIVS_VOLTAGE / 2 - double( channel ) / binsPerDiv / 2;
                    graphHistogram.push_back( QVector3D( float( MARGIN_RIGHT ), float( y ), 0 ) );
                    graphHistogram.push_back( QVector3D( float( MARGIN_RIGHT - bins[ bin ] / max ), float( y ), 0 ) );
                }
            }
        }
    }
}


void GraphGenerator::generateGraphsTYspectrum( PPresult *result ) {
    // printf( "GraphGenerator::generateGraphsTYspectrum()\n" );
    ready = true;
    result->vaChannelSpectrum.resize( scope->spectrum.size() );
    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        ChannelGraph &graphSpectrum = result->vaChannelSpectrum[ channel ];
        const SampleValues &samples = useSpecSamplesOf( channel, result, scope );

        // Check if this channel is used and available at the data analyzer
        if ( samples.sample.empty() ) {
            // Delete all vector arrays
            graphSpectrum.clear();
            continue;
        }
        // Check if the sample count has changed
        size_t sampleCount = samples.sample.size();
        size_t neededSize = sampleCount * 2;

        // Set size directly to avoid reallocations
        graphSpectrum.reserve( neededSize );

        // The spectrum always uses the normal ten-division frequency grid.
        double horizontalFactor = samples.interval / scope->horizontal.frequencybase;

        // Fill vector array
        std::vector< double >::const_iterator dataIterator = samples.sample.begin();
        const double magnitude = scope->spectrum[ channel ].magnitude;
        const double offset = scope->spectrum[ channel ].offset;
        // The limit is the lowest value shown in the spectrum. Place it on the channel's
        // offset marker so the spectrum grows upwards from the marker instead of being
        // split symmetrically around 0 dB.
        const double baseline = postprocessing->spectrumLimit - postprocessing->spectrumReference;

        for ( unsigned int position = 0; position < sampleCount; ++position ) {
            graphSpectrum.push_back( QVector3D( float( position * horizontalFactor - DIVS_TIME / 2 ),
                                                float( ( *( dataIterator++ ) - baseline ) / magnitude + offset ), 0.0f ) );
        }
    }
}


void GraphGenerator::generateGraphsXY( PPresult *result ) {
    // puts("generateGraphXY()");
    result->vaChannelVoltage.resize( scope->voltage.size() );

    // Delete all spectrum graphs
    for ( ChannelGraph &data : result->vaChannelSpectrum )
        data.clear();

    // Generate voltage graphs for pairs of channels
    for ( ChannelID channel = 0; channel < scope->voltage.size(); channel += 2 ) {
        // We need pairs of channels.
        if ( channel + 1 == scope->voltage.size() ) {
            result->vaChannelVoltage[ channel ].clear();
            continue;
        }

        const ChannelID xChannel = channel;
        const ChannelID yChannel = channel + 1;

        const SampleValues &xSamples = useVoltSamplesOf( xChannel, result, scope );
        const SampleValues &ySamples = useVoltSamplesOf( yChannel, result, scope );

        // The channels need to be active
        if ( !xSamples.sample.size() || !ySamples.sample.size() ) {
            result->vaChannelVoltage[ xChannel ].clear();
            result->vaChannelVoltage[ yChannel ].clear();
            continue;
        }

        // Check if the sample count has changed
        const size_t sampleCount = std::min( xSamples.sample.size(), ySamples.sample.size() );
        ChannelGraph &graphXY = result->vaChannelVoltage[ yChannel ]; // color of y channel
        graphXY.reserve( sampleCount * 2 );

        // Fill vector array
        std::vector< double >::const_iterator xIterator = xSamples.sample.begin();
        std::vector< double >::const_iterator yIterator = ySamples.sample.begin();
        const double xGain = scope->gain( xChannel );
        const double yGain = scope->gain( yChannel );
        const double xOffset = ( scope->trigger.offset - 0.5 ) * DIVS_TIME;
        const double yOffset = scope->voltage[ yChannel ].offset;

        for ( unsigned int position = 0; position < sampleCount; ++position ) {
            graphXY.push_back(
                QVector3D( float( *( xIterator++ ) / xGain + xOffset ), float( *( yIterator++ ) / yGain + yOffset ), 0.0 ) );
        }
    }
}
