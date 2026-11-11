#ifndef PARQUEO_PROTOCOLO_H
#define PARQUEO_PROTOCOLO_H
#include <stdint.h>
#include <stddef.h>

// Direcciones de 7 bits. Coinciden con Parqueo_A/B_Conservado.
enum class Formato : uint8_t { CuatroEstados=4, DosMascaras=2 };
struct Grupo {
    uint8_t direccion;
    Formato formato;
    bool conectado=false;
    uint8_t ocupados=0, validos=0, recibidos=0;
    uint8_t datos[4]={255,255,255,255};
    uint8_t errorLectura=1; // 0 correcto; 1 longitud; 2 contenido; 3 bus sin iniciar.
    uint8_t errorEscritura=255; // 255=no intentada; 0=sin error reportado por Wire.
    uint32_t fallos=0;
};
struct EstadoNivel {
    uint8_t ocupados=0, validos=0;
};
inline bool decodificar(Grupo &g)
{
    g.conectado=false;g.ocupados=0;g.validos=0;
    if(g.recibidos!=static_cast<uint8_t>(g.formato)){
        g.errorLectura=1;return false;
    }
    uint8_t e=0,v=0;
    if(g.formato==Formato::CuatroEstados){
        // Codigo de Eddy: cada byte es 0=libre, 1=ocupado, 2=sin datos.
        for(uint8_t i=0;i<4;i++){
            if(g.datos[i]>2){g.errorLectura=2;return false;}
            if(g.datos[i]!=2)v|=(uint8_t)(1U<<i);
            if(g.datos[i]==1)e|=(uint8_t)(1U<<i);
        }
    }else{
        // Codigo de B: [mascara ocupados, mascara validos], solo bits 0..3.
        if((g.datos[0]&0xF0U)||(g.datos[1]&0xF0U)){
            g.errorLectura=2;return false;
        }
        e=g.datos[0];v=g.datos[1];
    }
    g.ocupados=e;g.validos=v;g.conectado=true;g.errorLectura=0;return true;
}
inline EstadoNivel combinar(const Grupo &a,const Grupo &b)
{
    EstadoNivel n;
    n.ocupados=(uint8_t)((a.ocupados&15U)|((b.ocupados&15U)<<4));
    n.validos=(uint8_t)((a.conectado?(a.validos&15U):0U)|
        ((b.conectado?(b.validos&15U):0U)<<4));
    return n;
}
inline unsigned contarBits(uint8_t valor)
{
    unsigned n=0;for(unsigned i=0;i<8;i++)n+=(valor>>i)&1U;return n;
}
inline unsigned contarLibres(const EstadoNivel &n)
{
    return contarBits((uint8_t)(~n.ocupados&n.validos));
}
inline int estadoPlaza(const EstadoNivel &n,unsigned i)
{
    if(i>=8||!(n.validos&(1U<<i)))return -1;
    return (n.ocupados&(1U<<i))?1:0;
}
#endif
