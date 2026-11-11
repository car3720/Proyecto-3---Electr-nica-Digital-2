#include <WiFi.h>
#include <Wire.h>
#include <WebServer.h>
#include "protocolo.h"
#include "pagina.h"

// ESP32 clasico. No modificar los pines de sensores o pantalla de las Nucleo.
constexpr int PIN_SDA=21, PIN_SCL=22;
constexpr uint32_t PERIODO_I2C_MS=200;
const char NOMBRE_AP[]="Parqueo-Demo";
const char CLAVE_AP[]="Parqueo2026";
// Opcional: completar si desean entrar desde la red WiFi del laboratorio.
const char WIFI_EXTERNA[]="";
const char CLAVE_EXTERNA[]="";

// A conservada: 0x30 y cuatro bytes 0/1/2. B de este paquete: 0x13 y cuatro estados, igual que A.
// Solo para la A UNIFICADA antigua, cambiar la siguiente linea a:
// Grupo grupoA{0x12,Formato::DosMascaras};
Grupo grupoA{0x30,Formato::CuatroEstados};
Grupo grupoB{0x13,Formato::CuatroEstados};

WebServer servidor(80);
EstadoNivel nivel;
bool busListo=false;
uint32_t ultimaConsulta=0, ultimoDiagnostico=0, secuencia=0;

void leerNucleo(Grupo &g)
{
    g.conectado=false;g.ocupados=0;g.validos=0;g.recibidos=0;
    g.errorEscritura=255;
    for(unsigned i=0;i<4;i++)g.datos[i]=255;
    if(!busListo){g.errorLectura=3;g.fallos++;return;}
    const size_t esperados=static_cast<uint8_t>(g.formato);
    size_t n=Wire.requestFrom(g.direccion,esperados);
    g.recibidos=(uint8_t)n;
    unsigned i=0;
    while(Wire.available()){
        int dato=Wire.read();
        if(i<4)g.datos[i]=(uint8_t)dato;
        i++;
    }
    if(i!=esperados||n!=esperados){g.errorLectura=1;g.fallos++;return;}
    if(!decodificar(g))g.fallos++;
}

void devolverGlobal(Grupo &g)
{
    if(!busListo||!g.conectado)return;
    // A conservada y B conservada reciben DOS bytes, sin registro previo.
    Wire.beginTransmission(g.direccion);
    Wire.write(nivel.ocupados);
    Wire.write(nivel.validos);
    g.errorEscritura=Wire.endTransmission();
}

void actualizarParqueo()
{
    EstadoNivel previo=nivel;
    bool previaA=grupoA.conectado,previaB=grupoB.conectado;
    leerNucleo(grupoA);leerNucleo(grupoB);
    nivel=combinar(grupoA,grupoB);
    if(((previo.ocupados^nivel.ocupados)&nivel.validos)||
        previo.validos!=nivel.validos||previaA!=grupoA.conectado||previaB!=grupoB.conectado)
        secuencia++;
    devolverGlobal(grupoA);devolverGlobal(grupoB);
    ultimaConsulta=millis();
}

String jsonGrupo(const Grupo &g)
{
    char texto[380];
    snprintf(texto,sizeof texto,
        "{\"direccion\":%u,\"conectada\":%s,\"esperados\":%u,\"recibidos\":%u,"
        "\"ocupados\":%u,\"validos\":%u,\"error_lectura\":%u,\"error_escritura\":%u,"
        "\"fallos\":%lu,\"datos\":[%u,%u,%u,%u]}",
        (unsigned)g.direccion,g.conectado?"true":"false",(unsigned)g.formato,
        (unsigned)g.recibidos,(unsigned)g.ocupados,(unsigned)g.validos,
        (unsigned)g.errorLectura,(unsigned)g.errorEscritura,(unsigned long)g.fallos,
        (unsigned)g.datos[0],(unsigned)g.datos[1],(unsigned)g.datos[2],(unsigned)g.datos[3]);
    return String(texto);
}

String jsonEstado()
{
    char texto[400],total[8];
    if(nivel.validos==255U)snprintf(total,sizeof total,"%u",contarLibres(nivel));
    else snprintf(total,sizeof total,"null");
    snprintf(texto,sizeof texto,
        "{\"firmware\":\"Parqueo_Mediana_AB_v5\",\"i2c_iniciado\":%s,"
        "\"ocupados\":%u,\"validos\":%u,\"conocidos\":%u,\"libres_confirmados\":%u,"
        "\"total_disponibles\":%s,\"secuencia\":%lu,\"ms\":%lu,\"edad_ms\":%lu,"
        "\"plazas\":[%d,%d,%d,%d,%d,%d,%d,%d]",
        busListo?"true":"false",(unsigned)nivel.ocupados,(unsigned)nivel.validos,
        contarBits(nivel.validos),contarLibres(nivel),total,(unsigned long)secuencia,
        (unsigned long)millis(),(unsigned long)(millis()-ultimaConsulta),
        estadoPlaza(nivel,0),estadoPlaza(nivel,1),estadoPlaza(nivel,2),estadoPlaza(nivel,3),
        estadoPlaza(nivel,4),estadoPlaza(nivel,5),estadoPlaza(nivel,6),estadoPlaza(nivel,7));
    String salida(texto);
    salida+=",\"a\":";salida+=jsonGrupo(grupoA);
    salida+=",\"b\":";salida+=jsonGrupo(grupoB);
    salida+="}";return salida;
}

void imprimirGrupo(const char *nombre,const Grupo &g)
{
    Serial.printf("%s dir=0x%02X conectada=%u RX=%u/%u V=0x%02X E=0x%02X lectura=%u escritura=%u datos=[%u,%u,%u,%u] fallos=%lu\n",
        nombre,(unsigned)g.direccion,(unsigned)g.conectado,(unsigned)g.recibidos,
        (unsigned)g.formato,(unsigned)g.validos,(unsigned)g.ocupados,
        (unsigned)g.errorLectura,(unsigned)g.errorEscritura,(unsigned)g.datos[0],
        (unsigned)g.datos[1],(unsigned)g.datos[2],(unsigned)g.datos[3],(unsigned long)g.fallos);
}

void setup()
{
    Serial.begin(115200);delay(200);
    Serial.println("\nESP32 PARQUEO MEDIANA AB v5");
    busListo=Wire.begin(PIN_SDA,PIN_SCL,100000);
    Wire.setTimeOut(30);
    Serial.println(busListo?"I2C iniciado: SDA=21, SCL=22":"ERROR: no se pudo iniciar I2C");
    WiFi.mode(WIFI_AP_STA);
    if(!WiFi.softAP(NOMBRE_AP,CLAVE_AP))Serial.println("ERROR: no se pudo crear Parqueo-Demo");
    if(WIFI_EXTERNA[0])WiFi.begin(WIFI_EXTERNA,CLAVE_EXTERNA);
    servidor.on("/",[](){
        servidor.sendHeader("Cache-Control","no-store");
        servidor.send_P(200,"text/html; charset=utf-8",PAGINA);
    });
    servidor.on("/api/estado",[](){
        servidor.sendHeader("Cache-Control","no-store");
        servidor.send(200,"application/json",jsonEstado());
    });
    servidor.begin();
    actualizarParqueo();
    Serial.print("WiFi: ");Serial.println(NOMBRE_AP);
    Serial.print("Abre http://");Serial.println(WiFi.softAPIP());
    imprimirGrupo("A",grupoA);imprimirGrupo("B",grupoB);
}

void loop()
{
    servidor.handleClient();
    uint32_t ahora=millis();
    if((uint32_t)(ahora-ultimaConsulta)>=PERIODO_I2C_MS)actualizarParqueo();
    if((uint32_t)(ahora-ultimoDiagnostico)>=2000U){
        ultimoDiagnostico=ahora;
        imprimirGrupo("A",grupoA);imprimirGrupo("B",grupoB);
        Serial.printf("Nivel E=0x%02X V=0x%02X libres_confirmados=%u conocidos=%u\n",
            (unsigned)nivel.ocupados,(unsigned)nivel.validos,contarLibres(nivel),contarBits(nivel.validos));
    }
    static bool ipExternaMostrada=false;
    if(!ipExternaMostrada&&WIFI_EXTERNA[0]&&WiFi.status()==WL_CONNECTED){
        ipExternaMostrada=true;Serial.print("IP en red externa: http://");Serial.println(WiFi.localIP());
    }
    delay(1);
}
