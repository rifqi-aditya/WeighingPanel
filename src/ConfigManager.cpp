#include "ConfigManager.h"

ConfigManager::ConfigManager() {}

bool ConfigManager::begin() {
    if (!LittleFS.begin(true)) {
        Serial.println("ConfigManager: LittleFS Mount Failed!");
        return false;
    }
    return load();
}

bool ConfigManager::load() {
    Serial.println("ConfigManager: Loading settings...");
    if (!LittleFS.exists(_filename)) {
        Serial.println("ConfigManager: Config file not found, creating default.");
        return save(); 
    }

    File configFile = LittleFS.open(_filename, "r");
    if (!configFile) {
        Serial.println("ConfigManager: Failed to open config file for reading.");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error) {
        Serial.print("ConfigManager: JSON Parse Error: ");
        Serial.println(error.c_str());
        return false;
    }

    _config.companyName = doc["companyName"] | "PT. Nama Perusahaan";
    _config.wifiSsid = doc["wifiSsid"] | "";
    _config.wifiPassword = doc["wifiPassword"] | "";
    _config.adminUser = doc["adminUser"] | "admin";
    _config.adminPass = doc["adminPass"] | "admin123";
    _config.useStaticIp = doc["useStaticIp"] | false;
    _config.staticIp = doc["staticIp"] | "192.168.1.100";
    _config.gateway = doc["gateway"] | "192.168.1.1";
    _config.subnet = doc["subnet"] | "255.255.255.0";
    _config.weightDecimals = doc["weightDecimals"] | 2;
    
    Serial.print("ConfigManager: Loaded Company Name: ");
    Serial.println(_config.companyName);
    Serial.print("ConfigManager: WiFi SSID: ");
    Serial.println(_config.wifiSsid);
    return true;
}

bool ConfigManager::save() {
    Serial.println("ConfigManager: Saving settings...");
    JsonDocument doc;
    doc["companyName"] = _config.companyName;
    doc["wifiSsid"] = _config.wifiSsid;
    doc["wifiPassword"] = _config.wifiPassword;
    doc["adminUser"] = _config.adminUser;
    doc["adminPass"] = _config.adminPass;
    doc["useStaticIp"] = _config.useStaticIp;
    doc["staticIp"] = _config.staticIp;
    doc["gateway"] = _config.gateway;
    doc["subnet"] = _config.subnet;
    doc["weightDecimals"] = _config.weightDecimals;

    File configFile = LittleFS.open(_filename, "w");
    if (!configFile) {
        Serial.println("ConfigManager: Failed to open config file for writing.");
        return false;
    }

    if (serializeJson(doc, configFile) == 0) {
        Serial.println("ConfigManager: Failed to serialize JSON.");
        configFile.close();
        return false;
    }

    configFile.close();
    Serial.println("ConfigManager: Settings saved and file closed.");
    return true;
}
