#include "BatteryStub.h"
#include <EEPROM.h>

// ============================================================================
// STUB / PROVIZORIU - vezi BatteryStub.h
// ============================================================================

BatteryStub::BatteryStub()
    : _level(40)
    , _charging(false)
    , _thresholdCritical(10)
    , _thresholdLow(20)
    , _thresholdMedium(60)
    , _thresholdFull(100)
{
    // EEPROM nu este accesat aici; begin() va fi apelat din setup().
    // Pragurile imită valorile pe care le-ar primi de la Battery Management.
}

void BatteryStub::begin()
{
    EEPROM.begin(EEPROM_SIZE);
    loadState();

    Serial.print("[BatteryStub] loaded level=");
    Serial.print(_level);
    Serial.print("% charging=");
    Serial.println(_charging ? "true" : "false");
}

void BatteryStub::loadState()
{
    StubState state;
    EEPROM.get(STUB_EEPROM_OFFSET, state);

    Serial.print("[BatteryStub] raw EEPROM: magic=0x");
    Serial.print(state.magic, HEX);
    Serial.print(" version=");
    Serial.print(state.version);
    Serial.print(" level=");
    Serial.print(state.level);
    Serial.print(" charging=");
    Serial.println(state.charging);

    if (state.magic != STUB_MAGIC || state.version != STUB_VERSION) {
        // Prima inițializare: stare default MEDIUM, fără încărcare.
        Serial.println("[BatteryStub] no valid state found, using default");
        _level = 40;
        _charging = false;
        saveState();
    }
    else {
        _level = state.level;
        _charging = (state.charging != 0);
    }
}

void BatteryStub::saveState()
{
    StubState state;
    state.magic = STUB_MAGIC;
    state.version = STUB_VERSION;
    state.level = _level;
    state.charging = _charging ? 1 : 0;

    EEPROM.put(STUB_EEPROM_OFFSET, state);
    bool ok = EEPROM.commit();

    Serial.print("[BatteryStub] saved state, commit=");
    Serial.println(ok ? "OK" : "FAIL");
}

void BatteryStub::update()
{
    while (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        processCommand(cmd);
    }
}

uint8_t BatteryStub::getLevelPercent()
{
    return _level;
}

bool BatteryStub::isCharging()
{
    return _charging;
}

BatteryState BatteryStub::getState()
{
    if (_charging) {
        return BatteryState::CHARGING;
    }
    if (_level <= _thresholdCritical) {
        return BatteryState::CRITICAL;
    }
    if (_level <= _thresholdLow) {
        return BatteryState::LOW;
    }
    return BatteryState::NORMAL;
}

void BatteryStub::setLevel(uint8_t level)
{
    if (level > 100) {
        level = 100;
    }
    _level = level;
    saveState();

    Serial.print("[BatteryStub] after setLevel: level=");
    Serial.println(_level);
}

void BatteryStub::setCharging(bool charging)
{
    _charging = charging;
    saveState();

    Serial.print("[BatteryStub] after setCharging: charging=");
    Serial.println(_charging ? "true" : "false");
}

void BatteryStub::setThresholds(uint8_t critical, uint8_t low, uint8_t medium, uint8_t full)
{
    _thresholdCritical = critical;
    _thresholdLow      = low;
    _thresholdMedium   = medium;
    _thresholdFull     = full;

    Serial.print("[BatteryStub] thresholds set: CRITICAL=");
    Serial.print(_thresholdCritical);
    Serial.print(" LOW=");
    Serial.print(_thresholdLow);
    Serial.print(" MEDIUM=");
    Serial.print(_thresholdMedium);
    Serial.print(" FULL=");
    Serial.println(_thresholdFull);
}

void BatteryStub::printStatus()
{
    Serial.print("[BatteryStub] level=");
    Serial.print(_level);
    Serial.print("% charging=");
    Serial.print(_charging ? "true" : "false");
    Serial.print(" state=");
    switch (getState()) {
        case BatteryState::NORMAL:   Serial.print("NORMAL");   break;
        case BatteryState::LOW:      Serial.print("LOW");      break;
        case BatteryState::CRITICAL: Serial.print("CRITICAL"); break;
        case BatteryState::CHARGING: Serial.print("CHARGING"); break;
        default:                     Serial.print("UNKNOWN");  break;
    }
    Serial.print(" thresholds=");
    Serial.print(_thresholdCritical);
    Serial.print('/');
    Serial.print(_thresholdLow);
    Serial.print('/');
    Serial.print(_thresholdMedium);
    Serial.print('/');
    Serial.println(_thresholdFull);
}

void BatteryStub::processCommand(const String& cmd)
{
    // Diagnostic: confirmăm că o comandă a fost primită.
    Serial.print("[BatteryStub] command received: ");
    Serial.println(cmd);

    if (cmd.equalsIgnoreCase("bat 10")) {
        setLevel(_thresholdCritical);
        setCharging(false);
        Serial.println("[BatteryStub] set CRITICAL");
        printStatus();
    }
    else if (cmd.equalsIgnoreCase("bat 30")) {
        setLevel(_thresholdLow);
        setCharging(false);
        Serial.println("[BatteryStub] set LOW");
        printStatus();
    }
    else if (cmd.equalsIgnoreCase("bat 60")) {
        setLevel(_thresholdMedium);
        setCharging(false);
        Serial.println("[BatteryStub] set MEDIUM (NORMAL state)");
        printStatus();
    }
    else if (cmd.equalsIgnoreCase("bat 80")) {
        uint8_t value = (_thresholdMedium + _thresholdFull) / 2;
        setLevel(value);
        setCharging(false);
        Serial.println("[BatteryStub] set GOOD (NORMAL state)");
        printStatus();
    }
    else if (cmd.equalsIgnoreCase("bat 40")) {
        uint8_t value = (_thresholdLow + _thresholdMedium) / 2;
        setLevel(value);
        setCharging(false);
        Serial.println("[BatteryStub] set NORMAL");
        printStatus();
    }
    else if (cmd.equalsIgnoreCase("bat 100")) {
        setLevel(_thresholdFull);
        setCharging(true);
        Serial.println("[BatteryStub] set FULL (CHARGING state)");
        printStatus();
    }
    else if (cmd.equalsIgnoreCase("bat charging")) {
        setCharging(!_charging);
        Serial.println("[BatteryStub] toggle charging");
        printStatus();
    }
    else if (cmd.equalsIgnoreCase("bat status")) {
        printStatus();
    }
    else if (cmd.startsWith("bat level ")) {
        int value = cmd.substring(10).toInt();
        if (value >= 0 && value <= 100) {
            setLevel(static_cast<uint8_t>(value));
            Serial.print("[BatteryStub] set level=");
            Serial.print(value);
            Serial.println("%");
            printStatus();
        } else {
            Serial.println("[BatteryStub] ERROR: level must be 0..100");
        }
    }
    else if (cmd.startsWith("bat thresholds ")) {
        // Format: bat thresholds <critical> <low> <medium> <full>
        String rest = cmd.substring(15);
        rest.trim();

        int values[4] = { -1, -1, -1, -1 };
        int parsed = 0;
        while (parsed < 4 && rest.length() > 0) {
            int spaceIdx = rest.indexOf(' ');
            String token;
            if (spaceIdx == -1) {
                token = rest;
                rest = "";
            } else {
                token = rest.substring(0, spaceIdx);
                rest = rest.substring(spaceIdx + 1);
                rest.trim();
            }
            token.trim();
            values[parsed++] = token.toInt();
        }

        if (parsed != 4) {
            Serial.println("[BatteryStub] ERROR: thresholds need 4 values (critical low medium full)");
        }
        else if (values[0] >= 0 && values[0] <= values[1] &&
                 values[1] <= values[2] && values[2] <= values[3] && values[3] <= 100) {
            setThresholds(static_cast<uint8_t>(values[0]),
                          static_cast<uint8_t>(values[1]),
                          static_cast<uint8_t>(values[2]),
                          static_cast<uint8_t>(values[3]));
            printStatus();
        } else {
            Serial.println("[BatteryStub] ERROR: thresholds must be 0 <= C <= L <= M <= F <= 100");
        }
    }
    else if (cmd.equalsIgnoreCase("bat help")) {
        Serial.println("[BatteryStub] Commands:");
        Serial.println("  bat 10                   -> CRITICAL state");
        Serial.println("  bat 30                   -> LOW state");
        Serial.println("  bat 60                   -> MEDIUM level, NORMAL state");
        Serial.println("  bat 80                   -> GOOD level, NORMAL state");
        Serial.println("  bat 40                   -> NORMAL level, NORMAL state");
        Serial.println("  bat 100                  -> FULL level, CHARGING state");
        Serial.println("  bat charging             -> toggle charging state");
        Serial.println("  bat level <n>            -> set level 0..100");
        Serial.println("  bat thresholds <c> <l> <m> <f> -> set Battery Manager thresholds");
        Serial.println("  bat status               -> print current stub state");
        Serial.println("Level/charging state is persisted in EEPROM across reboots.");
    }
}
