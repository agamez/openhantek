// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>

#include "hantekdso/enums.h"

class SiSpinBox;
struct DsoSettingsScope;
namespace Dso {
struct ControlSpecification;
}

/// \brief Dock window for the trigger settings.
/// It contains the settings for the trigger mode, source and slope.
class TriggerDock : public QDockWidget {
    Q_OBJECT

  public:
    /// \brief Initializes the trigger settings docking window.
    /// \param settings The target settings object.
    /// \param spec
    /// \param parent The parent widget.
    /// \param flags Flags for the window manager.
    TriggerDock( DsoSettingsScope *scope, const Dso::ControlSpecification *mSpec, QWidget *parent );

    /// \brief Changes the trigger mode if the new mode is supported.
    /// \param mode The trigger mode.
    void setMode( Dso::TriggerMode mode );

    /// \brief Enables/disables single-shot triggering.
    /// \param single true to stop after the next trigger event.
    void setSingle( bool single );

    /// \brief Changes the trigger source if the new source is supported.
    /// \param id The number of the channel, that should be used as trigger.
    /// \param smooth Don't trigger on glitches
    void setSource( int id, bool smooth );

    /// \brief Changes the trigger slope if the new slope is supported.
    /// \param slope The trigger slope.
    void setSlope( Dso::Slope slope );

    /// \brief Changes the pulse trigger width.
    /// \param width Pulse width in seconds.
    void setPulseWidth( double width );

    /// \brief Changes the pulse trigger polarity.
    /// \param polarity The pulse polarity.
    void setPulsePolarity( Dso::PulsePolarity polarity );

    /// \brief Changes the pulse trigger width condition.
    /// \param condition The pulse width condition.
    void setPulseCondition( Dso::PulseWidthCondition condition );

    /// \brief Changes the pulse trigger tolerance.
    /// \param percent Equal pulse width tolerance in percent.
    void setPulseTolerance( double percent );

  public slots:
    /// \brief Loads settings into GUI
    /// \param scope Settings to load
    void loadSettings( DsoSettingsScope *scope );

  protected:
    void closeEvent( QCloseEvent *event );
    void updatePulseControls();

    QGridLayout *dockLayout;   ///< The main layout for the dock window
    QWidget *dockWidget;       ///< The main widget for the dock window
    QLabel *modeLabel;         ///< The label for the trigger mode combobox
    QCheckBox *singleCheckBox; ///< Stop after the next trigger event
    QLabel *sourceLabel;       ///< The label for the trigger source combobox
    QLabel *slopeLabel;        ///< The label for the trigger slope combobox
    QLabel *pulseWidthLabel;   ///< The label for the pulse width spinbox
    QLabel *pulseConditionLabel;
    QLabel *pulsePolarityLabel;
    QLabel *pulseToleranceLabel;
    QComboBox *modeComboBox;   ///< Select the triggering mode
    QComboBox *sourceComboBox; ///< Select the source for triggering
    QComboBox *slopeComboBox;  ///< Select the slope that causes triggering
    SiSpinBox *pulseWidthSiSpinBox;
    QComboBox *pulseConditionComboBox;
    QComboBox *pulsePolarityComboBox;
    QDoubleSpinBox *pulseToleranceSpinBox;

    DsoSettingsScope *scope; ///< The settings provided by the parent class
    const Dso::ControlSpecification *mSpec;

    QStringList sourceStandardStrings; ///< Strings for the standard trigger sources

  signals:
    void modeChanged( Dso::TriggerMode );               ///< The trigger mode has been changed
    void singleChanged( bool );                         ///< The single-shot condition has been changed
    void sourceChanged( unsigned int id, bool smooth ); ///< The trigger source has been changed
    void slopeChanged( Dso::Slope );                    ///< The trigger slope has been changed
    void pulseWidthChanged( double );                    ///< The pulse trigger width has been changed
    void pulsePolarityChanged( Dso::PulsePolarity );     ///< The pulse trigger polarity has been changed
    void pulseConditionChanged( Dso::PulseWidthCondition );
    void pulseToleranceChanged( double ); ///< The equal pulse width tolerance has been changed
};
