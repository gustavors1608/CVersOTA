# 🚀 Cversota - Controle de Versão e update via OTA 🚀
![MIT License](https://img.shields.io/badge/License-MIT-green.svg?style=for-the-badge) 
![GitHub language count](https://img.shields.io/github/languages/count/gustavors1608/cversota?style=for-the-badge) 
![Plataforma](https://img.shields.io/badge/Plataforma-ESP32-green?style=for-the-badge)


Esta lib foi criada com o objetivo de ser possivel realizar um update do firmware e spiffs do esp32, atravez de um servidor, de forma confiavel, rapida, com facil modificação e por fim, selecionando quais dispositivos deveriam ter determinada versão ou não, sendo assim se criou esse projeto, espero a sua contribuição também para a evolução do mesmo.

Biblioteca para controle de versão e auto update de firmware e Spiffs de forma automática, lib otimizada pra projetos em produção, onde o dispositivo iot acessa um link contendo um json de verificação para conferir de deve se atualizar, dessa forma o sistema verifica se deve atualizar o firmware ou Spiffs e o faz, sendo possivel até mesmo escolher baixar se for versao de testes (em casos que o device for "beta tester"), entre varias outras features que pode-se encontrar abaixo.



## 🖇️ Features 🖇️
Algumas caracteristicas do projeto:

- Intervalos de tempo para verificação dinamicos: ex: verificar nova versão a cada 24 horas
- Classificação de download obrigatorio ou não (usado para download em beta testers)
- Possivel alterar os arquivos e pastas armazenados no spiffs via ota
- Classifica se o rssi do wifi está adequado para a atualização
- Possibilidade de mudar o link que contém os binarios do firmware ou spiffs (podendo o arquivo de versão ficar em um server e os binarios no github por exemplo)
- Otimização de memoria flash
- Classificação e retorno de codigos de erro para o log do seu jeito

Sendo o projeto dividido em 2 partes:

- A biblioteca em si, contida nesse repositorio, onde é usada no firmware do dispositivo.
- Json de versão, encontrado abaixo, informa aos dispositivos informações sobre as ultimas versões e onde encontrar a mesma.



## 🗺️ Arquivo de Versão 🗺️

Para o projeto funcionar, o dev deve fornecer um link contendo um arquivo json no formato abaixo para o device verificar link dos binarios do firmware e do spiffs atualizados entre outros dados que você pode ver abaixo:

Json: 
``` Json
{
  "fwVersion": "1.9.0",
  "fwObri": true,
  "fwUrl": "https://example.com.br/device_name/.../firmware.bin",
  "fsVersion": "1.0.0",
  "fsObri": false,
  "fsUrl": "https://example.com.br/device_name/.../spiffs.bin"
}
```

| Parameter   | Type     | Descrição                |
| :--------   | :------- | :------------------------- |
| `fwVersion` | `string` | Versão do firmware disponibilizado no link abaixo |
| `fwObri`    | `bool`   | É obrigatorio? se não for somente os device em beta irão atualizar |
| `fwUrl`     | `string` | Url que contem o binario do firmware (firmware já compilado), deve fornecer **somente o binario**, nenhum html a mais nem nada. |
| `fsVersion` | `string` | Versão do FileSystem, a cada novo arquivo ou modificação feita, deve alterar aqui. |
| `fsObri`    | `bool`   | A atualização do FS é obrigatoria? se não somente os beta users irão atualizar. |
| `fsUrl`     | `string`   | Url que contem o arquivo de imagem binaria do SPIFFS, ao criar um novo esquema de arquivos, compilar o mesmo e por o link do mesmo aqui. |

#### Relembrando: **Os arquivos binarios, devem ser disponibilizados sem nenhum outro dado, sem html nem nada, e devem conter a extenção `.bin`**


## 🤖 Firmware example 🤖

Veja como o uso é simples, não é mesmo? 🙃


```cpp / arduino
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

```

Após realizar o upload do codigo acima para o seu esp32, o mesmo irá verificar a cada 6h o link que contém o json, caso você queira atualizar a versão, é só seguir o seguinte passo a passo:
1. Atualizar a alteracão da versão no firmware (tem que passar a versão do novo firmware para o metodo da lib ```ota_system.set_atual_version(fw_version,fs_version);``` no caso do exemplo acima )
2. Compilar a sua nova vesão do firmware ou spiffs.
3. Copiar o binario após a complilação.
4. Enviar  o binario para o servidor (pode ser utilizado o dropbox, github etc).
5. Alterar no arquivo json a nova versão (por o mesmo numero que você colocou no firmware, se tá '1.0.0' você deve alterar para '1.0.1' por exemplo).
6. Informar nesse json tambem se essa atualização é obrigatoria.
7. Carregar esse arquivo json para o server.
8. Veja a magia acontecendo no esp32. 🧙‍♂️, então dentro 6 horas (ou menos, de acordo com o intervalo que você configurar) o seu dispositivo irá utilizar a versão que você dispos no link com o binario do link.

## notas:
- criar funcao de call back para quando começar a atualizar, notificar uma função
- usar somente a noite ou madrugada, quando o dispositivo não for usado, dessa forma, pode demorar pra atualizar sem problemas etc


## ✏️ Author ✏️
Em caso de duvidas ou sugestões...

- [Github: @gustavors1608](https://www.github.com/gustavors1608)

- [Instagram: @gustavo_stroschon](https://www.instagram.com/gustavo_stroschon)
