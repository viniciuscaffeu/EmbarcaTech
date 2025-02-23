// main.c
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"
#include "lwip/tcp.h"



#define WIFI_SSID "CLARO_2G17B736"
#define WIFI_PASSWORD "Pikachu2026"
#define THINGSPEAK_API_KEY "PQM3CQZSKJVE83KD"
#define THINGSPEAK_HOST "api.thingspeak.com"
#define THINGSPEAK_PORT 80

void send_to_thingspeak(float temperature) {
    struct tcp_pcb *pcb;
    err_t err;

    pcb = tcp_new();
    if (!pcb) {
        printf("Failed to create PCB\n");
        return;
    }

    ip_addr_t server_ip;
    if (dns_gethostbyname(THINGSPEAK_HOST, &server_ip, NULL, NULL) != ERR_OK) {
        printf("DNS lookup failed\n");
        tcp_close(pcb);
        return;
    }

    if (tcp_connect(pcb, &server_ip, THINGSPEAK_PORT, NULL) != ERR_OK) {
        printf("Connection failed\n");
        tcp_close(pcb);
        return;
    }

    char request[256];
    snprintf(request, sizeof(request),
             "GET /update?api_key=%s&field1=%.2f HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n\r\n",
             THINGSPEAK_API_KEY, temperature, THINGSPEAK_HOST);

    tcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    tcp_output(pcb);

    sleep_ms(1000); // Aguarda resposta

    tcp_close(pcb);
}

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Wi-Fi connection failed\n");
        return -1;
    }
    printf("Connected to Wi-Fi\n");

    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4); // Canal do sensor interno

    while (true) {
        uint16_t raw = adc_read();
        float voltage = raw * 3.3f / (1 << 12);
        float temperature = 27 - (voltage - 0.706) / 0.001721;

        printf("Temperature: %.2f C\n", temperature);
        send_to_thingspeak(temperature);

        sleep_ms(15000); // ThingSpeak aceita uma atualização a cada 15s
    }

    cyw43_arch_deinit();
    return 0;
}
