#include <stdio.h>             // Biblioteca padrão para entrada e saída, utilizada para printf.
#include "pico/stdlib.h"       // Biblioteca padrão para funções básicas do Pico, como GPIO e temporização.
#include "hardware/adc.h"      // Biblioteca para controle do ADC (Conversor Analógico-Digital).
#include "hardware/i2c.h"      // Biblioteca para controle do I2C.
#include "ssd1306.h"           // Biblioteca para controle do display OLED.

// Definições
#define ADC_TEMPERATURE_CHANNEL 4   // Canal ADC que corresponde ao sensor de temperatura interno
#define I2C_SDA 14                  // Pino SDA do I2C
#define I2C_SCL 15                  // Pino SCL do I2C

// Função para converter o valor lido do ADC para temperatura em graus Celsius
float adc_to_temperature(uint16_t adc_value) {
    // Constantes fornecidas no datasheet do RP2040
    const float conversion_factor = 3.3f / (1 << 12);  // Conversão de 12 bits (0-4095) para 0-3.3V
    float voltage = adc_value * conversion_factor;     // Converte o valor ADC para tensão
    float temperature = 27.0f - (voltage - 0.706f) / 0.001721f;  // Equação fornecida para conversão
    return temperature;
}

int main() {
    // Inicializa a comunicação serial para permitir o uso de printf
    stdio_init_all();

    // Inicializa o módulo ADC do Raspberry Pi Pico
    adc_init();

    // Seleciona o canal 4 do ADC (sensor de temperatura interno)
    adc_set_temp_sensor_enabled(true);  // Habilita o sensor de temperatura interno
    adc_select_input(ADC_TEMPERATURE_CHANNEL);  // Seleciona o canal do sensor de temperatura

    // Inicializa o I2C para o display OLED
    i2c_init(i2c1, 400 * 1000); // 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display OLED
    ssd1306_init();

    // Preparar área de renderização para o display
    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    // Limpa o display
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    // Loop infinito para leitura contínua do valor de temperatura
    while (true) {
        // Lê o valor do ADC no canal selecionado (sensor de temperatura)
        uint16_t adc_value = adc_read();

        // Converte o valor do ADC para temperatura em graus Celsius
        float temperature = adc_to_temperature(adc_value);

        // Imprime a temperatura na comunicação serial
        printf("Temperatura: %.2f °C\n", temperature);

        // Limpa o display antes de exibir a nova temperatura
        memset(ssd, 0, ssd1306_buffer_length);

        // Exibe a temperatura no display OLED
        char temp_str[20];
        snprintf(temp_str, sizeof(temp_str), "Temp: %.2f C", temperature);
        ssd1306_draw_string(ssd, 5, 10, temp_str);  // Exibe a temperatura na linha 10

        // Renderiza o buffer no display
        render_on_display(ssd, &frame_area);

        // Atraso de 1000 milissegundos (1 segundo) entre as leituras
        sleep_ms(1000);
    }

    return 0;
}