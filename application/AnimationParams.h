#ifndef MATRIXSERVER_ANIMATIONPARAMS_H
#define MATRIXSERVER_ANIMATIONPARAMS_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <matrixserver.pb.h>

namespace matrixserver {

/**
 * @brief Thread-safe parameter store for animation applications.
 * Allows apps to register parameters that can be changed via the web interface.
 */
class AnimationParams {
public:
    AnimationParams();
    ~AnimationParams() = default;

    // Parameter Registration
    void registerFloat(const std::string& key, const std::string& label, float min, float max, float def, float step = 0.01f, const std::string& group = "");
    void registerInt(const std::string& key, const std::string& label, int min, int max, int def, const std::string& group = "");
    void registerBool(const std::string& key, const std::string& label, bool def, const std::string& group = "");
    void registerEnum(const std::string& key, const std::string& label, const std::vector<std::string>& options, const std::string& def, const std::string& group = "");

    // Parameter Access
    float getFloat(const std::string& key) const;
    int getInt(const std::string& key) const;
    bool getBool(const std::string& key) const;
    std::string getString(const std::string& key) const;

    // Mass Update
    void applyUpdate(const matrixserver::AppParamUpdate& update);
    
    // Serialization
    matrixserver::AppParamSchema toSchema(const std::string& appName) const;
    matrixserver::AppParamValues toValues(const std::string& appName) const;

    // Direct registry access (mostly for internal use)
    void setFloat(const std::string& key, float value);
    void setInt(const std::string& key, int value);
    void setBool(const std::string& key, bool value);
    void setString(const std::string& key, const std::string& value);

private:
    struct ParamRecord {
        matrixserver::AppParamDef def;
        float floatVal;
        int32_t intVal;
        bool boolVal;
        std::string stringVal;
    };

    mutable std::mutex mutex_;
    std::map<std::string, ParamRecord> params_;
};

} // namespace matrixserver

#endif // MATRIXSERVER_ANIMATIONPARAMS_H
