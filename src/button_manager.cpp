#include "button_manager.h"

// Construtor
ButtonManager::ButtonManager(uint8_t* pins, uint8_t count) {
    num_buttons = count;

    // Alocar memória para os estados dos botões
    button_states = new ButtonState[count];

    // Inicializar todos os estados
    for (uint8_t i = 0; i < count; i++) {
        button_states[i].pin = pins[i];
        button_states[i].is_pressed = false;
        button_states[i].press_start_time = 0;
        button_states[i].hold_reported = false;
        button_states[i].last_hold_report = 0;
    }
}

// Destrutor - liberar memória alocada
ButtonManager::~ButtonManager() {
    delete[] button_states;
}

// Função interna para encontrar o índice do botão pelo pino
int ButtonManager::findButtonIndex(uint8_t pin) {
    for (uint8_t i = 0; i < num_buttons; i++) {
        if (button_states[i].pin == pin) {
            return i;
        }
    }
    return -1; // Pino não encontrado
}

// Inicializar os pinos como entradas
void ButtonManager::begin(bool use_pullup) {
    for (uint8_t i = 0; i < num_buttons; i++) {
        if (use_pullup) {
            pinMode(button_states[i].pin, INPUT_PULLUP);
        } else {
            pinMode(button_states[i].pin, INPUT);
        }
    }
}

// Verificar evento de botão
ButtonEvent ButtonManager::checkEvent(uint8_t pin, unsigned long hold_threshold) {
    // Encontrar o índice do botão pelo pino
    int index = findButtonIndex(pin);

    // Verificar se o pino é válido
    if (index == -1) {
        return BUTTON_NONE;
    }

    // Ler o estado atual (LOW = pressionado com pullup)
    bool is_pressed_now = (digitalRead(pin) == LOW);

    // Obter a referência para este botão específico
    ButtonState* btn = &button_states[index];

    // Calcular quanto tempo o botão está pressionado
    unsigned long press_duration = 0;
    if (btn->is_pressed) {
        press_duration = millis() - btn->press_start_time;
    }

    // Botão acabou de ser pressionado
    if (is_pressed_now && !btn->is_pressed) {
        btn->is_pressed = true;
        btn->press_start_time = millis();  // Aqui é o lugar correto para definir o tempo inicial
        btn->hold_reported = false;
        btn->last_hold_report = 0;
        return BUTTON_PRESS;
    }
    // Botão continua pressionado
    else if (is_pressed_now && btn->is_pressed) {
        // Verificar se atingiu o limiar de hold
        if (press_duration >= hold_threshold) {
            // Primeiro evento de HOLD
            if (!btn->hold_reported) {
                btn->hold_reported = true;
                btn->last_hold_report = millis();
                Serial.println("primeiro evento de hold");
                return BUTTON_HOLD;
            }

            // Eventos subsequentes de HOLD - a cada intervalo de hold_threshold
            unsigned long now = millis();
            if (now - btn->last_hold_report >= hold_threshold) {
                Serial.println("subsequente evento de hold");
                btn->last_hold_report = now;
                return BUTTON_HOLD;
            }
        }
    }
    // Botão foi solto
    else if (!is_pressed_now && btn->is_pressed) {
        btn->is_pressed = false;

        // Se não foi um hold e passou do tempo de debounce, foi um clique
        if (!btn->hold_reported && press_duration >= 50) {
            return BUTTON_CLICK;
        }
    }

    return BUTTON_NONE;
}

// Obter o tempo que um botão está pressionado
unsigned long ButtonManager::getPressTime(uint8_t pin) {
    int index = findButtonIndex(pin);

    if (index == -1) {
        return 0;
    }

    ButtonState* btn = &button_states[index];

    if (btn->is_pressed) {
        return millis() - btn->press_start_time;
    }
    return 0;
}