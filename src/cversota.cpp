/***********************************************************************
 * @brief Controle de Versão de firmware e atualizacao de forma        *  
 * automatica via ota e por intervalos de tempo baseado em unixtime    *
 ***********************Arquivo de Biblioteca***************************
 * @date   10/2022                                                     *
 * @author Gustavo Rodolfo Stroschon                                   *
 * @instagram @gustavo_stroschon                                       *
 ***********************************************************************/
// dependencias
#include <Arduino.h> // precisa do millis, por enquanto
#include <WiFi.h>
#include <HTTPUpdate.h>
#include <SPIFFS.h>
//#include <TimeLib.h>
#include <ArduinoJson.h>


//intensidade minima ota (para não cair a conexão ao começar a baixar os binarios)
#define rssi_min_download -95 // realizar testes para descobrir ate quanto é possivel utilizar 

// Tamanho do Objeto JSON vindo do server de verificação de versão
#define  __cvers_json_size             JSON_OBJECT_SIZE(5) + 288 //https://arduinojson.org/v6/assistant




/// @brief Controle de Versao de firmware via OTA
class cversota{

  private:
  
    bool beta_tester_user = false;
    unsigned long interval_vrf_ver = 0; 
    char url_version_json[100];
    char fw_version[7];
    char fs_version[7]; // "0.0.0"

    bool net_stable = false;

    // Dados do arquivo de versão web
    struct VCS {
      char    fwVersion[6];
      bool    fwObri;
      char    fwURL[101];
      char    fsVersion[6];
      bool    fsObri;
      char    fsURL[101];
    };
    VCS vcs;

    /// @brief verifica o arquivo de versao no server e passa os valores do json para dentro da struct
    /// @return true = sucesso ou false = erro no request ou json error
    bool cvers_check(){
      // Obtém arquivo de versão
      WiFiClientSecure client;

      // Alterado em 20/06/2022 para compatibilizar com nova versão da biblioteca
      client.setInsecure();

      HTTPClient http;
      http.begin(client, url_version_json);
      int httpCode = http.GET();
      String s = http.getString();
      http.end();
      s.trim();

      if (httpCode != HTTP_CODE_OK){
        // Erro obtendo arquivo de versão
        Serial.println("ERRO HTTP " + String(httpCode) + " " + http.errorToString(httpCode));
        return false;
      }else{

        // Tratamento do arquivo
        StaticJsonDocument<96> filter; //usado para avisar quais valores serao usados, assim economizando memoria
        filter["fwVersion"] = true;
        filter["fwObri"] = true;
        filter["fwUrl"] = true;
        filter["fsVersion"] = true;
        filter["fsObri"] = true;
        filter["fsUrl"] = true;
        StaticJsonDocument<__cvers_json_size> jsonVCS;

        if (deserializeJson(jsonVCS, s ,DeserializationOption::Filter(filter))){
          // Arquivo de versão inválido
          Serial.println(F("Arquivo de versão inválido"));
          return false;
        }

        // Armazena dados na estrutura VCS
        strlcpy(vcs.fwVersion, jsonVCS["fwVersion"] | " ", sizeof(vcs.fwVersion));
        vcs.fwObri = jsonVCS["fwObri"] | false;
        strlcpy(vcs.fwURL, jsonVCS["fwUrl"] | "", sizeof(vcs.fwURL));

        strlcpy(vcs.fsVersion, jsonVCS["fsVersion"] | "", sizeof(vcs.fsVersion));
        vcs.fsObri = jsonVCS["fsObri"] | false;
        strlcpy(vcs.fsURL, jsonVCS["fsURL"] | "", sizeof(vcs.fsURL));

        //Serial.println(F("Dados recebidos:"));
        //serializeJsonPretty(jsonVCS, Serial);
        //Serial.println();

        return true;
      }
    }

    /// @brief verifica se deve atualizar e faz a atualizacao na pratica
    void cvers_update(){

      // SPIFFS
      if (!SPIFFS.begin(true)) {
        Serial.println(F("SPIFFS ERRO"));
        while (true);
      }else{
        Serial.println("spiffs iniciado com sucesso");
      }

      WiFiClientSecure client;

      // Alterado em 20/06/2022 para compatibilizar com nova versão da biblioteca
      client.setInsecure();

      // ESP32
      httpUpdate.rebootOnUpdate(false);

      // Callback - Progresso
      Update.onProgress([](size_t progresso, size_t total){
          byte porcentagem = (progresso * 100 / total);
          static byte porcentagem_anterior;
          if(porcentagem != porcentagem_anterior){ //pra nao printar valores repetidos
            Serial.print(porcentagem);
            Serial.print(' ');
          }
          porcentagem_anterior = porcentagem;
      });
  
      //se for obrigatorio ou tiver em versao beta
      if(vcs.fwObri == true || vcs.fsObri == true || this->beta_tester_user == true){
        if (strcmp(this->fs_version, vcs.fsVersion) != 0){
          // Atualiza Sistema de Arquivos
          Serial.println(F("Atualizando SPIFFS..."));
          //já que vai atualizar o spiffs, entao desliga ele
          SPIFFS.end();

          t_httpUpdate_return result = httpUpdate.updateSpiffs(client, vcs.fsURL);

          // Verifica resultado
          switch(result){
            case HTTP_UPDATE_FAILED:
                Serial.println("Falha: " + httpUpdate.getLastErrorString());
              break;
            case HTTP_UPDATE_NO_UPDATES:
                Serial.println(F("Nenhuma atualização disponível"));
              break;
            case HTTP_UPDATE_OK:
                Serial.println("Atualizado");
              break;
          }

          yield();
          SPIFFS.begin();
          delay(500);
        }

        if(strcmp(this->fw_version, vcs.fwVersion) != 0){
          // Atualiza Software
          Serial.println(F("Atualizando Firmware..."));
          
          t_httpUpdate_return result = httpUpdate.update(client, vcs.fwURL);

          // Verifica resultado
          switch(result){
            case HTTP_UPDATE_FAILED:
                Serial.println("Falha: " + httpUpdate.getLastErrorString());
              break;
            case HTTP_UPDATE_NO_UPDATES:
                Serial.println(F("Nenhuma atualização disponível"));
              break;
            case HTTP_UPDATE_OK:
                Serial.println("Atualizado, reiniciando...");
                delay(500);
                ESP.restart();
              break;
            default:
                //seria o caso registrar um log...
              break;
          }
        }
      }
      
    }

  public:

    /// @brief controle de versionamento e atualizacoes via ota
    /// @param interval_vrf_version a cada 6 horas por exemplo, intervalo em ms
    /// @param url_vrf_version url onde deve buscar os dados sobre a versao e os links de firmware e fs
    cversota(unsigned long interval_vrf_version, const char url_vrf_version[100]){
      this->interval_vrf_ver = interval_vrf_version;

      strlcpy(this->url_version_json, url_vrf_version, sizeof(this->url_version_json));
    }

    /// @brief usado para informar a lib qual a versao atual, assim se tiver uma versao diferente disponivel, baixa...
    /// @param fw_version versao do Firmware ex: "1.5.3"
    /// @param fs_version versao do FileSistem (SPIFFS) ex: "5.8.2"
    /// @example set_atual_version("0.1.1", "1.0.9");
    void set_atual_version(const char* fw_version, const char* fs_version){
      strlcpy(this->fw_version, fw_version, sizeof(this->fw_version));
      strlcpy(this->fs_version, fs_version, sizeof(this->fs_version));
    }
    
    /// @brief define se deve baixar as versoes nao obrigatorias, sendo entao um usuario/device que pode testar as novas versoes
    /// @param beta true = atualiza com versoes nao obrigatorias
    void set_beta_tester(bool beta = false){
      this->beta_tester_user = beta;
    }

    /// @brief define a potencia do sinal da conexao wifi, caso esteja muito baixo, não atualiza pois pode ter erros etc
    /// @param rssi intensidade do sinal retornada por: WiFi.RSSI()
    void set_rssi_wifi(int8_t rssi){
      if(rssi > rssi_min_download){//numero negativo, regra invertida, -80 é maior que -100 ...
        //-80 > -100 = true
        this->net_stable = true;
      }else{
        this->net_stable = false;
      }
    }

    /// @brief verifica os novos dados sobre novas versoes e se for diferente da versao atual entao baixa o conteudo e atualiza
    /// @param force_verify_update caso tenha alguma situacao que nao pode esperar x tempo para verificar atualizacao, entao so mandar esse parametro como true
    /// @todo melhorias futuras: retornar codigos de erro por exemplo, quando n reconhecer o json de versao etc
    void check_update(bool force_verify_update = false){
      //se ja passou do tempo que deveria verificar, entao verifica
      static unsigned long next_verify_version = 0;

      if(next_verify_version <= millis() || force_verify_update == true){ // juntar com alguma outra coisa pra ter alguma redundancia, exemplo, ao apertar botao verifica...
        if ((WiFi.status() == WL_CONNECTED) && (this->net_stable == true)){ // verifica se tem wifi e se ta estavel a rede
          
          Serial.println(F("Verificando versão do fw e fs... "));
  
          if(!this->cvers_check()){
            Serial.println("ocorreu um erro ao verificar a versao");
          }
        
          // ja "agenda" a próxima verificação para daqui x tempo
          next_verify_version = (millis() + this->interval_vrf_ver);

          // verifica se a versao do firmware ou do spiffs esta diferente da que veio de server
          if ((strcmp(this->fs_version, vcs.fsVersion) != 0) || (strcmp(this->fw_version, vcs.fwVersion) != 0)) {
            this->cvers_update(); // atualiza de fato
          }
        }else{
          //sem conexao adequada para atualizar ou verificar atualizacoes
          Serial.println("sem conexao adequada para atualizar ou verificar atualizacoes");
        }
      }


    }

};




