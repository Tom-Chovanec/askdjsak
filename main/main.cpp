#include "freertos/idf_additions.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_event.h"
#include "tcp_server.hpp"
#include "esp_log.h"
#include "driver/gpio.h"
#include <string.h>
#include <led.hpp>
#include <button.hpp>
#include <string>
#include <format>
#include <array>
#include <fcntl.h>

static const char* TAG = "MAIN";

extern "C" void app_main(void) {
    std::array<Led, 2> leds = {
        Led(GPIO_NUM_25),
        Led(GPIO_NUM_26),
    };

    std::array<Button, 2> buttons = {
        Button(GPIO_NUM_32),
        Button(GPIO_NUM_33),
    };

    TCPServer server(8075);

    std::string receivedData = "";
    while (true) {
        server.accept();

        for (int i = 0; i < buttons.size(); i++) {
            if (buttons[i].getPress()) {
                std::string tmp = std::format("{}on", i);
                ESP_LOGI(TAG, "Sending message: %s", tmp.c_str());

                for (auto& clientSocket : server.getClientSockets()) 
                    server.send(clientSocket, tmp);

            } else if (buttons[i].getRelease()) {
                std::string tmp = std::format("{}off", i);
                ESP_LOGI(TAG, "Sending message: %s", tmp.c_str());

                for (auto& clientSocket : server.getClientSockets()) 
                    server.send(clientSocket, tmp);
            }
        }

        for (auto& clientSocket : server.getClientSockets()) {
            int result = server.recieve(clientSocket, receivedData, 10);
            if (result < 1) continue;

            ESP_LOGI(TAG, "Received message \"%s\"", receivedData.c_str());

            int idx = receivedData[0] - '0';
            std::string tmp = receivedData.substr(1);

            if (receivedData.substr(1) == "on") {
                leds[idx].turnOn();
            } else if (receivedData.substr(1) == "off") {
                leds[idx].turnOff();
            }
        }

        vTaskDelay(1);
    }
}
