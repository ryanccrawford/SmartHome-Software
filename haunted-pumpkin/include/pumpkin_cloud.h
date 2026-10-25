#pragma once

// Sinric Pro custom-device implementation. The matching portal template must
// expose Power, Brightness and Mode. Color is available in the firmware as an
// optional fourth capability if the user's license permits it.
#include <SinricProDevice.h>
#include <Capabilities/PowerStateController.h>
#include <Capabilities/BrightnessController.h>
#include <Capabilities/ModeController.h>
#include <Capabilities/ColorController.h>

class SinricProPumpkin final : public SinricProDevice,
                              public PowerStateController<SinricProPumpkin>,
                              public BrightnessController<SinricProPumpkin>,
                              public ModeController<SinricProPumpkin>,
                              public ColorController<SinricProPumpkin> {
  friend class PowerStateController<SinricProPumpkin>;
  friend class BrightnessController<SinricProPumpkin>;
  friend class ModeController<SinricProPumpkin>;
  friend class ColorController<SinricProPumpkin>;

public:
  explicit SinricProPumpkin(const String &deviceId)
      : SinricProDevice(deviceId, "LIGHT") {}
};