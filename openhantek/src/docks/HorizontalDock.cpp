// SPDX-License-Identifier: GPL-2.0+

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDebug>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QPushButton>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QToolButton>

#include <cmath>

#include "HorizontalDock.h"
#include "dockwindows.h"

#include "hantekdso/hantekdsocontrol.h"
#include "scopesettings.h"
#include "sispinbox.h"
#include "utils/printutils.h"
#include "viewconstants.h"

static int row = 0;

template < typename... Args > struct SELECT {
    template < typename C, typename R > static constexpr auto OVERLOAD_OF( R ( C::*pmf )( Args... ) ) -> decltype( pmf ) {
        return pmf;
    }
};

HorizontalDock::HorizontalDock( DsoSettingsScope *scope, const Dso::ControlSpecification *spec, QWidget *parent )
    : QDockWidget( tr( "Horizontal" ), parent ), scope( scope ) {

    // Initialize elements
    this->samplerateLabel = new QLabel( tr( "Samplerate" ) );
    this->samplerateSiSpinBox = new SiSpinBox( UNIT_SAMPLES );
    this->samplerateSiSpinBox->setMinimum( 1 );
    this->samplerateSiSpinBox->setMaximum( 1e8 );
    this->samplerateSiSpinBox->setUnitPostfix( tr( "/s" ) );

    this->recordLengthLabel = new QLabel( tr( "Capture depth" ) );
    this->recordLengthSpinBox = new QSpinBox();
    this->recordLengthSpinBox->setRange( int( HantekDsoControl::SAMPLESIZE_MIN ),
                                         int( HantekDsoControl::SAMPLESIZE ) );
    this->recordLengthSpinBox->setSingleStep( 1000 );
    this->recordLengthSpinBox->setSuffix( tr( " S" ) );
    this->recordLengthSpinBox->setToolTip( tr( "Number of samples captured per channel" ) );
    this->recordPositionLabel = new QLabel( tr( "Position" ) );
    this->recordPositionLabel->setToolTip( tr( "Position within the captured record" ) );
    this->recordPositionSlider = new QSlider( Qt::Horizontal );
    this->recordPositionSlider->setRange( 0, 1000 );
    this->recordPositionSlider->setMinimumWidth( 24 );
    this->recordPositionSlider->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Preferred );
    this->recordPositionSlider->setToolTip( tr( "Move the 10-division window through the captured record" ) );
    this->recordPositionLeftButton = new QToolButton();
    this->recordPositionLeftButton->setText( "<" );
    this->recordPositionLeftButton->setFixedWidth( 20 );
    this->recordPositionLeftButton->setToolTip( tr( "Show the previous window" ) );
    this->recordPositionRightButton = new QToolButton();
    this->recordPositionRightButton->setText( ">" );
    this->recordPositionRightButton->setFixedWidth( 20 );
    this->recordPositionRightButton->setToolTip( tr( "Show the next window" ) );
    this->goToTriggerButton = new QPushButton( tr( "Go to trigger" ) );
    this->goToTriggerButton->setToolTip( tr( "Center the window on the trigger" ) );
    this->recordPositionControlsWidget = new QWidget();
    this->recordPositionControlsWidget->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Preferred );
    QHBoxLayout *recordPositionLayout = new QHBoxLayout( recordPositionControlsWidget );
    recordPositionLayout->setContentsMargins( 0, 0, 0, 0 );
    recordPositionLayout->addWidget( recordPositionLeftButton );
    recordPositionLayout->addWidget( recordPositionSlider, 1 );
    recordPositionLayout->addWidget( recordPositionRightButton );

    timebaseSteps << 1.0 << 2.0 << 5.0 << 10.0;

    this->timebaseLabel = new QLabel( tr( "Timebase" ) );
    this->timebaseSiSpinBox = new SiSpinBox( UNIT_SECONDS );
    this->timebaseSiSpinBox->setSteps( timebaseSteps );
    this->timebaseSiSpinBox->setMinimum( 1e-9 );
    this->timebaseSiSpinBox->setMaximum( 1e3 );

    this->formatLabel = new QLabel( tr( "Format" ) );
    this->formatComboBox = new QComboBox();
    for ( Dso::GraphFormat format : Dso::GraphFormatEnum )
        this->formatComboBox->addItem( Dso::graphFormatString( format ) );

    this->fullRecordCheckBox = new QCheckBox( tr( "Record navigation" ) );
    this->fullRecordCheckBox->setToolTip( tr( "Retain the complete capture and navigate it in a 10-division window" ) );

    this->calfreqLabel = new QLabel( tr( "Calibration out" ) );
    this->calfreqSiSpinBox = new SiSpinBox( UNIT_HERTZ );
    this->calfreqSiSpinBox->setSteps( spec->calfreqSteps );
    this->calfreqSiSpinBox->setMinimum( spec->calfreqSteps.first() );
    this->calfreqSiSpinBox->setMaximum( spec->calfreqSteps.last() );

    this->dockLayout = new QGridLayout();
    this->dockLayout->setColumnMinimumWidth( 0, 64 );
    this->dockLayout->setColumnStretch( 1, 1 );
    this->dockLayout->setSpacing( DOCK_LAYOUT_SPACING );

    row = 0; // allows flexible shift up/down
    this->dockLayout->addWidget( this->timebaseLabel, row, 0 );
    this->dockLayout->addWidget( this->timebaseSiSpinBox, row++, 1 );
    this->dockLayout->addWidget( this->samplerateLabel, row, 0 );
    this->dockLayout->addWidget( this->samplerateSiSpinBox, row++, 1 );
    this->dockLayout->addWidget( this->recordLengthLabel, row, 0 );
    this->dockLayout->addWidget( this->recordLengthSpinBox, row++, 1 );
    this->dockLayout->addWidget( this->formatLabel, row, 0 );
    this->dockLayout->addWidget( this->formatComboBox, row++, 1 );
    this->dockLayout->addWidget( this->fullRecordCheckBox, row++, 0, 1, 2 );
    this->dockLayout->addWidget( this->recordPositionLabel, row, 0 );
    this->dockLayout->addWidget( this->recordPositionControlsWidget, row++, 1 );
    this->dockLayout->addWidget( this->goToTriggerButton, row++, 0, 1, 2 );
    this->dockLayout->addWidget( this->calfreqLabel, row, 0 );
    this->dockLayout->addWidget( this->calfreqSiSpinBox, row++, 1 );

    this->dockWidget = new QWidget();
    SetupDockWidget( this, dockWidget, dockLayout );

    // Load settings into GUI
    this->loadSettings( scope );

    // Connect signals and slots
    connect( this->samplerateSiSpinBox, SELECT< double >::OVERLOAD_OF( &QDoubleSpinBox::valueChanged ), this,
             &HorizontalDock::samplerateSelected );
    connect( this->timebaseSiSpinBox, SELECT< double >::OVERLOAD_OF( &QDoubleSpinBox::valueChanged ), this,
             &HorizontalDock::timebaseSelected );
    connect( this->recordLengthSpinBox, SELECT< int >::OVERLOAD_OF( &QSpinBox::valueChanged ), this,
             &HorizontalDock::recordLengthSelected );
    connect( this->recordPositionSlider, &QSlider::valueChanged, this, &HorizontalDock::recordPositionSelected );
    connect( this->recordPositionLeftButton, &QToolButton::clicked, this, &HorizontalDock::recordPositionLeft );
    connect( this->recordPositionRightButton, &QToolButton::clicked, this, &HorizontalDock::recordPositionRight );
    connect( this->goToTriggerButton, &QPushButton::clicked, this, &HorizontalDock::goToTrigger );
    connect( this->fullRecordCheckBox, &QCheckBox::toggled, this, &HorizontalDock::fullRecordSelected );
    connect( this->formatComboBox, SELECT< int >::OVERLOAD_OF( &QComboBox::currentIndexChanged ), this,
             &HorizontalDock::formatSelected );
    connect( this->calfreqSiSpinBox, SELECT< double >::OVERLOAD_OF( &QDoubleSpinBox::valueChanged ), this,
             &HorizontalDock::calfreqSelected );
}

void HorizontalDock::loadSettings( DsoSettingsScope *scope ) {
    // Set values. Full record and capture depth must be set before the timebase
    // and samplerate, because both affect the available samplerate range.
    {
        QSignalBlocker blocker( fullRecordCheckBox );
        fullRecordCheckBox->setChecked( scope->horizontal.fullRecord );
    }
    this->setRecordLength( scope->horizontal.recordLength );
    this->setRecordPosition( scope->horizontal.recordPosition );
    this->setTimebase( scope->horizontal.timebase );
    this->setSamplerate( scope->horizontal.samplerate );
    this->setFormat( scope->horizontal.format );
    this->setCalfreq( scope->horizontal.calfreq );
    updateRecordPositionControls();
}

/// \brief Don't close the dock, just hide it.
/// \param event The close event that should be handled.
void HorizontalDock::closeEvent( QCloseEvent *event ) {
    this->hide();
    event->accept();
}


double HorizontalDock::setSamplerate( double samplerate ) {
    // printf( "HD::setSamplerate( %g )\n", samplerate );
    QSignalBlocker blocker( timebaseSiSpinBox );
    timebaseSiSpinBox->setMaximum( scope->horizontal.maxTimebase );
    blocker = QSignalBlocker( samplerateSiSpinBox );
    samplerateSiSpinBox->setValue( samplerate );
    updateRecordPositionControls();
    return samplerateSiSpinBox->value();
}


double HorizontalDock::setTimebase( double timebase ) {
    // printf( "HD::setTimebase( %g )\n", timebase );
    QSignalBlocker blocker( timebaseSiSpinBox );
    // timebaseSteps are repeated in each decade
    double decade = pow( 10, floor( log10( timebase ) ) );
    double vNorm = timebase / decade;
    for ( int i = 0; i < timebaseSteps.size() - 1; ++i ) {
        if ( timebaseSteps.at( i ) <= vNorm && vNorm < timebaseSteps.at( i + 1 ) ) {
            timebaseSiSpinBox->setValue( decade * timebaseSteps.at( i ) );
            break;
        }
    }
    // printf( "return %g\n", timebaseSiSpinBox->value() );
    calculateSamplerateSteps( timebase );
    updateRecordPositionControls();
    return timebaseSiSpinBox->value();
}


void HorizontalDock::setRecordLength( unsigned int recordLength ) {
    if ( !recordLength )
        recordLength = HantekDsoControl::SAMPLESIZE;
    recordLength = qBound( HantekDsoControl::SAMPLESIZE_MIN, recordLength, HantekDsoControl::SAMPLESIZE );
    {
        QSignalBlocker blocker( recordLengthSpinBox );
        recordLengthSpinBox->setValue( int( recordLength ) );
    }
    scope->horizontal.recordLength = recordLength;
    updateRecordPositionControls();
}


void HorizontalDock::setRecordPosition( double position ) {
    position = qBound( 0.0, position, 1.0 );
    {
        QSignalBlocker blocker( recordPositionSlider );
        recordPositionSlider->setValue( int( position * 1000.0 + 0.5 ) );
    }
    scope->horizontal.recordPosition = position;
}


void HorizontalDock::updateRecordPositionControls() {
    const bool fullRecord = scope->horizontal.fullRecord;
    const bool canNavigate = fullRecord && scope->horizontal.recordLength && scope->horizontal.samplerate > 0.0 &&
                             double( scope->horizontal.recordLength ) / scope->horizontal.samplerate >
                                 scope->horizontal.timebase * DIVS_TIME;
    recordPositionLabel->setVisible( fullRecord );
    recordPositionControlsWidget->setVisible( fullRecord );
    goToTriggerButton->setVisible( fullRecord );
    recordPositionSlider->setEnabled( canNavigate );
    recordPositionLeftButton->setEnabled( canNavigate );
    recordPositionRightButton->setEnabled( canNavigate );
    goToTriggerButton->setEnabled( canNavigate );
}


double HorizontalDock::recordPositionStep() const {
    const double samples = scope->horizontal.recordLength;
    const double visible = scope->horizontal.samplerate * scope->horizontal.timebase * DIVS_TIME;
    if ( !( samples > visible ) || !( visible > 0.0 ) )
        return 0.0;
    return visible / 4.0 / ( samples - visible );
}


int HorizontalDock::setFormat( Dso::GraphFormat format ) {
    QSignalBlocker blocker( formatComboBox );
    if ( format >= Dso::GraphFormat::TY && format <= Dso::GraphFormat::XY ) {
        formatComboBox->setCurrentIndex( format );
        return format;
    }
    return -1;
}


double HorizontalDock::setCalfreq( double calfreq ) {
    QSignalBlocker blocker( calfreqSiSpinBox );
    calfreqSiSpinBox->setValue( calfreq );
    return calfreqSiSpinBox->value();
}


void HorizontalDock::setSamplerateLimits( double minimum, double maximum ) {
    // printf( "HD::setSamplerateLimits( %g, %g )\n", minimum, maximum );
    QSignalBlocker blocker( samplerateSiSpinBox );
    if ( bool( minimum ) )
        this->samplerateSiSpinBox->setMinimum( minimum );
    if ( bool( maximum ) )
        this->samplerateSiSpinBox->setMaximum( maximum );
}


void HorizontalDock::setSamplerateSteps( int mode, const QList< double > steps ) {
    // printf( "HD::setSamplerateSteps( %d )\n", mode );
    // qDebug() << "HD::setSamplerateSteps" << mode << steps;
    samplerateSteps = steps;
    // Assume that method is invoked for fixed samplerate devices only
    QSignalBlocker samplerateBlocker( samplerateSiSpinBox );
    samplerateSiSpinBox->setMode( mode );
    samplerateSiSpinBox->setSteps( steps );
    samplerateSiSpinBox->setMinimum( steps.first() );
    samplerateSiSpinBox->setMaximum( steps.last() );
    // Make reasonable adjustments to the timebase spinbox
    QSignalBlocker timebaseBlocker( timebaseSiSpinBox );
    timebaseSiSpinBox->setMinimum( pow( 10, floor( log10( 1.0 / steps.last() ) ) ) );
    calculateSamplerateSteps( timebaseSiSpinBox->value() );
}


/// \brief Called when the samplerate spinbox changes its value.
/// \param samplerate The samplerate in samples/second.
void HorizontalDock::samplerateSelected( double samplerate ) {
    // printf( "HD::samplerateSelected( %g )\n", samplerate );
    scope->horizontal.samplerate = samplerate;
    updateRecordPositionControls();
    emit samplerateChanged( samplerate );
}


/// \brief Called when the timebase spinbox changes its value.
/// \param timebase The timebase in seconds.
void HorizontalDock::timebaseSelected( double timebase ) {
    // printf( "HD::timebaseSelected( %g )\n", timebase );
    scope->horizontal.timebase = timebase;
    calculateSamplerateSteps( timebase );
    updateRecordPositionControls();
    emit timebaseChanged( timebase );
}


void HorizontalDock::fullRecordSelected( bool enabled ) {
    scope->horizontal.fullRecord = enabled;
    updateRecordPositionControls();
    calculateSamplerateSteps( scope->horizontal.timebase );
    emit fullRecordChanged( enabled );
}


void HorizontalDock::recordLengthSelected( int recordLength ) {
    scope->horizontal.recordLength = unsigned( recordLength );
    updateRecordPositionControls();
    calculateSamplerateSteps( scope->horizontal.timebase );
    emit recordLengthChanged( unsigned( recordLength ) );
}


void HorizontalDock::recordPositionSelected( int position ) {
    scope->horizontal.recordPosition = double( position ) / 1000.0;
    emit recordPositionChanged( scope->horizontal.recordPosition );
}


void HorizontalDock::recordPositionLeft() {
    setRecordPosition( qBound( 0.0, scope->horizontal.recordPosition - recordPositionStep(), 1.0 ) );
    emit recordPositionChanged( scope->horizontal.recordPosition );
}


void HorizontalDock::recordPositionRight() {
    setRecordPosition( qBound( 0.0, scope->horizontal.recordPosition + recordPositionStep(), 1.0 ) );
    emit recordPositionChanged( scope->horizontal.recordPosition );
}


void HorizontalDock::goToTrigger() {
    const double samples = scope->horizontal.recordLength;
    const double visible = scope->horizontal.samplerate * scope->horizontal.timebase * DIVS_TIME;
    if ( !( samples > visible ) || !( visible > 0.0 ) )
        return;
    setRecordPosition( 0.5 );
    emit recordPositionChanged( scope->horizontal.recordPosition );
}


void HorizontalDock::calculateSamplerateSteps( double timebase ) {
    int size = samplerateSteps.size();
    if ( size ) {
        // Number of samples that have to fit into one division. The triggered modes use half
        // of the capture for the trigger window; Full record must not change that. Roll uses
        // its smaller free-running buffer unless Full record captures the complete block.
        const unsigned requestedSamples = scope->horizontal.recordLength ? scope->horizontal.recordLength
                                                                         : HantekDsoControl::SAMPLESIZE;
        double samplesPerScreen = double( requestedSamples ) / 2.0;
        if ( !scope->horizontal.fullRecord && scope->trigger.mode == Dso::TriggerMode::ROLL )
            samplesPerScreen = double( HantekDsoControl::SAMPLESIZE_ROLL + 256 );
        const double maxSamplesPerDiv = samplesPerScreen / DIVS_TIME;
        // search appropriate min & max sample rate
        double min = samplerateSteps[ 0 ];
        double max = samplerateSteps[ 0 ];
        for ( int id = 0; id < size; ++id ) {
            double sRate = samplerateSteps[ id ];
            // printf( "sRate %g, sRate*timebase %g\n", sRate, sRate * timebase );
            // min must be < maxRate
            // find minimal samplerate to get at least this number of samples per div
            if ( id < size - 1 && sRate * timebase <= 10 ) { // 10 samples/div
                min = sRate;
            }
            // max must be > minRate
            // find max samplerate to get not more than this number of samples per div
            if ( id && sRate * timebase <= maxSamplesPerDiv ) {
                max = sRate;
            }
        }
        min = qMax( min, qMin( 10e3, max ) ); // not less than 10kS unless max is smaller
        // printf( "HD::cSS( %g )  -> %g, %g\n", timebase, min, max );
        setSamplerateLimits( min, max );
    }
}


/// \brief Called when the format combo box changes its value.
/// \param index The index of the combo box item.
void HorizontalDock::formatSelected( int index ) {
    scope->horizontal.format = Dso::GraphFormat( index );
    emit formatChanged( scope->horizontal.format );
}


/// \brief Called when the calfreq spinbox changes its value.
/// \param calfreq The calibration frequency in hertz.
void HorizontalDock::calfreqSelected( double calfreq ) {
    // printf( "calfreqSelected: %g\n", calfreq );
    scope->horizontal.calfreq = calfreq;
    emit calfreqChanged( calfreq );
}
