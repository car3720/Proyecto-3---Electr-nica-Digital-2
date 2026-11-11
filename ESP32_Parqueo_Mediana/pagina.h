#ifndef PARQUEO_PAGINA_H
#define PARQUEO_PAGINA_H
static const char PAGINA[] PROGMEM=R"HTML(
<!doctype html><html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Parqueo · Nivel 1</title>
<style>
:root{font-family:system-ui,sans-serif;color:#e7efff;background:#0c1422;color-scheme:dark}*{box-sizing:border-box}
body{max-width:1050px;margin:auto;padding:24px}header{display:flex;justify-content:space-between;gap:14px;align-items:center}
h1{margin:4px 0;font-size:32px}h2{font-size:17px;margin-top:28px}p,small{color:#bdcbe0}.estado{border:1px solid #536786;padding:9px 13px;border-radius:24px}
.resumen{background:#18283e;padding:22px;border-radius:16px;margin-top:22px}.numero{font-size:62px;font-weight:bold;color:#99edbe}
.plazas{display:grid;grid-template-columns:repeat(4,1fr);gap:12px}.plaza{border:1px solid #64728a;border-radius:14px;padding:20px;background:#29364b;min-height:112px}
.plaza strong{display:block;font-size:25px;margin-bottom:14px}.libre{background:#104432;border-color:#389768}.ocupado{background:#592633;border-color:#d06b75}
.registro,details{background:#18283e;border-radius:14px;margin-top:24px;padding:18px}button{padding:8px 12px;color:inherit;background:#263d59;border:1px solid #60718d;border-radius:8px;cursor:pointer}
ol{padding-left:23px;line-height:1.8}table{width:100%;border-collapse:collapse;font-size:14px}td,th{text-align:left;padding:10px;border-bottom:1px solid #425269}
.aviso{color:#ffcc8b}a{color:#a6d3ff}summary{cursor:pointer;font-weight:bold}details p{font-size:14px}pre{white-space:pre-wrap;overflow-wrap:anywhere}
@media(max-width:600px){body{padding:16px}.plazas{grid-template-columns:repeat(2,1fr)}header{align-items:flex-start}table{font-size:12px}td,th{padding:7px}}
</style></head><body>
<header><div><small>CENTRO DE CONTROL</small><h1>Parqueo · Nivel 1</h1></div><span class="estado" id="conexion">Conectando…</span></header>
<section class="resumen"><div>Total de espacios disponibles</div><div class="numero" id="total">—</div><p id="detalle">Esperando los ocho estados.</p></section>
<h2>Grupo A · P1–P4</h2><section class="plazas" id="grupoA"></section>
<h2>Grupo B · P5–P8</h2><section class="plazas" id="grupoB"></section>
<section class="registro"><header><strong>Cambios de ocupación</strong><button id="limpiar">Limpiar</button></header><ol id="eventos"><li>Esperando datos.</li></ol><small>Los avisos se registran mientras la página está abierta.</small></section>
<details open><summary>Conexión con las Nucleo</summary>
<p id="mensajeBus">Esperando diagnóstico del ESP32.</p><table><thead><tr><th>Placa</th><th>Lectura</th><th>Sensores válidos</th><th>Retorno</th></tr></thead><tbody><tr><td id="direccionA">A</td><td id="lecturaA">—</td><td id="validosA">—</td><td id="retornoA">—</td></tr><tr><td id="direccionB">B</td><td id="lecturaB">—</td><td id="validosB">—</td><td id="retornoB">—</td></tr></tbody></table>
<p id="ayuda" class="aviso"></p><p><a href="/api/estado" target="_blank" rel="noopener">Ver datos del ESP32</a></p><small>Actualización web cada 500 ms · Parqueo Mediana AB v5</small></details>
<script>
let anterior=null,ultimoDato=0;
const $=id=>document.getElementById(id);
for(let i=0;i<8;i++){
 const tarjeta=document.createElement('article');tarjeta.className='plaza';tarjeta.id='p'+i;
 tarjeta.innerHTML='<strong>P'+(i+1)+'</strong><span>Sin datos</span>';
 $(i<4?'grupoA':'grupoB').appendChild(tarjeta);
}
function registrar(texto){
 const item=document.createElement('li');item.textContent=new Date().toLocaleTimeString()+' · '+texto;$('eventos').prepend(item);
 while($('eventos').children.length>20)$('eventos').lastElementChild.remove();
}
function contar(v){let n=0;for(let i=0;i<4;i++)n+=(v>>i)&1;return n;}
function actualizar(d){
 if(!Array.isArray(d.plazas)||d.plazas.length!==8||!d.a||!d.b)throw new Error('Formato de respuesta incorrecto');
 ultimoDato=Date.now();$('conexion').textContent='ESP32 conectado';
 for(let i=0;i<8;i++){
  const s=d.plazas[i],tarjeta=$('p'+i);tarjeta.className='plaza '+(s===0?'libre':s===1?'ocupado':'');
  tarjeta.querySelector('span').textContent=s===0?'Libre':s===1?'Ocupado':'Sin datos';
  if(anterior&&anterior.plazas[i]!==s)registrar('P'+(i+1)+': '+(s===0?'libre':s===1?'ocupado':'sin datos'));
 }
 if(!anterior){$('eventos').textContent='';registrar('Estado inicial recibido');}
 $('total').textContent=d.total_disponibles===null?'—':d.total_disponibles;
 $('detalle').textContent=d.conocidos===8?'Ocho espacios con datos confirmados.':d.libres_confirmados+' libres confirmados; '+(8-d.conocidos)+' espacios sin datos. El total completo aún no está disponible.';
 for(const nombre of ['A','B']){
  const g=d[nombre.toLowerCase()];$('direccion'+nombre).textContent=nombre+' · 0x'+g.direccion.toString(16).toUpperCase();
  $('lectura'+nombre).textContent=g.conectada?'Correcta ('+g.recibidos+' bytes)':g.error_lectura===2?'Formato no compatible':'Sin paquete válido ('+g.recibidos+'/'+g.esperados+')';
  $('validos'+nombre).textContent=contar(g.validos)+' / 4';
  $('retorno'+nombre).textContent=g.error_escritura===255?'Sin enviar':g.error_escritura===0?'Enviado':'Error '+g.error_escritura;
 }
 $('mensajeBus').textContent=d.i2c_iniciado?'El ESP32 está consultando ambas placas por I2C.':'No se pudo iniciar el bus I2C en el ESP32.';
 $('ayuda').textContent=!d.a.conectada||!d.b.conectada?'Falta comunicación: revisar programas cargados, SDA/SCL, GND y resistencias pull-up.':d.conocidos<8?'Las placas responden; todavía faltan lecturas válidas de sensores.':d.a.error_escritura!==0||d.b.error_escritura!==0?'La web recibe estados, pero falla el retorno a alguna Nucleo: revisar su recepción I2C.':'Los ocho estados están disponibles y se enviaron a ambas placas.';
 anterior=d;
}
function sinConexion(){
 if(Date.now()-ultimoDato<3000)return;
 $('conexion').textContent='Sin conexión con ESP32';$('total').textContent='—';
 $('detalle').textContent='Revisa la conexión WiFi y la IP del Monitor Serial.';
 for(let i=0;i<8;i++){$('p'+i).className='plaza';$('p'+i).querySelector('span').textContent='Sin conexión';}
 $('mensajeBus').textContent='Diagnóstico no actualizado: se perdió la conexión con el ESP32.';
 for(const n of ['A','B'])for(const campo of ['lectura','validos','retorno'])$(campo+n).textContent='—';
 $('ayuda').textContent='Esperando respuesta actual del ESP32.';anterior=null;
}
async function consultar(){
 const control=new AbortController(),timer=setTimeout(()=>control.abort(),2000);
 try{const r=await fetch('/api/estado',{cache:'no-store',signal:control.signal});if(!r.ok)throw new Error('HTTP '+r.status);actualizar(await r.json());}
 catch(e){sinConexion();}finally{clearTimeout(timer);setTimeout(consultar,500);}
}
$('limpiar').onclick=()=>{$('eventos').textContent='';};
consultar();setInterval(sinConexion,1000);
</script></body></html>
)HTML";
#endif
