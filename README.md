# GPS Tracker — Embedded Firmware for STM32F413

Bare-metal embedded firmware for a GPS tracking device built on the **STM32F413** (ARM Cortex-M4) using **FreeRTOS** (CMSIS-RTOS2). The system communicates with a Quectel GSM modem via AT commands over UART to establish TCP connections and transmit location data to a remote server.

## Architecture

The firmware follows a layered architecture with clear separation of concerns:

```
┌─────────────────────────────────────────┐
│  APP       cli_app, led_app, tcp_app    │
├─────────────────────────────────────────┤
│  API       uart_api, modem_api, tcp_api │
│            led_api, cmd_api, debug_api  │
├─────────────────────────────────────────┤
│  Driver    gpio_driver, uart_driver     │
│            tim_driver                   │
├─────────────────────────────────────────┤
│  Utility   ring_buffer, message,        │
│            buffer, string_util          │
├─────────────────────────────────────────┤
│  RTOS      FreeRTOS / CMSIS-RTOS2       │
└─────────────────────────────────────────┘
```

## Key Features

- **GSM modem driver** — Full AT command handler with callback-based response parsing, APN configuration, network registration, PDP context activation, and error recovery
- **TCP socket management** — Connect, send, and disconnect operations managed through an asynchronous message queue job system
- **CLI interface** — UART-based command line with argument parsing and tokenization for runtime control (LED control, TCP commands, debug)
- **LED control subsystem** — State machine-driven LED patterns managed via FreeRTOS message queues
- **Ring buffer** — Opaque-handle circular buffer for UART data reception
- **Concurrency** — Multiple FreeRTOS tasks synchronized with mutexes, event flags, and message queues

## Hardware

- **MCU:** STM32F413ZGJ (ARM Cortex-M4, 100 MHz)
- **Modem:** Quectel GSM module (UART, AT commands)
- **Peripherals:** 2x USART (debug + modem), GPIO (LEDs, modem control), Timer (TIM13)

## Build

Built with **STM32CubeIDE**. Open `STM32CubeIDE_Project.ioc` to view the pin and peripheral configuration.

## Project Structure

```
Source/
├── APP/            # Application layer (main, CLI, LED app, TCP app)
├── API/            # Hardware abstraction (UART, modem, TCP, LED, debug)
├── Driver/         # Low-level peripheral drivers (GPIO, UART, Timer)
├── Utility/        # Ring buffer, message types, string utilities
└── ThirdParty/     # STM32 HAL/LL drivers, FreeRTOS, CMSIS
```
