#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>

// Enum para os eventos possíveis de botão
enum ButtonEvent {
    BUTTON_NONE,     // Nenhum evento
    BUTTON_PRESS,    // Botão acabou de ser pressionado
    BUTTON_PRESSED,  // Botão está continuamente pressionado
    BUTTON_CLICK,    // Clique rápido
    BUTTON_HOLD      // Pressionamento longo
};

class ButtonManager {
private:
    // Estrutura para rastrear o estado dos botões
    typedef struct {
        uint8_t pin;                   // Número do pino
        bool is_pressed;               // Estado atual do botão
        unsigned long press_start_time; // Quando o botão foi pressionado
        bool hold_reported;            // Se o evento "hold" já foi reportado
        unsigned long last_hold_report; // Último momento em que o HOLD foi reportado
    } ButtonState;

    ButtonState* button_states;  // Array dinâmico para estados dos botões
    uint8_t num_buttons;         // Número de botões gerenciados

    // Função interna para encontrar o índice do botão pelo pino
    int findButtonIndex(uint8_t pin);

public:
    // Construtor e destrutor
    ButtonManager(uint8_t* pins, uint8_t count);
    ~ButtonManager();

    // Inicializa os pinos dos botões
    void begin(bool use_pullup = true);

    // Verifica eventos de botão usando número do pino
    ButtonEvent checkEvent(uint8_t pin, unsigned long hold_threshold = 1000);

    // Retorna o tempo que o botão está sendo pressionado
    unsigned long getPressTime(uint8_t pin);

    // Retorna o número de botões
    uint8_t getButtonCount() { return num_buttons; }
};

#endif // BUTTON_MANAGER_H