// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "utils/enumclass.h"
#include <QMetaType>
#include <QString>
namespace Dso {
/// \enum ChannelMode
/// \brief The channel display modes.
enum class ChannelMode {
    Voltage, ///< Standard voltage view
    Spectrum ///< Spectrum view
};

/// \enum GraphFormat
/// \brief The possible viewing formats for the graphs on the scope.
enum GraphFormat {
    TY, ///< The standard mode
    XY  ///< CH1 on X-axis, CH2 on Y-axis
};

extern Enum< Dso::GraphFormat, Dso::GraphFormat::TY, Dso::GraphFormat::XY > GraphFormatEnum;

/// \enum Coupling
/// \brief The coupling modes for the channels.
enum class Coupling {
    DC, ///< No filtering
    AC, ///< Offset filtered out by capacitor
    GND ///< Channel is grounded
};

/// \enum TriggerMode
/// \brief The different triggering modes.
enum class TriggerMode {
    ROLL = 0,   ///< Free running without any trigger
    AUTO = 1,   ///< Automatic without trigger event
    NORMAL = 2, ///< Normal hardware trigger (or software trigger) mode
    PULSE = 4   ///< Trigger on a pulse with the configured width
    // Value 3 was the removed SINGLE mode. Keep PULSE at 4 so old configs stay compatible.
};

/// \enum Slope
/// \brief The slope that causes a trigger.
enum class Slope : uint8_t {
    Positive = 0, ///< From lower to higher voltage
    Negative = 1, ///< From higher to lower voltage
    Both = 2      ///< At 1st level crossing up or down
};
extern Enum< Dso::Slope, Dso::Slope::Positive, Dso::Slope::Both > SlopeEnum;

/// \enum PulsePolarity
/// \brief The pulse polarity used by pulse-width triggering.
enum class PulsePolarity {
    Positive, ///< High-going pulse
    Negative, ///< Low-going pulse
    Both      ///< Positive or negative pulse
};
extern Enum< Dso::PulsePolarity, Dso::PulsePolarity::Positive, Dso::PulsePolarity::Both > PulsePolarityEnum;

/// \enum PulseWidthCondition
/// \brief How the measured pulse width is compared to the configured width.
enum class PulseWidthCondition {
    Equal,   ///< Within configured tolerance
    Greater, ///< Longer than or equal to the configured width
    Less     ///< Shorter than or equal to the configured width
};
extern Enum< Dso::PulseWidthCondition, Dso::PulseWidthCondition::Equal, Dso::PulseWidthCondition::Less > PulseWidthConditionEnum;

/// \enum InterpolationMode
/// \brief The different interpolation modes for the graphs.
enum InterpolationMode {
    INTERPOLATION_OFF = 0, ///< Just dots for each sample
    INTERPOLATION_LINEAR,  ///< Sample dots connected by lines
    INTERPOLATION_COUNT    ///< Total number of interpolation modes
};

// QString channelModeString(ChannelMode mode);
QString graphFormatString( GraphFormat format );
QString couplingString( Coupling coupling );
QString triggerModeString( TriggerMode mode );
QString slopeString( Slope slope );
QString pulsePolarityString( PulsePolarity polarity );
QString pulseWidthConditionString( PulseWidthCondition condition );
// QString interpolationModeString(InterpolationMode interpolation);
} // namespace Dso

Q_DECLARE_METATYPE( Dso::TriggerMode )
Q_DECLARE_METATYPE( Dso::Slope )
Q_DECLARE_METATYPE( Dso::PulsePolarity )
Q_DECLARE_METATYPE( Dso::PulseWidthCondition )
Q_DECLARE_METATYPE( Dso::Coupling )
Q_DECLARE_METATYPE( Dso::GraphFormat )
Q_DECLARE_METATYPE( Dso::ChannelMode )
Q_DECLARE_METATYPE( Dso::InterpolationMode )
