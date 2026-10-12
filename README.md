# Starbie

Um bichinho virtual numa placa de circuito impresso feita no KiCad. O Starbie anda numa tela OLED, reage a toques nos botões e ao movimento da placa, e mostra a temperatura e a umidade do ambiente.
<img width="1222" height="851" alt="image" src="https://github.com/user-attachments/assets/df7b532e-4272-41ce-80f2-56daa018be7d" />


## O que ele faz

- O bichinho vagueia pela tela. A tela normal não mostra nome nem valores.
- **Botão 1, primeiro toque:** abre o menu radial.
- **Inclinar a placa com o menu aberto:** move a bolinha, e cada direção corresponde a uma ação.
- **Botão 1, segundo toque:** escolhe a ação destacada.
- **Botão 2:** mostra ou esconde as estatísticas (alegria, energia, fome, temperatura e umidade).
- **Sacudir a placa:** faz o Starbie tremer e acorda ele do soninho.

Ações do menu: `NAP` (cima), `PLAY` (direita), `FEED` (baixo) e `DANCE` (esquerda).

## Hardware

| Peça | Função |
|---|---|
| Seeed XIAO ESP32-C3 (U1) | Microcontrolador |
| Display OLED 0,96" 128x64 I2C (J1) | Tela do bichinho |
| MPU6050 em header de 8 pinos (J2) | Acelerômetro, para o menu por inclinação e para sacudir |
| DHT11 (U2) com resistor de 10 kΩ (R1) | Temperatura e umidade |
| 2 botões Cherry MX (SW1 e SW2) | Controles |

### Ligações

| Função | Pino do XIAO | GPIO |
|---|---|---|
| SDA (OLED e MPU6050) | D4 | 6 |
| SCL (OLED e MPU6050) | D5 | 7 |
| Dados do DHT11 | D1 | 3 |
| Botão 1 | D2 | 4 |
| Botão 2 | D3 | 5 |

Os botões ligam o pino ao GND e usam `INPUT_PULLUP` no firmware.

## Estrutura do repositório

```
.
├── kicad2.kicad_pro        Projeto do KiCad
├── kicad2.kicad_sch        Esquemático
├── kicad2.kicad_pcb        Placa (com zona de GND em B.Cu)
├── fp-lib-table            Tabela de bibliotecas de footprints
├── Imported_Parts.pretty/  Footprints do OLED e do XIAO
├── Gerbers/                Arquivos de fabricação
└── Firmware/
    └── Starbie/
        └── Starbie.ino     Código para a IDE do Arduino
```

## Placa

Projetada no KiCad 10. O roteamento está completo, e o DRC passa sem erros, só com dois avisos de footprint (J1 e U1 diferentes da cópia da biblioteca `Imported Parts`), que não afetam a fabricação.

Para fabricar, use os arquivos da pasta `Gerbers/`: os `.gbr` de cada camada e o arquivo de furação `.drl`.

## Firmware

O programa é um único sketch do Arduino, sem PlatformIO e sem biblioteca personalizada.

1. Instale a [IDE do Arduino](https://www.arduino.cc/en/software).
2. Em **File > Preferences**, adicione em *Additional boards manager URLs*:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. Em **Tools > Board > Boards Manager**, instale **esp32 by Espressif Systems**.
4. Em **Tools > Board > esp32**, escolha **XIAO_ESP32C3**.
5. Em **Sketch > Include Library > Manage Libraries**, instale:
   - Adafruit GFX Library
   - Adafruit SSD1306
   - Adafruit MPU6050
   - DHT sensor library

   Se pedir dependências, escolha *Install all*.
6. Abra `Firmware/Starbie/Starbie.ino`, mantendo a pasta `Starbie` e o arquivo com o mesmo nome, e clique em **Upload**.

### Personalizar

As opções ficam no topo do `Starbie.ino`:

- Pinos: `I2C_SDA_PIN`, `I2C_SCL_PIN`, `DHT_PIN`, `BUTTON_ONE_PIN`, `BUTTON_TWO_PIN`.
- `USE_DHT11 = false` se a sua versão não tiver o sensor.
- `TILT_SWAP_XY`, `TILT_INVERT_X` e `TILT_INVERT_Y` se a bolinha do menu se mover na direção errada.
- O vetor `MENU_ITEMS` muda os rótulos e os efeitos do menu.
- `PET_SPRITE` é o bitmap 32x32 do bichinho. Dá para gerar o seu no [image2cpp](https://javl.github.io/image2cpp/).

## Estado do projeto

O firmware compila para a placa XIAO_ESP32C3. Ainda não foi testado numa placa montada.
