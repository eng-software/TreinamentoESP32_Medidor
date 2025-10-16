 # Treinamento: Medidor de pressão com Sensor SMP3011, display OLED SSD1306 com ESP32 usando ESP-IDF 5.4.2 e LVGL

## Sensor SMP3011  

O **SMP3011** é um **sensor de pressão** que se comunica via protocolo **I²C**.  

### 🔑 Características principais  
- Leitura de temperatura e pressão
- Conversor Analógico-Digital (**ADC**) de **24 bits** para **pressão**  
- Conversor Analógico-Digital (**ADC**) de **16 bits** para **temperatura**  

---

## Display SSD1306

O **SSD1306** é um **display OLED** que se comunica via protocolo **I²C**.  

### 🔑 Características principais  
- Monocromático
- Resolução (LxA) 128x64


## Hardware  e conexões com ESP32
  
O sensor de pressão e o display OLED estão ligados em portas I2C diferentes do ESP32.
* Porta I2C0:
  - SDA: 5
  - SCL: 4
  - Frequencia: 400KHz
  - Conectado ao display SSD1306

* Porta I2C1:
  - SDA: 33
  - SCL: 32
  - Frequencia: 400KHz
  - Conectado ao SMP3011

* Led de Status
  - Led azul para indicar status no GPIO16 

## Software C/C++

Nesta versão foi criada uma classe I2C do ESP32.  
A intenção desta alteração é permitir o compartilhamento do I2C com outros dispositivos.  
O recurso de I2C é instanciado em CGlobalResources.cpp , de forma a deixar o objeto global.  
O SMP3011 usa o objeto I2C para transmitir e receber dados.  

