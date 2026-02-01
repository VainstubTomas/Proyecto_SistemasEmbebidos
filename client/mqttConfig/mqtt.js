import mqtt from 'mqtt';
import fs from 'fs';
import ParkingEvent from '../models/ParkingEvent.js';
import { enviarNotificacionPago } from '../utils/mails.js';

//Broker Configuration
const BROKER_URL = 'j72b9212.ala.us-east-1.emqxsl.com';
const BROKER_PORT = 8883;
const MQTT_BROKER_URL = `mqtts://${BROKER_URL}:${BROKER_PORT}`;

//certificacion de autoridad del broker
const CA_CERT = fs.readFileSync('./emqxsl-ca.crt');

//MQTT client
let client = null;

//topics
//wildcard '#' porque todos los topicos comienzan con pci/
const TOPIC_STATUS_BASE = 'embebidos/#';

/**
 * Inicializa la conexión MQTT
 */
function init() {
    if (client) return;

    client = mqtt.connect(MQTT_BROKER_URL, {
        username:'SistemasEmbebidos',
        password:'SistemasEmbebidos',
        //clean para recordar usuario cuando se desconecte
        clean: true,
        //credencial
        ca:CA_CERT
    });

    // Conexión establecida
    client.on('connect', () => {
        console.log('Cliente MQTT conectado al broker.');

        // sub to topic base
        client.subscribe(TOPIC_STATUS_BASE, (err) => {
            if (err) {
                console.error('Error al suscribirse a tópicos:', err);
            } else {
                console.log(`Suscrito a la base de tópicos: ${TOPIC_STATUS_BASE}`);
            }
        });
    });

    // Error handler
    client.on('error', (err) => {
        console.error('Error en el cliente MQTT:', err.message);
    });

    // Recepción de mensajes
    // Recepción de mensajes
client.on('message', async (topic, message) => {
    try {
        const payload = message.toString();
        console.log(`[MQTT_RECIBIDO] Tópico: ${topic}, Payload: ${payload}`);

        if (topic === 'embebidos/parking/evento') {
            const datos = JSON.parse(payload);

            // Cálculos de tiempo
            const tiempoMilisegundos = datos.diferenciaTiempo;
            const tiempoSegundos = tiempoMilisegundos / 1000;
            const tiempoMinutos = tiempoSegundos / 60;

            // Cálculo de tarifa PROPORCIONAL: $5 por segundo
            const tarifaPorSegundo = 5;
            const tarifaTotal = Math.round(tiempoSegundos * tarifaPorSegundo);

            console.log(`╔════════════════════════════════════════╗`);
            console.log(`║        EVENTO DE PARKING               ║`);
            console.log(`╠════════════════════════════════════════╣`);
            console.log(`Espacio: ${datos.espacio.padEnd(29)}`);
            console.log(`Tiempo: ${tiempoSegundos.toFixed(2)} seg (${tiempoMinutos.toFixed(2)} min)${' '.repeat(Math.max(0, 6 - tiempoSegundos.toFixed(2).length))}`);
            console.log(`Tarifa: $${tarifaTotal} ($5/seg)${' '.repeat(19 - tarifaTotal.toString().length)}`);

            // Guardar en la base de datos
            try {
                const nuevoEvento = new ParkingEvent({
                    plaza: datos.espacio,
                    duracionMs: datos.diferenciaTiempo,
                    startedAt: new Date(datos.tiempoInicial),
                    endedAt: new Date(datos.tiempoFinal),
                    tarifa: tarifaTotal
                });

                await nuevoEvento.save();
                console.log('[BD] Evento guardado en la base de datos');
            } catch (dbError) {
                console.error('[BD] Error al guardar en BD:', dbError.message);
            }

            //notificacion email
            try {
                await enviarNotificacionPago(tarifaTotal, datos.espacio, tiempoMinutos);
            } catch (error) {
                console.log("[MQTT] Error enviado mail: ", error);
            }
        }

    } catch (e) {
        console.error('Error al procesar mensaje MQTT:', e);
    }
});
}

export default {
    init
};