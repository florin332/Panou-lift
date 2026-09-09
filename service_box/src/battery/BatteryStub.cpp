#include "BatteryStub.h"
#include <EEPROM.h>

// ============================================================================
// STUB / PROVIZORIU - vezi BatteryStub.h
// ============================================================================

BatteryStub::BatteryStub()
    : _level(40)
    , _charging(false)
{
    // EEPROM nu este accesat aici; begin() va fi apelat din setup().
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

void BatteryStub::printStatus()
{
    Serial.print("[BatteryStub] level=");
    Serial.print(_level);
    Serial.print("% charging=");
    Serial.println(_charging ? "true" : "false");
}

void BatteryStub::processCommand(const String& cmd)
{
    // Diagnostic: confirmăm că o comandă a fost primită.
    Serial.print("[BatteryStub] command received: ");
    Serial.println(cmd);

    if (cmd.equalsIgnoreCase("bat low")) {
        setLevel(10);
        setCharging(false);
        Serial.println("[BatteryStub] set LOW");
        printStatus();
    }
    else if (cmd.equalsIgnoreCase("bat medium")) {
        setLevel(40);
        setCharging(false);
        Serial.println("[BatteryStub] set MEDIUM");
        printStatus();
    }
    else if (cmd.equalsIgnoreCase("bat good")) {
        setLevel(80);
        setCharging(false);
        Serial.println("[BatteryStub] set GOOD");
        printStatus();
    }
    else if (cmd.equalsIgnoreCase("bat full")) {
        setLevel(100);
        setCharging(true);
        Serial.println("[BatteryStub] set FULL (100% + charging)");
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
    else if (cmd.equalsIgnoreCase("bat help")) {
        Serial.println("[BatteryStub] Commands:");
        Serial.println("  bat low        -> 10%, not charging");
        Serial.println("  bat medium     -> 40%, not charging");
        Serial.println("  bat good       -> 80%, not charging");
        Serial.println("  bat full       -> 100%, charging");
        Serial.println("  bat charging   -> toggle charging state");
        Serial.println("  bat level <n>  -> set level 0..100");
        Serial.println("  bat status     -> print current stub state");
        Serial.println("State is persisted in EEPROM across reboots.");
    }
}
