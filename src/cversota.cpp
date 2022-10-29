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
#define rssi_min_download -85 // por esse valor mais proximo a zero para evitar que demora 30 minuto spara fazer o upload

// Tamanho do Objeto JSON vindo do server de verificação de versão
#define  __cvers_json_size             JSON_OBJECT_SIZE(5) + 288 //https://arduinojson.org/v6/assistant




/// @brief Controle de Versao de firmware via OTA
class cversota{

  private:
  
    bool beta_tester_user = false;
    unsigned long interval_vrf_ver = 0; 
    char url_version_json[100];
    char fw_version[12];
    char fs_version[12]; // "0.0.0" a "xx.xxx.xxxx"

    bool net_stable = false;

    // Dados do arquivo de versão web
    struct VCS {
      char    fwVersion[12];
      bool    fwObri;
      char    fwURL[101];
      char    fsVersion[12];
      bool    fsObri;
      char    fsURL[101];
    };
    VCS vcs;

    public:

    struct errorStatus{
      /*
        0 = tudo ok
        1 = erro ao verificar a versao no server (server diferente de 200)
        2 = arquivo de versao invalido no quesito json
        3 = erro ao abrir sistema de arquivos
        4 = internet instavel
        5 = erro no update ( verificar code_error_httpupdate)
        6 = ainda n deu o intervalo para verificar atualizacao
      */
      uint8_t code = 0;
      /*
        0 = nenhuma info
        1 = versao disponivel apenas para betatester
        2 = fw atualizado
        3 = fs atualizado
        4 = valores requisitados com sucesso
        5 = fs e fw atualizados
     */
      uint8_t info = 0;
     
      int code_http; //codigo http da ultima request
      int code_error_httpupdate; // codigo de erro da lib que atualiza a flash
    };
    errorStatus Error;


    /// @brief verifica o arquivo de versao no server e passa os valores do json para dentro da struct
    /// @return true = sucesso ou false = erro no request ou json error
    /// @todo dar um callback quando for atualizar
    bool cvers_check(){
      // Obtém arquivo de versão
      WiFiClientSecure client;

      // Alterado em 20/06/2022 para compatibilizar com nova versão da biblioteca
      client.setInsecure();

      HTTPClient http;
      http.begin(client, url_version_json);

      int httpCode = http.GET(); // da pra usar direto, sem precisar de variavel, mas vale a pena?
      Error.code_http = httpCode;

      String s = http.getString();
      http.end();
      s.trim();

      if (httpCode != HTTP_CODE_OK){
        // Erro obtendo arquivo de versão
        Error.code = 1;
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
          Error.code = 2;
          return false;
        }

        // Armazena dados na estrutura VCS
        strlcpy(vcs.fwVersion, jsonVCS["fwVersion"] | " ", sizeof(vcs.fwVersion));
        vcs.fwObri = jsonVCS["fwObri"] | false;
        strlcpy(vcs.fwURL, jsonVCS["fwUrl"] | "", sizeof(vcs.fwURL));

        strlcpy(vcs.fsVersion, jsonVCS["fsVersion"] | "", sizeof(vcs.fsVersion));
        vcs.fsObri = jsonVCS["fsObri"] | false;
        strlcpy(vcs.fsURL, jsonVCS["fsURL"] | "", sizeof(vcs.fsURL));

        Error.info = 4;
        Error.code = 0;

        return true;
      }
    }

    /// @brief verifica se deve atualizar e faz a atualizacao na pratica
    void cvers_update(){

      // SPIFFS
      if (!SPIFFS.begin(true)) {
        Error.code = 3;
        return;
      }

      WiFiClientSecure client;

      // Alterado em 20/06/2022 para compatibilizar com nova versão da biblioteca
      client.setInsecure();

      // ESP32
      httpUpdate.rebootOnUpdate(false);

      //Callback - Progresso - bonito porem inutil no mundo real, se ninguem vai ver a serial, por que mostrar o progresso
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
          Serial.println("atualizando o fs");

          SPIFFS.end();          //já que vai atualizar o spiffs, entao desliga ele

          t_httpUpdate_return result = httpUpdate.updateSpiffs(client, vcs.fsURL);

          // Verifica resultado
          switch(result){
            case HTTP_UPDATE_FAILED:
                Error.code_error_httpupdate = httpUpdate.getLastError(); 
                Error.code = 5;
              break;
            case HTTP_UPDATE_NO_UPDATES:
                Error.code = 0;
                Error.info = 2;
              break;
            case HTTP_UPDATE_OK:
                Error.code = 0;
                Error.info = 2;
              break;
          }

          yield();
          SPIFFS.begin();
          delay(500);
        }

        if(strcmp(this->fw_version, vcs.fwVersion) != 0){
          // Atualiza Software
          Serial.println("atualizando o fw");
          t_httpUpdate_return result = httpUpdate.update(client, vcs.fwURL);
          
          // Verifica resultado
          switch(result){
            case HTTP_UPDATE_FAILED:
                Error.code_error_httpupdate = httpUpdate.getLastError(); 
                Error.code = 5;
              break;
            case HTTP_UPDATE_NO_UPDATES:
                Error.code = 0;
                Error.info = 2;
              break;
            case HTTP_UPDATE_OK:
                Error.code = 0;
                Error.info = 2;
                delay(500);
                ESP.restart();
              break;
          }
        }
      }else{
        //versao disponivel somente para beta tester
        Error.info = 1;
      }
      
    }



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
        Error.code = 4;
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
  
          if(!this->cvers_check()){
            return; // nao tem por que perder tempo em continuar se nem sabe a real versao no server
          }
        
          // ja "agenda" a próxima verificação para daqui x tempo
          next_verify_version = (millis() + this->interval_vrf_ver);

          // verifica se a versao do firmware ou do spiffs esta diferente da que veio de server
          if ((strcmp(this->fs_version, vcs.fsVersion) != 0) || (strcmp(this->fw_version, vcs.fwVersion) != 0)) {
            this->cvers_update(); // atualiza de fato
          }else{
            Error.info = 5;
          }
        }else{
          //sem conexao adequada para atualizar ou verificar atualizacoes
          Error.code = 4;
        }
      }//else if(millis() > 50 days ){ next_verify_version = 0 } ou reset 
      else{
        //ainda n ta no tempo 
        Error.code = 6;
      }


    }

};




