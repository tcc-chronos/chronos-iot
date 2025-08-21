# Chronos IoT (ESP32 + FIWARE Descomplicado)

Projeto do **TCC Chronos** para simulação e integração de dispositivos IoT usando **ESP32 + DHT22**, publicando dados via **MQTT Ultralight** no **FIWARE Descomplicado**.

O dispositivo simulado lê **temperatura** e **umidade**, envia periodicamente ao **IoT Agent MQTT**, que repassa os dados ao **Orion Context Broker**. O histórico é armazenado pelo **STH-Comet** e pode ser consultado via API.

---

## Estrutura do Repositório

```
chronos-iot/
│
├── sketch.ino                  # Código do ESP32 (telemetria DHT22 via MQTT)
├── diagram.json                # Diagrama do circuito (ESP32 + DHT22) para simulação no Wokwi
├── wokwi.toml                  # Configuração para extensão Wokwi no VS Code
├── README.md                   # Este arquivo
└── docs/FIWARE.postman.json    # Coleção Postman com endpoints de provisionamento e consulta
```

---

## Tecnologias

* [ESP32 DevKit](https://www.espressif.com/en/products/devkits)
* [DHT22](https://learn.adafruit.com/dht) – sensor de temperatura/umidade
* [Wokwi Simulator](https://wokwi.com) – simulação de hardware
* [Mosquitto MQTT](https://mosquitto.org/) – broker MQTT
* [FIWARE Orion Context Broker](https://fiware-orion.readthedocs.io/) – gerenciamento de entidades NGSI
* [IoT Agent MQTT Ultralight](https://fiware-iotagent-ul.readthedocs.io/) – tradução MQTT → NGSI
* [STH-Comet](https://fiware-sth-comet.readthedocs.io/) – persistência histórica

---

## Executando a Simulação no Wokwi

1. Acesse [wokwi.com](https://wokwi.com) e crie um novo projeto ESP32.
2. Substitua o conteúdo pelo `sketch.ino` e `diagram.json` deste repositório.
3. Ajuste o **MQTT\_BROKER** no código para o IP público da sua VM Azure com FIWARE:

   ```cpp
   const char* MQTT_BROKER = "YOUR_PUBLIC_IP";
   ```
4. Clique em **Play** para iniciar a simulação.
5. O console serial mostrará logs do Wi-Fi, conexão MQTT e publicações do sensor.

---

## Provisionamento no FIWARE

Use a coleção [FIWARE.postman.json](./docs/FIWARE.postman.json) (importar no Postman ou Insomnia). A ordem dos requests é:

1. **Health Check**

   - IoT Agent: `GET http://<IP_VM>:4041/iot/about`
   - Orion: `GET http://<IP_VM>:1026/version`
   - Comet: `GET http://<IP_VM>:8666/version`

2. **Criar Service Group**

   ```http
   POST http://<IP_VM>:4041/iot/services
   Headers:
     fiware-service: smart
     fiware-servicepath: /
   Body:
   {
     "services": [{
       "apikey": "TEF",
       "cbroker": "http://<IP_VM>:1026",
       "entity_type": "Sensor",
       "resource": ""
     }]
   }
   ```

3. **Provisionar Device** (`esp32-chronos01`)
   Mapeia atributos Ultralight `t` e `h` para `temperature` e `humidity`.

4. **Criar Subscription Orion - Comet**
   Informa que mudanças em `temperature`/`humidity` devem ser persistidas pelo STH-Comet.

---

## Consultando os Dados no Comet

Exemplo para buscar as últimas 30 medidas de temperatura:

```bash
curl --location \
'http://<IP_VM>:8666/STH/v1/contextEntities/type/Sensor/id/urn:ngsi-ld:Chronos:ESP32:001/attributes/temperature?lastN=30' \
--header 'fiware-service: smart' \
--header 'fiware-servicepath: /'
```

E para umidade:

```bash
curl --location \
'http://<IP_VM>:8666/STH/v1/contextEntities/type/Sensor/id/urn:ngsi-ld:Chronos:ESP32:001/attributes/humidity?lastN=30' \
--header 'fiware-service: smart' \
--header 'fiware-servicepath: /'
```

---

## Observações

- No **Wokwi**, o SSID/pwd são simulados (`Wokwi-GUEST`, `""`).
- No **real ESP32**, ajuste para a rede Wi-Fi da sua instalação.
- No `diagram.json`, o resistor de **10kΩ** deve ser **pull-up** entre DATA e 3V3.
- Para manter o mesmo IP da VM Azure, configure o endereço como **estático**.
- As portas a liberar no **NSG (firewall Azure)** são:

  - `1883/tcp` (MQTT)
  - `4041/tcp` (IoT Agent)
  - `1026/tcp` (Orion)
  - `8666/tcp` (STH-Comet)

> Para demais dúvidas, seguir processo completo de provisionamento do [Fiware Descomplicado](https://github.com/fabiocabrini/fiware)
