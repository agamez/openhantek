// SPDX-License-Identifier: GPL-2.0+

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QSignalBlocker>

#include <cmath>

#include "TriggerDock.h"
#include "dockwindows.h"

#include "dsosettings.h"
#include "hantekdso/controlspecification.h"
#include "sispinbox.h"
#include "utils/printutils.h"

#include <algorithm>


TriggerDock::TriggerDock( DsoSettingsScope *scope, const Dso::ControlSpecification *spec, QWidget *parent )
    : QDockWidget( tr( "Trigger" ), parent ), scope( scope ), mSpec( spec ) {

    // Initialize lists for comboboxes
    for ( ChannelID channel = 0; channel < mSpec->channels; ++channel )
        this->sourceStandardStrings << tr( "CH%1" ).arg( channel + 1 );
    // add "smooth" source
    for ( ChannelID channel = 0; channel < mSpec->channels; ++channel )
        this->sourceStandardStrings << tr( "CH%1 smooth" ).arg( channel + 1 );

    // Initialize elements
    modeLabel = new QLabel( tr( "Mode" ) );
    modeComboBox = new QComboBox();
    for ( Dso::TriggerMode mode : mSpec->triggerModes )
        modeComboBox->addItem( Dso::triggerModeString( mode ) );

    slopeLabel = new QLabel( tr( "Slope" ) );
    slopeComboBox = new QComboBox();
    for ( Dso::Slope slope : Dso::SlopeEnum )
        slopeComboBox->addItem( Dso::slopeString( slope ) );

    pulseWidthLabel = new QLabel( tr( "Width" ) );
    pulseWidthSiSpinBox = new SiSpinBox( UNIT_SECONDS );
    pulseWidthSiSpinBox->setMinimum( 1e-9 );
    pulseWidthSiSpinBox->setMaximum( 10.0 );
    pulseWidthSiSpinBox->setSingleStep( 1e-6 );
    pulseWidthSiSpinBox->setMode( -1 );

    pulseConditionLabel = new QLabel( tr( "Condition" ) );
    pulseConditionComboBox = new QComboBox();
    for ( Dso::PulseWidthCondition condition : Dso::PulseWidthConditionEnum )
        pulseConditionComboBox->addItem( Dso::pulseWidthConditionString( condition ) );

    pulsePolarityLabel = new QLabel( tr( "Polarity" ) );
    pulsePolarityComboBox = new QComboBox();
    for ( Dso::PulsePolarity polarity : Dso::PulsePolarityEnum )
        pulsePolarityComboBox->addItem( Dso::pulsePolarityString( polarity ) );

    pulseToleranceLabel = new QLabel( tr( "Tolerance" ) );
    pulseToleranceSpinBox = new QDoubleSpinBox();
    pulseToleranceSpinBox->setMinimum( 0.0 );
    pulseToleranceSpinBox->setMaximum( 100.0 );
    pulseToleranceSpinBox->setDecimals( 1 );
    pulseToleranceSpinBox->setSingleStep( 1.0 );
    pulseToleranceSpinBox->setSuffix( tr( " %" ) );

    sourceLabel = new QLabel( tr( "Source" ) );
    sourceComboBox = new QComboBox();
    sourceComboBox->addItems( sourceStandardStrings );

    dockLayout = new QGridLayout();
    dockLayout->setColumnMinimumWidth( 0, 64 );
    dockLayout->setColumnStretch( 1, 1 );
    dockLayout->setSpacing( DOCK_LAYOUT_SPACING );
    dockLayout->addWidget( modeLabel, 0, 0 );
    dockLayout->addWidget( modeComboBox, 0, 1 );
    dockLayout->addWidget( sourceLabel, 1, 0 );
    dockLayout->addWidget( sourceComboBox, 1, 1 );
    dockLayout->addWidget( slopeLabel, 2, 0 );
    dockLayout->addWidget( slopeComboBox, 2, 1 );
    dockLayout->addWidget( pulseWidthLabel, 3, 0 );
    dockLayout->addWidget( pulseWidthSiSpinBox, 3, 1 );
    dockLayout->addWidget( pulseConditionLabel, 4, 0 );
    dockLayout->addWidget( pulseConditionComboBox, 4, 1 );
    dockLayout->addWidget( pulsePolarityLabel, 5, 0 );
    dockLayout->addWidget( pulsePolarityComboBox, 5, 1 );
    dockLayout->addWidget( pulseToleranceLabel, 6, 0 );
    dockLayout->addWidget( pulseToleranceSpinBox, 6, 1 );

    dockWidget = new QWidget();
    SetupDockWidget( this, dockWidget, dockLayout );

    // Load settings into GUI
    loadSettings( scope );

    // Connect signals and slots
    connect( modeComboBox, static_cast< void ( QComboBox::* )( int ) >( &QComboBox::currentIndexChanged ), [this]( int index ) {
        this->scope->trigger.mode = mSpec->triggerModes[ unsigned( index ) ];
        updatePulseControls();
        emit modeChanged( this->scope->trigger.mode );
    } );
    connect( slopeComboBox, static_cast< void ( QComboBox::* )( int ) >( &QComboBox::currentIndexChanged ), [this]( int index ) {
        this->scope->trigger.slope = Dso::Slope( index );
        emit slopeChanged( this->scope->trigger.slope );
    } );
    connect( sourceComboBox, static_cast< void ( QComboBox::* )( int ) >( &QComboBox::currentIndexChanged ),
             [this]( unsigned index ) {
                 bool smooth = index >= mSpec->channels;
                 this->scope->trigger.smooth = smooth;
                 this->scope->trigger.source = index & ( mSpec->channels - 1 );
                 emit sourceChanged( index & ( mSpec->channels - 1 ), smooth );
             } );
    connect( pulseWidthSiSpinBox, static_cast< void ( QDoubleSpinBox::* )( double ) >( &QDoubleSpinBox::valueChanged ),
             [this]( double width ) {
                 this->scope->trigger.pulseWidth = width;
                 emit pulseWidthChanged( width );
             } );
    connect( pulseConditionComboBox, static_cast< void ( QComboBox::* )( int ) >( &QComboBox::currentIndexChanged ),
             [this]( int index ) {
                 this->scope->trigger.pulseCondition = Dso::PulseWidthCondition( index );
                 updatePulseControls();
                 emit pulseConditionChanged( this->scope->trigger.pulseCondition );
             } );
    connect( pulsePolarityComboBox, static_cast< void ( QComboBox::* )( int ) >( &QComboBox::currentIndexChanged ),
             [this]( int index ) {
                 this->scope->trigger.pulsePolarity = Dso::PulsePolarity( index );
                 emit pulsePolarityChanged( this->scope->trigger.pulsePolarity );
             } );
    connect( pulseToleranceSpinBox, static_cast< void ( QDoubleSpinBox::* )( double ) >( &QDoubleSpinBox::valueChanged ),
             [this]( double percent ) {
                 this->scope->trigger.pulseTolerancePercent = percent;
                 emit pulseToleranceChanged( percent );
             } );
}

void TriggerDock::loadSettings( DsoSettingsScope *scope ) {
    // Set values
    setMode( scope->trigger.mode );
    setSlope( scope->trigger.slope );
    setSource( int( scope->trigger.source ), scope->trigger.smooth );
    setPulseWidth( scope->trigger.pulseWidth );
    setPulseCondition( scope->trigger.pulseCondition );
    setPulsePolarity( scope->trigger.pulsePolarity );
    setPulseTolerance( scope->trigger.pulseTolerancePercent );
    updatePulseControls();
}

/// \brief Don't close the dock, just hide it
/// \param event The close event that should be handled.
void TriggerDock::closeEvent( QCloseEvent *event ) {
    this->hide();

    event->accept();
}

void TriggerDock::setMode( Dso::TriggerMode mode ) {
    int index = int( std::find( mSpec->triggerModes.begin(), mSpec->triggerModes.end(), mode ) - mSpec->triggerModes.begin() );
    QSignalBlocker blocker( modeComboBox );
    modeComboBox->setCurrentIndex( index );
    updatePulseControls();
    emit modeChanged( this->scope->trigger.mode );
}

void TriggerDock::setSlope( Dso::Slope slope ) {
    QSignalBlocker blocker( slopeComboBox );
    slopeComboBox->setCurrentIndex( int( slope ) );
}

void TriggerDock::setSource( int id, bool smooth ) {
    if ( smooth )
        id += mSpec->channels;
    if ( id >= this->sourceStandardStrings.count() )
        return;
    QSignalBlocker blocker( sourceComboBox );
    sourceComboBox->setCurrentIndex( id );
}

void TriggerDock::setPulseWidth( double width ) {
    QSignalBlocker blocker( pulseWidthSiSpinBox );
    pulseWidthSiSpinBox->setValue( width );
}

void TriggerDock::setPulsePolarity( Dso::PulsePolarity polarity ) {
    QSignalBlocker blocker( pulsePolarityComboBox );
    pulsePolarityComboBox->setCurrentIndex( int( polarity ) );
}

void TriggerDock::setPulseCondition( Dso::PulseWidthCondition condition ) {
    QSignalBlocker blocker( pulseConditionComboBox );
    pulseConditionComboBox->setCurrentIndex( int( condition ) );
    updatePulseControls();
}

void TriggerDock::setPulseTolerance( double percent ) {
    QSignalBlocker blocker( pulseToleranceSpinBox );
    pulseToleranceSpinBox->setValue( percent );
}

void TriggerDock::updatePulseControls() {
    const bool pulseMode = scope->trigger.mode == Dso::TriggerMode::PULSE;
    const bool showTolerance = pulseMode && scope->trigger.pulseCondition == Dso::PulseWidthCondition::Equal;
    slopeLabel->setVisible( !pulseMode );
    slopeComboBox->setVisible( !pulseMode );
    pulseWidthLabel->setVisible( pulseMode );
    pulseWidthSiSpinBox->setVisible( pulseMode );
    pulseConditionLabel->setVisible( pulseMode );
    pulseConditionComboBox->setVisible( pulseMode );
    pulsePolarityLabel->setVisible( pulseMode );
    pulsePolarityComboBox->setVisible( pulseMode );
    pulseToleranceLabel->setVisible( showTolerance );
    pulseToleranceSpinBox->setVisible( showTolerance );
}
