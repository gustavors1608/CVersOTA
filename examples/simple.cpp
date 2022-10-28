
/*****************************************************************************
 * @author Gustavo Rodolfo Stroschon                                         *
 * @date 28/10/2022
******************************************************************************/
#include "cversota.cpp"

#define  fw_version  "1.0.0" // Versão atual do firmware
#define  fs_version  "1.0.6" // Versão atual do file system
#define  url_verify  "https://example.com.br/device_name/.../versao.json" //arquivo json de novas versão 

#define interval_verify 1000*60*60*6 //a cada quanto tempo deve verificar o link de versao acima, em ms, nesse caso seria a cada 6 horas

cversota ota_system(interval_verify, url_verify);//inicia a lib

void setup() {
  Serial.begin(115200);

  // Conecta ao WiFi
  WiFi.begin("SSID_WIFI", "PASSWORD_WIFI");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("WiFi conectado (" + String(WiFi.RSSI()) + ") ");
  
  //funções do cversota abaixo
  ota_system.set_atual_version(fw_version,fs_version); // define a versao atual do firmware e do fileSystem
  ota_system.set_rssi_wifi(WiFi.RSSI()); // passa a intensidade do sinal do wifi, o rssi
  ota_system.set_beta_tester(false); // define se o usuario deve fazer donwnload de versães não obrigatorias

}
void loop() {
  ota_system.check_update(); //verifica novas atualizações
}