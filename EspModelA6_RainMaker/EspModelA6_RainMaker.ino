#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"
#include "AppInsights.h"
#include "PCF8574.h"  // Biblioteca PCF8574

#define SDA 4
#define SCL 15

// Endereço I2C do PCF8574
PCF8574 pcf8574(0x24, SDA, SCL);

// Definição de pinos (P0 a P5) correspondentes aos 6 relés
const int num_relays = 6;
int relays[] = {P0, P1, P2, P3, P4, P5};

bool switch_state[num_relays] = {true, true, true, true, true, true};  // Estados iniciais

const char *service_name = "PROV_1234";
const char *pop = "abcd1234";

void sysProvEvent(arduino_event_t *sys_event)
{
    switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
        Serial.printf("\nProvisioning Started with name \"%s\" and PoP \"%s\" on BLE\n", service_name, pop);
        printQR(service_name, pop, "ble");
        break;
    case ARDUINO_EVENT_PROV_INIT:
        wifi_prov_mgr_disable_auto_stop(10000);
        break;
    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
        wifi_prov_mgr_stop_provisioning();
        break;
    default:;
    }
}

void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx)
{
    const char *device_name = device->getDeviceName();
    const char *param_name = param->getParamName();

    if (strstr(device_name, "Relay")) {  // Checa se é um dos relés
        int relay_num = atoi(&device_name[5]);  // Extrai o número do relé (Relay1, Relay2, ...)
        if (relay_num >= 1 && relay_num <= num_relays) {
            int index = relay_num - 1;
            switch_state[index] = val.val.b;
            Serial.printf("Received value = %s for %s - %s\n", val.val.b ? "true" : "false", device_name, param_name);
            
            pcf8574.digitalWrite(relays[index], switch_state[index] ? LOW : HIGH);  // Controla o relé
            param->updateAndReport(val);
        }
    }
}

void setup()
{
    Serial.begin(115200);
    pcf8574.begin();  // Inicializa o PCF8574
    
    for (int i = 0; i < num_relays; i++) {
        pcf8574.pinMode(relays[i], OUTPUT);  // Define pinos do PCF8574 como saída
        pcf8574.digitalWrite(relays[i], switch_state[i] ? LOW : HIGH);  // Configura estado inicial dos relés
    }

    Node my_node = RMaker.initNode("ESP RainMaker Node");

    // Cria dispositivos de relés
    for (int i = 0; i < num_relays; i++) {
        char relay_name[10];
        snprintf(relay_name, sizeof(relay_name), "Relay%d", i + 1);
        Switch *relay_switch = new Switch(relay_name);
        relay_switch->addCb(write_callback);
        my_node.addDevice(*relay_switch);
    }

    RMaker.enableOTA(OTA_USING_TOPICS);
    RMaker.enableTZService();
    RMaker.enableSchedule();
    RMaker.enableScenes();
    initAppInsights();
    RMaker.enableSystemService(SYSTEM_SERV_FLAGS_ALL, 2, 2, 2);

    RMaker.start();
    
    WiFi.onEvent(sysProvEvent);
    WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, pop, service_name);
}

void loop()
{
    // Código para leitura de botão ou outras funcionalidades
    delay(100);
}
