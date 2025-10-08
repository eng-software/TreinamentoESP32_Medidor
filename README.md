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

Esta versão além de utlizar C e C++ em sua implementação, utiliza o recurso do FreeRTOS
O main foi alterado de **man.c** para **main.cpp**.   
A troca de **.c** para **.cpp** indica para o compilador que aquele arquivo é um código em C++.  
  
A rotina de inicialização do display foi separada em um arquivo C. pois o componente do framework IDF que inicializa o display e configura o LVGL não tem suporte a C++.  
O sendor foi separado no arquivo **cSMP3011.cpp** e foi criada uma classe que inicializa e controla o sensor.  

No **main.cpp** fica a chamada para inicializar o display, o sensor e para manter o display atualizado.

O polling do sensor SMP3011 agora é uma task do FreeRTOS, deixando o while do main responsável por atualizar o display e a task de ler o sensor.  
Outra task foi adicionada apenas para fazer o LED piscar, indicando que outras tarefas podem ser executadas "em paralelo".

