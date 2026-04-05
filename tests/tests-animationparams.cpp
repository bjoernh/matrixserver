#include "catch.hpp"
#include <AnimationParams.h>
#include <matrixserver.pb.h>

TEST_CASE("AnimationParams register and get float", "[animationparams]") {
    matrixserver::AnimationParams params;
    params.registerFloat("speed", "Speed", 0.0f, 1.0f, 0.5f, 0.01f, "general");
    REQUIRE(params.getFloat("speed") == Approx(0.5f));
}

TEST_CASE("AnimationParams register and get int", "[animationparams]") {
    matrixserver::AnimationParams params;
    params.registerInt("count", "Count", 0, 100, 42, "general");
    REQUIRE(params.getInt("count") == 42);
}

TEST_CASE("AnimationParams register and get bool", "[animationparams]") {
    matrixserver::AnimationParams params;
    params.registerBool("enabled", "Enabled", true, "general");
    REQUIRE(params.getBool("enabled") == true);

    matrixserver::AnimationParams params2;
    params2.registerBool("disabled", "Disabled", false);
    REQUIRE(params2.getBool("disabled") == false);
}

TEST_CASE("AnimationParams register and get enum", "[animationparams]") {
    matrixserver::AnimationParams params;
    std::vector<std::string> options = {"red", "green", "blue"};
    params.registerEnum("color", "Color", options, "green", "palette");
    REQUIRE(params.getString("color") == "green");
}

TEST_CASE("AnimationParams get unregistered float", "[animationparams]") {
    matrixserver::AnimationParams params;
    REQUIRE(params.getFloat("nonexistent") == Approx(0.0f));
}

TEST_CASE("AnimationParams get unregistered int", "[animationparams]") {
    matrixserver::AnimationParams params;
    REQUIRE(params.getInt("nonexistent") == 0);
}

TEST_CASE("AnimationParams get unregistered bool", "[animationparams]") {
    matrixserver::AnimationParams params;
    REQUIRE(params.getBool("nonexistent") == false);
}

TEST_CASE("AnimationParams get unregistered string", "[animationparams]") {
    matrixserver::AnimationParams params;
    REQUIRE(params.getString("nonexistent") == "");
}

TEST_CASE("AnimationParams setFloat and getFloat", "[animationparams]") {
    matrixserver::AnimationParams params;
    params.registerFloat("speed", "Speed", 0.0f, 1.0f, 0.5f);
    params.setFloat("speed", 0.75f);
    REQUIRE(params.getFloat("speed") == Approx(0.75f));
}

TEST_CASE("AnimationParams setInt and getInt", "[animationparams]") {
    matrixserver::AnimationParams params;
    params.registerInt("count", "Count", 0, 100, 10);
    params.setInt("count", 77);
    REQUIRE(params.getInt("count") == 77);
}

TEST_CASE("AnimationParams setBool and getBool", "[animationparams]") {
    matrixserver::AnimationParams params;
    params.registerBool("active", "Active", false);

    params.setBool("active", true);
    REQUIRE(params.getBool("active") == true);

    params.setBool("active", false);
    REQUIRE(params.getBool("active") == false);
}

TEST_CASE("AnimationParams set unregistered key is noop", "[animationparams]") {
    matrixserver::AnimationParams params;

    // None of these should crash or create entries
    params.setFloat("ghost", 1.0f);
    params.setInt("ghost", 1);
    params.setBool("ghost", true);
    params.setString("ghost", "value");

    REQUIRE(params.getFloat("ghost") == Approx(0.0f));
    REQUIRE(params.getInt("ghost") == 0);
    REQUIRE(params.getBool("ghost") == false);
    REQUIRE(params.getString("ghost") == "");
}

TEST_CASE("AnimationParams applyUpdate float", "[animationparams]") {
    matrixserver::AnimationParams params;
    params.registerFloat("speed", "Speed", 0.0f, 1.0f, 0.5f);

    matrixserver::AppParamUpdate update;
    update.set_key("speed");
    update.set_floatval(0.75f);
    params.applyUpdate(update);

    REQUIRE(params.getFloat("speed") == Approx(0.75f));
}

TEST_CASE("AnimationParams applyUpdate int", "[animationparams]") {
    matrixserver::AnimationParams params;
    params.registerInt("count", "Count", 0, 200, 10);

    matrixserver::AppParamUpdate update;
    update.set_key("count");
    update.set_intval(99);
    params.applyUpdate(update);

    REQUIRE(params.getInt("count") == 99);
}

TEST_CASE("AnimationParams applyUpdate bool", "[animationparams]") {
    matrixserver::AnimationParams params;
    params.registerBool("active", "Active", false);

    matrixserver::AppParamUpdate update;
    update.set_key("active");
    update.set_boolval(true);
    params.applyUpdate(update);

    REQUIRE(params.getBool("active") == true);
}

TEST_CASE("AnimationParams applyUpdate unknown key", "[animationparams]") {
    matrixserver::AnimationParams params;
    params.registerFloat("speed", "Speed", 0.0f, 1.0f, 0.5f);

    matrixserver::AppParamUpdate update;
    update.set_key("nonexistent");
    update.set_floatval(0.9f);

    // Should not crash and should not affect any existing value
    REQUIRE_NOTHROW(params.applyUpdate(update));
    REQUIRE(params.getFloat("speed") == Approx(0.5f));
    REQUIRE(params.getFloat("nonexistent") == Approx(0.0f));
}

TEST_CASE("AnimationParams toSchema", "[animationparams]") {
    matrixserver::AnimationParams params;
    params.registerFloat("speed", "Speed", 0.0f, 1.0f, 0.5f, 0.01f, "motion");
    params.registerInt("count", "Count", 1, 10, 5, "general");
    params.registerBool("active", "Active", true, "general");
    std::vector<std::string> options = {"low", "medium", "high"};
    params.registerEnum("quality", "Quality", options, "medium", "rendering");

    matrixserver::AppParamSchema schema = params.toSchema("TestApp");

    REQUIRE(schema.appname() == "TestApp");
    REQUIRE(schema.params_size() == 4);
}

TEST_CASE("AnimationParams toValues", "[animationparams]") {
    matrixserver::AnimationParams params;
    params.registerFloat("speed", "Speed", 0.0f, 1.0f, 0.5f);
    params.registerInt("count", "Count", 0, 100, 10);
    params.registerBool("active", "Active", false);
    std::vector<std::string> options = {"a", "b", "c"};
    params.registerEnum("mode", "Mode", options, "a");

    params.setFloat("speed", 0.8f);
    params.setInt("count", 42);
    params.setBool("active", true);
    params.setString("mode", "b");

    matrixserver::AppParamValues values = params.toValues("TestApp");

    REQUIRE(values.appname() == "TestApp");
    REQUIRE(values.values_size() == 4);

    bool foundSpeed = false;
    bool foundCount = false;
    bool foundActive = false;
    bool foundMode = false;

    for (int i = 0; i < values.values_size(); ++i) {
        const auto& entry = values.values(i);
        if (entry.key() == "speed") {
            REQUIRE(entry.floatval() == Approx(0.8f));
            foundSpeed = true;
        } else if (entry.key() == "count") {
            REQUIRE(entry.intval() == 42);
            foundCount = true;
        } else if (entry.key() == "active") {
            REQUIRE(entry.boolval() == true);
            foundActive = true;
        } else if (entry.key() == "mode") {
            REQUIRE(entry.stringval() == "b");
            foundMode = true;
        }
    }

    REQUIRE(foundSpeed);
    REQUIRE(foundCount);
    REQUIRE(foundActive);
    REQUIRE(foundMode);
}
