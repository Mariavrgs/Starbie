# Starbie – devlog

Um bichinho virtual minúsculo, controlado por movimento: basicamente um Tamagotchi de mesa. Totalmente não é um Starboy.

| Semana | Tier | Horas registradas | Entradas |
|---|---|---|---|
| Semana 1 | Tier 1 | _preencha_ | 1 |

---

## Entrada 1 – Placa, firmware e entrega (11/10/2026)

**Horas desta entrada:** _preencha_

### Placa

- Montei no KiCad 10 uma placa com um XIAO ESP32-C3, um display OLED 0,96" (J1), um header de 8 pinos para o MPU6050 (J2), um DHT11 com resistor de 10 kΩ e dois botões Cherry MX.
- Rotei todas as redes: a barra de status mostra 0 itens não roteados.
- Rodei o DRC: nenhum erro, só 2 avisos dizendo que os footprints J1 e U1 diferem da cópia na biblioteca `Imported Parts`. Não afetam a fabricação, então deixei como está.
- Adicionei uma zona de cobre de GND em B.Cu cobrindo a placa toda.
- Exportei os Gerbers e o arquivo de furação para a pasta `Gerbers/`.

### Firmware

- Escrevi o `Starbie.ino` para a IDE do Arduino, com as ligações da minha placa: SDA no GPIO6, SCL no GPIO7, DHT11 no GPIO3 e os botões nos GPIO4 e GPIO5.
- Controles: botão 1 abre o menu radial e escolhe a ação, inclinar a placa move a bolinha, botão 2 mostra as estatísticas e sacudir faz o Starbie tremer.
- Menu com `NAP`, `PLAY`, `FEED` e `DANCE` (troquei o `PET` do exemplo por `DANCE`).
- Adicionei o efeito de brilho que pisca atrás do bichinho e desenhei uma estrelinha 32x32 como sprite.
- O sketch compila para a placa XIAO_ESP32C3, usando 25% da memória de programa.

### Problemas que apareceram

- A busca pela placa `xiao_esp32c3` não achava nada, porque o pacote do ESP32 ainda não estava instalado no gerenciador de placas.
- O compilador reclamou de `Adafruit_GFX.h`, porque as bibliotecas ainda não estavam instaladas.
- O botão de Debug da IDE dava erro de arquivo `.elf` não encontrado. Ele não é necessário, o certo é usar Verify e Upload.
- O Upload falhou por falta de uma porta, já que não há uma placa conectada.
- A zona de GND só apareceu depois que desenhei o contorno no KiCad e escolhi a rede GND.

### Próximos passos

- Subir tudo no GitHub, com `Gerbers/`, `Firmware/` e o `README.md`.
- Testar o firmware numa placa montada (ainda não testei em hardware).
