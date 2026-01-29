import mqtt from 'mqtt';
import fs from 'fs';

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
        username:'user',
        password:'FinalPCI123',
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
    client.on('message', (topic, message) => {
        try {
            const payload = message.toString();
            
            // Log of receipted data
            console.log(`[MQTT_RECIBIDO] Tópico: ${topic}, Payload: ${payload}`);

        } catch (e) {
            console.error('Error al procesar mensaje MQTT:', e);
        }
    });
}

export default {
    init
};