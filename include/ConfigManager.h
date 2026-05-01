#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

struct AppConfig {
    String companyName;
    String wifiSsid;
    String wifiPassword;
    String adminUser;
    String adminPass;
    bool useStaticIp;
    String staticIp;
    String gateway;
    String subnet;
    int weightDecimals;
    
    // Default values
    AppConfig() : 
        companyName("CV. DUTA SURYA"),
        wifiSsid(""),
        wifiPassword(""),
        adminUser("admin"),
        adminPass("admin123"),
        useStaticIp(false),
        staticIp("192.168.1.100"),
        gateway("192.168.1.1"),
        subnet("255.255.255.0"),
        weightDecimals(2)
    {}
};

class ConfigManager {
public:
    ConfigManager();
    bool begin();
    bool load();
    bool save();

    AppConfig& getConfig() { return _config; }
    void setCompanyName(String name) { _config.companyName = name; }
    void setWiFi(String ssid, String pass) { _config.wifiSsid = ssid; _config.wifiPassword = pass; }
    void setStaticIp(bool use, String ip, String gw, String sn) { 
        _config.useStaticIp = use; 
        _config.staticIp = ip; 
        _config.gateway = gw; 
        _config.subnet = sn; 
    }
    void setCredentials(String user, String pass) { _config.adminUser = user; _config.adminPass = pass; }
    void setWeightDecimals(int decimals) { _config.weightDecimals = decimals; }

private:
    AppConfig _config;
    const char* _filename = "/config.json";
};

#endif
