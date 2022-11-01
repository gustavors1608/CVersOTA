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
#include <ArduinoJson.h>


// certificate for https://gustavo.usina.dev
// AAA Certificate Services, valid until Sun Dec 31 2028, size: 1964 bytes  criado dia 31/10/22 no site: https://projects.petrucci.ch/esp32/?page=ssl.php&url=https%3A%2F%2Fgustavo.usina.dev
const char* rootCACertificate = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFfjCCBGagAwIBAgIQZ970PvF72uJP9ZQGBtLAhDANBgkqhkiG9w0BAQwFADB7\n" \
"MQswCQYDVQQGEwJHQjEbMBkGA1UECAwSR3JlYXRlciBNYW5jaGVzdGVyMRAwDgYD\n" \
"VQQHDAdTYWxmb3JkMRowGAYDVQQKDBFDb21vZG8gQ0EgTGltaXRlZDEhMB8GA1UE\n" \
"AwwYQUFBIENlcnRpZmljYXRlIFNlcnZpY2VzMB4XDTA0MDEwMTAwMDAwMFoXDTI4\n" \
"MTIzMTIzNTk1OVowgYUxCzAJBgNVBAYTAkdCMRswGQYDVQQIExJHcmVhdGVyIE1h\n" \
"bmNoZXN0ZXIxEDAOBgNVBAcTB1NhbGZvcmQxGjAYBgNVBAoTEUNPTU9ETyBDQSBM\n" \
"aW1pdGVkMSswKQYDVQQDEyJDT01PRE8gUlNBIENlcnRpZmljYXRpb24gQXV0aG9y\n" \
"aXR5MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAkehUktIKVrGsDSTd\n" \
"xc9EZ3SZKzejfSNwAHG8U9/E+ioSj0t/EFa9n3Byt2F/yUsPF6c947AEYe7/EZfH\n" \
"9IY+Cvo+XPmT5jR62RRr55yzhaCCenavcZDX7P0N+pxs+t+wgvQUfvm+xKYvT3+Z\n" \
"f7X8Z0NyvQwA1onrayzT7Y+YHBSrfuXjbvzYqOSSJNpDa2K4Vf3qwbxstovzDo2a\n" \
"5JtsaZn4eEgwRdWt4Q08RWD8MpZRJ7xnw8outmvqRsfHIKCxH2XeSAi6pE6p8oNG\n" \
"N4Tr6MyBSENnTnIqm1y9TBsoilwie7SrmNnu4FGDwwlGTm0+mfqVF9p8M1dBPI1R\n" \
"7Qu2XK8sYxrfV8g/vOldxJuvRZnio1oktLqpVj3Pb6r/SVi+8Kj/9Lit6Tf7urj0\n" \
"Czr56ENCHonYhMsT8dm74YlguIwoVqwUHZwK53Hrzw7dPamWoUi9PPevtQ0iTMAR\n" \
"gexWO/bTouJbt7IEIlKVgJNp6I5MZfGRAy1wdALqi2cVKWlSArvX31BqVUa/oKMo\n" \
"YX9w0MOiqiwhqkfOKJwGRXa/ghgntNWutMtQ5mv0TIZxMOmm3xaG4Nj/QN370EKI\n" \
"f6MzOi5cHkERgWPOGHFrK+ymircxXDpqR+DDeVnWIBqv8mqYqnK8V0rSS527EPyw\n" \
"TEHl7R09XiidnMy/s1Hap0flhFMCAwEAAaOB8jCB7zAfBgNVHSMEGDAWgBSgEQoj\n" \
"PpbxB+zirynvgqV/0DCktDAdBgNVHQ4EFgQUu69+Aj36pvE8hI6t7jiY7NkyMtQw\n" \
"DgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQFMAMBAf8wEQYDVR0gBAowCDAGBgRV\n" \
"HSAAMEMGA1UdHwQ8MDowOKA2oDSGMmh0dHA6Ly9jcmwuY29tb2RvY2EuY29tL0FB\n" \
"QUNlcnRpZmljYXRlU2VydmljZXMuY3JsMDQGCCsGAQUFBwEBBCgwJjAkBggrBgEF\n" \
"BQcwAYYYaHR0cDovL29jc3AuY29tb2RvY2EuY29tMA0GCSqGSIb3DQEBDAUAA4IB\n" \
"AQB/8lY1sG2VSk50rzribwGLh9Myl+34QNJ3UxHXxxYuxp3mSFa+gKn4vHjSyGMX\n" \
"roztFjH6HxjJDsfuSHmfx8m5vMyIFeNoYdGfHUthgddWBGPCCGkm8PDlL9/ACiup\n" \
"BfQCWmqJ17SEQpXj6/d2IF412cDNJQgTTHE4joewM4SRmR6R8ayeP6cdYIEsNkFU\n" \
"oOJGBgusG8eZNoxeoQukntlCRiTFxVuBrq2goNyfNriNwh0V+oitgRA5H0TwK5/d\n" \
"EFQMBzSxNtEU/QcCPf9yVasn1iyBQXEpjUH0UFcafmVgr8vFKHaYrrOoU3aL5iFS\n" \
"a+oh0IQOSU6IU9qSLucdCGbX\n" \
"-----END CERTIFICATE-----\n" \
"";






//intensidade minima ota (para não cair a conexão ao começar a baixar os binarios)
#define rssi_min_download -90 // por esse valor mais proximo a zero para evitar que demora 30 minuto spara fazer o upload

// Tamanho do Objeto JSON vindo do server de verificação de versão
#define  __cvers_json_size             JSON_OBJECT_SIZE(5) + 288 //https://arduinojson.org/v6/assistant




/// @brief Controle de Versao de firmware via OTA
class cversota{

  private:
    //calbacks
  	void (*_onRun_firmware)(void);
    void (*_onRun_spiffs)(void);
    void (*_onRun_complete)(void);
  
    //vars de uso geral
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
        0 = sem erros
        1 = erro ao verificar a versao no server (server diferente de 200)
        2 = arquivo de versao invalido no quesito json
        3 = erro ao abrir sistema de arquivos
        4 = internet instavel
        5 = erro no update ( verificar code_error_httpupdate)
        6 = ainda n deu o intervalo para verificar atualizacao
      */
      uint8_t code = 0;
      /*
        0 = nenhuma info (em casos de erros por exemplo, nao vai ter dado nenhuma info provavelmente)
        1 = versao disponivel apenas para betatester
        2 = valores requisitados com sucesso
        3 = fs e fw atualizados
     */
      uint8_t info = 0;
     
      int code_http; //codigo http da ultima request
      int code_error_httpupdate; // codigo de erro da lib que atualiza a flash
    };
    errorStatus Error;

    /// @brief nome da função que vai ser executado ao iniciar uma atualização de SPIFFS e passa pra dentro da classe
    /// @param callback 
    void on_update_spiffs(void (*callback)(void)){
      _onRun_spiffs = callback;
    }
    /// @brief nome da função que vai ser executado ao iniciar uma atualização de firmware
    /// @param callback 
    void on_update_firmware(void (*callback)(void)){
      _onRun_firmware = callback;
    }

    /// @brief nome da funcao que deve ser chamada ao concluir uma atualização
    /// @param callback 
    void on_complete_update(void (*callback)(void)){
      _onRun_complete = callback;
    }

    /// @brief executa a funcao passada por paramentro
    /// @param callback nome da funcao ou var com a mesma
    void run(void (*callback)(void)){
      if(callback != NULL)
        callback();
    }



    /// @brief verifica o arquivo de versao no server e passa os valores do json para dentro da struct
    /// @return true = sucesso ou false = erro no request ou json error
    bool cvers_check(){
      WiFiClientSecure client;

      // Alterado em 20/06/2022 para compatibilizar com nova versão da biblioteca
      //client.setInsecure();
      client.setCACert(rootCACertificate);

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

        Error.info = 2;
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
      //client.setInsecure();
      client.setCACert(rootCACertificate);

      // ESP32
      httpUpdate.rebootOnUpdate(false);
  
      //se for obrigatorio ou tiver em versao beta
      if(vcs.fwObri == true || vcs.fsObri == true || this->beta_tester_user == true){
        if (strcmp(this->fs_version, vcs.fsVersion) != 0){
          // Atualiza Sistema de Arquivos
          run(this->_onRun_spiffs); //executa o callback

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
              break;
            case HTTP_UPDATE_OK:
                Error.code = 0;
              break;
          }
          //callback a ser chamado quando concluir a atualização
          run(_onRun_complete);

          yield();
          SPIFFS.begin();
          delay(500);
        }

        if(strcmp(this->fw_version, vcs.fwVersion) != 0){
          // Atualiza Software
          run(this->_onRun_firmware); //executa o callback

          t_httpUpdate_return result = httpUpdate.update(client, vcs.fwURL);
          
          // Verifica resultado
          switch(result){
            case HTTP_UPDATE_FAILED:
                Error.code_error_httpupdate = httpUpdate.getLastError(); 
                Error.code = 5;
              break;
            case HTTP_UPDATE_NO_UPDATES:
                Error.code = 0;
              break;
            case HTTP_UPDATE_OK:
                Error.code = 0;

                run(_onRun_complete);

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

      //poderia verificar se tdas as vars da classe foram preenchidas
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
            Error.info = 3;
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




