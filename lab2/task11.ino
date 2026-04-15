#include <Arduino_HS300x.h>
#include <Arduino_BMI270_BMM150.h>
#include <Arduino_APDS9960.h>

void setup() {
  Serial.begin(115200);
  delay(1500);

  if (!HS300x.begin()) {
    Serial.println("Failed to initialize HS300x.");
    while (1);
  }

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU.");
    while (1);
  }

  if (!APDS.begin()) {
    Serial.println("Failed to initialize APDS9960.");
    while (1);
  }
}

void loop() {
  // 1. Read Sensor Data
  static float rh = 0.0;
  static float temp = 0.0;
  static float mag = 0.0;
  static int r = 0, g = 0, b = 0, clear = 0;

  rh = HS300x.readHumidity();
  temp = HS300x.readTemperature();

  if (IMU.magneticFieldAvailable()) {
    float mx, my, mz;
    IMU.readMagneticField(mx, my, mz);
    // Use the 3D magnitude of the magnetic field
    mag = sqrt((mx * mx) + (my * my) + (mz * mz)); 
  }

  if (APDS.colorAvailable()) {
    APDS.readColor(r, g, b, clear);
  }

  // 2. Compute Binary Flags (Thresholds may need tuning for your room)
  int humid_jump = (rh > 60.0) ? 1 : 0;
  int temp_rise = (temp > 28.0) ? 1 : 0;
  int mag_shift = (mag > 150.0) ? 1 : 0; 
  int light_or_color_change = (clear < 15 || clear > 800) ? 1 : 0; 

  // 3. Rule-Based Logic
  String raw_event = "BASELINE_NORMAL";

  if (humid_jump == 1 && temp_rise == 1) {
    raw_event = "BREATH_OR_WARM_AIR_EVENT";
  } else if (mag_shift == 1) {
    raw_event = "MAGNETIC_DISTURBANCE_EVENT";
  } else if (light_or_color_change == 1) {
    raw_event = "LIGHT_OR_COLOR_CHANGE_EVENT";
  }

  // 4. Cooldown / Debounce Logic
  static unsigned long lastEventTime = 0;
  static String active_event = "BASELINE_NORMAL";
  const unsigned long COOLDOWN_MS = 3000; // 3 second cooldown

  if (raw_event != "BASELINE_NORMAL") {
    // Only trigger a new event if the cooldown period has passed
    if (millis() - lastEventTime > COOLDOWN_MS) {
      active_event = raw_event;
      lastEventTime = millis();
    }
  } else {
    // If sensors are normal, only revert to baseline after cooldown expires
    if (millis() - lastEventTime > COOLDOWN_MS) {
      active_event = "BASELINE_NORMAL";
    }
  }

  // 5. Formatted Serial Output
  Serial.print("raw,rh="); Serial.print(rh, 2);
  Serial.print(",temp="); Serial.print(temp, 2);
  Serial.print(",mag="); Serial.print(mag, 2);
  Serial.print(",r="); Serial.print(r);
  Serial.print(",g="); Serial.print(g);
  Serial.print(",b="); Serial.print(b);
  Serial.print(",clear="); Serial.println(clear);

  Serial.print("flags,humid_jump="); Serial.print(humid_jump);
  Serial.print(",temp_rise="); Serial.print(temp_rise);
  Serial.print(",mag_shift="); Serial.print(mag_shift);
  Serial.print(",light_or_color_change="); Serial.println(light_or_color_change);

  Serial.print("event,"); Serial.println(active_event);

  delay(250); 
}