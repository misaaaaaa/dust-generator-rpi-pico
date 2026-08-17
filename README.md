# Generador de dust

Genera pulsos de trigger aleatorios en el estilo de un módulo **Eurorack Dust** — una señal de gate de duración y posición aleatoria dentro de cada ventana de 1 segundo.

**Hardware:** Raspberry Pi Pico 2 W  
**SDK:** Pico SDK 2.3.0

## Compilación

### Con la extensión de VS Code (recomendado)

1. Instala la extensión [Raspberry Pi Pico](https://marketplace.visualstudio.com/items?itemName=raspberry-pi.raspberry-pi-pico) en VS Code.
2. Abre esta carpeta como workspace.
3. La extensión descargará el SDK y el toolchain ARM automáticamente.
4. Haz clic en **"Compilación"** en la barra de estado inferior.
5. El archivo `build/dust-generator.uf2` quedará listo para flashear.

### Desde terminal (macOS/Linux)

```bash
# Instalar dependencias (macOS)
brew install cmake
brew install --cask gcc-arm-embedded

mkdir build && cd build
cmake .. -DPICO_BOARD=pico2_w
make -j$(nproc)
```

### Flashear

1. Mantén presionado el botón **BOOTSEL** del Pico 2 W y conéctalo por USB.
2. Aparecerá como unidad de almacenamiento.
3. Copia `build/dust-generator.uf2` a esa unidad. El Pico reiniciará automáticamente.

## Pinout

| Pin físico | GPIO | Función |
|------------|------|---------|
| Pin 31 | GP26 / ADC0 | Potenciómetro (densidad) |
| Pin 24 | GP18 / PWM | LED externo (brillo = densidad) |
| Pin 26 | GP20 | Salida Dust (gate) |
| LED onboard | CYW43 | LED de estado (parpadeo) |

## Descripción de componentes

### Potenciómetro (GP26 / ADC0)
Controla la **densidad de pulsos**: determina cuántos gates se generan por segundo (0 a 4 por segundo). El valor analógico se lee en cada ventana de 1 segundo y se mapea linealmente al rango `[0, MAX_CLICKS_PER_SEC]`.

### LED externo (GP18 / PWM)
Indica visualmente la **densidad actual**: a mayor giro del potenciómetro, mayor brillo. El duty cycle del PWM sigue directamente el valor del ADC (resolución de 12 bits, wrap en 4095).

### LED onboard (CYW43)
**Indicador de vida** del firmware: parpadea a 1 Hz (500 ms encendido / 500 ms apagado) de forma continua mientras el programa está corriendo.

### Salida Dust (GP20)
Señal de **gate digital** (3.3 V). Genera entre 0 y 4 pulsos por segundo en instantes aleatorios dentro de cada ventana de 1 segundo. La duración de cada pulso es aleatoria entre 15 ms y 50 ms.
