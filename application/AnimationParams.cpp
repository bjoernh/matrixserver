#include "AnimationParams.h"
#include <iostream>

namespace matrixserver {

AnimationParams::AnimationParams() : mutex_(), params_() {}

void AnimationParams::registerFloat(const std::string& key, const std::string& label, float min, float max, float def, float step,
                                    const std::string& group) {
    std::lock_guard<std::mutex> lock(mutex_);
    ParamRecord record;
    record.def.set_key(key);
    record.def.set_label(label);
    record.def.set_type("float");
    record.def.set_minval(min);
    record.def.set_maxval(max);
    record.def.set_step(step);
    record.def.set_defaultval(def);
    record.def.set_group(group);
    record.floatVal = def;
    params_[key] = record;
}

void AnimationParams::registerInt(const std::string& key, const std::string& label, int min, int max, int def, const std::string& group) {
    std::lock_guard<std::mutex> lock(mutex_);
    ParamRecord record;
    record.def.set_key(key);
    record.def.set_label(label);
    record.def.set_type("int");
    record.def.set_minval(static_cast<float>(min));
    record.def.set_maxval(static_cast<float>(max));
    record.def.set_step(1.0f);
    record.def.set_defaultval(static_cast<float>(def));
    record.def.set_group(group);
    record.intVal = def;
    params_[key] = record;
}

void AnimationParams::registerBool(const std::string& key, const std::string& label, bool def, const std::string& group) {
    std::lock_guard<std::mutex> lock(mutex_);
    ParamRecord record;
    record.def.set_key(key);
    record.def.set_label(label);
    record.def.set_type("bool");
    record.def.set_defaultval(def ? 1.0f : 0.0f);
    record.def.set_group(group);
    record.boolVal = def;
    params_[key] = record;
}

void AnimationParams::registerEnum(const std::string& key, const std::string& label, const std::vector<std::string>& options, const std::string& def,
                                   const std::string& group) {
    std::lock_guard<std::mutex> lock(mutex_);
    ParamRecord record;
    record.def.set_key(key);
    record.def.set_label(label);
    record.def.set_type("enum");
    for (const std::string& opt : options) {
        record.def.add_enumoptions(opt);
    }
    record.def.set_group(group);
    record.stringVal = def;
    params_[key] = record;
}

float AnimationParams::getFloat(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = params_.find(key);
    if (it != params_.end())
        return it->second.floatVal;
    return 0.0f;
}

int AnimationParams::getInt(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = params_.find(key);
    if (it != params_.end())
        return it->second.intVal;
    return 0;
}

bool AnimationParams::getBool(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = params_.find(key);
    if (it != params_.end())
        return it->second.boolVal;
    return false;
}

std::string AnimationParams::getString(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = params_.find(key);
    if (it != params_.end())
        return it->second.stringVal;
    return "";
}

void AnimationParams::applyUpdate(const matrixserver::AppParamUpdate& update) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = params_.find(update.key());
    if (it != params_.end()) {
        const std::string& type = it->second.def.type();
        if (type == "float") {
            it->second.floatVal = update.floatval();
        } else if (type == "int") {
            it->second.intVal = update.intval();
        } else if (type == "bool") {
            it->second.boolVal = update.boolval();
        } else if (type == "enum") {
            it->second.stringVal = update.stringval();
        }
    }
}

matrixserver::AppParamSchema AnimationParams::toSchema(const std::string& appName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    matrixserver::AppParamSchema schema;
    schema.set_appname(appName);
    for (auto const& x : params_) {
        matrixserver::AppParamDef* def = schema.add_params();
        def->CopyFrom(x.second.def);
    }
    return schema;
}

matrixserver::AppParamValues AnimationParams::toValues(const std::string& appName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    matrixserver::AppParamValues values;
    values.set_appname(appName);
    for (auto const& x : params_) {
        matrixserver::AppParamUpdate* v = values.add_values();
        v->set_key(x.first);
        const std::string& type = x.second.def.type();
        if (type == "float") {
            v->set_floatval(x.second.floatVal);
        } else if (type == "int") {
            v->set_intval(x.second.intVal);
        } else if (type == "bool") {
            v->set_boolval(x.second.boolVal);
        } else if (type == "enum") {
            v->set_stringval(x.second.stringVal);
        }
    }
    return values;
}

void AnimationParams::setFloat(const std::string& key, float value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = params_.find(key);
    if (it != params_.end())
        it->second.floatVal = value;
}

void AnimationParams::setInt(const std::string& key, int value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = params_.find(key);
    if (it != params_.end())
        it->second.intVal = value;
}

void AnimationParams::setBool(const std::string& key, bool value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = params_.find(key);
    if (it != params_.end())
        it->second.boolVal = value;
}

void AnimationParams::setString(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = params_.find(key);
    if (it != params_.end())
        it->second.stringVal = value;
}

} // namespace matrixserver
