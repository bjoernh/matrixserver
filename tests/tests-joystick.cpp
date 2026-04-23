#include "catch.hpp"
#include <Joystick.h>
#include <matrixserver.pb.h>

TEST_CASE("Joystick fallback logic and Simulator Data mapping", "[joystick]") {
    // /dev/input/js_not_exist does not exist, so it should trigger simulator fallback
    Joystick joystick("/dev/input/js_not_exist");
    
    SECTION("Fallback to Simulator Mode when file doesn't exist") {
        // Without any simulator input, isFound() returns false (lazy activation).
        // A slot only becomes active once a simulator client sends data for it.
        REQUIRE(joystick.isFound() == false);
        REQUIRE(joystick.getButton(0) == false); // Should be default false
    }

    SECTION("Simulator Data Mapping (Protobuf)") {
        matrixserver::JoystickData protoData;
        protoData.set_joystickid(99); // Use arbitrary ID
        protoData.set_buttona(true);
        protoData.set_buttonx(true);
        protoData.set_axisx(0.75f);
        protoData.set_righttrigger(0.5f);
        protoData.set_buttondpadleft(true);

        Joystick::updateSimulatorState(protoData);

        // Instantiate joystick 99 and force it into fallback mode
        Joystick joy99("/dev/input/js99");
        REQUIRE(joy99.isFound() == true);
        
        // Button A maps to button 0
        REQUIRE(joy99.getButton(0) == true);
        // Button X maps to button 2
        REQUIRE(joy99.getButton(2) == true);
        // Button B (1) is false
        REQUIRE(joy99.getButton(1) == false);

        // Axis X maps to axis 0
        REQUIRE(joy99.getAxis(0) == Approx(0.75f));
        // Right trigger maps to axis 5
        REQUIRE(joy99.getAxis(5) == Approx(0.5f));
        // Dpad Left maps to axis 6 with -1.0f
        REQUIRE(joy99.getAxis(6) == Approx(-1.0f));

        // Test press functionality
        REQUIRE(joy99.getButtonPress(0) == true); // Button A was just pressed
        REQUIRE(joy99.getButtonPress(0) == false); // Reset after read
    }

    SECTION("Simulator Mode Clears State") {
        matrixserver::JoystickData protoData;
        protoData.set_joystickid(100);
        protoData.set_buttona(true);
        Joystick::updateSimulatorState(protoData);

        Joystick joy100("/dev/input/js100");
        REQUIRE(joy100.getButtonPress(0) == true);
        
        // Push update again
        protoData.set_buttona(false);
        Joystick::updateSimulatorState(protoData);
        protoData.set_buttona(true);
        Joystick::updateSimulatorState(protoData);

        joy100.clearAllButtonPresses();
        REQUIRE(joy100.getButtonPress(0) == false); // Should have been cleared
    }
}
